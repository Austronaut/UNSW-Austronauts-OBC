#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <msp430.h>
#include <stdint.h>

/* ---------------------------------------------------------------------
 * Pin map — MSP430FR5994, eUSCI_B1, direct register access, CCS.
 * No dev board: these are a starting point (TI SLAA721E reference
 * pinout for CC110x on FR5994). Rewire / redefine as needed.
 * ------------------------------------------------------------------- */

/* eUSCI_B1 SPI lines (peripheral-owned, 3-wire mode) */
#define SPI_PORT_SEL0   P5SEL0
#define SPI_PORT_SEL1   P5SEL1
#define SPI_SIMO_BIT    BIT0   /* P5.0 - UCB1SIMO (MOSI) */
#define SPI_SOMI_BIT    BIT1   /* P5.1 - UCB1SOMI (MISO) */
#define SPI_CLK_BIT     BIT2   /* P5.2 - UCB1CLK          */

/* Chip select — plain GPIO, manually driven (not eUSCI STE) */
#define CS_PORT_DIR     P4DIR
#define CS_PORT_OUT     P4OUT
#define CS_BIT          BIT4   /* P4.4 - CS (active low) */

#define GDO0_PORT_DIR   P3DIR
#define GDO0_PORT_IN    P3IN
#define GDO0_BIT        BIT0   /* P3.0 - GDO0 (provisional) */

#define GDO2_PORT_DIR   P3DIR
#define GDO2_PORT_IN    P3IN
#define GDO2_BIT        BIT1   /* P3.1 - GDO2 (provisional) */

/* CC1101 command strobe / access-mode bits (address byte framing) */
#define CC1101_WRITE_BURST      0x40
#define CC1101_READ_SINGLE      0x80
#define CC1101_READ_BURST       0xC0

/* ---------------------------------------------------------------------
 * HAL primitives
 * ------------------------------------------------------------------- */

/* One-time startup: configure SMCLK source/divider, mux eUSCI_B1 pins,
 * set CS/GDO0/GDO2 direction, clear PM5CTL0 LOCKLPM5, release eUSCI
 * from reset. Call once from main() before any other HAL_SPI_* call. */
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
void HAL_SPI_ResetCC1101(void);

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

#endif /* HAL_SPI_H */
