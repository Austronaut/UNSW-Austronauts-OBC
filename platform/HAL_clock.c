#include <driverlib.h>
#include "hal_clock.h"

void HAL_Clock_Init(void)
{
    /* DCO ~1 MHz (low range, lowest select) - DriverLib handles the
     * CSCTL0_H password unlock/re-lock internally. */
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_0);

    /* Route MCLK and SMCLK from DCOCLK, no division. ACLK from REFOCLK. */
    CS_initClockSignal(CS_MCLK,  CS_DCOCLK_SELECT,  CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT,  CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK,  CS_VLOCLK_SELECT, CS_CLOCK_DIVIDER_1);
}
