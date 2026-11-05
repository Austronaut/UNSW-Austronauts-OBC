#include "hal/hal_clock.h"

#include <stdint.h>

#include <msp430.h>

// hal_clock_init() hardcodes register values for exactly 8 MHz (DCOFSEL_3,
// all dividers /1), so the macros cannot change on their own. Changing them
// means changing those writes too, and above 8 MHz also setting FRAM wait
// states in FRCTL0.NWAITS (SLASE54D 8.3, fSYSTEM).
#if HAL_CLOCK_MCLK_HZ != 8000000UL || HAL_CLOCK_SMCLK_HZ != 8000000UL
#error "hal_clock_init() only supports MCLK = SMCLK = 8 MHz"
#endif

// Cycles to wait after changing DCOFSEL before removing the /4 dividers.
// SLAZ681O CS12 needs >= 10 us; MCLK is DCO/4 = 2 MHz here, so 60 cycles
// is 30 us (still 15 us if the DCO is later raised to 16 MHz).
#define DCO_SETTLE_CYCLES 60

// Both 0 until hal_clock_init() runs, so callers can detect a missing init.
// No initialisers: static storage starts at 0, so these go in .bss rather
// than .data and need no stored initial values in FRAM.
static uint32_t mclk_hz;
static uint32_t smclk_hz;

hal_status_t hal_clock_init(void)
{
    // CSCTL0 upper byte is the CS password (CSKEY). Writing A5h unlocks
    // writes to the other CS registers (SLAU367P 3.3.1, Table 3-4).
    CSCTL0_H = CSKEY_H;

    // CSCTL2 source muxes (SLAU367P 3.3.3, Table 3-6):
    //   SELA = VLOCLK  ACLK from the internal VLO; needs no crystal startup
    //   SELS = DCOCLK  SMCLK from the DCO
    //   SELM = DCOCLK  MCLK from the DCO
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Erratum SLAZ681O CS12: changing DCOFSEL can overshoot the datasheet
    // frequency for up to 10 us, so divide every clock by 4 while it
    // changes. CSCTL3 DIVx (SLAU367P 3.3.4, Table 3-7).
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;

    // CSCTL1 DCO frequency (SLAU367P 3.3.2, Table 3-5): DCORSEL = 1 selects
    // the high range, DCOFSEL = 3 selects 8 MHz within it.
    // Accuracy is 8 MHz +/-3.5% (SLASE54D 8.12.3.3, fDCO8).
    CSCTL1 = DCORSEL | DCOFSEL_3;

    __delay_cycles(DCO_SETTLE_CYCLES);

    // Remove the dividers: MCLK = SMCLK = DCO = 8 MHz.
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;

    // Re-lock with a wrong key written as a BYTE to CSCTL0_H, as in TI's
    // SLAZ681O CS12 example. SLAU367P 3.3.1 Table 3-4 says any non-A5h
    // write causes a PUC; in practice byte writes only lock (cf. the
    // byte/word rule documented for PMMCTL0 in SLAU367P 2.3). Never write
    // a wrong key to CSCTL0 as a word.
    CSCTL0_H = 0;

    mclk_hz = HAL_CLOCK_MCLK_HZ;
    smclk_hz = HAL_CLOCK_SMCLK_HZ;
    return HAL_STATUS_OK;
}

uint32_t hal_clock_get_mclk_hz(void)
{
    return mclk_hz;
}

uint32_t hal_clock_get_smclk_hz(void)
{
    return smclk_hz;
}
