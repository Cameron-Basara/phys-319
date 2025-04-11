#include <msp430.h>

// Function Prototypes
void uart_init();
void uart_send_string(const char *str);
void uart_send_char(char c);
void uart_send_number(unsigned int num);
void adc12_init();
void start_adc_conversion();
void delay_ms(unsigned int ms);
void set_servo_speed(int speed);
void rotate_servo(int speed, unsigned int duration_ms);

// Global Variables
volatile unsigned int sensor_value = 0;
volatile unsigned char data_ready = 0;
int current_angle = 0;

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    uart_init();
    adc12_init();
    __enable_interrupt();

    // Configure P1.2 as Timer_A output for servo control
    P1DIR |= BIT2;
    P1SEL |= BIT2;

    // Configure Timer_A
    TA0CCR0 = 20000 - 1;        // Set period to 20 ms (50 Hz)
    TA0CCTL1 = OUTMOD_7;        // Set output mode to Reset/Set
    TA0CTL = TASSEL_2 | MC_1 | ID_0; // SMCLK as source, up mode, no divider

    while (1) {
        // Rotate CCW for 360 degrees
        rotate_servo(-1, 6000); // Adjust duration as needed

        // Stop
        set_servo_speed(0);
        delay_ms(500); // Brief pause

        // Rotate CW for 360 degrees
        rotate_servo(1, 6000); // Adjust duration as needed

        // Stop
        set_servo_speed(0);
        delay_ms(500); // Brief pause
    }
}

void rotate_servo(int speed, unsigned int duration_ms) {
    unsigned int steps = duration_ms / 250; // Number of 250 ms steps
    unsigned int i =0;
    for (i = 0; i < steps; i++) {
        set_servo_speed(speed);
        delay_ms(250); // Allow time for servo to move

        start_adc_conversion();
        while (!data_ready);
        data_ready = 0;

        // Transmit current angle and raw ADC value over UART
        uart_send_number(current_angle);
        uart_send_char(',');
        uart_send_number(sensor_value);
        uart_send_string("\r\n");

        // Update angle
        if (speed > 0) {
            current_angle += 15; // Adjust angle increment as needed
            if (current_angle >= 360) {
                current_angle -= 360;
            }
        } else if (speed < 0) {
            current_angle -= 15; // Adjust angle decrement as needed
            if (current_angle < 0) {
                current_angle += 360;
            }
        }
    }
}

void set_servo_speed(int speed) {
    unsigned int pulse_width;
    if (speed > 0) {
        pulse_width = 1400; // Adjust for CW rotation
    } else if (speed < 0) {
        pulse_width = 1750; // Adjust for CCW rotation
    } else {
        pulse_width = 1520; // Stop
    }
    TA0CCR1 = pulse_width;
}

void uart_init() {
    P4SEL |= BIT4 | BIT5;  // P4.4 = TX, P4.5 = RX
    UCA1CTL1 |= UCSWRST;   // Hold USCI in reset
    UCA1CTL1 |= UCSSEL_2;  // Use SMCLK
    UCA1BR0 = 109;         // Baud rate 9600 (1MHz / 104 = 9600 baud)
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;    // Modulation
    UCA1CTL1 &= ~UCSWRST;  // Initialize USCI state machine
}

void uart_send_char(char c) {
    while (!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = c;
}

void uart_send_string(const char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

void uart_send_number(unsigned int num) {
    char buffer[6];
    int i = 0;
    if (num == 0) {
        uart_send_char('0');
    } else {
        while (num > 0) {
            buffer[i++] = (num % 10) + '0';
            num /= 10;
        }
        while (i > 0) {
            uart_send_char(buffer[--i]);
        }
    }
}

void adc12_init() {
    P6SEL |= BIT0;  // Select P6.0 as ADC input (A0)
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON | ADC12MSC;
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_0;
    ADC12CTL2 = ADC12RES_2;
    ADC12MCTL0 = ADC12INCH_0;
    ADC12IE = ADC12IE0;
    ADC12CTL0 |= ADC12ENC;
}

void start_adc_conversion() {
    ADC12CTL0 |= ADC12SC | ADC12ENC;
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void) {
    if (ADC12IV == ADC12IV_ADC12IFG0) {
        sensor_value = ADC12MEM0;
        data_ready = 1;
    }
}

void delay_ms(unsigned int ms) {
    while (ms--) {
        __delay_cycles(1000);   
    }
}
