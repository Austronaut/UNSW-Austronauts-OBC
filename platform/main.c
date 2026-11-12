#include <driverlib.h>
#include "hal_clock.h"
#include "hal_spi.h"

#define TEST_BYTE 0xAA

int main(void)
{
    WDT_A_hold(WDT_A_BASE);        /* stop watchdog - must be first, or the
                                     * chip resets itself before anything runs */

    HAL_Clock_Init();              /* SMCLK -> ~1 MHz DCO, must precede
                                     * HAL_SPI_Init (assumes this frequency) */

    HAL_SPI_Init();                /* eUSCI_B1 SPI + CS/GDO0/GDO2 pins +
                                     * LOCKLPM5 unlock */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0 | GPIO_PIN1);
    
    while (1)
    {
        /* --- SPI / clock / CPOL / CPHA / CS test --- */
        HAL_SPI_CS_Assert();            /* should pulse P4.4 low */

        SPI_TransferByte(TEST_BYTE);    /* each call clocks 8 bits on P5.2,
                                          * data on P5.0 (MOSI) */
        SPI_TransferByte(TEST_BYTE);
        SPI_TransferByte(TEST_BYTE);
        SPI_TransferByte(TEST_BYTE);

        HAL_SPI_CS_Deassert();          /* P4.4 back high */

        /* --- GDO0 / GDO2 direction test --- */
        if (GPIO_getInputPinValue(GDO0_PORT, GDO0_PIN))
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        else
            GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

        if (GPIO_getInputPinValue(GDO2_PORT, GDO2_PIN))
            GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);
        else
            GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);

        __delay_cycles(50000);   /* ~50ms pause - makes the CS pulse easy
                                   * to find and trigger on with a scope */
    }
}
