#include <msp430.h>
#include "leds.h"

void leds_init() {
    P1DIR |= BIT0 + BIT1;			//P1.0 (Red) & P1.6 (Grn) -> Output

    P1OUT &= ~BIT0;					//Red LED -> OFF
    P1OUT &= ~BIT1;					//Green LED -> OFF
}

void leds_toggle() {
    P1OUT ^= (BIT0+BIT1);           //Toggle LEDs
}

void leds_set_red(bool on) {
    if (on) {
        P1OUT |= BIT0;	            //Red LED -> ON
    } else {
        P1OUT &= ~BIT0;             //Red LED -> OFF
    }
}

void leds_set_green(bool on) {
    if (on) {
        P1OUT |= BIT1;	            //Green LED -> ON
    } else {
        P1OUT &= ~BIT1;             //Green LED -> OFF
    }
}
