#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Configure GPIO
    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction

	PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode

    P1OUT &= ~BIT1;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= BIT1;                          // Set P1.0 to output direction

	P5DIR &= ~BIT5;							// P5.5 - set it to input
	P5REN |= BIT5;							// Pull up down enable
	P5OUT |= BIT5;							// Pull up enable
	
	P5DIR &= ~BIT6;							// P5.6 - set it to input
	P5REN |= BIT6;							// Pull up down enable
	P5OUT |= BIT6;							// Pull up enable
	
	while (1) {
		if (!(P5IN & BIT5)) {
			P1OUT |= BIT0;
		} else {
			P1OUT &= ~BIT0;
		}

		if (!(P5IN & BIT6)) {
			P1OUT |= BIT1;
		} else {
			P1OUT &= ~BIT1;
		}
	}
}
