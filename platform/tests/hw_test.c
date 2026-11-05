#include "tests/hw_test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <msp430.h>

#include "hal/hal_clock.h"
#include "hal/hal_status.h"
#include "hal/hal_uart.h"

// Stored in FRAM (.TI.persistent, which the linker places in writable
// memory) so it survives Restart and resets, and a case can be re-run
// without the debugger. Reflashing resets it to 0. PERSISTENT is TI-specific
// and needs an explicit initialiser; standard C has no way to survive a
// reset. Test code only: persistent state in flight code needs a design of
// its own.
#pragma PERSISTENT(hw_test_case)
volatile uint8_t hw_test_case = 0;

volatile uint16_t hw_test_failed_line;
volatile uint16_t hw_test_flush_cycles;

volatile uint16_t hw_probe_rises;
volatile uint16_t hw_probe_start_fall;
volatile uint16_t hw_probe_stop_rise;
volatile uint16_t hw_probe_busy_fall;
volatile uint16_t hw_probe_cpt[2];

// Records the failing line and ends the case.
#define CHECK(cond)                                 \
    do {                                            \
        if (!(cond)) {                              \
            hw_test_failed_line = __LINE__;         \
            return false;                           \
        }                                           \
    } while (0)

// Registers that hal_uart_init() writes for eUSCI_A0. Compared before and
// after a failing call to prove it left the hardware unchanged. A debugger
// restart does not always reset peripherals, so absolute reset values
// cannot be assumed.
typedef struct {
    uint16_t ctlw0;
    uint16_t brw;
    uint16_t mctlw;
    uint8_t  p2sel0;
    uint8_t  p2sel1;
} uart_a0_snapshot_t;

static uart_a0_snapshot_t snapshot_a0(void)
{
    uart_a0_snapshot_t s = {
        .ctlw0 = UCA0CTLW0,
        .brw = UCA0BRW,
        .mctlw = UCA0MCTLW,
        .p2sel0 = P2SEL0,
        .p2sel1 = P2SEL1,
    };
    return s;
}

static bool snapshots_equal(const uart_a0_snapshot_t *a, const uart_a0_snapshot_t *b)
{
    return a->ctlw0 == b->ctlw0 && a->brw == b->brw && a->mctlw == b->mctlw &&
           a->p2sel0 == b->p2sel0 && a->p2sel1 == b->p2sel1;
}

static bool pins_routed_to_a0(void)
{
    return (P2SEL1 & (BIT0 | BIT1)) == (BIT0 | BIT1) &&
           (P2SEL0 & (BIT0 | BIT1)) == 0;
}

static const hal_uart_config_t config_115200_8n1 = {
    .baud_rate_bps = 115200,
    .parity = HAL_UART_PARITY_NONE,
    .stop_bits = HAL_UART_STOP_BITS_1,
};

// Expected register values come from SLAU367P Table 30-5 and Table 30-8.

static bool test_init_115200_8n1(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_status_get_contract_violations() == violations);
    CHECK(UCA0CTLW0 == 0x0080);         // SMCLK, 8N1, out of reset
    CHECK(UCA0BRW == 0x0004);
    CHECK(UCA0MCTLW == 0x5551);         // UCBRS 0x55, UCBRF 5, UCOS16
    CHECK(pins_routed_to_a0());
    return true;
}

static bool test_init_9600_8n1(void)
{
    hal_uart_config_t config = config_115200_8n1;
    config.baud_rate_bps = 9600;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config) == HAL_STATUS_OK);
    CHECK(UCA0CTLW0 == 0x0080);
    CHECK(UCA0BRW == 0x0034);           // 52
    CHECK(UCA0MCTLW == 0x4911);         // UCBRS 0x49, UCBRF 1, UCOS16
    return true;
}

static bool test_init_before_clock(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    uart_a0_snapshot_t before = snapshot_a0();
    hal_status_t status = hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1);
    uart_a0_snapshot_t after = snapshot_a0();
    hal_clock_init();
    CHECK(status == HAL_STATUS_NOT_INITIALISED);
    CHECK(hal_status_get_contract_violations() == violations + 1);
    CHECK(snapshots_equal(&before, &after));
    return true;
}

static bool test_init_unsupported_instance(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    uart_a0_snapshot_t before = snapshot_a0();
    uint16_t a1_ctlw0 = UCA1CTLW0;
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A1, &config_115200_8n1) == HAL_STATUS_UNSUPPORTED);
    uart_a0_snapshot_t after = snapshot_a0();
    CHECK(hal_status_get_contract_violations() == violations);   // runtime result, not a bug
    CHECK(snapshots_equal(&before, &after));
    CHECK(UCA1CTLW0 == a1_ctlw0);
    return true;
}

static bool test_init_unsupported_baud(void)
{
    hal_uart_config_t config = config_115200_8n1;
    config.baud_rate_bps = 12345;
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    uart_a0_snapshot_t before = snapshot_a0();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config) == HAL_STATUS_UNSUPPORTED);
    uart_a0_snapshot_t after = snapshot_a0();
    CHECK(hal_status_get_contract_violations() == violations);
    CHECK(snapshots_equal(&before, &after));
    return true;
}

static bool test_init_null_config(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    uart_a0_snapshot_t before = snapshot_a0();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, NULL) == HAL_STATUS_INVALID_ARG);
    uart_a0_snapshot_t after = snapshot_a0();
    CHECK(hal_status_get_contract_violations() == violations + 1);
    CHECK(snapshots_equal(&before, &after));
    return true;
}

// Proves the debug trap re-arms: in Debug builds it must hold twice.
static bool test_init_null_config_twice(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, NULL) == HAL_STATUS_INVALID_ARG);
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, NULL) == HAL_STATUS_INVALID_ARG);
    CHECK(hal_status_get_contract_violations() == violations + 2);
    return true;
}

static bool test_init_9600_8e2(void)
{
    hal_uart_config_t config = {
        .baud_rate_bps = 9600,
        .parity = HAL_UART_PARITY_EVEN,
        .stop_bits = HAL_UART_STOP_BITS_2,
    };
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config) == HAL_STATUS_OK);
    CHECK(UCA0CTLW0 == 0xC880);         // UCPEN | UCPAR (even) | UCSPB | SMCLK
    CHECK(UCA0BRW == 0x0034);
    CHECK(UCA0MCTLW == 0x4911);
    return true;
}

static bool test_init_invalid_parity(void)
{
    hal_uart_config_t config = config_115200_8n1;
    config.parity = (hal_uart_parity_t)7;   // deliberately not a valid value
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    uart_a0_snapshot_t before = snapshot_a0();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config) == HAL_STATUS_INVALID_ARG);
    uart_a0_snapshot_t after = snapshot_a0();
    CHECK(hal_status_get_contract_violations() == violations + 1);
    CHECK(snapshots_equal(&before, &after));
    return true;
}

// Generous budget for cases that expect a write or flush to complete: about
// 100 loops per byte at 9600 8E2, so this covers well over 100 bytes.
#define WRITE_TIMEOUT_LOOPS 20000

// Ten bytes, so the 100-byte string below is easy to count.
#define TEN_X "xxxxxxxxxx"

static bool test_write_ok(void)
{
    static const char text[] = "hello\r\n";
    size_t written = 0;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    uint16_t violations = hal_status_get_contract_violations();
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                         sizeof(text) - 1, WRITE_TIMEOUT_LOOPS, &written) == HAL_STATUS_OK);
    CHECK(written == sizeof(text) - 1);
    CHECK(hal_status_get_contract_violations() == violations);
    return true;
}

static bool test_write_before_init(void)
{
    static const char text[] = "x";
    size_t written = 99;                    // must be overwritten with 0
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text, 1,
                         WRITE_TIMEOUT_LOOPS, &written) == HAL_STATUS_NOT_INITIALISED);
    CHECK(written == 0);
    CHECK(hal_status_get_contract_violations() == violations + 1);
    return true;
}

// NULL with length 0 is a valid empty write, not a programmer error.
static bool test_write_empty(void)
{
    size_t written = 99;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    uint16_t violations = hal_status_get_contract_violations();
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, NULL, 0, 0, &written) == HAL_STATUS_OK);
    CHECK(written == 0);
    CHECK(hal_status_get_contract_violations() == violations);
    return true;
}

// Runs straight after init so nothing else is being sent. TXBUF accepts the
// first byte at once; the second only fits if the first has already moved
// to the shift register (SLAU367P 30.3.8), so 1 or 2 bytes are accepted.
static bool test_write_no_wait(void)
{
    static const char text[] = "0123456789";
    size_t written = 0;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                         sizeof(text) - 1, 0, &written) == HAL_STATUS_TIMEOUT);
    CHECK(written >= 1 && written <= 2);
    return true;
}

// 50 loops is about 4100 cycles, enough for only a handful of bytes at
// 115200 baud. If the budget were wrongly applied per byte, all 100 would
// be accepted, since each byte needs only about 9 loops.
static bool test_write_budget_per_call(void)
{
    static const char text[] = TEN_X TEN_X TEN_X TEN_X TEN_X
                               TEN_X TEN_X TEN_X TEN_X TEN_X;
    size_t written = 0;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                         sizeof(text) - 1, 50, &written) == HAL_STATUS_TIMEOUT);
    CHECK(written > 2 && written < sizeof(text) - 1);
    return true;
}

static bool uart_a0_idle(void)
{
    return (UCA0STATW & UCBUSY) == 0;
}

// Nothing written since init, so a zero-budget flush must succeed at once.
// Don't type in the terminal while it runs: received data sets UCBUSY.
static bool test_flush_idle(void)
{
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, 0) == HAL_STATUS_OK);
    CHECK(uart_a0_idle());
    return true;
}

static bool test_flush_after_write(void)
{
    static const char text[] = "flush\r\n";
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                         sizeof(text) - 1, WRITE_TIMEOUT_LOOPS, NULL) == HAL_STATUS_OK);
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, WRITE_TIMEOUT_LOOPS) == HAL_STATUS_OK);
    CHECK(uart_a0_idle());
    CHECK((UCA0IFG & UCTXIFG) != 0);
    return true;
}

// A byte takes about 700 cycles at 115200; flush(0) runs long before that.
static bool test_flush_no_wait(void)
{
    static const char text[] = "z";
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config_115200_8n1) == HAL_STATUS_OK);
    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text, 1,
                         WRITE_TIMEOUT_LOOPS, NULL) == HAL_STATUS_OK);
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, 0) == HAL_STATUS_TIMEOUT);
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, WRITE_TIMEOUT_LOOPS) == HAL_STATUS_OK);
    return true;
}

// Re-init sets UCSWRST, which cuts off anything still sending (SLAU367P
// 30.3.1). If flush returned early, the last character of a line would be
// lost or garbled, so the terminal shows the result as well as the checks.
static bool flush_then_reinit(const hal_uart_config_t *config)
{
    static const char text[] = "ABCDEFGH\r\n";
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, config) == HAL_STATUS_OK);
    for (uint8_t i = 0; i < 5; i++) {
        CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text,
                             sizeof(text) - 1, WRITE_TIMEOUT_LOOPS, NULL) == HAL_STATUS_OK);
        CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, WRITE_TIMEOUT_LOOPS) == HAL_STATUS_OK);
        CHECK(uart_a0_idle());
        CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, config) == HAL_STATUS_OK);
    }
    return true;
}

static bool test_flush_then_reinit_115200_8n1(void)
{
    return flush_then_reinit(&config_115200_8n1);
}

// Prints at 9600, so the 115200 terminal shows garbage for this case.
static bool test_flush_then_reinit_9600_8e2(void)
{
    static const hal_uart_config_t config = {
        .baud_rate_bps = 9600,
        .parity = HAL_UART_PARITY_EVEN,
        .stop_bits = HAL_UART_STOP_BITS_2,
    };
    return flush_then_reinit(&config);
}

// Measures flush with Timer_A0 counting SMCLK, the clock the UART also
// uses, so DCO error cancels. At 9600 8N1 one bit is about 833 cycles,
// much longer than one flush loop (about 82), so returning a bit early is
// measurable.
//
// Two bytes are written and the timer starts when UCTXIFG sets again for
// the second byte. Case 23 measured that moment on the TX pin: it is the
// END of that byte's start bit, so 9 bits (8 data + stop, about 7500
// cycles) remain. Flush must not return before they have gone. Returning
// at the start of the stop bit would read about 6670. A flush that ignored
// UCBUSY and always waited its full frame time would read about 11,600.
static bool test_flush_timing_9600(void)
{
    static const char text[] = "UU";
    hal_uart_config_t config = config_115200_8n1;
    config.baud_rate_bps = 9600;
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, &config) == HAL_STATUS_OK);

    TA0CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;

    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text, 2,
                         WRITE_TIMEOUT_LOOPS, NULL) == HAL_STATUS_OK);
    while ((UCA0IFG & UCTXIFG) == 0) {
    }
    uint16_t start = TA0R;
    hal_status_t status = hal_uart_flush(HAL_UART_INSTANCE_A0, WRITE_TIMEOUT_LOOPS);
    uint16_t elapsed = (uint16_t)(TA0R - start);

    TA0CTL = MC__STOP;
    hw_test_flush_cycles = elapsed;

    CHECK(status == HAL_STATUS_OK);
    CHECK(elapsed >= 7400);                 // stop bit ends at about 7486
    CHECK(elapsed <= 7486 + 400);           // UCBUSY exit, not the fallback
    return true;
}

static bool test_flush_before_init(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A0, 0) == HAL_STATUS_NOT_INITIALISED);
    CHECK(hal_status_get_contract_violations() == violations + 1);
    return true;
}

static bool test_flush_invalid_instance(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_flush((hal_uart_instance_t)7, 0) == HAL_STATUS_INVALID_ARG);
    CHECK(hal_status_get_contract_violations() == violations + 1);
    return true;
}

// A1's init returns UNSUPPORTED, so it is never initialised.
static bool test_flush_unsupported_instance(void)
{
    uint16_t violations = hal_status_get_contract_violations();
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A1, &config_115200_8n1) == HAL_STATUS_UNSUPPORTED);
    CHECK(hal_uart_flush(HAL_UART_INSTANCE_A1, 0) == HAL_STATUS_NOT_INITIALISED);
    CHECK(hal_status_get_contract_violations() == violations + 1);
    return true;
}

// Guards the UCBUSY timing hal_uart_flush() relies on, measured against the
// real TX pin: UCBUSY must not fall before the last stop bit has ended.
//
// Sends "UU" (one bit is about 833 SMCLK cycles at 9600). 'U' is 0x55,
// sent LSB first as start 0, data 1 0 1 0 1 0 1 0, so the pin changes on
// every data bit. Its parity bit is 0 (four 1s, even parity), so with or
// without parity the last rising edge is the start of the first stop bit.
// Time 0 is when UCTXIFG sets for the second byte, which this case first
// measured to be the END of that byte's start bit. A tight loop then
// timestamps, with Timer_A0 on SMCLK:
//   hw_probe_start_fall  first falling edge on P2.0
//   hw_probe_stop_rise   last rising edge on P2.0 (start of the stop bits)
//   hw_probe_busy_fall   UCBUSY first reads 0
//   hw_probe_cpt[0..1]   each time UCTXCPTIFG sets (recorded only; cleared
//                        after each sighting, so two can be seen)
// 0xFFFF means not seen. Reading P2IN while P2.0 is muxed to the UART is
// not documented (SLAU367P 12.2.1); hw_probe_rises >= 4 shows it works.
// First measured at 8N1: stop bit 6653 to about 7486, UCBUSY and
// UCTXCPTIFG both at 7489.
#define BIT_CYCLES_9600     833u
#define PROBE_TOLERANCE     100u        // sampling period plus bit rounding

static bool probe_tx_flag_timing(const hal_uart_config_t *config, uint16_t stop_bits)
{
    static const char text[] = "UU";
    hal_clock_init();
    CHECK(hal_uart_init(HAL_UART_INSTANCE_A0, config) == HAL_STATUS_OK);

    hw_probe_rises = 0;
    hw_probe_start_fall = 0xFFFF;
    hw_probe_stop_rise = 0xFFFF;
    hw_probe_busy_fall = 0xFFFF;
    hw_probe_cpt[0] = 0xFFFF;
    hw_probe_cpt[1] = 0xFFFF;

    TA0CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;

    CHECK(hal_uart_write(HAL_UART_INSTANCE_A0, (const uint8_t *)text, 2,
                         WRITE_TIMEOUT_LOOPS, NULL) == HAL_STATUS_OK);
    while ((UCA0IFG & UCTXIFG) == 0) {
    }
    uint16_t t0 = TA0R;
    UCA0IFG &= ~UCTXCPTIFG;                 // drop any flag from byte 1
    uint8_t last_pin = P2IN & BIT0;
    uint8_t cpt_seen = 0;

    for (;;) {
        uint16_t t = (uint16_t)(TA0R - t0);
        uint8_t pin = P2IN & BIT0;
        if (pin != last_pin) {
            if (pin != 0) {
                hw_probe_rises++;
                hw_probe_stop_rise = t;
            } else if (hw_probe_start_fall == 0xFFFF) {
                hw_probe_start_fall = t;
            }
            last_pin = pin;
        }
        if (hw_probe_busy_fall == 0xFFFF && (UCA0STATW & UCBUSY) == 0) {
            hw_probe_busy_fall = t;
        }
        if (cpt_seen < 2 && (UCA0IFG & UCTXCPTIFG) != 0) {
            hw_probe_cpt[cpt_seen++] = t;
            UCA0IFG &= ~UCTXCPTIFG;
        }
        if (t > 14000) {                    // well past the last stop bit
            break;
        }
    }
    TA0CTL = MC__STOP;

    CHECK(hw_probe_rises >= 4);             // P2IN really follows the TX pin
    CHECK(hw_probe_stop_rise != 0xFFFF);
    CHECK(hw_probe_busy_fall != 0xFFFF);

    uint16_t stop_end = hw_probe_stop_rise + stop_bits * BIT_CYCLES_9600;
    CHECK(hw_probe_busy_fall + PROBE_TOLERANCE >= stop_end);    // not early
    CHECK(hw_probe_busy_fall <= stop_end + 3 * PROBE_TOLERANCE); // not stuck
    return true;
}

static bool test_probe_tx_timing_9600_8n1(void)
{
    hal_uart_config_t config = config_115200_8n1;
    config.baud_rate_bps = 9600;
    return probe_tx_flag_timing(&config, 1);
}

// Checks UCBUSY waits for BOTH stop bits, not just the first.
static bool test_probe_tx_timing_9600_8e2(void)
{
    static const hal_uart_config_t config = {
        .baud_rate_bps = 9600,
        .parity = HAL_UART_PARITY_EVEN,
        .stop_bits = HAL_UART_STOP_BITS_2,
    };
    return probe_tx_flag_timing(&config, 2);
}

// Indexed by case number; order must match the list in hw_test.h.
static bool (*const test_cases[HW_TEST_CASE_COUNT])(void) = {
    test_init_115200_8n1,
    test_init_9600_8n1,
    test_init_before_clock,
    test_init_unsupported_instance,
    test_init_unsupported_baud,
    test_init_null_config,
    test_init_null_config_twice,
    test_init_9600_8e2,
    test_init_invalid_parity,
    test_write_ok,
    test_write_before_init,
    test_write_empty,
    test_write_no_wait,
    test_write_budget_per_call,
    test_flush_idle,
    test_flush_after_write,
    test_flush_no_wait,
    test_flush_then_reinit_115200_8n1,
    test_flush_then_reinit_9600_8e2,
    test_flush_timing_9600,
    test_flush_before_init,
    test_flush_invalid_instance,
    test_flush_unsupported_instance,
    test_probe_tx_timing_9600_8n1,
    test_probe_tx_timing_9600_8e2,
};

// Waits, at register level, for eUSCI_A0 to go idle, so main()'s console
// re-init cannot cut off a test's last bytes. Bounded, since received data
// can also hold UCBUSY. Runs after every case, including failed ones.
static void drain_uart_a0(void)
{
    if ((UCA0CTLW0 & UCSWRST) != 0) {
        return;                             // in reset: nothing is sending
    }
    for (uint16_t i = 0; i < 20000 && (UCA0STATW & UCBUSY) != 0; i++) {
        __delay_cycles(64);
    }
}

bool hw_test_run(uint8_t test_case)
{
    hw_test_failed_line = 0;

    if (test_case >= HW_TEST_CASE_COUNT) {
        hal_clock_init();
        hw_test_failed_line = __LINE__;
        return false;
    }

    bool passed = test_cases[test_case]();
    drain_uart_a0();
    return passed;
}
