#ifndef AX25_LINE_H
#define AX25_LINE_H

#include <stdint.h>

#include "ax25/ax25_bits.h"

/*
 * Line coding for 9600 baud G3RUH AX.25.
 *
 * Transmit order: HDLC bits -> NRZI encode -> G3RUH scramble -> modulator.
 * Receive order:  demodulator -> G3RUH descramble -> NRZI decode -> HDLC.
 *
 * This order matches Direwolf (wb2osz/direwolf @ eda1383):
 *   TX: src/hdlc_send.c send_bit_nrzi() feeds tone_gen_put_bit(), which
 *       scrambles in src/gen_tone.c (lines 451-457).
 *   RX: src/hdlc_rec.c (lines 486-494) descrambles, then NRZI decodes.
 * gr-satellites (daniestevez/gr-satellites @ 4210dc4,
 * python/components/deframers/ax25_deframer.py lines 38-48) NRZI-decodes
 * before descrambling. Both receive stages are linear over GF(2), so they
 * commute, and both decoders accept this encoder's output (proven in
 * test_line.c).
 *
 * NRZI: data 0 -> toggle the line level, data 1 -> hold it.
 * Scrambler: multiplicative (self-synchronising), polynomial x^17 + x^12 + 1:
 *   out[n] = in[n] ^ out[n-12] ^ out[n-17]       (scramble)
 *   out[n] = in[n] ^ in[n-12]  ^ in[n-17]        (descramble)
 * Initial states (level 0, register 0) match Direwolf.
 */
#define AX25_G3RUH_MASK 0x1FFFFul /* 17-bit shift register */

typedef struct {
	uint8_t
	    level; /* encoder: current line level; decoder: previous level */
} ax25_nrzi_t;

typedef struct {
	uint32_t reg; /* last 17 line bits, newest in bit 0 (uint32_t: int is
			 16-bit on MSP430) */
} ax25_g3ruh_t;

void ax25_nrzi_init(ax25_nrzi_t *s);
uint8_t ax25_nrzi_encode_bit(ax25_nrzi_t *s, uint8_t data_bit);
uint8_t ax25_nrzi_decode_bit(ax25_nrzi_t *s, uint8_t line_bit);

void ax25_g3ruh_init(ax25_g3ruh_t *s);
uint8_t ax25_g3ruh_scramble_bit(ax25_g3ruh_t *s, uint8_t bit);
uint8_t ax25_g3ruh_descramble_bit(ax25_g3ruh_t *s, uint8_t bit);

/* In-place whole-buffer versions, continuing from the given state. */
void ax25_nrzi_encode_buf(ax25_nrzi_t *s, ax25_bitbuf_t *bits);
void ax25_nrzi_decode_buf(ax25_nrzi_t *s, ax25_bitbuf_t *bits);
void ax25_g3ruh_scramble_buf(ax25_g3ruh_t *s, ax25_bitbuf_t *bits);
void ax25_g3ruh_descramble_buf(ax25_g3ruh_t *s, ax25_bitbuf_t *bits);

#endif /* AX25_LINE_H */
