#ifndef AX25_BITS_H
#define AX25_BITS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Bit buffer over caller-provided storage.
 *
 * Packing: bit i (in transmission order) lives in byte i / 8 at bit
 * position i % 8, i.e. LSB-first within each byte. This matches AX.25's
 * LSB-first bit order, so a frame byte pushed at a byte boundary keeps its
 * value. Hardware that shifts bytes out MSB-first (e.g. the CC1101 TX FIFO)
 * must re-pack before handing bytes over.
 */
typedef struct {
	uint8_t *data;
	size_t capacity_bits;
	size_t length_bits;
} ax25_bitbuf_t;

/* Number of storage bytes needed to hold `bits` bits. */
#define AX25_BITS_TO_BYTES(bits) (((bits) + 7u) / 8u)

void ax25_bitbuf_init(ax25_bitbuf_t *b, uint8_t *storage, size_t storage_bytes);
void ax25_bitbuf_clear(ax25_bitbuf_t *b);

/* Appends one bit (any non-zero value counts as 1). Returns false if full. */
bool ax25_bitbuf_push(ax25_bitbuf_t *b, uint8_t bit);

/* Read / overwrite an existing bit. index must be < length_bits. */
uint8_t ax25_bitbuf_get(const ax25_bitbuf_t *b, size_t index);
void ax25_bitbuf_set(ax25_bitbuf_t *b, size_t index, uint8_t bit);

#endif /* AX25_BITS_H */
