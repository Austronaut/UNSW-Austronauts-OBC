#include <msp430.h>
#include "clock.h"

void clock_init_8mhz(void)
{
    CSCTL0_H = CSKEY_H;                     // unlock Clock System registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // no division on any clock
    CSCTL0_H = 0;                           // lock Clock System registers
}