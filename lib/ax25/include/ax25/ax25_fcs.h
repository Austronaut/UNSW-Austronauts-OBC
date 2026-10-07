#ifndef AX25_FCS_H
#define AX25_FCS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * AX.25 Frame Check Sequence: CRC-16/X.25.
 *   reflected polynomial 0x8408 (0x1021 bit-reversed), init 0xFFFF,
 *   final XOR 0xFFFF. Check value: "123456789" -> 0x906E.
 * On air the FCS is sent low byte first.
 */
#define AX25_FCS_INIT	0xFFFFu
#define AX25_FCS_XOROUT 0xFFFFu
#define AX25_FCS_LEN	2u

/* Bitwise update of a running CRC (start from AX25_FCS_INIT). */
uint16_t ax25_fcs_update(uint16_t crc, uint8_t byte);

/* Complete FCS of a buffer, final XOR applied. */
uint16_t ax25_fcs(const uint8_t *data, size_t len);

/* True if the last two bytes of frame are a valid FCS (low byte first)
 * over the bytes before them. */
bool ax25_fcs_check(const uint8_t *frame, size_t len_with_fcs);

#endif /* AX25_FCS_H */
