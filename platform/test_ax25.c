#include <driverlib.h>
#include "hal_clock.h"
#include "hal_spi.h"
#include "cc1101.h"
#include "ax25.h"
#include <string.h>

/* AX.25 addressing. TODO: set SRC_CALL to a real callsign before transmitting. */
#define DST_CALL "CQ"
#define DST_SSID 0
#define SRC_CALL "NOCALL"
#define SRC_SSID 0

/* Message is "TESTAX25 NN": NN counts 01..99, then wraps to 00 */
static const char msg_prefix[] = "TESTAX25 ";
#define PREFIX_LEN (sizeof(msg_prefix) - 1)
#define MSG_LEN	   (PREFIX_LEN + 2)

/* Results are globals so they can be read in CCS's Expressions view. */
volatile uint8_t g_init_err;   /* 0                                         */
volatile uint8_t g_mismatch;   /* 0   - config registers that read back wrong */
volatile uint8_t g_first_bad;  /* 0xFF if none, else index of first mismatch  */
volatile uint8_t g_pa0;	       /* 0xC0 - PATABLE[0] for +10 dBm               */
volatile uint8_t g_marc_tx;    /* 0x13 - TX, read a few ms after STX          */
volatile uint8_t g_marc_after; /* 0x01 - IDLE once the packet has finished    */
volatile int8_t g_ax25_err;    /* 0   - else AX25_ERR_* from create_hdr/encode */
volatile uint8_t g_count;      /*       counter in the last message sent       */

#define RED_LED	  GPIO_PIN0 /* P1.0 */
#define GREEN_LED GPIO_PIN1 /* P1.1 */

int
main(void)
{
	uint8_t readback[CC1101_CONFIG_LEN];
	uint8_t pa[8];
	uint8_t i;
	ax25_hdr hdr;
	uint8_t msg[MSG_LEN];
	uint8_t count = 0;
	int8_t frame_len = 0;

	/* Variable-length packet: CC1101 length byte first, then the AX.25
	 * frame */
	uint8_t packet[1 + AX25_OVERHEAD + MSG_LEN];

	WDT_A_hold(WDT_A_BASE);
	HAL_Clock_Init();
	HAL_SPI_Init();

	memcpy(msg, msg_prefix, PREFIX_LEN);
	g_ax25_err = ax25_create_hdr(&hdr, (const uint8_t *)DST_CALL, DST_SSID,
	    (const uint8_t *)SRC_CALL, SRC_SSID);

	GPIO_setAsOutputPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

	while (1) {
		g_init_err = CC1101_Init();

		/* Burst read all 47 config registers and compare with the table
		 */
		HAL_SPI_ReadBurstReg(
		    CC1101_CONFIG_START, readback, CC1101_CONFIG_LEN);
		g_mismatch = 0;
		g_first_bad = 0xFF;
		for (i = 0; i < CC1101_CONFIG_LEN; i++) {
			if (readback[i] != cc1101_config[i]) {
				if (g_mismatch == 0) {
					g_first_bad = i;
				}
				g_mismatch++;
			}
		}

		HAL_SPI_ReadBurstReg(CC1101_PATABLE, pa, 8);
		g_pa0 = pa[0];

		/* Next message: "TESTAX25 01", "TESTAX25 02", ... */
		count = (uint8_t)((count + 1) % 100);
		msg[PREFIX_LEN] = (uint8_t)('0' + count / 10);
		msg[PREFIX_LEN + 1] = (uint8_t)('0' + count % 10);
		g_count = count;

		if (g_ax25_err == AX25_OK) {
			frame_len = ax25_encode(&hdr, msg, MSG_LEN, &packet[1],
			    sizeof(packet) - 1);
			if (frame_len < 0) {
				g_ax25_err = frame_len;
			}
		}

		/* Load one packet and transmit it. Antenna must be attached. */
		if (g_ax25_err == AX25_OK) {
			packet[0] = (uint8_t)frame_len;
			HAL_SPI_Strobe(CC1101_SFTX);
			HAL_SPI_WriteBurstReg(
			    CC1101_TXFIFO, packet, (uint8_t)(frame_len + 1));
			CC1101_SetTx();
		}

		__delay_cycles(3000); /* ~3 ms: past calibration, still inside
					 the ~288 ms packet */
		g_marc_tx = HAL_SPI_ReadStatus(CC1101_MARCSTATE) & 0x1F;

		__delay_cycles(
		    400000); /* ~400 ms: packet finished, chip back in IDLE */
		g_marc_after = HAL_SPI_ReadStatus(CC1101_MARCSTATE) & 0x1F;

		GPIO_setOutputLowOnPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

		if (g_init_err != 0) {
			GPIO_setOutputHighOnPin(
			    GPIO_PORT_P1, RED_LED); /* handshake timeout */
		}
		else if (g_ax25_err == AX25_OK && g_mismatch == 0 &&
			 g_pa0 == 0xC0 &&
			 g_marc_tx == CC1101_MARC_TX &&
			 g_marc_after == CC1101_MARC_IDLE) {
			GPIO_setOutputHighOnPin(
			    GPIO_PORT_P1, GREEN_LED); /* all good */
		}
		else {
			GPIO_setOutputHighOnPin(GPIO_PORT_P1,
			    RED_LED | GREEN_LED); /* see g_ variables */
		}

		__delay_cycles(500000); /* ~500 ms between transmissions */
	}
}
