#include "ax25/ax25_line.h"

/* Tap positions for x^17 + x^12 + 1: bit 16 holds the bit from 17 steps
 * ago, bit 11 the bit from 12 steps ago (same as Direwolf's >>16 and >>11). */
#define G3RUH_TAP_17 16u
#define G3RUH_TAP_12 11u

void
ax25_nrzi_init(ax25_nrzi_t *s)
{
	s->level = 0u;
}

uint8_t
ax25_nrzi_encode_bit(ax25_nrzi_t *s, uint8_t data_bit)
{
	if (data_bit == 0u) {
		s->level ^= 1u;
	}
	return s->level;
}

uint8_t
ax25_nrzi_decode_bit(ax25_nrzi_t *s, uint8_t line_bit)
{
	line_bit = (line_bit != 0u) ? 1u : 0u;
	const uint8_t data_bit = (line_bit == s->level) ? 1u : 0u;
	s->level = line_bit;
	return data_bit;
}

void
ax25_g3ruh_init(ax25_g3ruh_t *s)
{
	s->reg = 0u;
}

static uint8_t
g3ruh_feedback(const ax25_g3ruh_t *s)
{
	return (uint8_t)(((s->reg >> G3RUH_TAP_17) ^ (s->reg >> G3RUH_TAP_12)) &
			 1u);
}

static void
g3ruh_shift_in(ax25_g3ruh_t *s, uint8_t bit)
{
	s->reg = ((s->reg << 1) | bit) & AX25_G3RUH_MASK;
}

uint8_t
ax25_g3ruh_scramble_bit(ax25_g3ruh_t *s, uint8_t bit)
{
	const uint8_t out =
	    (uint8_t)(((bit != 0u) ? 1u : 0u) ^ g3ruh_feedback(s));
	g3ruh_shift_in(s, out); /* scrambler feeds back its own output */
	return out;
}

uint8_t
ax25_g3ruh_descramble_bit(ax25_g3ruh_t *s, uint8_t bit)
{
	bit = (bit != 0u) ? 1u : 0u;
	const uint8_t out = (uint8_t)(bit ^ g3ruh_feedback(s));
	g3ruh_shift_in(s, bit); /* descrambler shifts in the received bit */
	return out;
}

void
ax25_nrzi_encode_buf(ax25_nrzi_t *s, ax25_bitbuf_t *bits)
{
	for (size_t i = 0u; i < bits->length_bits; i++) {
		ax25_bitbuf_set(
		    bits, i, ax25_nrzi_encode_bit(s, ax25_bitbuf_get(bits, i)));
	}
}

void
ax25_nrzi_decode_buf(ax25_nrzi_t *s, ax25_bitbuf_t *bits)
{
	for (size_t i = 0u; i < bits->length_bits; i++) {
		ax25_bitbuf_set(
		    bits, i, ax25_nrzi_decode_bit(s, ax25_bitbuf_get(bits, i)));
	}
}

void
ax25_g3ruh_scramble_buf(ax25_g3ruh_t *s, ax25_bitbuf_t *bits)
{
	for (size_t i = 0u; i < bits->length_bits; i++) {
		ax25_bitbuf_set(bits, i,
		    ax25_g3ruh_scramble_bit(s, ax25_bitbuf_get(bits, i)));
	}
}

void
ax25_g3ruh_descramble_buf(ax25_g3ruh_t *s, ax25_bitbuf_t *bits)
{
	for (size_t i = 0u; i < bits->length_bits; i++) {
		ax25_bitbuf_set(bits, i,
		    ax25_g3ruh_descramble_bit(s, ax25_bitbuf_get(bits, i)));
	}
}
