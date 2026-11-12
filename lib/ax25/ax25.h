#ifndef AX25_H
#define AX25_H

#include <stddef.h>
#include <stdint.h>

/*
 * AX.25 framing:
 * [dst addr 7][src addr 7][ctrl 0x03][pid 0xF0][info max=256][FCS 2]
 */
#define AX25_HDR_LEN   16
#define AX25_FCS_LEN   2
#define AX25_OVERHEAD  (AX25_HDR_LEN + AX25_FCS_LEN)
#define AX25_MAX_INFO  256
#define AX25_MAX_FRAME (AX25_OVERHEAD + AX25_MAX_INFO)
#define AX25_ADDR_LEN  7

/* Header structure for AX.25 framing */
typedef struct {
	uint8_t contents[AX25_HDR_LEN];
} ax25_hdr;

/* Error vals */
enum {
	AX25_OK = 0,
	AX25_ERR_ARG = -1,
	AX25_ERR_CALL = -2,	/* bad callsign or SSID */
	AX25_ERR_TOO_LONG = -3, /* info_len > AX25_MAX_INFO */
	AX25_ERR_NO_SPACE = -4, /* out_cap too small */
};

int8_t ax25_create_hdr(ax25_hdr *hdr, const uint8_t *dst, uint8_t dst_ssid,
    const uint8_t *src, uint8_t src_ssid);

uint16_t ax25_fcs(const uint8_t *data, size_t len);

int8_t ax25_encode(const ax25_hdr *hdr, const uint8_t *info, size_t info_len,
    uint8_t *out, size_t out_cap);

#endif
