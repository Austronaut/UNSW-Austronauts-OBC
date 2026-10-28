#include "hal/hal_clock.h"

#include <stdint.h>

#include <msp430.h>

// Above 8 MHz, MCLK outruns FRAM unless wait states are configured in
// FRCTL0.NWAITS (FR5994 datasheet, recommended operating conditions;
// SLAU367 FRAM Controller chapter). This driver does not set NWAITS.
#if HAL_CLOCK_MCLK_HZ > 8000000UL
#error "MCLK above 8 MHz requires FRAM wait states (FRCTL0.NWAITS)"
#endif

// Cycles to wait after changing DCOFSEL before removing the /4 dividers.
// Value taken from TI's MSP430FR599x CS code examples.
#define DCO_SETTLE_CYCLES 60

// 0 until hal_clock_init() runs, so callers can detect a missing init.
static uint32_t smclk_hz = 0;

void hal_clock_init(void)
{
    // CSCTL0 upper byte is the CS password (CSKEY). Writing 0xA5 unlocks
    // writes to CSCTL1..CSCTL6 (SLAU367 3.3, CSCTL0).
    CSCTL0_H = CSKEY_H;

    // CSCTL2 source muxes (SLAU367 3.3, CSCTL2):
    //   SELA = VLOCLK  ACLK from the internal VLO; needs no crystal startup
    //   SELS = DCOCLK  SMCLK from the DCO
    //   SELM = DCOCLK  MCLK from the DCO
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Errata workaround (DCO frequency change, see FR5994 errata sheet):
    // changing DCOFSEL can briefly overshoot the target frequency, so divide
    // every clock by 4 while it changes. CSCTL3 DIVx (SLAU367 3.3, CSCTL3).
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;

    // CSCTL1 DCO frequency (SLAU367 3.3, CSCTL1; DCO table in FR5994
    // datasheet): DCORSEL = 1 selects the high range, DCOFSEL = 3 selects
    // 8 MHz within it.
    CSCTL1 = DCORSEL | DCOFSEL_3;

    __delay_cycles(DCO_SETTLE_CYCLES);

    // Remove the dividers: MCLK = SMCLK = DCO = 8 MHz.
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;

    // Any write other than 0xA5 to CSKEY re-locks the CS registers.
    CSCTL0_H = 0;

    smclk_hz = HAL_CLOCK_SMCLK_HZ;
}

uint32_t hal_clock_get_smclk_hz(void)
{
    return smclk_hz;
}
