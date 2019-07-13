/* 
 * File:  Functions for the final project 
 * Author: Valentina Pericu, Serena Guelfi, Elsa Bunz 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include "parser.h"

#define MIN_RPM (-8000)
#define MAX_RPM (8000)

extern parser_state pstate;
extern int sat_RPM_min, sat_RPM_max, RPM_1, RPM_2;

// Process the payload and p gives back the two encoded values
// return 1 if payload is valid, 0 if invalid
int checkPayload (int* n1, int* n2){
    // search index of comma
    // divide in two sub char arrays
    // convert to integer (check?)
    // process n1
    char find = ',';
    int index;
    char payload_recv [100];
    strcpy(payload_recv, pstate.msg_payload);

    const char *ptr = strchr(payload_recv, find);
    if(ptr){
        index = ptr - payload_recv;
    }
    int i;
    char arr_n1[100];
    for(i = 0; i < index; i++ ){
        arr_n1[i] = payload_recv[i]; 
    }
    arr_n1[index] = '\0';
    
    if(sscanf(arr_n1, "%d", n1) == EOF) {
        return 0;
    }
    
    char arr_n2[100];
     
    for(i = index+1; i < sizeof(payload_recv); i++ ){
        arr_n2[i-index-1] = payload_recv[i]; 
    }
     
    if(sscanf(arr_n2, "%d", n2) == EOF) {
        return 0;
    }
    

     
     return 1;

}

// checks whether the given values are in the range, and respect the min - max properties
int isValid(int min, int max){
    
    if(min >= max){
        return 0;
    }
    if(min>0 || max < 0){
        return 0;
    }
    if(min < MIN_RPM || max > MAX_RPM){
        return 0;
    }
    
    return 1;
}

int saturate(int n){
    if(n< sat_RPM_min ){
       return sat_RPM_min;
    }
    if(n > sat_RPM_max){
        return sat_RPM_max;
    }
    
    return n;
    
}

// Perform initialization
void InitDevices() {

    // Init UART
    U2BRG = 11; // (7372800 / 4) / (16 * 9600) - 1
    U2MODEbits.UARTEN = 1; // enable UART2
    U2STAbits.UTXEN = 1; // enable U2TX, enable transmission (must be after UARTEN)
    IEC1bits.U2RXIE = 1; // enable interrupt for UART

    // ADC 
    ADPCFG = 0xFFFF; // set all port to digital
    // Temperature
    ADCHSbits.CH0SA = 3; // Set AN3 as input to channel 0
    ADPCFGbits.PCFG3 = 0; // set the input port we need to analog

    // General setup ADC
    ADCHSbits.CH0NA = 0; // ground as negative input to channel 0
    ADCON1bits.ASAM = 1; //automatic start of the sampling
    ADCON1bits.SSRC = 7; //automatic mode (auto convert)
    ADCON2bits.CHPS = 0; // select channel 0
    ADCON2bits.CSCNA = 0; //disable scan mode
    ADCON2bits.SMPI = 0; //interrupt after each conversion
    ADCON3bits.ADCS = 15; //Period Tad
    ADCON3bits.SAMC = 16; //sample time 16 Tad      
    ADCON1bits.ADON = 1; // turn on the ADC module

    // Init Buttons
    TRISEbits.TRISE8 = 1; // set the pin of the button as input
    TRISDbits.TRISD0 = 1; // set the pin of the button as input   
    // Enable Interrupt for Button
    IEC0bits.INT0IE = 1;
    IEC1bits.INT1IE = 1;

    // set up PWM
    double fcy = 7372800 / 4.0;
    double fpwm = 1000; // 1kHz frequency
    // ptper = fcy/ (fpwm * prescaler) - 1
    // prescaler needs to be 1:1 to obtain a number that fits in 15bits

    PTCONbits.PTOPS = 0; // postscaler
    PTCONbits.PTCKPS = 0; // prescaler 1:1
    PTCONbits.PTMOD = 0; // free running mode
    PTCONbits.PTEN = 1; //turn on the PWM time base module
    PWMCON1bits.PEN2H = 1; // set PWM2H as output pin
    PWMCON1bits.PEN3H = 1; // set PWM3H as output pin
    PTPER = fcy / (fpwm * 1) - 1;

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
    tmr_setup_period(2, 5000);

    // Init Timer 2 to check for timeout
    IEC0bits.T2IE = 1;

    // Init LED
    TRISBbits.TRISB0 = 0; // LED D3
    LATBbits.LATB0 = 0;
    TRISBbits.TRISB1 = 0; // LED D4
    LATBbits.LATB1 = 0;


}

// Functions from previous assignments
// TIMER setup
void choose_prescaler(int ms, int* tckps, int* pr) {
    // Fcy = 1843200 Hz ?> 1843,2 clock ticks in 1 ms
    long ticks = 1843.2 * ms; // there can be an approximation
    if (ticks <= 65535) {// if ticks is > 65535 it cannot be put in PR1 (only 16 bits )
        *tckps = 0;
        *pr = ticks;
        return;
    }
    ticks = ticks / 8; // prescaler 1:8;
    if (ticks <= 65535) {
        *tckps = 1;
        *pr = ticks;
        return;
    }
    ticks = ticks / 8; // prescaler 1:64;
    if (ticks <= 65535) {
        *tckps = 2;
        *pr = ticks;
        return;
    }
    ticks = ticks / 4; // prescaler 1:256;
    *tckps = 3;
    *pr = ticks;
    return;
}

#define TIMER1 (1)
#define TIMER2 (2)
#define TIMER4 (4)

void tmr_setup_period(int n, int ms) {
    int tckps, pr;
    choose_prescaler(ms, &tckps, &pr);
    switch (n) {
        case TIMER1:
        {
            TMR1 = 0; // reset the current value;
            T1CONbits.TCKPS = tckps;
            PR1 = pr;
            T1CONbits.TON = 1;
            break;
        }
        case TIMER2:
        {
            TMR2 = 0; // reset the current value;
            T2CONbits.TCKPS = tckps;
            PR2 = pr;
            T2CONbits.TON = 1;
            break;
        }
        case TIMER4:
        {
            TMR4 = 0; // reset the current value;
            T4CONbits.TCKPS = tckps;
            PR4 = pr;
            T4CONbits.TON = 1;
            break;
        }
    }
}

void tmr_wait_period(int n) {
    switch (n) {
        case TIMER1:
        {
            while (IFS0bits .T1IF == 0) {
            }
            IFS0bits .T1IF = 0; // set to zero to be able to recognize the next time the timer has expired
            break;
        }
        case TIMER2:
        {
            while (IFS0bits .T2IF == 0) {
            }
            IFS0bits .T2IF = 0; // set to zero to be able to recognize the next time the timer has expired
            break;
        }
        case TIMER4:
        {
            while (IFS1bits .T4IF == 0) {
            }
            IFS1bits .T4IF = 0; // set to zero to be able to recognize the next time the timer has expired
            break;
        }
    }
}

// UART
#define BUFFER_SIZE 60

typedef struct {
    char buffer[BUFFER_SIZE];
    int readIndex;
    int writeIndex;
} CircularBuffer;

void write_buffer(volatile CircularBuffer* cb, char value) {
    cb->buffer[cb->writeIndex] = value;
    cb->writeIndex++;
    if (cb->writeIndex == BUFFER_SIZE)
        cb->writeIndex = 0;
}

int read_buffer(volatile CircularBuffer* cb, char* value) {
    IEC1bits.U2RXIE = 0;
    if (cb->readIndex == cb->writeIndex) {
        IEC1bits.U2RXIE = 1;
        return 0;
    }
    *value = cb->buffer[cb->readIndex];
    cb->readIndex++;
    if (cb->readIndex == BUFFER_SIZE)
        cb->readIndex = 0;
    IEC1bits.U2RXIE = 1;
    return 1;
}

int avl_in_buffer(volatile CircularBuffer* cb) {
    IEC1bits.U2RXIE = 0;
    int wri = cb->writeIndex;
    int rdi = cb->readIndex;
    IEC1bits.U2RXIE = 1;
    if (wri >= rdi) {
        return wri - rdi;
    } else {
        return wri - rdi + BUFFER_SIZE;
    }
}

// send a string character by character to the UART
void send_UART(char* str, int max) {
    int i = 0;
    for (i = 0; str [ i ] != '\0' && i < max; i++) {
        //check that the buffer is not full
        while(U2STAbits.UTXBF);
        // send the char back to the UART2
        U2TXREG = str[i];
    }
}


// SPI
void put_char_SPI(char c) {
    while (SPI1STATbits.SPITBF == 1); // wait for previous transmissions to finish
    SPI1BUF = c;
}

void write_string_LCD(char* str, int max) {
    int i = 0;
    for (i = 0; str [ i ] != '\0' && i < max; i++) {
        put_char_SPI(str[ i ]);
    }
}

void move_cursor_first_row() {
    put_char_SPI(0x80);
}

void move_cursor_second_row() {
    put_char_SPI(0xC0);
}

void clear_LCD(int n) {
    switch(n){
        case 1:
            move_cursor_first_row();
            int i = 0;
            for (i = 0; i < 16; i++) {
                put_char_SPI(' '); // write spaces to ?clear? the LCD from previous characters
            }
            break;
        case 2:
            move_cursor_second_row();
            for (i = 0; i < 16; i++) {
                put_char_SPI(' '); // write spaces to ?clear? the LCD from previous characters
            }
    }

}


#endif	/* XC_HEADER_TEMPLATE_H */