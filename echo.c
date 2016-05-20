#include "usi_uart.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <string.h>

int main( void ) {
    const char echo_str[] = "\nEcho: ";

    unsigned char i;

    uuart_flush_buffers();
    uuart_init_receiver();

    // Enable global interrupts
    cli();

    for (;;) {
        if ( uuart_data_in_rx_buffer() ) {
            for ( i = 0; i < strlen( echo_str ); i++ ) {
                uuart_tx_byte( (unsigned int)echo_str[i] );
            }
            uuart_tx_byte( uuart_rx_byte() );
        }
        sleep_enable();
    }

    return 0;
}
