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
 *
 * Policy:
 * - Frequencies are fixed after hal_clock_init(). Save power with low-power
 *   modes, which stop clocks, rather than by changing frequencies: drivers
 *   cache values derived from them (e.g. UART baud and frame timing).
 * - LPM3.5 and LPM4.5 reset all CS registers (SLAU367P 3.2.8), so call
 *   hal_clock_init() again after waking from them.
 * - Drivers read frequencies through the getters below. The HAL_CLOCK_*_HZ
 *   macros exist only for compile-time uses such as __delay_cycles().
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include <stdint.h>

#include "hal/hal_status.h"

/**
 * MCLK (CPU clock) frequency after hal_clock_init(), in Hz. For compile-time
 * uses only, such as __delay_cycles(); drivers use hal_clock_get_mclk_hz().
 */
#define HAL_CLOCK_MCLK_HZ   8000000UL

/**
 * SMCLK (peripheral clock) frequency after hal_clock_init(), in Hz. For
 * compile-time uses only; drivers use hal_clock_get_smclk_hz().
 */
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
 *
 * @retval HAL_STATUS_OK  The clocks are configured. This is the only value
 *                        returned today; the status exists because the
 *                        flight board's crystals can fail to start
 *                        (HAL_CONVENTIONS 7.1).
 */
hal_status_t hal_clock_init(void);

/**
 * @brief  Returns the current MCLK (CPU clock) frequency.
 *
 * @pre    None.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      Yes, once hal_clock_init() has returned; see
 *      hal_clock_get_smclk_hz().
 *
 * @return MCLK frequency in Hz, or 0 if hal_clock_init() has not been called.
 */
uint32_t hal_clock_get_mclk_hz(void);

/**
 * @brief  Returns the current SMCLK frequency.
 *
 * @pre    None.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      Yes, once hal_clock_init() has returned. The value is 32 bits, which
 *      this CPU reads in two 16-bit accesses, so a read during
 *      hal_clock_init() could see half the old and half the new value.
 *      hal_clock_init() runs once at startup, before interrupts are enabled.
 *
 * @return SMCLK frequency in Hz, or 0 if hal_clock_init() has not been called.
 */
uint32_t hal_clock_get_smclk_hz(void);

#endif // HAL_CLOCK_H
