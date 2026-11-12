#include "hal_spi.h"
#include "cc1101.h"

/* ---------------------------------------------------------------------
 * Register image, addresses 0x00..0x2E (burst-written in one go).
 *
 * Values assume a 26 MHz crystal. Derived from the datasheet formulas:
 *   FREQ     = 433.92e6 * 2^16 / 26e6      = 1093745 = 0x10B071
 *   DRATE    = (256+M) * 2^E * 26e6 / 2^28 -> E=5, M=67 -> 1001 baud
 *   DEVIATN  = 26e6/2^17 * (8+M) * 2^E     -> E=4, M=0  -> 25.4 kHz
 *   RX BW    = 26e6 / (8*(4+M)*2^E)        -> E=3, M=0  -> 101.6 kHz
 * RX-side tuning (AGC, FOCCFG, BSCFG, FSCAL, TEST) is carried over from
 * the ELECHOUSE defaults; review it against SmartRF Studio when RX is
 * brought up.
 * ------------------------------------------------------------------- */
const uint8_t cc1101_config[CC1101_CONFIG_LEN] =
{
    0x29,   /* 0x00 IOCFG2   GDO2: CHIP_RDYn (reset default)            */
    0x2E,   /* 0x01 IOCFG1   GDO1: high impedance                       */
    0x06,   /* 0x02 IOCFG0   GDO0: high from sync sent to end of packet */
    0x07,   /* 0x03 FIFOTHR  reset default                              */
    0xD3,   /* 0x04 SYNC1    sync word high byte                        */
    0x91,   /* 0x05 SYNC0    sync word low byte                         */
    0x3D,   /* 0x06 PKTLEN   max packet length 61 bytes                 */
    0x04,   /* 0x07 PKTCTRL1 append RSSI/LQI status, no address check   */
    0x05,   /* 0x08 PKTCTRL0 CRC on, variable packet length             */
    0x00,   /* 0x09 ADDR                                                */
    0x00,   /* 0x0A CHANNR   channel 0                                  */
    0x06,   /* 0x0B FSCTRL1  IF frequency                               */
    0x00,   /* 0x0C FSCTRL0                                             */
    0x10,   /* 0x0D FREQ2    433.92 MHz                                 */
    0xB0,   /* 0x0E FREQ1                                               */
    0x71,   /* 0x0F FREQ0                                               */
    0xC5,   /* 0x10 MDMCFG4  RX BW 101.6 kHz, DRATE_E = 5               */
    0x43,   /* 0x11 MDMCFG3  DRATE_M = 67 -> ~1 kbaud                   */
    0x02,   /* 0x12 MDMCFG2  2-FSK, DC filter on, 16/16 sync detect     */
    0x02,   /* 0x13 MDMCFG1  FEC off, 2 preamble bytes, CHANSPC_E = 2   */
    0xF8,   /* 0x14 MDMCFG0  CHANSPC_M -> ~200 kHz channel spacing      */
    0x40,   /* 0x15 DEVIATN  E=4, M=0 -> 25.4 kHz                       */
    0x07,   /* 0x16 MCSM2    reset default                              */
    0x30,   /* 0x17 MCSM1    return to IDLE after TX and RX             */
    0x18,   /* 0x18 MCSM0    auto-calibrate on IDLE -> RX/TX            */
    0x16,   /* 0x19 FOCCFG                                              */
    0x1C,   /* 0x1A BSCFG                                               */
    0xC7,   /* 0x1B AGCCTRL2                                            */
    0x00,   /* 0x1C AGCCTRL1                                            */
    0xB2,   /* 0x1D AGCCTRL0                                            */
    0x87,   /* 0x1E WOREVT1  reset default                              */
    0x6B,   /* 0x1F WOREVT0  reset default                              */
    0xFB,   /* 0x20 WORCTRL  reset default                              */
    0x56,   /* 0x21 FREND1                                              */
    0x10,   /* 0x22 FREND0   PA_POWER = 0 -> uses PATABLE[0]            */
    0xE9,   /* 0x23 FSCAL3                                              */
    0x2A,   /* 0x24 FSCAL2                                              */
    0x00,   /* 0x25 FSCAL1                                              */
    0x1F,   /* 0x26 FSCAL0                                              */
    0x41,   /* 0x27 RCCTRL1  reset default                              */
    0x00,   /* 0x28 RCCTRL0  reset default                              */
    0x59,   /* 0x29 FSTEST                                              */
    0x7F,   /* 0x2A PTEST    reset default                              */
    0x3F,   /* 0x2B AGCTEST  reset default                              */
    0x81,   /* 0x2C TEST2                                               */
    0x35,   /* 0x2D TEST1                                               */
    0x09    /* 0x2E TEST0                                               */
};

/* PA settings for the 433 MHz band, from the datasheet table:
 * -30, -20, -15, -10, 0, +5, +7, +10 dBm */
static const uint8_t pa_table_433[8] =
    { 0x12, 0x0E, 0x1D, 0x34, 0x60, 0x84, 0xC8, 0xC0 };

uint8_t CC1101_Init(void)
{
    uint8_t err = HAL_SPI_ResetCC1101();

    if (err != 0)
    {
        return err;
    }

    HAL_SPI_WriteBurstReg(CC1101_CONFIG_START, cc1101_config, CC1101_CONFIG_LEN);
    CC1101_SetPA(10);

    return 0;
}

void CC1101_SetChannel(uint8_t channel)
{
    HAL_SPI_WriteReg(CC1101_CHANNR, channel);
}

void CC1101_SetPA(int8_t dbm)
{
    uint8_t pa[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t idx;

    if      (dbm <= -30) idx = 0;
    else if (dbm <= -20) idx = 1;
    else if (dbm <= -15) idx = 2;
    else if (dbm <= -10) idx = 3;
    else if (dbm <=   0) idx = 4;
    else if (dbm <=   5) idx = 5;
    else if (dbm <=   7) idx = 6;
    else                 idx = 7;

    /* FSK/GFSK reads PATABLE[0] (FREND0.PA_POWER = 0), so the chosen
     * value goes in the first byte and the rest stay zero. */
    pa[0] = pa_table_433[idx];
    HAL_SPI_WriteBurstReg(CC1101_PATABLE, pa, 8);
}

void CC1101_SetTx(void)
{
    HAL_SPI_Strobe(CC1101_SIDLE);
    HAL_SPI_Strobe(CC1101_STX);
}

void CC1101_SetIdle(void)
{
    HAL_SPI_Strobe(CC1101_SIDLE);
}