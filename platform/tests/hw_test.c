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
};

bool hw_test_run(uint8_t test_case)
{
    hw_test_failed_line = 0;

    if (test_case >= HW_TEST_CASE_COUNT) {
        hal_clock_init();
        hw_test_failed_line = __LINE__;
        return false;
    }

    return test_cases[test_case]();
}
