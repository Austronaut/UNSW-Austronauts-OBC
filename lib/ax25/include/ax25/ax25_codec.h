#ifndef AX25_CODEC_H
#define AX25_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ax25/ax25_bits.h"
#include "ax25/ax25_frame.h"
#include "ax25/ax25_hdlc.h"
#include "ax25/ax25_line.h"
#include "ax25/ax25_status.h"

/*
 * Full pipelines between frame bytes (as built by ax25_ui_frame_build,
 * FCS included) and the line bitstream handed to / taken from a modem.
 *
 *   encode: HDLC (flags + stuffing) -> [pad] -> NRZI -> [G3RUH scramble]
 *   decode: [G3RUH descramble] -> NRZI -> HDLC deframe -> FCS check
 */
typedef struct {
	size_t
	    lead_flags; /* >= 1; extra flags give the receiver time to lock */
	size_t trail_flags; /* >= 1 */
	bool scramble; /* true for G3RUH 9600; false for plain NRZI (e.g. 1200
			  AFSK) */
	bool pad_to_byte; /* append partial-flag bits so length_bits % 8 == 0 */
} ax25_tx_config_t;

#define AX25_DEFAULT_LEAD_FLAGS	 16u
#define AX25_DEFAULT_TRAIL_FLAGS 4u

ax25_tx_config_t ax25_tx_config_default(void);

/* Upper bound on line bits for one frame, padding included. */
#define AX25_ENCODED_MAX_BITS(frame_len, lead_flags, trail_flags)              \
	(AX25_HDLC_MAX_BITS(frame_len, lead_flags, trail_flags) + 7u)

/* Encodes one frame as one transmission: out is cleared first and line
 * state starts from reset. */
ax25_status_t ax25_encode(const uint8_t *frame, size_t len,
    const ax25_tx_config_t *cfg, ax25_bitbuf_t *out);

/* Called for each received frame that passes the FCS check. len includes
 * the FCS, so data/len can go straight to ax25_ui_frame_parse(). */
typedef void (*ax25_rx_frame_cb_t)(void *user, const uint8_t *data, size_t len);

typedef struct {
	uint32_t frames_ok;
	uint32_t fcs_errors;
	uint32_t aborts;
	uint32_t overflows;
} ax25_rx_stats_t;

typedef struct {
	bool descramble;
	ax25_g3ruh_t g3ruh;
	ax25_nrzi_t nrzi;
	ax25_hdlc_rx_t hdlc;
	ax25_rx_frame_cb_t on_frame;
	void *user;
	ax25_rx_stats_t stats;
} ax25_rx_t;

void ax25_rx_init(ax25_rx_t *rx, uint8_t *frame_buf, size_t frame_buf_len,
    bool descramble, ax25_rx_frame_cb_t on_frame, void *user);
void ax25_rx_bit(ax25_rx_t *rx, uint8_t line_bit);
void ax25_rx_bits(ax25_rx_t *rx, const ax25_bitbuf_t *bits);

#endif /* AX25_CODEC_H */
