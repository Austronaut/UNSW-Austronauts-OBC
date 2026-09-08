#ifndef UART_H
#define UART_H

// Configures eUSCI_A0 for UART at 9600 baud on P2.0 (TX) / P2.1 (RX).
// Assumes clock_init_8mhz() has already been called - the 9600 baud
// timing is calculated against an 8MHz clock and will be wrong otherwise.
void uart_init(void);

// Blocks until the byte is fully handed to the transmit buffer, then returns.
void uart_send_char(char c);

// Sends a null-terminated string, one byte at a time.
void uart_send_string(char *text);

#endif // UART_H
