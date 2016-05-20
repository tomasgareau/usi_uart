Half-duplex UART using the USI module for the attiny85, based on Atmel
application note AVR307.

See "echo.c" for an example program that implements a simple echo function.

See "usi_uart.h" for constant definitions & prototypes. A brief description:

    > reverse_byte
        - reverses a single byte in O(1) time with a lookup table

    > uuart_flush_buffers
        - clears the rx and tx buffers

    > uuart_init_receiver
        - initializes the receiver driver

    > uuart_rx_byte
        - receives a single byte

    > uuart_data_in_rx_buffer
        - returns false if the receive buffer is empty

    > uuart_init_transmitter
        - initializes the transmitter driver. called by uuart_tx_byte, so you
          don't need to call it yourself.

    > uuart_tx_byte
        - sends a single byte
