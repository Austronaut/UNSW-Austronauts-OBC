#include "hal/hal_uart.h"

#include <stdbool.h>
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

// Per instance: loops of at least HAL_UART_LOOP_CYCLES that cover one frame
// plus one bit at the current settings. 0 means not initialised. Set only
// when hal_uart_init() succeeds; a failed re-init leaves the old
// configuration running, so it leaves this unchanged too.
static uint16_t frame_loops[HAL_UART_INSTANCE_COUNT];

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

// Builds the UCAxCTLW0 parity and stop-bit fields for config (SLAU367P
// 30.4.1, Table 30-8). Every other field is left 0, which selects UART mode
// (UCMODEx = 00, UCSYNC = 0), LSB first and 8 data bits. Returns false if
// config holds an invalid parity or stop-bit value; *frame_bits is then
// unchanged.
static bool encode_frame_bits(const hal_uart_config_t *config,
                              uint16_t *frame_bits)
{
    uint16_t bits;

    switch (config->parity) {
    case HAL_UART_PARITY_NONE: bits = 0;               break;
    case HAL_UART_PARITY_EVEN: bits = UCPEN | UCPAR;   break; // UCPAR = 1: even
    case HAL_UART_PARITY_ODD:  bits = UCPEN;           break; // UCPAR = 0: odd
    default:                   return false;
    }

    switch (config->stop_bits) {
    case HAL_UART_STOP_BITS_1:                         break;
    case HAL_UART_STOP_BITS_2: bits |= UCSPB;          break;
    default:                   return false;
    }

    *frame_bits = bits;
    return true;
}

// Returns how many loops of at least HAL_UART_LOOP_CYCLES MCLK cycles cover
// one frame plus one bit for config. After UCTXIFG sets, the last byte is in
// the shift register and finishes within one frame; the extra bit covers
// the wait for the next BITCLK before it starts (SLAU367P 30.3.8). This is
// conservative: hw_test case 23 measured UCTXIFG setting at the end of the
// start bit, so only frame - 1 bits actually remain. Rounds up, multiplying
// before dividing so no fraction is lost. Loops are counted in MCLK cycles,
// so mclk_hz is the CPU clock, not the UART's SMCLK.
static uint16_t compute_frame_loops(const hal_uart_config_t *config,
                                    uint32_t mclk_hz)
{
    uint32_t bits = 1 + 8 + 1;                      // start, data, margin
    bits += (config->parity == HAL_UART_PARITY_NONE) ? 0 : 1;
    bits += (config->stop_bits == HAL_UART_STOP_BITS_2) ? 2 : 1;

    uint32_t cycles_per_loop_times_baud = config->baud_rate_bps * HAL_UART_LOOP_CYCLES;
    uint32_t cycles_times_baud = bits * mclk_hz;
    return (uint16_t)((cycles_times_baud + cycles_per_loop_times_baud - 1) /
                      cycles_per_loop_times_baud);
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
        return HAL_STATUS_INVALID_ARG;
    }

    uint16_t frame_bits;
    if (!encode_frame_bits(config, &frame_bits)) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_INVALID_ARG;
    }

    if (instance != HAL_UART_INSTANCE_A0) {
        return HAL_STATUS_UNSUPPORTED;
    }

    uint32_t smclk_hz = hal_clock_get_smclk_hz();
    if (smclk_hz == 0) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_NOT_INITIALISED;
    }

    const baud_setting_t *baud = find_baud_setting(smclk_hz, config->baud_rate_bps);
    if (baud == NULL) {
        return HAL_STATUS_UNSUPPORTED;
    }

    // Set by the same hal_clock_init() call as SMCLK, so non-zero here.
    uint16_t new_frame_loops = compute_frame_loops(config, hal_clock_get_mclk_hz());

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

    frame_loops[instance] = new_frame_loops;
    return HAL_STATUS_OK;
}

hal_status_t hal_uart_write(hal_uart_instance_t instance,
                            const uint8_t *data, size_t length_bytes,
                            uint32_t timeout_loops, size_t *bytes_written)
{
    if (bytes_written != NULL) {
        *bytes_written = 0;
    }

    // All checks come before the length 0 case, so a bad call with no data
    // is still caught.
    if ((unsigned int)instance >= HAL_UART_INSTANCE_COUNT ||
        (data == NULL && length_bytes > 0)) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_INVALID_ARG;
    }
    if (frame_loops[instance] == 0) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_NOT_INITIALISED;
    }

    volatile uint16_t *ifg = reg(instance, OFS_UCA0IFG);
    volatile uint16_t *txbuf = reg(instance, OFS_UCA0TXBUF);
    uint32_t loops_left = timeout_loops;
    size_t sent = 0;
    hal_status_t status = HAL_STATUS_OK;

    while (sent < length_bytes) {
        // UCTXIFG = 1 means UCAxTXBUF can take a byte; writing it clears
        // the flag until the byte moves to the shift register (SLAU367P
        // 30.3.8, 30.4.11).
        if ((*ifg & UCTXIFG) != 0) {
            *txbuf = data[sent];
            sent++;
        } else if (loops_left > 0) {
            loops_left--;
            // Fixed delay so one loop is the same length in every build.
            __delay_cycles(HAL_UART_LOOP_CYCLES);
        } else {
            status = HAL_STATUS_TIMEOUT;
            break;
        }
    }

    if (bytes_written != NULL) {
        *bytes_written = sent;
    }
    return status;
}

hal_status_t hal_uart_flush(hal_uart_instance_t instance, uint32_t timeout_loops)
{
    if ((unsigned int)instance >= HAL_UART_INSTANCE_COUNT) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_INVALID_ARG;
    }
    if (frame_loops[instance] == 0) {
        HAL_DEBUG_TRAP();
        return HAL_STATUS_NOT_INITIALISED;
    }

    // Only reads registers. UCTXCPTIFG is deliberately not used: SLAU367P
    // Table 30-6 and erratum USCI42 (SLAZ681O) disagree on when it sets,
    // and clearing it would mean writing UCAxIFG, which a future RX or TX
    // interrupt handler also uses.
    volatile uint16_t *ifg = reg(instance, OFS_UCA0IFG);
    volatile uint16_t *statw = reg(instance, OFS_UCA0STATW);
    uint32_t loops_left = timeout_loops;

    // Phase 1: wait until UCAxTXBUF is empty. The last byte is then in the
    // shift register, and every earlier byte has finished, since a byte
    // only moves there once the previous one is done (SLAU367P 30.3.8).
    while ((*ifg & UCTXIFG) == 0) {
        if (loops_left == 0) {
            return HAL_STATUS_TIMEOUT;
        }
        loops_left--;
        __delay_cycles(HAL_UART_LOOP_CYCLES);
    }

    // Phase 2: wait until the eUSCI is idle (UCBUSY = 0, SLAU367P 30.4.5),
    // or until one frame plus one bit has passed. SLAU367P does not say
    // exactly when UCBUSY falls; hw_test cases 23 and 24 measured it on the
    // TX pin at the end of the last stop bit (both stop bits with 2), and
    // guard that. UCBUSY also covers receiving, so incoming data could hold
    // it at 1 after the last byte has left; the frame time bounds that wait.
    uint16_t frame_left = frame_loops[instance];
    while ((*statw & UCBUSY) != 0 && frame_left > 0) {
        if (loops_left == 0) {
            return HAL_STATUS_TIMEOUT;
        }
        loops_left--;
        frame_left--;
        __delay_cycles(HAL_UART_LOOP_CYCLES);
    }

    return HAL_STATUS_OK;
}
