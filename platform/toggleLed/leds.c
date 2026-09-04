#include <msp430.h>
#include "leds.h"

void leds_init() {
    P1DIR |= BIT0 + BIT1;			//P1.0 (Red) & P1.6 (Grn) -> Output

    P1OUT |= BIT0;					//Red LED -> ON
    P1OUT &= ~BIT1;					//Green LED -> OFF
}

void leds_toggle() {
    P1OUT ^= (BIT0+BIT1);           //Toggle LEDs
}
