#include <msp430.h>
#include "uart.h"

#include "clock.h"
#define UART_BAUD_RATE 9600

void uart_init(void)
{       
    P2SEL0 &= ~(BIT0 | BIT1);       // configuring pins as UART instead of GPIO
    P2SEL1 |= BIT0 | BIT1;          // P2.0/P2.1 -> UART function

    UCA0CTLW0 = UCSWRST;            // put eUSCI in reset before configuring
    UCA0CTLW0 |= UCSSEL__SMCLK;     // clock source = SMCLK (Sub-Main Clock)
    UCA0BRW = CLOCK_SMCLK_HZ / 16 / UART_BAUD_RATE; // computes baud rate
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900; // correction for minute baud rate timing error 
    UCA0CTLW0 &= ~UCSWRST;          // release from reset, start eUSCI
}

void uart_send_char(char c)
{
    while (!(UCA0IFG & UCTXIFG));   // wait until ready to transmit
    UCA0TXBUF = c;
}

void uart_send_string(char *text)
{
    unsigned int i = 0;
    while (text[i] != '\0')
    {
        uart_send_char(text[i]);
        i++;
    }
}
