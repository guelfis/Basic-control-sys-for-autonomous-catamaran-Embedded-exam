// Assignment PWM - Elsa Bunz, Serena Guelfi, Valentina Pericu
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
// Include header with functions of previous assignments
#include "utils.h"
#include "parser.h"

typedef struct {
    int n;
    int N;
} heartbeat;

#define MAX_TASKS 6

heartbeat schedInfo[MAX_TASKS];

volatile CircularBuffer cb;
parser_state pstate;

// Task 0: Temperature 10 Hz

void task0() {

}

// Task 1: Read messages 10 Hz

void task1() {

    // read the circular buffer
    char value;
    int read = read_buffer(&cb, &value);
    if(read == 1){
        int message = parse_byte(&pstate, value);
    }
    // call the parser

    // if parser returns 1 --> new message
    // handle the message

    // switch 3 messages

}

// Task 2: Refresh PWM 10 Hz

void task2() {

}

// Task 3: Blink D4 1 Hz

void task3() {

}

// Task 4: Send MCFBK 5 Hz

void task4() {

}

// Task 5: Update LCD 10 Hz?

void task5() {

}

void scheduler() {
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        schedInfo[i].n++;
        if (schedInfo[i].n == schedInfo[i].N) {
            switch (i) {
                case 0:
                    task0();
                    break;
                case 1:
                    task1();
                    break;
                case 2:
                    task2();
                    break;
                case 3:
                    task3();
                    break;
                case 4:
                    task4();
                    break;
                case 5:
                    task5();
                    break;
            }
            schedInfo[i].n = 0;
        }
    }
}

void InitDevices() {
    
    // Init UART
    U2BRG = 11; // (7372800 / 4) / (16 * 9600) - 1
    U2MODEbits.UARTEN = 1; // enable UART2
    U2STAbits.UTXEN = 1; // enable U2TX, enable transmission (must be after UARTEN)
    IEC1bits.U2RXIE = 1; // enable interrupt for UART
    
    //Init LCD
    SPI1CONbits.MSTEN = 1; // master mode
    SPI1CONbits.MODE16 = 0; // 8-bit mode
    SPI1CONbits.PPRE = 3; // 1:1 primary prescaler
    SPI1CONbits.SPRE = 6; // 2:1 secondary prescaler
    SPI1STATbits.SPIEN = 1; // enable SPI

    // Wait period for LCD
    tmr_setup_period(1, 1000);
    tmr_wait_period(1);
    // Init heartbeat as 100 because the max frequency we have is 10 Hz
    tmr_setup_period(1, 100);

    // Init LED
    TRISBbits.TRISB0 = 0; // LED D3
    LATBbits.LATB0 = 0;


}


// interrupt for UART     
void __attribute__((__interrupt__, __auto_psv__)) _U2RXInterrupt() {
    IFS1bits.U2RXIF = 0;
    char val = U2RXREG;
    write_buffer(&cb, val);
}

int main(void) {
    
    // Buffer initialization
    cb.writeIndex = 0;
    cb.readIndex = 0;
    
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
    schedInfo[2].N = 10;
    // Task 4: Send MCFBK 5 Hz
    schedInfo[2].N = 2;
    // Task 5: Update LCD 10 Hz?
    schedInfo[2].N = 1;



    while (1) {
        scheduler();
        tmr_wait_period(1);
    }




    return 0;
}







