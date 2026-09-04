#include <msp430.h>
#include "button.h"

void button_init(void)
{
    P5DIR &= ~BIT5;   // input
    P5REN |= BIT5;    // enable pull resistor
    P5OUT |= BIT5;    // select pull-up
}

bool button_is_pressed(void)
{
    return (P5IN & BIT5) == 0;   // pull-up: low means pressed
}
