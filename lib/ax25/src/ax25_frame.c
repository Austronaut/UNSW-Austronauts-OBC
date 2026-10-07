#include "ax25/ax25_frame.h"

#define SSID_C_BIT	0x80u
#define SSID_RESERVED	0x60u
#define SSID_EXT_BIT	0x01u
#define SSID_SHIFT	1u
#define SSID_FIELD_MASK 0x1Eu
#define CALLSIGN_PAD	' '

static bool
is_callsign_char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

ax25_status_t
ax25_address_set(ax25_address_t *addr, const char *callsign, uint8_t ssid)
{
	if (addr == NULL || callsign == NULL) {
		return AX25_ERR_ARG;
	}
	size_t len = 0u;
	while (len <= AX25_CALLSIGN_MAX_LEN && callsign[len] != '\0') {
		if (!is_callsign_char(callsign[len])) {
			return AX25_ERR_CALLSIGN;
		}
		len++;
	}
	if (len == 0u || len > AX25_CALLSIGN_MAX_LEN) {
		return AX25_ERR_CALLSIGN;
	}
	if (ssid > AX25_SSID_MAX) {
		return AX25_ERR_SSID;
	}
	for (size_t i = 0u; i < len; i++) {
		addr->callsign[i] = callsign[i];
	}
	addr->callsign[len] = '\0';
	addr->ssid = ssid;
	return AX25_OK;
}

ax25_status_t
ax25_ui_frame_init(ax25_ui_frame_t *frame, const char *dest_call,
    uint8_t dest_ssid, const char *src_call, uint8_t src_ssid,
    const uint8_t *info, size_t info_len)
{
	if (frame == NULL || (info == NULL && info_len > 0u)) {
		return AX25_ERR_ARG;
	}
	ax25_status_t st = ax25_address_set(&frame->dest, dest_call, dest_ssid);
	if (st != AX25_OK) {
		return st;
	}
	st = ax25_address_set(&frame->src, src_call, src_ssid);
	if (st != AX25_OK) {
		return st;
	}
	frame->is_command = true;
	frame->control = AX25_CONTROL_UI;
	frame->pid = AX25_PID_NO_LAYER3;
	frame->info = info;
	frame->info_len = info_len;
	return AX25_OK;
}

/* Writes one 7-byte address field. Caller has checked space. */
static ax25_status_t
write_address(uint8_t *out, const ax25_address_t *addr, bool c_bit, bool last)
{
	/* Re-validate: the struct may have been filled by hand. */
	ax25_address_t checked;
	ax25_status_t st =
	    ax25_address_set(&checked, addr->callsign, addr->ssid);
	if (st != AX25_OK) {
		return st;
	}
	size_t i = 0u;
	for (; checked.callsign[i] != '\0'; i++) {
		out[i] = (uint8_t)((uint8_t)checked.callsign[i] << 1);
	}
	for (; i < AX25_CALLSIGN_MAX_LEN; i++) {
		out[i] = (uint8_t)((uint8_t)CALLSIGN_PAD << 1);
	}
	uint8_t ssid_byte =
	    (uint8_t)(SSID_RESERVED | (uint8_t)(checked.ssid << SSID_SHIFT));
	if (c_bit) {
		ssid_byte |= SSID_C_BIT;
	}
	if (last) {
		ssid_byte |= SSID_EXT_BIT;
	}
	out[AX25_CALLSIGN_MAX_LEN] = ssid_byte;
	return AX25_OK;
}

static bool
is_ui_control(uint8_t control)
{
	return (uint8_t)(control & (uint8_t)~AX25_CONTROL_PF_BIT) ==
	       AX25_CONTROL_UI;
}

ax25_status_t
ax25_ui_frame_build(
    const ax25_ui_frame_t *frame, uint8_t *out, size_t out_cap, size_t *out_len)
{
	if (frame == NULL || out == NULL || out_len == NULL ||
	    (frame->info == NULL && frame->info_len > 0u)) {
		return AX25_ERR_ARG;
	}
	if (!is_ui_control(frame->control)) {
		return AX25_ERR_UNSUPPORTED;
	}
	if (frame->info_len > AX25_INFO_MAX_LEN) {
		return AX25_ERR_INFO_TOO_LONG;
	}
	const size_t total =
	    AX25_UI_HEADER_LEN + frame->info_len + AX25_FCS_LEN;
	if (out_cap < total) {
		return AX25_ERR_NO_SPACE;
	}

	/* Command: dest C=1, src C=0. Response: dest C=0, src C=1. */
	ax25_status_t st =
	    write_address(&out[0], &frame->dest, frame->is_command, false);
	if (st != AX25_OK) {
		return st;
	}
	st = write_address(
	    &out[AX25_ADDR_FIELD_LEN], &frame->src, !frame->is_command, true);
	if (st != AX25_OK) {
		return st;
	}

	size_t pos = 2u * AX25_ADDR_FIELD_LEN;
	out[pos++] = frame->control;
	out[pos++] = frame->pid;
	for (size_t i = 0u; i < frame->info_len; i++) {
		out[pos++] = frame->info[i];
	}
	const uint16_t fcs = ax25_fcs(out, pos);
	out[pos++] = (uint8_t)(fcs & 0xFFu); /* low byte first */
	out[pos++] = (uint8_t)(fcs >> 8);

	*out_len = pos;
	return AX25_OK;
}

/* Parses one 7-byte address field; reports its C bit and extension bit. */
static ax25_status_t
read_address(
    const uint8_t *in, ax25_address_t *addr, bool *c_bit, bool *ext_bit)
{
	char call[AX25_CALLSIGN_MAX_LEN + 1u];
	size_t len = 0u;
	bool padding = false;

	for (size_t i = 0u; i < AX25_CALLSIGN_MAX_LEN; i++) {
		if ((in[i] & SSID_EXT_BIT) != 0u) {
			return AX25_ERR_ADDRESS; /* extension bit only belongs
						    on the SSID byte */
		}
		const char c = (char)(in[i] >> 1);
		if (c == CALLSIGN_PAD) {
			padding = true;
		}
		else if (padding || !is_callsign_char(c)) {
			return AX25_ERR_ADDRESS; /* character after padding, or
						    invalid character */
		}
		else {
			call[len++] = c;
		}
	}
	call[len] = '\0';

	const uint8_t ssid_byte = in[AX25_CALLSIGN_MAX_LEN];
	const uint8_t ssid =
	    (uint8_t)((ssid_byte & SSID_FIELD_MASK) >> SSID_SHIFT);
	if (ax25_address_set(addr, call, ssid) != AX25_OK) {
		return AX25_ERR_ADDRESS;
	}
	/* Reserved bits are not checked: v2.2 says set them to 1, but receivers
	 * should not reject stations that leave them 0. */
	*c_bit = (ssid_byte & SSID_C_BIT) != 0u;
	*ext_bit = (ssid_byte & SSID_EXT_BIT) != 0u;
	return AX25_OK;
}

ax25_status_t
ax25_ui_frame_parse(const uint8_t *data, size_t len, ax25_ui_frame_t *frame)
{
	if (data == NULL || frame == NULL) {
		return AX25_ERR_ARG;
	}
	if (len < AX25_FRAME_MIN_LEN) {
		return AX25_ERR_TOO_SHORT;
	}
	if (!ax25_fcs_check(data, len)) {
		return AX25_ERR_FCS;
	}

	bool dest_c, dest_ext, src_c, src_ext;
	ax25_status_t st =
	    read_address(&data[0], &frame->dest, &dest_c, &dest_ext);
	if (st != AX25_OK) {
		return st;
	}
	if (dest_ext) {
		return AX25_ERR_ADDRESS; /* address field cannot end after the
					    destination */
	}
	st = read_address(
	    &data[AX25_ADDR_FIELD_LEN], &frame->src, &src_c, &src_ext);
	if (st != AX25_OK) {
		return st;
	}
	if (!src_ext) {
		return AX25_ERR_UNSUPPORTED; /* digipeater path follows */
	}

	const size_t pos = 2u * AX25_ADDR_FIELD_LEN;
	if (!is_ui_control(data[pos])) {
		return AX25_ERR_UNSUPPORTED;
	}
	const size_t info_len = len - AX25_UI_HEADER_LEN - AX25_FCS_LEN;
	if (info_len > AX25_INFO_MAX_LEN) {
		return AX25_ERR_INFO_TOO_LONG;
	}

	frame->is_command = dest_c && !src_c;
	frame->control = data[pos];
	frame->pid = data[pos + 1u];
	frame->info = &data[AX25_UI_HEADER_LEN];
	frame->info_len = info_len;
	return AX25_OK;
}
