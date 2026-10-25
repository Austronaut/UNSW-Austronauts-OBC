#include "ax25/ax25_fcs.h"

#define AX25_FCS_POLY_REFLECTED 0x8408u

uint16_t
ax25_fcs_update(uint16_t crc, uint8_t byte)
{
	crc = (uint16_t)(crc ^ byte);
	for (uint8_t i = 0u; i < 8u; i++) {
		if ((crc & 1u) != 0u) {
			crc = (uint16_t)((crc >> 1) ^ AX25_FCS_POLY_REFLECTED);
		}
		else {
			crc = (uint16_t)(crc >> 1);
		}
	}
	return crc;
}

uint16_t
ax25_fcs(const uint8_t *data, size_t len)
{
	uint16_t crc = AX25_FCS_INIT;
	for (size_t i = 0u; i < len; i++) {
		crc = ax25_fcs_update(crc, data[i]);
	}
	return (uint16_t)(crc ^ AX25_FCS_XOROUT);
}

bool
ax25_fcs_check(const uint8_t *frame, size_t len_with_fcs)
{
	if (frame == NULL || len_with_fcs < AX25_FCS_LEN) {
		return false;
	}
	const size_t data_len = len_with_fcs - AX25_FCS_LEN;
	const uint16_t expected = ax25_fcs(frame, data_len);
	const uint16_t received =
	    (uint16_t)(frame[data_len] | ((uint16_t)frame[data_len + 1u] << 8));
	return expected == received;
}
