/* hal_spi.c — internal only, no header declaration */
static uint8_t SPI_TransferByte(uint8_t data)
{
    while (!(UCB1IFG & UCTXIFG));   /* wait for TX buffer ready */
    UCB1TXBUF = data;
    while (!(UCB1IFG & UCRXIFG));   /* wait for RX complete */
    return UCB1RXBUF;
}
