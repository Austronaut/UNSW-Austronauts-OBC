/**
 * @file   hal_uart.h
 * @brief  Driver for the eUSCI_A UART peripherals.
 *
 * Supports eUSCI_A0 only, on P2.0 (TXD) and P2.1 (RXD). On the LaunchPad
 * this is the eZ-FET backchannel UART. A1 to A3 return HAL_STATUS_UNSUPPORTED.
 *
 * Requires hal_clock_init() before hal_uart_init(), since baud settings are
 * selected by the SMCLK frequency.
 *
 * Transmission is queue-and-flush: hal_uart_write() returns once bytes are
 * accepted for sending, not once they are on the wire. Today it polls the
 * hardware; a later interrupt-driven version will accept bytes into a buffer
 * instead, without changing callers.
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include <stddef.h>
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
 * @post   On HAL_STATUS_OK, the instance's pins are muxed to the UART, the
 *         transmitter is idle and ready, and UART interrupts are disabled.
 *         Calling again reconfigures the instance and aborts any character
 *         in progress. On any error, the hardware is left unchanged.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      No.
 *
 * @retval HAL_STATUS_OK               The UART is configured and running.
 * @retval HAL_STATUS_INVALID_ARG      instance is out of range, config is
 *                                     NULL, or config holds an invalid
 *                                     parity or stop-bit value.
 * @retval HAL_STATUS_NOT_INITIALISED  hal_clock_init() has not been called.
 * @retval HAL_STATUS_UNSUPPORTED      instance is not A0, or baud_rate_bps
 *                                     is not supported at the current SMCLK.
 */
hal_status_t hal_uart_init(hal_uart_instance_t instance,
                           const hal_uart_config_t *config);

/**
 * Minimum MCLK cycles one timeout loop takes in hal_uart_write(). A loop
 * also includes a few cycles of polling overhead; see each contract.
 */
#define HAL_UART_LOOP_CYCLES 64

/**
 * @brief  Queues bytes for transmission on a UART.
 *
 * Returns once every byte has been accepted for sending. Accepted bytes may
 * still be in transit when this returns; call hal_uart_flush() to wait until
 * they have left the pin.
 *
 * @param[in]  instance       UART to write to. Must be less than
 *                            HAL_UART_INSTANCE_COUNT.
 * @param[in]  data           Bytes to send. Must not be NULL unless
 *                            length_bytes is 0.
 * @param[in]  length_bytes   Number of bytes to send. 0 sends nothing.
 * @param[in]  timeout_loops  Waiting budget for the whole call, in loops of
 *                            at least HAL_UART_LOOP_CYCLES MCLK cycles. 0
 *                            accepts only what fits without waiting.
 * @param[out] bytes_written  If not NULL, set on every return to the number
 *                            of bytes accepted, including 0 on error.
 *
 * @pre    hal_uart_init() has returned HAL_STATUS_OK for this instance.
 * @post   The first *bytes_written bytes of data will be sent, in order.
 *
 * @par Blocking
 *      Yes. Waits while the transmitter is busy, at most timeout_loops
 *      loops of about HAL_UART_LOOP_CYCLES + 20 MCLK cycles each (about
 *      10 us at 8 MHz; overhead measured from the Debug build). One byte
 *      takes about 10 bit times (about 700 MCLK cycles at 115200 baud), so
 *      a full write needs about 9 loops per byte at that rate.
 * @par ISR-safe
 *      No. Not reentrant: calls for the same instance must not overlap.
 *
 * @retval HAL_STATUS_OK               All length_bytes bytes were accepted.
 * @retval HAL_STATUS_INVALID_ARG      instance is out of range, or data is
 *                                     NULL with length_bytes above 0.
 * @retval HAL_STATUS_NOT_INITIALISED  instance has not been initialised.
 * @retval HAL_STATUS_TIMEOUT          The budget ran out first; only the
 *                                     first *bytes_written bytes were
 *                                     accepted.
 */
hal_status_t hal_uart_write(hal_uart_instance_t instance,
                            const uint8_t *data, size_t length_bytes,
                            uint32_t timeout_loops, size_t *bytes_written);

#endif // HAL_UART_H
