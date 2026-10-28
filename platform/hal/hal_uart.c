#include "hal/hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include <msp430.h>

#include "hal/hal_clock.h"

// Packs the UCAxMCTLW fields (SLAU367P 30.4.4): UCBRSx in bits 15-8,
// UCBRFx in bits 7-4, UCOS16 in bit 0.
#define MCTLW(ucbrs, ucbrf, ucos16) \
    ((uint16_t)(((ucbrs) << 8) | ((ucbrf) << 4) | (ucos16)))

// One row of SLAU367P Table 30-5 (Recommended Settings for Typical
// Crystals and Baud Rates), pre-packed into register values.
typedef struct {
    uint32_t smclk_hz;
    uint32_t baud_rate_bps;
    uint16_t brw;       // UCAxBRW: prescaler UCBRx (SLAU367P 30.4.3)
    uint16_t mctlw;     // UCAxMCTLW: modulation and oversampling
} baud_setting_t;

// Copied row for row from SLAU367P Table 30-5, BRCLK = 8000000. The 460800
// row uses low-frequency mode (UCOS16 = 0), where UCBRFx is unused.
static const baud_setting_t baud_settings[] = {
    { 8000000UL,   9600UL, 52, MCTLW(0x49,  1, 1) },
    { 8000000UL,  19200UL, 26, MCTLW(0xB6,  0, 1) },
    { 8000000UL,  38400UL, 13, MCTLW(0x84,  0, 1) },
    { 8000000UL,  57600UL,  8, MCTLW(0xF7, 10, 1) },
    { 8000000UL, 115200UL,  4, MCTLW(0x55,  5, 1) },
    { 8000000UL, 230400UL,  2, MCTLW(0xBB,  2, 1) },
    { 8000000UL, 460800UL, 17, MCTLW(0x4A,  0, 0) },
};

// Register block base address of each eUSCI_A instance (msp430fr5994.h).
// Every instance has the same register layout, so the OFS_UCA0xxx offsets
// apply to all of them.
static const uint16_t base_addresses[HAL_UART_INSTANCE_COUNT] = {
    EUSCI_A0_BASE,
    EUSCI_A1_BASE,
    EUSCI_A2_BASE,
    EUSCI_A3_BASE,
};

static volatile uint16_t *reg(hal_uart_instance_t instance, uint16_t offset)
{
    return (volatile uint16_t *)(uintptr_t)(base_addresses[instance] + offset);
}

static const baud_setting_t *find_baud_setting(uint32_t smclk_hz,
                                               uint32_t baud_rate_bps)
{
    size_t i;

    for (i = 0; i < sizeof(baud_settings) / sizeof(baud_settings[0]); i++) {
        if (baud_settings[i].smclk_hz == smclk_hz &&
            baud_settings[i].baud_rate_bps == baud_rate_bps) {
            return &baud_settings[i];
        }
    }
    return NULL;
}

// Routes P2.0 to UCA0TXD and P2.1 to UCA0RXD: P2SEL1 = 1, P2SEL0 = 0
// (SLASE54D Table 9-23). The eUSCI controls the pin direction.
// TODO(stage 5): move to the board layer.
static void configure_pins_a0(void)
{
    // Clear SEL0 before setting SEL1, so the pin never passes through
    // SEL1:SEL0 = 11 (TB0CLK/ACLK on P2.0).
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |= BIT0 | BIT1;
}

hal_status_t hal_uart_init(hal_uart_instance_t instance,
                           const hal_uart_config_t *config)
{
    if ((unsigned int)instance >= HAL_UART_INSTANCE_COUNT || config == NULL) {
        HAL_DEBUG_TRAP();
        return HAL_ERR_INVALID_ARG;
    }

    // UCAxCTLW0 frame bits (SLAU367P 30.4.1, Table 30-8). Leaving the other
    // bits 0 selects UART mode (UCMODEx = 00, UCSYNC = 0), LSB first and
    // 8 data bits.
    uint16_t frame_bits;
    switch (config->parity) {
    case HAL_UART_PARITY_NONE: frame_bits = 0;               break;
    case HAL_UART_PARITY_EVEN: frame_bits = UCPEN | UCPAR;   break; // UCPAR = 1: even
    case HAL_UART_PARITY_ODD:  frame_bits = UCPEN;           break; // UCPAR = 0: odd
    default:
        HAL_DEBUG_TRAP();
        return HAL_ERR_INVALID_ARG;
    }
    switch (config->stop_bits) {
    case HAL_UART_STOP_BITS_1:                               break;
    case HAL_UART_STOP_BITS_2: frame_bits |= UCSPB;          break;
    default:
        HAL_DEBUG_TRAP();
        return HAL_ERR_INVALID_ARG;
    }

    if (instance != HAL_UART_INSTANCE_A0) {
        return HAL_ERR_UNSUPPORTED;
    }

    uint32_t smclk_hz = hal_clock_get_smclk_hz();
    if (smclk_hz == 0) {
        HAL_DEBUG_TRAP();
        return HAL_ERR_NOT_INITIALISED;
    }

    const baud_setting_t *baud = find_baud_setting(smclk_hz, config->baud_rate_bps);
    if (baud == NULL) {
        return HAL_ERR_UNSUPPORTED;
    }

    // All checks passed; only now touch the hardware. Sequence from
    // SLAU367P 30.3.1: hold in reset, configure, mux pins, release.

    // UCSWRST = 1 holds the module in reset (and sets UCTXIFG, clears the
    // error flags and UCRXIE/UCTXIE). UCSSEL = 10b clocks it from SMCLK.
    *reg(instance, OFS_UCA0CTLW0) = UCSWRST | UCSSEL__SMCLK | frame_bits;

    *reg(instance, OFS_UCA0BRW) = baud->brw;
    *reg(instance, OFS_UCA0MCTLW) = baud->mctlw;

    // Auto baud-rate detection and IrDA off (SLAU367P 30.4.8, 30.4.9).
    // Both reset to 0 on PUC; written so a re-init never inherits them.
    *reg(instance, OFS_UCA0ABCTL) = 0;
    *reg(instance, OFS_UCA0IRCTL) = 0;

    configure_pins_a0();

    // Release from reset. The transmitter is idle with UCTXIFG = 1.
    *reg(instance, OFS_UCA0CTLW0) &= ~UCSWRST;

    return HAL_OK;
}
