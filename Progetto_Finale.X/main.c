// Final Project - Elsa Bunz, Serena Guelfi, Valentina Pericu

// DSPIC30F4011 Configuration Bit Settings
// 'C' source line config statements
// FOSC
#pragma config FPR = XT                 // Primary Oscillator Mode (XT)
#pragma config FOS = PRI                // Oscillator Source (Primary Oscillator)
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
#pragma config FWPSB = WDTPSB_16        // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_512       // WDT Prescaler A (1:512)
#pragma config WDT = WDT_OFF            // Watchdog Timer (Disabled)

// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config BODENV = BORV20          // Brown Out Voltage (Reserved)
#pragma config BOREN = PBOR_ON          // PBOR Enable (Enabled)
#pragma config LPOL = PWMxL_ACT_HI      // Low-side PWM Output Polarity (Active High)
#pragma config HPOL = PWMxH_ACT_HI      // High-side PWM Output Polarity (Active High)
#pragma config PWMPIN = RST_IOPIN       // PWM Output Pin Reset (Control with PORT/TRIS regs)
#pragma config MCLRE = MCLR_EN          // Master Clear Enable (Enabled)

// FGS
#pragma config GWRP = GWRP_OFF          // General Code Segment Write Protect (Disabled)
#pragma config GCP = CODE_PROT_OFF      // General Segment Code Protection (Disabled)

// FICD
#pragma config ICS = ICS_PGD            // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// Include header with specific functions
#include "utils.h"
#include "parser.h"

typedef struct {
    int n;
    int N;
} heartbeat;

#define MAX_TASKS (6)
#define CONTROLMODE (0)
#define TIMEOUT (1)
#define SAFEMODE (2)

int curr_state;

heartbeat schedInfo[MAX_TASKS];

volatile CircularBuffer cb;
parser_state pstate;

int sat_RPM_min, sat_RPM_max, RPM_1, RPM_2;
double count_temp;
double sum_temp, avg_temp;
double temp1, temp2;


// Task 0: Temperature 10 Hz
void task0() {

    IFS0bits.ADIF = 0;
    while (!IFS0bits.ADIF);
    double val = ADCBUF0;
    //convert to volt
    double mV = (val / 1023) * 5000;
    //convert to celsius
    double temp = 25 + (mV - 750) / 10;
    sum_temp = sum_temp + temp;
    count_temp = count_temp + 1;

    if (count_temp == 10) {
        // Print the temperature
        avg_temp = sum_temp / count_temp;
        sum_temp = 0;
        count_temp = 0;
        move_cursor_second_row();
        // send temperature
        char mctem_msg[16];
        sprintf(mctem_msg, "$MCTEM,%.2f*", avg_temp);
        send_UART(mctem_msg, 16);
    }
}

// Task 1: Read messages 10 Hz

void task1() {

    move_cursor_first_row();
    // read the circular buffer
    char value;
    int avl = avl_in_buffer(&cb);
    int i;
    for (i = 0; i < avl; i++) {
        read_buffer(&cb, &value);
        // call the parser
        int message = parse_byte(&pstate, value);
        // if parser returns 1 --> new message
        if (message == 1) {
            // handle the message
            // HL
            // REF or SAT or ENA
            char type_recv [6];
            strcpy(type_recv, pstate.msg_type);
            char payload_recv [100];
            strcpy(payload_recv, pstate.msg_payload);
            char ref_msg [6] = "HLREF\0";
            char ena_msg [6] = "HLENA\0";
            char sat_msg [6] = "HLSAT\0";
            char empty_msg = '\0';

            // HLREF message

            if (strcmp(type_recv, ref_msg) == 0) {
                if (curr_state != SAFEMODE) {
                    int n1, n2;
                    // get n1, n2
                    checkPayload(&n1, &n2);
                    // saturate n1 and n2
                    RPM_1 = saturate(n1);
                    RPM_2 = saturate(n2);
                }
                TMR2 = 0;
                if (curr_state == TIMEOUT) {
                    curr_state = CONTROLMODE;
                    IFS0bits.T2IF = 0; // reset interrupt flag of timer
                    T2CONbits.TON = 1; // turn on timer
                    LATBbits.LATB1 = 0; // turn off D4
                }
            }                // HLENA message
            else if (strcmp(type_recv, ena_msg) == 0 && (payload_recv[0] == empty_msg)) {
                // exit safe mode...
                if (curr_state == SAFEMODE) {
                    curr_state = CONTROLMODE;

                    // send positive ack
                    send_UART("$MCACK,ENA,1*", 16);

                    // re-enable button interrupt
                    IFS0bits.INT0IF = 0; // reset interrupt flag of button S5
                    IFS1bits.INT1IF = 0; // reset interrupt flag of button S6
                    IEC0bits.INT0IE = 1;
                    IEC1bits.INT1IE = 1;
                    // Enable timer 2
                    TMR2 = 0;
                    IFS0bits.T2IF = 0; // reset interrupt flag of timer
                    IEC0bits.T2IE = 1;
                    T2CONbits.TON = 1; // turn on timer

                } else {
                    // send negative ack
                    send_UART("$MCACK,ENA,0*", 16);
                }
            }                // HLSAT message
            else if (strcmp(type_recv, sat_msg) == 0) {
                int min, max;
                checkPayload(&min, &max);
                if (isValid(min, max)) {
                    // change min and max
                    sat_RPM_min = min;
                    sat_RPM_max = max;
                    // saturate current RPM values
                    RPM_1 = saturate(RPM_1);
                    RPM_2 = saturate(RPM_2);
                    // Refresh PWM
                    task2();

                    // send positive ack
                    send_UART("$MCACK,SAT,1*", 16);
                } else {
                    // send negative ack
                    send_UART("$MCACK,SAT,0*", 16);
                }


            }                // Invalid message
            else {
                // Error
            }

        }


    }
}

// Task 2: Refresh PWM2 and PWM3  10 Hz
void task2() {
    // Calculate the duty cycle corresponding to the desired RPM value for motor 1
    double val2 = (RPM_1 + 10000.0) / 10000.0;
    double ptc2 = val2 * PTPER;
    PDC2 = ptc2;


    // Calculate the duty cycle corresponding to the desired RPM value for motor 2
    double val3 = (RPM_2 + 10000.0) / 10000.0;
    double ptc3 = val3 * PTPER;
    PDC3 = ptc3;

    temp1 = val2;
    temp2 = val3;
}

// Task 3: Blink D4 1 Hz

void task3() {
    if (curr_state == TIMEOUT) {

        int ledValue = LATBbits.LATB1;
        LATBbits.LATB1 = 1 - ledValue;
    }

}

// Task 4: Send MCFBK 5 Hz

void task4() {
    char str[25];
    sprintf(str, "$MCFBK,%d,%d,%d*", RPM_1, RPM_2, curr_state);
    send_UART(str, 25);
}

// Task 5: Update LCD 10 Hz

void task5() {
    // First line
    move_cursor_first_row();
    char state;
    switch (curr_state) {
        case 0:
            state = 'C';
            break;
        case 1:
            state = 'T';
            break;
        case 2:
            state = 'H';
            break;
    }

    char str1[16];
    sprintf(str1, "STA:%c TEM:%.1f", state, avg_temp);
    write_string_LCD(str1, 16);

    // Second line
    clear_LCD(2);
    move_cursor_second_row();
    char str2[16];
    sprintf(str2, "RPM:%d,%d", RPM_1, RPM_2);
    write_string_LCD(str2, 16);
}

void scheduler() {
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        schedInfo[i].n++;
        if (schedInfo[i].n == schedInfo[i].N) {
            switch (i) {
                case 0:
                    // Read temperature,
                    //calculate, print and stamp the  average temperature
                    task0();
                    break;
                case 1:
                    // Process messages
                    task1();
                    break;
                case 2:
                    // Refresh PWM
                    task2();
                    break;
                case 3:
                    // Blink LED D4
                    task3();
                    break;
                case 4:
                    // Send MCFBK
                    task4();
                    break;
                case 5:
                    // Update LCD
                    task5();
                    break;
            }
            schedInfo[i].n = 0;
        }
    }
}



// Handle the button interrupt
void button_interrupt(){
    // disable interrupt for button S5 and S6
    IEC0bits.INT0IE = 0;
    IEC1bits.INT1IE = 0;

    // Turn off LED D4 if we were in Timeout mode
    if (curr_state == TIMEOUT) {
        LATBbits.LATB1 = 0;
    }
    // enter safe mode
    curr_state = SAFEMODE;
    // Disable timer 2 in order not to enter in timeout mode
    IEC0bits.T2IE = 0;
    T2CONbits.TON = 0; // turn off timer
    // stop motors
    RPM_1 = 0;
    RPM_2 = 0;
    task2(); 
}
// Interrupt for button S5
void __attribute__((__interrupt__, __auto_psv__)) _INT0Interrupt() {
    button_interrupt();
}

// Interrupt for button S6
void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt() {
    button_interrupt();
}

// interrupt service routine for Timer 2 --> TIMEOUTMODE
void __attribute__((__interrupt__, __auto_psv__)) _T2Interrupt() {
    IFS0bits.T2IF = 0; // reset interrupt flag of timer
    T2CONbits.TON = 0; // turn off the timer

    curr_state = TIMEOUT;
    LATBbits.LATB1 = 1; // turn on LED D4
    // stop motors
    RPM_1 = 0;
    RPM_2 = 0;
}

// interrupt for UART     
void __attribute__((__interrupt__, __auto_psv__)) _U2RXInterrupt() {
    IFS1bits.U2RXIF = 0;
    char val = U2RXREG;
    write_buffer(&cb, val);
}

int main(void) {
    
    // initialize state
    curr_state = 0;
    // initialize temperature values
    count_temp = 0;
    sum_temp = 0;
    // Initialize saturation values
    sat_RPM_min = MIN_RPM;
    sat_RPM_max = MAX_RPM;
    // Buffer initialization
    cb.writeIndex = 0;
    cb.readIndex = 0;
    //initialize motors
    RPM_1 = 0;
    RPM_2 = 0;

    // parser initialization
    pstate.state = STATE_DOLLAR;
    pstate.index_type = 0;
    pstate.index_payload = 0;

    InitDevices();
    schedInfo[0].n = 0;
    schedInfo[1].n = 0;
    schedInfo[2].n = 0;
    schedInfo[3].n = 0;
    schedInfo[4].n = 0;
    schedInfo[5].n = 0;

    // Task 0: Temperature 10 Hz
    schedInfo[0].N = 1;
    // Task 1: Read messages 10 Hz
    schedInfo[1].N = 1;
    // Task 2: Refresh PWM 10 Hz
    schedInfo[2].N = 1;
    // Task 3: Blink D4 1 Hz
    schedInfo[3].N = 10;
    // Task 4: Send MCFBK 5 Hz
    schedInfo[4].N = 2;
    // Task 5: Update LCD 10 Hz?
    schedInfo[5].N = 1;



    int count = 0;
    int ledValue = 0;
    while (1) {

        if (count == 5) {
            LATBbits.LATB0 = 1 - ledValue;
            ledValue = 1 - ledValue;
            count = -1;
        }
        count = count + 1;
        scheduler();
        tmr_wait_period(1);
    }




    return 0;
}







