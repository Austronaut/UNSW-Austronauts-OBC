#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <driverlib.h>
#include <stdint.h>

/* ---------------------------------------------------------------------
 * Pin map — MSP430FR5994, eUSCI_B1, via DriverLib GPIO/EUSCI constants.
 * No dev board yet: TI SLAA721E reference pinout for SPI lines; GDO0/
 * GDO2 moved off P2.0/P2.1 (LaunchPad eZ-FET backchannel UART conflict)
 * to P3.0/P3.1 - provisional, re-verify against SLAU627 / board
 * silkscreen once the LaunchPad is physically available.
 * ------------------------------------------------------------------- */

#define SPI_EUSCI_BASE      EUSCI_B1_BASE

/* eUSCI_B1 SPI lines (peripheral-owned, 3-wire mode) */
#define SPI_PORT            GPIO_PORT_P5
#define SPI_SIMO_PIN        GPIO_PIN0   /* P5.0 - UCB1SIMO (MOSI) */
#define SPI_SOMI_PIN        GPIO_PIN1   /* P5.1 - UCB1SOMI (MISO) */
#define SPI_CLK_PIN         GPIO_PIN2   /* P5.2 - UCB1CLK          */

/* Chip select — plain GPIO, manually driven (not eUSCI STE) */
#define CS_PORT             GPIO_PORT_P4
#define CS_PIN              GPIO_PIN4   /* P4.4 - CS (active low), alt. func TB0.5 unused */

/* GDO0 / GDO2 — plain GPIO inputs (provisional pins, see note above) */
#define GDO0_PORT           GPIO_PORT_P3
#define GDO0_PIN            GPIO_PIN0   /* P3.0 - GDO0, alt. func A12/C12 unused */

#define GDO2_PORT           GPIO_PORT_P3
#define GDO2_PIN            GPIO_PIN1   /* P3.1 - GDO2, alt. func A13/C13 unused */

/* Assumed SMCLK frequency (Hz) - must match hal_clock.c's DCO setting.
 * Used to compute the eUSCI SPI clock divider at init. */
#define ASSUMED_SMCLK_HZ    1000000UL
#define SPI_TARGET_HZ        500000UL

/* CC1101 command strobe / access-mode bits (address byte framing) */
#define CC1101_WRITE_BURST      0x40
#define CC1101_READ_SINGLE      0x80
#define CC1101_READ_BURST       0xC0

/* ---------------------------------------------------------------------
 * HAL primitives
 *
 * NOTE: the raw single-byte SPI transfer (EUSCI_B_SPI_transmitData /
 * EUSCI_B_SPI_receiveData, with DriverLib's isBusy/interrupt-status
 * polling) is intentionally NOT declared here. It is `static` inside
 * hal_spi.c and used only by WriteReg/ReadReg/WriteBurstReg/
 * ReadBurstReg/Strobe below, so every SPI transaction is forced
 * through the CS + WaitReady pattern - nothing outside this file can
 * move a byte without going through that sequence.
 * ------------------------------------------------------------------- */

/* One-time startup: mux eUSCI_B1 pins, set CS/GDO0/GDO2 direction,
 * unlock LPM5 GPIO lock, configure and enable eUSCI_B1 SPI (mode 0,
 * MSB-first, 3-wire, SMCLK-sourced). Call once from main(), after
 * HAL_Clock_Init(). */
void HAL_SPI_Init(void);

/* Assert / deassert chip select (active low). Manual, not eUSCI STE,
 * so the MISO-ready handshake can run between assert and the first
 * clocked byte. */
void HAL_SPI_CS_Assert(void);
void HAL_SPI_CS_Deassert(void);

/* Poll MISO until it goes low (CC1101 ready). Call after CS_Assert,
 * and again after issuing SRES during reset. Returns 0 on success,
 * non-zero on timeout (bus/wiring fault) — do not block forever. */
uint8_t HAL_SPI_WaitReady(void);

/* Full CC1101 reset/wake sequence: CS pulse, SRES strobe, wait-ready
 * handshake per datasheet timing. Leaves the chip in IDLE state. */
uint8_t HAL_SPI_ResetCC1101(void);

/* Single-register access */
void    HAL_SPI_WriteReg(uint8_t addr, uint8_t value);
uint8_t HAL_SPI_ReadReg(uint8_t addr);

/* Burst register access (address byte gets CC1101_WRITE_BURST /
 * CC1101_READ_BURST OR'd in internally) */
void HAL_SPI_WriteBurstReg(uint8_t addr, const uint8_t *buffer, uint8_t len);
void HAL_SPI_ReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t len);

/* Command strobe — address byte only, no data phase. Returns the
 * chip status byte returned during the address transfer. */
uint8_t HAL_SPI_Strobe(uint8_t strobe);

/* temp header*/
static uint8_t SPI_TransferByte(uint8_t data);

#endif /* HAL_SPI_H */
