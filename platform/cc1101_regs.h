#ifndef CC1101_REGS_H
#define CC1101_REGS_H

/* Only the addresses needed so far (Day 3). The full register map gets
 * added from the CC1101 datasheet as the config setters are written. */

/* Command strobes (address byte only, no data phase) */
#define CC1101_SRES         0x30   /* reset chip */
#define CC1101_SIDLE        0x36   /* enter IDLE */
#define CC1101_SNOP         0x3D   /* no operation, returns status byte */
#define CC1101_SRX          0x34   /* enter RX */
#define CC1101_STX          0x35   /* enter TX */
#define CC1101_SFRX         0x3A   /* flush RX FIFO */
#define CC1101_SFTX         0x3B   /* flush TX FIFO */

/* Configuration registers: IOCFG2 (0x00) .. TEST0 (0x2E) are 47
 * contiguous bytes, so one burst write loads them all. */
#define CC1101_CONFIG_START 0x00
#define CC1101_CONFIG_LEN   47
#define CC1101_CHANNR       0x0A

/* PA table and FIFOs (burst access) */
#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F

/* Configuration register used for write/readback testing */
#define CC1101_SYNC1        0x04   /* reset default 0xD3 */

/* Status registers (read with the burst bit set) */
#define CC1101_PARTNUM      0x30   /* expect 0x00 */
#define CC1101_VERSION      0x31   /* commonly 0x14 (0x04 on some silicon) */
#define CC1101_MARCSTATE    0x35   /* low 5 bits, see values below */
#define CC1101_MARC_IDLE    0x01
#define CC1101_MARC_TX      0x13

#endif /* CC1101_REGS_H */