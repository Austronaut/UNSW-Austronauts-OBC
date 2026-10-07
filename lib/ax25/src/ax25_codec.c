#include "ax25/ax25_codec.h"

ax25_tx_config_t
ax25_tx_config_default(void)
{
	ax25_tx_config_t cfg;
	cfg.lead_flags = AX25_DEFAULT_LEAD_FLAGS;
	cfg.trail_flags = AX25_DEFAULT_TRAIL_FLAGS;
	cfg.scramble = true;
	cfg.pad_to_byte = true;
	return cfg;
}

/* Pads with the start of another flag (0,1,1,1,1,1,1): after the closing
 * flag this can never form a flag, an abort, or a frame. */
static ax25_status_t
pad_to_byte(ax25_bitbuf_t *bits)
{
	uint8_t i = 0u;
	while ((bits->length_bits % 8u) != 0u) {
		if (!ax25_bitbuf_push(
			bits, (uint8_t)((AX25_HDLC_FLAG >> i) & 1u))) {
			return AX25_ERR_NO_SPACE;
		}
		i++;
	}
	return AX25_OK;
}

ax25_status_t
ax25_encode(const uint8_t *frame, size_t len, const ax25_tx_config_t *cfg,
    ax25_bitbuf_t *out)
{
	if (frame == NULL || cfg == NULL || out == NULL ||
	    cfg->lead_flags == 0u || cfg->trail_flags == 0u) {
		return AX25_ERR_ARG;
	}
	ax25_bitbuf_clear(out);

	ax25_status_t st = ax25_hdlc_encode(
	    frame, len, cfg->lead_flags, cfg->trail_flags, out);
	if (st != AX25_OK) {
		return st;
	}
	if (cfg->pad_to_byte) {
		st = pad_to_byte(out);
		if (st != AX25_OK) {
			return st;
		}
	}

	ax25_nrzi_t nrzi;
	ax25_nrzi_init(&nrzi);
	ax25_nrzi_encode_buf(&nrzi, out);

	if (cfg->scramble) {
		ax25_g3ruh_t g3ruh;
		ax25_g3ruh_init(&g3ruh);
		ax25_g3ruh_scramble_buf(&g3ruh, out);
	}
	return AX25_OK;
}

void
ax25_rx_init(ax25_rx_t *rx, uint8_t *frame_buf, size_t frame_buf_len,
    bool descramble, ax25_rx_frame_cb_t on_frame, void *user)
{
	rx->descramble = descramble;
	ax25_g3ruh_init(&rx->g3ruh);
	ax25_nrzi_init(&rx->nrzi);
	ax25_hdlc_rx_init(&rx->hdlc, frame_buf, frame_buf_len);
	rx->on_frame = on_frame;
	rx->user = user;
	rx->stats.frames_ok = 0u;
	rx->stats.fcs_errors = 0u;
	rx->stats.aborts = 0u;
	rx->stats.overflows = 0u;
}

void
ax25_rx_bit(ax25_rx_t *rx, uint8_t line_bit)
{
	uint8_t bit = line_bit;
	if (rx->descramble) {
		bit = ax25_g3ruh_descramble_bit(&rx->g3ruh, bit);
	}
	bit = ax25_nrzi_decode_bit(&rx->nrzi, bit);

	size_t len = 0u;
	switch (ax25_hdlc_rx_bit(&rx->hdlc, bit, &len)) {
	case AX25_HDLC_RX_FRAME:
		if (ax25_fcs_check(rx->hdlc.buf, len)) {
			rx->stats.frames_ok++;
			if (rx->on_frame != NULL) {
				rx->on_frame(rx->user, rx->hdlc.buf, len);
			}
		}
		else {
			rx->stats.fcs_errors++;
		}
		break;
	case AX25_HDLC_RX_ABORT:
		rx->stats.aborts++;
		break;
	case AX25_HDLC_RX_OVERFLOW:
		rx->stats.overflows++;
		break;
	case AX25_HDLC_RX_NONE:
	default:
		break;
	}
}

void
ax25_rx_bits(ax25_rx_t *rx, const ax25_bitbuf_t *bits)
{
	for (size_t i = 0u; i < bits->length_bits; i++) {
		ax25_rx_bit(rx, ax25_bitbuf_get(bits, i));
	}
}
