#ifndef CC1101_H
#define CC1101_H

#include <stdint.h>
#include "cc1101_regs.h"

/* Register image burst-written by CC1101_Init(): 433.92 MHz, 1 kbaud,
 * 2-FSK, 25 kHz deviation, channel 0, 16/16 sync word, CRC on.
 * Exposed so a test can read the registers back and compare. */
extern const uint8_t cc1101_config[CC1101_CONFIG_LEN];

/* Reset the chip, burst-load cc1101_config, load the PA table for
 * +10 dBm. Returns 0 on success, non-zero if the MISO handshake timed
 * out. Leaves the chip in IDLE. */
uint8_t CC1101_Init(void);

/* Same as ELECHOUSE setChannel(): writes CHANNR. Channel 0 = 433.92 MHz
 * with this config; each step adds the channel spacing (about 200 kHz). */
void CC1101_SetChannel(uint8_t channel);

/* Output power in dBm, 433 MHz table (same selection as ELECHOUSE setPA):
 * <=-30, -20, -15, -10, 0, 5, 7, 10. Values between steps round up. */
void CC1101_SetPA(int8_t dbm);

/* IDLE then STX, like ELECHOUSE SetTx(). Load the TX FIFO first. */
void CC1101_SetTx(void);

/* Back to IDLE (ELECHOUSE setSidle()). */
void CC1101_SetIdle(void);

#endif /* CC1101_H */