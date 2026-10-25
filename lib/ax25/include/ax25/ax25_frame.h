#ifndef AX25_FRAME_H
#define AX25_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ax25/ax25_fcs.h"
#include "ax25/ax25_status.h"

/*
 * AX.25 v2.2 UI frame build/parse (no digipeater path).
 *
 * Byte layout (before HDLC framing):
 *   dest address (7) | src address (7) | control (1) | PID (1) | info (0..256)
 * | FCS (2, low byte first)
 *
 * Address field: 6 callsign characters, space padded, each shifted left
 * one bit, then the SSID byte:
 *   bit 7     C bit (command/response, v2.2 section 6.1.2)
 *   bits 6-5  reserved, sent as 1
 *   bits 4-1  SSID
 *   bit 0     address extension: 1 on the last address byte (src here)
 *
 * v2.2 command frames have dest C=1, src C=0; responses the opposite.
 * A UI beacon is a command frame.
 */
#define AX25_CALLSIGN_MAX_LEN 6u
#define AX25_SSID_MAX	      15u
#define AX25_ADDR_FIELD_LEN   7u
#define AX25_CONTROL_UI	      0x03u
#define AX25_CONTROL_PF_BIT   0x10u
#define AX25_PID_NO_LAYER3    0xF0u
#define AX25_INFO_MAX_LEN     256u

#define AX25_UI_HEADER_LEN                                                     \
	(2u * AX25_ADDR_FIELD_LEN + 2u) /* dest + src + control + PID */
#define AX25_FRAME_MIN_LEN (AX25_UI_HEADER_LEN + AX25_FCS_LEN)
#define AX25_FRAME_MAX_LEN                                                     \
	(AX25_UI_HEADER_LEN + AX25_INFO_MAX_LEN + AX25_FCS_LEN)

typedef struct {
	char callsign[AX25_CALLSIGN_MAX_LEN +
		      1]; /* NUL-terminated, no padding */
	uint8_t ssid;
} ax25_address_t;

typedef struct {
	ax25_address_t dest;
	ax25_address_t src;
	bool is_command;
	uint8_t control;
	uint8_t pid;
	const uint8_t
	    *info; /* after parse: points into the caller's frame buffer */
	size_t info_len;
} ax25_ui_frame_t;

/* Validates and stores a callsign (1..6 of A-Z, 0-9) and SSID (0..15). */
ax25_status_t ax25_address_set(
    ax25_address_t *addr, const char *callsign, uint8_t ssid);

/* Fills a command UI frame with control 0x03 and PID 0xF0. */
ax25_status_t ax25_ui_frame_init(ax25_ui_frame_t *frame, const char *dest_call,
    uint8_t dest_ssid, const char *src_call, uint8_t src_ssid,
    const uint8_t *info, size_t info_len);

/* Serialises frame (including FCS) into out. *out_len receives the length. */
ax25_status_t ax25_ui_frame_build(const ax25_ui_frame_t *frame, uint8_t *out,
    size_t out_cap, size_t *out_len);

/* Checks the FCS and parses a received frame (len includes the FCS). */
ax25_status_t ax25_ui_frame_parse(
    const uint8_t *data, size_t len, ax25_ui_frame_t *frame);

#endif /* AX25_FRAME_H */
