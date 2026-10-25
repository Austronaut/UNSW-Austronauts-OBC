#ifndef AX25_STATUS_H
#define AX25_STATUS_H

/* Result codes shared by every layer of the AX.25 library. */
typedef enum {
	AX25_OK = 0,
	AX25_ERR_ARG,		/* NULL pointer or otherwise invalid argument */
	AX25_ERR_CALLSIGN,	/* callsign empty, too long, or has an invalid character */
	AX25_ERR_SSID,		/* SSID outside 0..15 */
	AX25_ERR_INFO_TOO_LONG, /* info field longer than AX25_INFO_MAX_LEN */
	AX25_ERR_NO_SPACE,	/* caller-provided output buffer is too small */
	AX25_ERR_TOO_SHORT,	/* received frame shorter than the minimum UI frame */
	AX25_ERR_FCS,		/* received frame failed the FCS check */
	AX25_ERR_ADDRESS,	/* malformed address field */
	AX25_ERR_UNSUPPORTED	/* valid AX.25 but outside scope (digipeater path, non-UI frame) */
} ax25_status_t;

#endif /* AX25_STATUS_H */
