#include "hal_spi.h"

/* ---------------------------------------------------------------------
 * Assumption pending confirmation: SMCLK = ~1 MHz (FR5994 POR default
 * DCO, no clock reconfiguration performed yet in Day 1 code). If your
 * ClockInit() sets a different SMCLK frequency, update ASSUMED_SMCLK_HZ
 * below (or replace this with a runtime-computed divider).
 * ------------------------------------------------------------------- */
#define ASSUMED_SMCLK_HZ   1000000UL
#define SPI_TARGET_HZ       500000UL
#define SPI_BR_DIVIDER      (ASSUMED_SMCLK_HZ / SPI_TARGET_HZ)   /* = 2 */

void HAL_SPI_Init(void)
{
    /* --- Pin muxing: CLK/MOSI/MISO to eUSCI_B1 peripheral function --- */
    SPI_PORT_SEL1 &= ~(SPI_SIMO_BIT | SPI_SOMI_BIT | SPI_CLK_BIT);
    SPI_PORT_SEL0 |=  (SPI_SIMO_BIT | SPI_SOMI_BIT | SPI_CLK_BIT);

    /* --- CS: plain GPIO output, idle high (deasserted) --- */
    CS_PORT_OUT |= CS_BIT;
    CS_PORT_DIR |= CS_BIT;

    /* --- GDO0 / GDO2: plain GPIO inputs --- */
    GDO0_PORT_DIR &= ~GDO0_BIT;
    GDO2_PORT_DIR &= ~GDO2_BIT;

    /* --- eUSCI_B1 SPI config --- */
    UCB1CTLW0 = UCSWRST;                       /* hold state machine in reset while configuring */
    UCB1CTLW0 |= UCCKPH;                       /* CPOL=0, CPHA=0 -> SPI Mode 0 (UCCKPL left 0) */
    UCB1CTLW0 |= UCMSB;                        /* MSB first */
    UCB1CTLW0 |= UCMST | UCSYNC;               /* master, synchronous mode */
    UCB1CTLW0 |= UCSSEL__SMCLK;                /* clock source = SMCLK */
    /* UCMODEx left at 00 = 3-pin SPI (CS handled manually, not by eUSCI STE) */

    UCB1BRW = SPI_BR_DIVIDER;                  /* SMCLK / divider = SPI bit clock */

    /* --- FRAM device: unlock GPIOs from POR high-impedance state --- */
    PM5CTL0 &= ~LOCKLPM5;

    UCB1CTLW0 &= ~UCSWRST;                     /* release eUSCI from reset, start operating */
}

void HAL_SPI_CS_Assert(void)
{
    CS_PORT_OUT &= ~CS_BIT;    /* active low */
}

void HAL_SPI_CS_Deassert(void)
{
    CS_PORT_OUT |= CS_BIT;
}

/* hal_spi.c — internal only, no header declaration */
static uint8_t SPI_TransferByte(uint8_t data)
{
    while (!(UCB1IFG & UCTXIFG));   /* wait for TX buffer ready */
    UCB1TXBUF = data;
    while (!(UCB1IFG & UCRXIFG));   /* wait for RX complete */
    return UCB1RXBUF;
}

