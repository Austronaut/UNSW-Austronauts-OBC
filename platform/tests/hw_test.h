/**
 * @file   hw_test.h
 * @brief  Self-checking hardware tests, run on the target from main().
 *
 * Select a case by pausing the target, setting hw_test_case from the
 * debugger, then restarting. Pause first: the debugger cannot write target
 * memory while the CPU is running, so a write made then is silently lost.
 * The value is kept in FRAM, so it survives Restart and resets until the
 * next reflash. Each case checks its own results; main() reports
 * pass or fail on the LED. On failure, hw_test_failed_line holds the line of
 * the first failed check in hw_test.c.
 *
 * Cases:
 *   0  UART init, 115200 8N1
 *   1  UART init, 9600 8N1
 *   2  UART init before clock init (traps in Debug builds)
 *   3  UART init on A1, unsupported
 *   4  UART init at 12345 baud, unsupported
 *   5  UART init with NULL config (traps in Debug builds)
 *   6  UART init with NULL config twice (traps twice in Debug builds)
 *   7  UART init, 9600 8E2
 *   8  UART init with an invalid parity value (traps in Debug builds)
 */

#ifndef HW_TEST_H
#define HW_TEST_H

#include <stdbool.h>
#include <stdint.h>

/** Number of test cases; valid case numbers are 0 to HW_TEST_CASE_COUNT - 1. */
#define HW_TEST_CASE_COUNT 9

/**
 * Case to run. Written by the developer from the debugger while the target
 * is paused. Kept in FRAM: it survives Restart and resets, and reflashing
 * sets it back to 0.
 */
extern volatile uint8_t hw_test_case;

/**
 * Line in hw_test.c of the first failed check, or 0 if no check failed.
 * Written only by hw_test.c.
 */
extern volatile uint16_t hw_test_failed_line;

/**
 * @brief  Runs one hardware test case and checks its results.
 *
 * @param[in]  test_case  Case to run, 0 to HW_TEST_CASE_COUNT - 1.
 *
 * @pre    The watchdog is stopped and GPIO is unlocked (LOCKLPM5 cleared).
 *         hal_clock_init() has NOT been called: each case controls clock
 *         init order itself.
 * @post   The clock is initialised. hw_test_failed_line is 0 on pass.
 *
 * @par Blocking
 *      Yes, for at most the duration of the selected case. Cases that hit
 *      a contract check hold in hal_debug_trap() in Debug builds until
 *      released from the debugger (HAL_CONVENTIONS 7.3).
 * @par ISR-safe
 *      No.
 *
 * @retval true   Every check in the case passed.
 * @retval false  A check failed (see hw_test_failed_line), or test_case is
 *                out of range.
 */
bool hw_test_run(uint8_t test_case);

#endif // HW_TEST_H
