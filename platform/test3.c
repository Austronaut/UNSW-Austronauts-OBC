#include <driverlib.h>
#include "hal_clock.h"
#include "hal_spi.h"
#include "cc1101_regs.h"

/* Results are globals so they can be added to CCS's Expressions view
 * and read while the chip is halted. Expected values in comments. */
volatile uint8_t g_reset_err;    /* 0    - handshake worked          */
volatile uint8_t g_partnum;      /* 0x00                              */
volatile uint8_t g_version;      /* 0x14 (0x04 on some silicon)       */
volatile uint8_t g_marcstate;    /* 0x01 - IDLE                       */
volatile uint8_t g_status;       /* typically 0x0F from SNOP in IDLE  */
volatile uint8_t g_rb1;          /* 0xAB                              */
volatile uint8_t g_rb2;          /* 0x54                              */

#define RED_LED    GPIO_PIN0     /* P1.0 */
#define GREEN_LED  GPIO_PIN1     /* P1.1 */

int main(void)
{
    WDT_A_hold(WDT_A_BASE);
    HAL_Clock_Init();
    HAL_SPI_Init();

    GPIO_setAsOutputPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

    while (1)
    {
        g_reset_err = HAL_SPI_ResetCC1101();

        g_partnum   = HAL_SPI_ReadStatus(CC1101_PARTNUM);
        g_version   = HAL_SPI_ReadStatus(CC1101_VERSION);
        g_marcstate = HAL_SPI_ReadStatus(CC1101_MARCSTATE) & 0x1F;
        g_status    = HAL_SPI_Strobe(CC1101_SNOP);

        /* Two different values, so a stuck bus can't pass by luck */
        HAL_SPI_WriteReg(CC1101_SYNC1, 0xAB);
        g_rb1 = HAL_SPI_ReadReg(CC1101_SYNC1);
        HAL_SPI_WriteReg(CC1101_SYNC1, 0x54);
        g_rb2 = HAL_SPI_ReadReg(CC1101_SYNC1);

        GPIO_setOutputLowOnPin(GPIO_PORT_P1, RED_LED | GREEN_LED);

        if (g_reset_err != 0)
        {
            /* MISO never went low: wiring, power or CSN problem */
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, RED_LED);
        }
        else if (g_partnum == 0x00 &&
                 (g_version == 0x14 || g_version == 0x04) &&
                 g_marcstate == 0x01 &&
                 g_rb1 == 0xAB && g_rb2 == 0x54)
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, GREEN_LED);   /* all good */
        }
        else
        {
            /* Chip answered, but something read back wrong */
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, RED_LED | GREEN_LED);
        }

        __delay_cycles(100000);   /* ~100 ms between runs, gives the scope a repeating trigger */
    }
}
