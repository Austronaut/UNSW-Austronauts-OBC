#include <driverlib.h>
#include "hal_clock.h"
#include "hal_spi.h"

/* 0xAA = 0b10101010 - alternating bits, easy to recognize on a scope
 * trace and makes the rising/falling edges of CPOL/CPHA obvious. */
#define TEST_BYTE 0xAA

int main(void)
{
    WDT_A_hold(WDT_A_BASE);
    HAL_Clock_Init();
    HAL_SPI_Init();

    /* Onboard LEDs (P1.0 red, P1.1 green) - used here only to visually
     * mirror GDO0/GDO2 input state for the jumper-wire test. */
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