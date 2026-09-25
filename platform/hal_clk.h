#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

/* One-time startup: unlock CS module, explicitly select DCO (~1 MHz,
 * DCOFSEL_0) as source for MCLK and SMCLK, no dividers, ACLK from
 * REFOCLK. Call once from main(), after WDT stop and before any
 * peripheral init (e.g. HAL_SPI_Init) that assumes a known SMCLK. */
void HAL_Clock_Init(void);

#endif /* HAL_CLOCK_H */
