#include "ax25/ax25_bits.h"

void
ax25_bitbuf_init(ax25_bitbuf_t *b, uint8_t *storage, size_t storage_bytes)
{
	/* Clamp so capacity_bits cannot overflow a 16-bit size_t (MSP430). */
	const size_t max_bytes = (size_t)-1 / 8u;
	if (storage_bytes > max_bytes) {
		storage_bytes = max_bytes;
	}
	b->data = storage;
	b->capacity_bits = (storage == NULL) ? 0u : storage_bytes * 8u;
	b->length_bits = 0u;
}

void
ax25_bitbuf_clear(ax25_bitbuf_t *b)
{
	b->length_bits = 0u;
}

void
ax25_bitbuf_set(ax25_bitbuf_t *b, size_t index, uint8_t bit)
{
	const uint8_t mask = (uint8_t)(1u << (index % 8u));
	if (bit != 0u) {
		b->data[index / 8u] |= mask;
	}
	else {
		b->data[index / 8u] &= (uint8_t)~mask;
	}
}

bool
ax25_bitbuf_push(ax25_bitbuf_t *b, uint8_t bit)
{
	if (b->length_bits >= b->capacity_bits) {
		return false;
	}
	ax25_bitbuf_set(b, b->length_bits,
	    bit); /* set/clear: storage may hold stale data */
	b->length_bits++;
	return true;
}

uint8_t
ax25_bitbuf_get(const ax25_bitbuf_t *b, size_t index)
{
	return (uint8_t)((b->data[index / 8u] >> (index % 8u)) & 1u);
}
