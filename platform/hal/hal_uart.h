/**
 * @file   hal_uart.h
 * @brief  Driver for the eUSCI_A UART peripherals.
 *
 * Supports eUSCI_A0 only, on P2.0 (TXD) and P2.1 (RXD). On the LaunchPad
 * this is the eZ-FET backchannel UART. A1 to A3 return HAL_ERR_UNSUPPORTED.
 *
 * Requires hal_clock_init() before hal_uart_init(), since baud settings are
 * selected by the SMCLK frequency.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>

#include "hal/hal_status.h"

/** eUSCI_A module to use. */
typedef enum {
    HAL_UART_INSTANCE_A0,
    HAL_UART_INSTANCE_A1,
    HAL_UART_INSTANCE_A2,
    HAL_UART_INSTANCE_A3,
    HAL_UART_INSTANCE_COUNT
} hal_uart_instance_t;

/** Parity bit added after the data bits. */
typedef enum {
    HAL_UART_PARITY_NONE,
    HAL_UART_PARITY_EVEN,
    HAL_UART_PARITY_ODD
} hal_uart_parity_t;

/** Number of stop bits. */
typedef enum {
    HAL_UART_STOP_BITS_1,
    HAL_UART_STOP_BITS_2
} hal_uart_stop_bits_t;

/** UART frame and speed settings. Data bits are always 8, LSB first. */
typedef struct {
    uint32_t             baud_rate_bps;
    hal_uart_parity_t    parity;
    hal_uart_stop_bits_t stop_bits;
} hal_uart_config_t;

/**
 * @brief  Configures an eUSCI_A instance as a UART and starts it.
 *
 * @param[in]  instance  UART to configure. Must be less than
 *                       HAL_UART_INSTANCE_COUNT.
 * @param[in]  config    Settings to apply. Must not be NULL. With SMCLK at
 *                       8 MHz, baud_rate_bps must be one of 9600, 19200,
 *                       38400, 57600, 115200, 230400 or 460800 (SLAU367P
 *                       Table 30-5).
 *
 * @pre    hal_clock_init() has been called.
 * @post   On HAL_OK, the instance's pins are muxed to the UART, the
 *         transmitter is idle and ready, and UART interrupts are disabled.
 *         Calling again reconfigures the instance and aborts any character
 *         in progress. On any error, the hardware is left unchanged.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      No.
 *
 * @retval HAL_OK                   The UART is configured and running.
 * @retval HAL_ERR_INVALID_ARG      instance is out of range, config is NULL,
 *                                  or config holds an invalid parity or
 *                                  stop-bit value.
 * @retval HAL_ERR_NOT_INITIALISED  hal_clock_init() has not been called.
 * @retval HAL_ERR_UNSUPPORTED      instance is not A0, or baud_rate_bps is
 *                                  not supported at the current SMCLK.
 */
hal_status_t hal_uart_init(hal_uart_instance_t instance,
                           const hal_uart_config_t *config);

#endif // HAL_UART_H
