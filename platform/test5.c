#include <driverlib.h>
#include "HAL_clock.h"
#include "HAL_SPI.h"
#include "cc1101.h"
#include "ax25/ax25_frame.h"

/* ---------------------------------------------------------------------
 * test5: same send path as test4, but the payload is an AX.25 UI frame.
 *
 * The radio config is untouched from test4 (433.92 MHz, ~1 kbaud 2-FSK,
 * variable length, CRC on). The CC1101 still adds its own preamble,
 * sync word, length byte and CRC. The AX.25 frame sits inside as the
 * packet payload:
 *
 *   CC1101:  preamble | sync | len | ............ payload ............ | CRC16
 *   AX.25:                         | dest(7) src(7) ctrl PID | data | FCS(2)
 *                                  '------- header ---------'       '-tail-'
 *
 *   header: dest + src callsigns (each char shifted left 1, then SSID
 *           byte), control 0x03 (UI), PID 0xF0 (no layer 3)
 *   tail:   FCS, CRC-16/X.25 over header + data, low byte first
 *
 * No HDLC flags, bit stuffing or NRZI: the CC1101 packet handler does
 * the framing on air. A receiver takes the CC1101 payload and parses it
 * as an AX.25 frame (e.g. ax25_ui_frame_parse()).
 *
 * Build: the AX.25 files test5 needs are copied from the OBC repo
 * (lib/ax25) into ax25/ in this project, so CCS compiles them and finds
 * "ax25/..." through the ${PROJECT_ROOT} include path. Only one test
 * with a main() can be in the build: test4.c is excluded.
 * ------------------------------------------------------------------- */

#define DEST_CALL     "CQ"
#define DEST_SSID     0u
#define SRC_CALL      "NOCALL"   /* put the team's licensed callsign here */
#define SRC_SSID      0u

/* PKTLEN in cc1101_config is 61, so the AX.25 frame can be at most 61
 * bytes: 16 header + up to 43 data + 2 FCS. */
#define MAX_FRAME_LEN 61u

/* Results are globals so they can be read in CCS's Expressions view. */
volatile uint8_t g_init_err;     /* 0                                         */
volatile uint8_t g_mismatch;     /* 0   - config registers that read back wrong */
volatile uint8_t g_first_bad;    /* 0xFF if none, else index of first mismatch  */
volatile uint8_t g_pa0;          /* 0xC0 - PATABLE[0] for +10 dBm               */
volatile uint8_t g_ax25_status;  /* 0   - AX25_OK from build + parse-back       */
volatile uint8_t g_frame_len;    /* 27  - AX.25 bytes (16 + 9 data + 2 FCS)     */
volatile uint8_t g_marc_tx;      /* 0x13 - TX, read a few ms after STX          */
volatile uint8_t g_marc_after;   /* 0x01 - IDLE once the packet has finished    */
volatile uint8_t g_count;        /* frame counter, also sent in the data        */

#define RED_LED    GPIO_PIN0     /* P1.0 */
#define GREEN_LED  GPIO_PIN1     /* P1.1 */

/* Builds the AX.25 UI frame for "TEST5 nnn" into packet[1..] and puts
 * its length in packet[0] (the CC1101 length byte). Parses the result
 * back as a self-check of the header and FCS. */
static ax25_status_t build_packet(uint8_t count, uint8_t packet[1 + MAX_FRAME_LEN],
                                  uint8_t *frame_len)
{
    uint8_t data[] = { 'T', 'E', 'S', 'T', '5', ' ', '1', '2', '3' };
    ax25_ui_frame_t f;
    ax25_ui_frame_t check;
    ax25_status_t st;
    size_t len;

    data[6] = (uint8_t)('0' + count / 100u);
    data[7] = (uint8_t)('0' + (count / 10u) % 10u);
    data[8] = (uint8_t)('0' + count % 10u);

    st = ax25_ui_frame_init(&f, DEST_CALL, DEST_SSID, SRC_CALL, SRC_SSID,
                            data, sizeof(data));
    if (st != AX25_OK) { return st; }

    /* Header + data + FCS, straight into the packet after the length byte */
    st = ax25_ui_frame_build(&f, &packet[1], MAX_FRAME_LEN, &len);
    if (st != AX25_OK) { return st; }

    st = ax25_ui_frame_parse(&packet[1], len, &check);
    if (st != AX25_OK) { return st; }

    packet[0]  = (uint8_t)len;
    *frame_len = (uint8_t)len;
    return AX25_OK;
}

int main(void)
{
    uint8_t readback[CC1101_CONFIG_LEN];
    uint8_t pa[8];
    uint8_t packet[1 + MAX_FRAME_LEN];
    uint8_t frame_len = 0;
    uint8_t i;

    WDT_A_hold(WDT_A_BASE);
    HAL_Clock_Init();
    HAL_SPI_Init();

    GPIO_setAsOutputPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

    while (1)
    {
        g_marc_tx    = 0xFF;
        g_marc_after = 0xFF;

        g_init_err = CC1101_Init();

        /* Burst read all 47 config registers and compare with the table */
        HAL_SPI_ReadBurstReg(CC1101_CONFIG_START, readback, CC1101_CONFIG_LEN);
        g_mismatch  = 0;
        g_first_bad = 0xFF;
        for (i = 0; i < CC1101_CONFIG_LEN; i++)
        {
            if (readback[i] != cc1101_config[i])
            {
                if (g_mismatch == 0) { g_first_bad = i; }
                g_mismatch++;
            }
        }

        HAL_SPI_ReadBurstReg(CC1101_PATABLE, pa, 8);
        g_pa0 = pa[0];

        g_ax25_status = (uint8_t)build_packet(g_count, packet, &frame_len);
        g_frame_len   = frame_len;

        if (g_init_err == 0 && g_ax25_status == AX25_OK)
        {
            /* Load length byte + AX.25 frame and transmit. Antenna must be attached. */
            HAL_SPI_Strobe(CC1101_SFTX);
            HAL_SPI_WriteBurstReg(CC1101_TXFIFO, packet, (uint8_t)(1u + frame_len));
            CC1101_SetTx();

            __delay_cycles(3000);    /* ~3 ms: past calibration, packet still going */
            g_marc_tx = HAL_SPI_ReadStatus(CC1101_MARCSTATE) & 0x1F;

            /* 2 preamble + 2 sync + 1 len + 27 frame + 2 CRC = 34 bytes,
             * ~272 ms at 1 kbaud */
            __delay_cycles(400000);  /* ~400 ms: packet finished, chip back in IDLE */
            g_marc_after = HAL_SPI_ReadStatus(CC1101_MARCSTATE) & 0x1F;
        }

        GPIO_setOutputLowOnPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

        if (g_init_err != 0)
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, RED_LED);                /* handshake timeout */
        }
        else if (g_mismatch == 0 && g_pa0 == 0xC0 && g_ax25_status == AX25_OK &&
                 g_marc_tx == CC1101_MARC_TX && g_marc_after == CC1101_MARC_IDLE)
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, GREEN_LED);              /* all good */
        }
        else
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, RED_LED | GREEN_LED);    /* see g_ variables */
        }

        g_count++;
        __delay_cycles(500000);  /* ~500 ms between transmissions */
    }
}