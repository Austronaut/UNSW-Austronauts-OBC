#include <msp430.h>
#include "button.h"

#define BUTTON_PIN BIT5   // P5.5, SW2

static void (*button_callback)(void) = 0; // holds the registered callback

void button_init(void)
{
    P5DIR &= ~BUTTON_PIN;   // input
    P5REN |= BUTTON_PIN;    // enable pull resistor
    P5OUT |= BUTTON_PIN;    // select pull-up
}

bool button_is_pressed(void)
{
    return (P5IN & BUTTON_PIN) == 0;   // pull-up: low means pressed
}

void button_interrupt_init(void (*callback)(void)) {
    button_callback = callback;
    button_init();
    P5IE |= BUTTON_PIN;     // Interupt Enable 
    P5IES |= BUTTON_PIN;    // Interupt Edge Select: falling edge (i.e moment of pressing)
    P5IFG &= ~BUTTON_PIN;
}

#pragma vector = PORT5_VECTOR
__interrupt void Port5_ISR(void)
{
    if (button_callback != 0)
    {
        button_callback();
    }
    P5IFG &= ~BUTTON_PIN;
}
