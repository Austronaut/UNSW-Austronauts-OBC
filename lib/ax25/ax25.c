#include "ax25.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * HELPER FUNCTIONS
 */
static uint8_t
valid_callsign_char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

/*
 * out: mem location where you place the encoded addr.
 * c_bit: 1 if command, 0 if not. A link layer detail; always set to 1.
 * last: 1 = this is the last address, 0 otherwise. (we are always last)
 * 6 callsign bytes shifted left by 1, then the SSID byte.
 */
static int8_t
encode_addr(uint8_t out[AX25_ADDR_LEN], const uint8_t *callsign, uint8_t ssid,
    uint8_t c_bit, uint8_t last)
{
	size_t len;

	if (callsign == NULL)
		return AX25_ERR_ARG;

	if (ssid > 15)
		return AX25_ERR_CALL;

	len = strlen((const char *)callsign);
	if (len > 6 || len == 0)
		return AX25_ERR_CALL;

	/*
	 * Build callsign. Expecting only upper case or digits for the
	 * callsign. Left-shift each callsign byte 1 and pad with spaces as per
	 * protocol spec.
	 */
	for (size_t i = 0; i < 6; i++) {
		char c = ' ';
		if (i < len) {
			c = callsign[i];
			if (!valid_callsign_char(c))
				return AX25_ERR_NO_SPACE;
		}
		out[i] = (uint8_t)((uint8_t)c << 1);
	}

	/* Build SSID byte */
	out[6] = (uint8_t)((c_bit ? 0x80 : 0x00) | 0x60 | (ssid << 1) |
			   (last ? 0x01 : 0x00));

	return AX25_OK;
}

/*
 * PUBLIC FUNCTIONS
 */

int8_t
ax25_create_hdr(ax25_hdr *hdr, const uint8_t *dst, uint8_t dst_ssid,
    const uint8_t *src, uint8_t src_ssid)
{
	int8_t err;

	/* ctrl = 0x03 means that this is a UI frame. Expected by decoder. */
	uint8_t ctrl = 0x03;

	/* pid = 0xf0 means that there is no layer 3. Expected by decoder. */
	uint8_t pid = 0xf0;

	if (hdr == NULL)
		return AX25_ERR_ARG;

	err = encode_addr(&hdr->contents[0], dst, dst_ssid, 1, 0);
	if (err != AX25_OK)
		return err;

	err = encode_addr(&hdr->contents[AX25_ADDR_LEN], dst, dst_ssid, 1, 0);
	if (err != AX25_OK)
		return err;

	hdr->contents[AX25_ADDR_LEN + AX25_ADDR_LEN] = ctrl;
	hdr->contents[AX25_ADDR_LEN + AX25_ADDR_LEN + 1] = pid;

	return AX25_OK;
}

/*
 * Frame Check Sequence, ie the CRC. Returns the 2 CRC bytes based on the data
 * of the frame.
 */
uint16_t
ax25_fcs(const uint8_t *data, size_t len)
{
	uint16_t crc = 0xFFFF;

	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int b = 0; b < 8; b++)
			crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0x8408)
					: (uint16_t)(crc >> 1);
	}
	return (uint16_t)(crc ^ 0xFFFF);
}

int8_t
ax25_encode(const ax25_hdr *hdr, const uint8_t *info, size_t info_len,
    uint8_t *out, size_t out_cap)
{
	size_t total;
	uint16_t fcs;

	if (hdr == NULL || out == NULL || (info_len > 0 && info == NULL))
		return AX25_ERR_ARG;

	if (info_len > AX25_MAX_INFO)
		return AX25_ERR_TOO_LONG;

	total = AX25_OVERHEAD + info_len;
	if (out_cap < total)
		return AX25_ERR_NO_SPACE;

	memcpy(out, hdr->contents, AX25_HDR_LEN);
	if (info_len > 0)
		memcpy(out + AX25_HDR_LEN, info, info_len);

	fcs = ax25_fcs(out, AX25_HDR_LEN + info_len);
	out[total - 2] = (uint8_t)(fcs & 0xFF);
	out[total - 1] = (uint8_t)(fcs >> 8);

	return (int8_t)total;
}
