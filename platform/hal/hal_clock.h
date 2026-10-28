/**
 * @file   hal_clock.h
 * @brief  Driver for the Clock System (CS) module.
 *
 * Configures the DCO to 8 MHz and sources MCLK and SMCLK from it, undivided.
 * ACLK is sourced from the VLO: 6 to 14 kHz, 9.4 kHz typical, uncalibrated
 * (SLASE54D 8.12.3.4). Do not use ACLK for accurate timing.
 *
 * Call hal_clock_init() once at startup, before initialising any module
 * whose timing depends on a clock frequency (e.g. hal_uart_init()).
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include <stdint.h>

/** MCLK (CPU clock) frequency after hal_clock_init(), in Hz. */
#define HAL_CLOCK_MCLK_HZ   8000000UL

/** SMCLK (peripheral clock) frequency after hal_clock_init(), in Hz. */
#define HAL_CLOCK_SMCLK_HZ  8000000UL

/**
 * @brief  Configures the DCO to 8 MHz and routes it to MCLK and SMCLK.
 *
 * @pre    None. Safe to call with the clock in any configuration, including
 *         the power-on default.
 * @post   MCLK = HAL_CLOCK_MCLK_HZ, SMCLK = HAL_CLOCK_SMCLK_HZ, ACLK = VLO.
 *         The CS registers are locked.
 *
 * @par Blocking
 *      Yes. Waits about 60 MCLK cycles for the DCO to settle.
 * @par ISR-safe
 *      No.
 */
void hal_clock_init(void);

/**
 * @brief  Returns the current SMCLK frequency.
 *
 * @pre    None.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      Yes.
 *
 * @return SMCLK frequency in Hz, or 0 if hal_clock_init() has not been called.
 */
uint32_t hal_clock_get_smclk_hz(void);

#endif // HAL_CLOCK_H
