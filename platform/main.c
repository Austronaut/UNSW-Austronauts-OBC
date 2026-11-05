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
//  Runs one self-checking hardware test (tests/hw_test.h), then blinks the
//  LED on P1.0 at 1 Hz if it passed or 5 Hz if it failed. Select the case by
//  pausing, setting hw_test_case from the debugger, then restarting.
//
//  The result is also printed on eUSCI_A0 (the LaunchPad backchannel UART)
//  at 115200 8N1, followed by a "tick N" line every second.
//
//           MSP430FR5994
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |           P1.0|-->LED
//        |           P2.0|-->UCA0TXD (backchannel to PC)
//
//  Originally derived from TI's MSP430FR5x9x P1.0 toggle example.
//******************************************************************************
#include <msp430.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hal/hal_clock.h"
#include "hal/hal_status.h"
#include "hal/hal_uart.h"
#include "tests/hw_test.h"

// Console speed. Must match the terminal, e.g. `screen <port> 115200`.
#define CONSOLE_BAUD_BPS 115200UL

// Enough for any console string at 9600 baud or faster (about 100 loops
// per byte at 9600, 9 at 115200); about 0.2 s if the UART never frees up.
#define CONSOLE_TIMEOUT_LOOPS 20000

// File scope and volatile so the debugger can always read them.
static volatile bool test_passed;
static volatile bool console_ready;

static const hal_uart_config_t console_config = {
    .baud_rate_bps = CONSOLE_BAUD_BPS,
    .parity = HAL_UART_PARITY_NONE,
    .stop_bits = HAL_UART_STOP_BITS_1,
};

static void console_write_str(const char *text)
{
    if (console_ready) {
        (void)hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                             strlen(text), CONSOLE_TIMEOUT_LOOPS, NULL);
    }
}

// Prints value in decimal. 16-bit keeps the division cheap: the MSP430 has
// no hardware divider, so every division is a library call.
static void console_write_u16(uint16_t value)
{
    char digits[6];                         // "65535" plus terminator
    char *p = &digits[sizeof(digits) - 1];
    *p = '\0';
    do {
        *--p = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    console_write_str(p);
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop WDT

    // Configure GPIO
    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    uint8_t test_case = hw_test_case;
    test_passed = hw_test_run(test_case);

    // hw_test_run() leaves the UART idle, so re-initialising it for the
    // console (a test may have left it at another baud rate) cuts nothing.
    console_ready = hal_uart_init(HAL_UART_INSTANCE_A0, &console_config) == HAL_STATUS_OK;

    console_write_str("\r\ncase ");
    console_write_u16(test_case);
    if (test_passed) {
        console_write_str(": PASS\r\n");
    } else {
        console_write_str(": FAIL line ");
        console_write_u16(hw_test_failed_line);
        console_write_str("\r\n");
    }

    uint16_t tick = 0;
    uint8_t toggles = 0;
    while(1)
    {
        P1OUT ^= BIT0;                      // Toggle LED
        if (test_passed) {
            __delay_cycles(HAL_CLOCK_MCLK_HZ / 2);   // 0.5 s: 1 Hz blink
        } else {
            __delay_cycles(HAL_CLOCK_MCLK_HZ / 10);  // 0.1 s: 5 Hz blink
        }

        // One tick per second whatever the blink rate: a toggle is 0.5 s at
        // 1 Hz (2 per second) and 0.1 s at 5 Hz (10 per second).
        toggles++;
        if (toggles >= (test_passed ? 2 : 10)) {
            toggles = 0;
            tick++;
            console_write_str("tick ");
            console_write_u16(tick);
            console_write_str("\r\n");
        }
    }
}
