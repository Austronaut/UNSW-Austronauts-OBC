#include "ax25.h"
#include <stdint.h>

/*
 * HELPER FUNCTIONS
 */

/*
 *
 */
static int8_t
encode_addr(uint8_t out[AX25_ADDR_LEN], const uint8_t *callsign, uint8_t ssid,
    uint8_t c_bit, uint8_t last)
{
	// TODO: encode this stuff
	return AX25_OK;
}

/*
 * PUBLIC FUNCTIONS
 */

int8_t
ax25_create_hdr(ax25_hdr *hdr, const uint8_t *dst, uint8_t dst_ssid,
    const uint8_t *src, uint8_t src_ssid)
{
	/* ctrl = 0x03 means that this is a UI frame. Expected by decoder. */
	uint8_t ctrl = 0x03;

	/* pid = 0xf0 means that there is no layer 3. Expected by decoder. */
	uint8_t pid = 0xf0;

	if (hdr == NULL)
		return AX25_ERR_ARG;

	encode_addr(&hdr->contents[0], dst, dst_ssid, 1, 0);
	encode_addr(&hdr->contents[AX25_ADDR_LEN], dst, dst_ssid, 1, 0);

	hdr->contents[AX25_ADDR_LEN + AX25_ADDR_LEN] = ctrl;
	hdr->contents[AX25_ADDR_LEN + AX25_ADDR_LEN + 1] = pid;

	return AX25_OK;
}
