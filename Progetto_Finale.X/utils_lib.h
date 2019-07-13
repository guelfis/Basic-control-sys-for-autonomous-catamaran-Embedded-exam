/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  


// TIMER

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

void tmr1_setup_period(int ms) {
T1CONbits.TON = 0;
TMR1 = 0; // reset the current value;
int tckps, pr;
choose_prescaler(ms, &tckps, &pr);
T1CONbits.TCKPS = tckps;
PR1 = pr;
T1CONbits.TON = 1;
return;
}

void tmr1_wait_period() {
while (IFS0bits .T1IF == 0) {}
// I will exit the above loop only when the timer 1 peripheral has expired
// and it has set the T1IF flag to one
IFS0bits .T1IF = 0; // set to zero to be able to recognize the next time the timer has expired
}


void tmr2_wait_ms(int ms) {
T2CONbits.TON = 0;
int tckps, pr;
choose_prescaler(ms, &tckps, &pr);
TMR2 = 0;
T2CONbits.TCKPS = tckps;
PR2 = pr;
T2CONbits.TON = 1;
while (IFS0bits .T2IF == 0) {}
IFS0bits .T2IF = 0;
T2CONbits.TON = 0;
}

// Interrupts - different timer functions
#define TIMER1 (1)
#define TIMER2 (2)
void tmr_setup_period(int n, int ms) {
int tckps, pr;
choose_prescaler(ms, &tckps, &pr);
switch (n) {
case TIMER1: {
TMR1 = 0; // reset the current value;
T1CONbits.TCKPS = tckps;
PR1 = pr;
T1CONbits.TON = 1;
break;
}
case TIMER2: {
TMR2 = 0; // reset the current value;
T2CONbits.TCKPS = tckps;
PR2 = pr;
T2CONbits.TON = 1;
break;
}}}

void tmr_wait_period(int n) {
switch (n) {
case TIMER1: {
while (IFS0bits .T1IF == 0) {}
IFS0bits .T1IF = 0; // set to zero to be able to recognize the next time the timer has expired
break;
}
case TIMER2: {
while (IFS0bits .T2IF == 0) {}
IFS0bits .T2IF = 0; // set to zero to be able to recognize the next time the timer has expired
break;
}}}


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

void clear_LCD() {
    move_cursor_first_row();
    int i = 0;
    for (i = 0; i < 16; i++) {
        put_char_SPI(' '); // write spaces to ?clear? the LCD from previous characters
    }
    move_cursor_first_row();
}

// UART
#define BUFFER_SIZE 60
typedef struct {
char buffer[BUFFER_SIZE];
int readIndex;
int writeIndex;
} CircularBuffer;

volatile CircularBuffer cb;

void write_buffer( volatile CircularBuffer* cb, char value)
{
cb->buffer[cb->writeIndex] = value;
cb->writeIndex++;
if (cb->writeIndex == BUFFER_SIZE)
cb->writeIndex = 0;
}

// different in Task 1 and 2 --> here 2
int read_buffer( volatile CircularBuffer* cb, char* value) {
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

int avl_in_buffer ( volatile CircularBuffer* cb) {
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

// We have to continue after here 




#endif	/* XC_HEADER_TEMPLATE_H */