/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  OBC platform entry point.
//
//  Stage 2 check: UART init. Initialises eUSCI_A0, then blinks the LED on
//  P1.0 at 1 Hz if hal_uart_init() returned HAL_OK, or at 5 Hz if it
//  returned an error. UART_INIT_TEST_CASE selects which case to exercise.
//
//           MSP430FR5994
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |           P1.0|-->LED
//
//  Originally derived from TI's MSP430FR5x9x P1.0 toggle example.
//******************************************************************************
#include <msp430.h>

#include <stddef.h>

#include "hal/hal_clock.h"
#include "hal/hal_uart.h"

// 0: 115200 8N1, expect HAL_OK
// 1: 9600 8N1, expect HAL_OK
// 2: UART init before clock init, expect trap, then HAL_ERR_NOT_INITIALISED
// 3: instance A1, expect HAL_ERR_UNSUPPORTED (no trap)
// 4: 12345 baud, expect HAL_ERR_UNSUPPORTED (no trap)
// 5: NULL config, expect trap, then HAL_ERR_INVALID_ARG
// 6: NULL config twice, expect two separate traps (re-arm), then HAL_ERR_INVALID_ARG
#define UART_INIT_TEST_CASE 0

// File scope and volatile so the debugger can always read it.
static volatile hal_status_t uart_status;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop WDT

    // Configure GPIO
    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    hal_uart_config_t config = {
        .baud_rate_bps = 115200,
        .parity = HAL_UART_PARITY_NONE,
        .stop_bits = HAL_UART_STOP_BITS_1,
    };
    const hal_uart_config_t *config_ptr = &config;
    hal_uart_instance_t instance = HAL_UART_INSTANCE_A0;

#if UART_INIT_TEST_CASE == 1
    config.baud_rate_bps = 9600;
#elif UART_INIT_TEST_CASE == 3
    instance = HAL_UART_INSTANCE_A1;
#elif UART_INIT_TEST_CASE == 4
    config.baud_rate_bps = 12345;
#elif UART_INIT_TEST_CASE == 5 || UART_INIT_TEST_CASE == 6
    config_ptr = NULL;
#endif

#if UART_INIT_TEST_CASE != 2
    hal_clock_init();
#endif
#if UART_INIT_TEST_CASE == 6
    uart_status = hal_uart_init(instance, config_ptr);  // first trap
#endif
    uart_status = hal_uart_init(instance, config_ptr);
#if UART_INIT_TEST_CASE == 2
    hal_clock_init();                       // late, so the blink timing is still right
#endif

    while(1)
    {
        P1OUT ^= BIT0;                      // Toggle LED
        if (uart_status == HAL_OK) {
            __delay_cycles(HAL_CLOCK_MCLK_HZ / 2);   // 0.5 s: 1 Hz blink
        } else {
            __delay_cycles(HAL_CLOCK_MCLK_HZ / 10);  // 0.1 s: 5 Hz blink
        }
    }
}
