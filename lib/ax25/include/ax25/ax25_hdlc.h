#ifndef AX25_HDLC_H
#define AX25_HDLC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ax25/ax25_bits.h"
#include "ax25/ax25_status.h"

/*
 * HDLC framing for AX.25:
 *   - 0x7E flags delimit frames; flags are never bit-stuffed.
 *   - Inside a frame a 0 is inserted after every run of five 1s.
 *   - All bytes go out LSB-first.
 * Bits here are NRZ data bits; NRZI and scrambling are in ax25_line.h.
 */
#define AX25_HDLC_FLAG 0x7Eu

/* Smallest AX.25 frame the deframer will report: two addresses, control
 * and FCS (an S-frame). Anything shorter between flags is noise. */
#define AX25_HDLC_MIN_FRAME_LEN 17u

/* Upper bound on bits produced by ax25_hdlc_encode() for a frame of
 * frame_len bytes: flags + data + worst-case stuffing (one per 5 bits). */
#define AX25_HDLC_MAX_BITS(frame_len, lead_flags, trail_flags)                 \
	(((lead_flags) + (trail_flags)) * 8u + (frame_len) * 8u +              \
	    ((frame_len) * 8u) / 5u)

/* Appends lead_flags flags, the bit-stuffed frame, then trail_flags flags
 * to out. Flag counts may be 0 (e.g. to share one flag between frames).
 * On AX25_ERR_NO_SPACE, out is left with a partial frame. */
ax25_status_t ax25_hdlc_encode(const uint8_t *frame, size_t len,
    size_t lead_flags, size_t trail_flags, ax25_bitbuf_t *out);

/* Streaming deframer: feed NRZ bits one at a time. */
typedef enum {
	AX25_HDLC_RX_NONE = 0, /* nothing to report */
	AX25_HDLC_RX_FRAME, /* a complete frame is in buf[0..frame_len) (FCS not
			       yet checked) */
	AX25_HDLC_RX_ABORT, /* seven or more 1s: frame in progress discarded */
	AX25_HDLC_RX_OVERFLOW /* frame longer than buf: discarded at the closing
				 flag */
} ax25_hdlc_rx_event_t;

typedef struct {
	uint8_t *buf;
	size_t capacity;
	size_t len;	   /* complete bytes collected so far */
	uint8_t pattern;   /* last 8 received bits, newest in bit 7 */
	uint8_t ones;	   /* current run of consecutive 1s */
	uint8_t byte_acc;  /* byte being assembled (LSB-first) */
	uint8_t bit_count; /* bits in byte_acc */
	bool in_frame;	   /* seen an opening flag */
	bool overflow;
} ax25_hdlc_rx_t;

void ax25_hdlc_rx_init(ax25_hdlc_rx_t *rx, uint8_t *buf, size_t capacity);

/* On AX25_HDLC_RX_FRAME, *frame_len receives the frame length; the bytes
 * stay valid until the next call. */
ax25_hdlc_rx_event_t ax25_hdlc_rx_bit(
    ax25_hdlc_rx_t *rx, uint8_t bit, size_t *frame_len);

#endif /* AX25_HDLC_H */
