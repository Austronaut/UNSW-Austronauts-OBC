#include "ax25/ax25_hdlc.h"

#define STUFF_AFTER_ONES 5u
#define ABORT_PATTERN	 0xFEu /* seven 1s in the newest 7 bits (bits 7..1) */

static bool
push_flag(ax25_bitbuf_t *out)
{
	for (uint8_t i = 0u; i < 8u; i++) {
		if (!ax25_bitbuf_push(
			out, (uint8_t)((AX25_HDLC_FLAG >> i) & 1u))) {
			return false;
		}
	}
	return true;
}

ax25_status_t
ax25_hdlc_encode(const uint8_t *frame, size_t len, size_t lead_flags,
    size_t trail_flags, ax25_bitbuf_t *out)
{
	if (out == NULL || (frame == NULL && len > 0u)) {
		return AX25_ERR_ARG;
	}
	for (size_t i = 0u; i < lead_flags; i++) {
		if (!push_flag(out)) {
			return AX25_ERR_NO_SPACE;
		}
	}

	uint8_t ones =
	    0u; /* flags end in 0, so the run always starts at zero */
	for (size_t i = 0u; i < len; i++) {
		for (uint8_t b = 0u; b < 8u; b++) {
			const uint8_t bit =
			    (uint8_t)((frame[i] >> b) & 1u); /* LSB-first */
			if (!ax25_bitbuf_push(out, bit)) {
				return AX25_ERR_NO_SPACE;
			}
			if (bit == 0u) {
				ones = 0u;
				continue;
			}
			ones++;
			if (ones == STUFF_AFTER_ONES) {
				if (!ax25_bitbuf_push(out, 0u)) {
					return AX25_ERR_NO_SPACE;
				}
				ones = 0u;
			}
		}
	}

	for (size_t i = 0u; i < trail_flags; i++) {
		if (!push_flag(out)) {
			return AX25_ERR_NO_SPACE;
		}
	}
	return AX25_OK;
}

static void
start_frame(ax25_hdlc_rx_t *rx)
{
	rx->in_frame = true;
	rx->overflow = false;
	rx->len = 0u;
	rx->byte_acc = 0u;
	rx->bit_count = 0u;
}

void
ax25_hdlc_rx_init(ax25_hdlc_rx_t *rx, uint8_t *buf, size_t capacity)
{
	rx->buf = buf;
	rx->capacity = (buf == NULL) ? 0u : capacity;
	rx->pattern = 0u;
	rx->ones = 0u;
	start_frame(rx);
	rx->in_frame = false;
}

ax25_hdlc_rx_event_t
ax25_hdlc_rx_bit(ax25_hdlc_rx_t *rx, uint8_t bit, size_t *frame_len)
{
	bit = (bit != 0u) ? 1u : 0u;
	rx->pattern = (uint8_t)((rx->pattern >> 1) | (uint8_t)(bit << 7));
	const uint8_t prev_ones = rx->ones;
	rx->ones = (bit != 0u)
		       ? (uint8_t)(rx->ones < 0xFFu ? rx->ones + 1u : rx->ones)
		       : 0u;

	if (rx->pattern == AX25_HDLC_FLAG) {
		/* Before this closing 0, the flag's leading 0 and six 1s were
		 * taken as data, so a byte-aligned frame leaves exactly 7 bits
		 * pending. */
		ax25_hdlc_rx_event_t ev = AX25_HDLC_RX_NONE;
		if (rx->in_frame && rx->bit_count == 7u &&
		    rx->len >= AX25_HDLC_MIN_FRAME_LEN) {
			if (rx->overflow) {
				ev = AX25_HDLC_RX_OVERFLOW;
			}
			else {
				if (frame_len != NULL) {
					*frame_len = rx->len;
				}
				ev = AX25_HDLC_RX_FRAME;
			}
		}
		start_frame(rx); /* a flag both closes one frame and may open
				    the next */
		return ev;
	}

	if ((rx->pattern & ABORT_PATTERN) == ABORT_PATTERN) {
		const bool was_in_frame = rx->in_frame;
		rx->in_frame = false; /* stay out until the next flag */
		return was_in_frame ? AX25_HDLC_RX_ABORT : AX25_HDLC_RX_NONE;
	}

	if (!rx->in_frame) {
		return AX25_HDLC_RX_NONE;
	}
	if (bit == 0u && prev_ones == STUFF_AFTER_ONES) {
		return AX25_HDLC_RX_NONE; /* stuffed bit: drop it */
	}

	rx->byte_acc = (uint8_t)((rx->byte_acc >> 1) |
				 (uint8_t)(bit << 7)); /* LSB-first */
	rx->bit_count++;
	if (rx->bit_count == 8u) {
		if (rx->len < rx->capacity) {
			rx->buf[rx->len] = rx->byte_acc;
		}
		else {
			rx->overflow = true;
		}
		if (rx->len < (size_t)-1) {
			rx->len++;
		}
		rx->byte_acc = 0u;
		rx->bit_count = 0u;
	}
	return AX25_HDLC_RX_NONE;
}
