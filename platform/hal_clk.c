#include <msp430.h>
#include "hal_clock.h"

void HAL_Clock_Init(void)
{
    CSCTL0_H = CSKEY_H;                     /* unlock CS registers (password 0xA5) */

    CSCTL1 = DCOFSEL_0;                     /* DCO ~1 MHz - explicit, matches hal_spi.c's
                                              * ASSUMED_SMCLK_HZ. Change both together. */

    CSCTL2 = SELA__REFOCLK                  /* ACLK  = REFOCLK (~32.768 kHz) */
           | SELS__DCOCLK                   /* SMCLK = DCOCLK */
           | SELM__DCOCLK;                  /* MCLK  = DCOCLK */

    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   /* no dividers on any of the three */

    CSCTL0_H = 0;                            /* re-lock CS registers */
}
