import serial
import re
import numpy as np
import matplotlib.pyplot as plt

# Constants 
PORT = "COM3"      # Change to your correct COM port
BAUD = 9600
VREF = 3.3         # ADC reference voltage
ADC_MAX = 4095     # 12-bit ADC

def adc_to_voltage(adc_val):
    return (adc_val / ADC_MAX) * VREF

def voltage_to_distance(voltage):
    if voltage < 0.4 or voltage > 2.5:
        return None
    return (27.86 / voltage) - 0.42  # From Sharp GP2D12 datasheet

# Plot Setup 
plt.ion()
fig, ax = plt.subplots(subplot_kw={'projection': 'polar'})
ax.set_theta_zero_location("N")
ax.set_theta_direction(-1)
ax.set_ylim(0, 100)  # Set max range as needed

# Store current points to replace them if angle repeats
data_points = {}

# Serial Communication 
try:
    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        print(f"Listening on {ser.name}...")

        while True:
            line = ser.readline().decode("ascii", errors="ignore").strip()
            if ',' in line:
                try:
                    angle_str, adc_str = line.split(',')
                    angle = int(angle_str) % 360
                    adc = int(adc_str)
                    voltage = adc_to_voltage(adc)
                    distance = voltage_to_distance(voltage)

                    if distance is not None:
                        theta = np.deg2rad(angle)

                        # Delete previous point at this angle
                        if angle in data_points:
                            data_points[angle].remove()

                        # Plot new point
                        dot, = ax.plot(theta, distance, 'bo')
                        data_points[angle] = dot

                        # Optional: print debug
                        print(f"Angle: {angle}°, ADC: {adc}, Distance: {distance:.1f} cm")

                        plt.pause(0.01)
                except ValueError:
                    continue

except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("Stopped by user.")
finally:
    plt.ioff()
    plt.show()
