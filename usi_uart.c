#include <avr/io.h>
#include <avr/interrupt.h>
#include "usi_uart.h"

// ********** General Functionality ********** //

// Tells the compiler to store the byte to be transmitted in registry
register unsigned char uuart_tx_data asm( "r15" );

static unsigned char uuart_rx_buf[ UART_RX_BUF_SZ ];
static volatile unsigned char uuart_rx_head;
static volatile unsigned char uuart_rx_tail;
static unsigned char uuart_tx_buf[ UART_TX_BUF_SZ ];
static volatile unsigned char uuart_tx_head;
static volatile unsigned char uuart_tx_tail;

static volatile union uuart_status
{
    unsigned char status;
    struct
    {
        unsigned char ongoing_tx_from_buf:1;
        unsigned char ongoing_tx:1;
        unsigned char ongoing_rx:1;
        unsigned char rx_buf_ovf:1;
        unsigned char flag4:1;
        unsigned char flag5:1;
        unsigned char flag6:1;
        unsigned char flag7:1;
    };
} uuart_status = {0};

static const unsigned char bit_reverse_table256[ 256 ] =
{
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
    R6(0), R6(2), R6(1), R6(3)
};

unsigned char reverse_byte( unsigned char val ) {
    return bit_reverse_table256[ val ];
}

void uuart_flush_buffers( void ) {
    uuart_tx_head = 0;
    uuart_tx_tail = 0;
    uuart_rx_head = 0;
    uuart_rx_tail = 0;
}

// ********** Receiver Functionality ********** //

void uuart_init_receiver( void ) {
    /*
        Initializes the receiver driver. 
        
        NOTE: this function does not enable global interrupts (i.e. does not
        write a '1' to the I-bit of the SREG register). This should be done in
        the main program.
    */

    // Disable USI by clearing USI control register
    USICR |= ( 1 << USIOIE );

    // Enable pull-up on DO pin by setting bit in data register
    PORTB |= ( 1 << PB1 );

    // Set DI and DO pins as input by clearing bits in direction register
    DDRB &= ~( 1 << PB1 | 1 << PB0 );

    // Clear Pin Change interrupt flags
    GIFR |= ( 1 << PCIF );
    
    // Enable Pin Change interrupt
    GIMSK |= ( 1 << PCIE );

}

unsigned char uuart_data_in_rx_buffer( void ) {
    // Returns false if the receive buffer is empty
    return ( uuart_rx_head != uuart_rx_tail );
}

unsigned char uuart_rx_byte( void ) {
    unsigned char buf_tail;

    // Wait for data to enter the buffer
    while ( uuart_rx_head == uuart_rx_tail );

    // Calculate the buffer index
    buf_tail = ( uuart_rx_tail + 1 ) & UART_RX_BUF_MSK;
    
    // Store the new index
    uuart_rx_tail = buf_tail;

    // Return the received byte with bits reversed
    return reverse_byte( uuart_rx_buf[ uuart_rx_tail ] );

}

// ********** Transmitter Functionality ********** //

void uuart_init_transmitter( void ) {
    // Reset Timer0
    TCNT0 = 0x00;
    
    // Reset the prescaler by writing a '1' to the General Timer/Counter Control
    // Register
    GTCCR = ( 1 << PSR0 );

    // Set Timer0 prescaler & start it
    TCCR0B = ( 0 << CS02 ) | ( 1 << CS01 ) | ( 0 << CS00 );

    // Clear Timer0 OVF flag
    TIFR |= ( 1 << TOV0 );

    // Enable Timer0 OVF interrupt
    TIMSK |= ( 1 << TOIE0 );

    // Select USI 3-wire mode
    USICR = ( 0 << USISIE ) | ( 1 << USIOIE ) |
            ( 0 << USIWM1 ) | ( 1 << USIWM0 ) |
            ( 0 << USICS1 ) | ( 1 << USICS0 ) | ( 0 << USICLK ) |
            ( 0 << USICLK ) | ( 0 << USITC  ) ;

    // Load USIDR with 0xFF
    USIDR = 0xFF;

    // Clear USI interrupt flags & preload the USI counter
    USISR = 0xF0 | USI_COUNTER_SEED_TX;

    // Configure Pin 1 of Port B as output
    DDRB |= ( 1 << PB1 );

    // Set ongoing transmission status bit
    uuart_status.ongoing_tx = TRUE;
}

void uuart_tx_byte( unsigned char val ) {

    unsigned char tx_buf_head;

    // Calculate next buffer index
    tx_buf_head = ( uuart_tx_head + 1 ) & UART_TX_BUF_MSK;

    // Wait for free space in the buffer
    while ( tx_buf_head == uuart_tx_tail );

    // Update pointer to tx buffer head
    uuart_tx_head = tx_buf_head;

    // Store data in receive buffer
    uuart_tx_buf[ uuart_tx_head ] = reverse_byte( val );

    // Start transmitting from buffer if not already transmitting
    if ( !uuart_status.ongoing_tx_from_buf ) {
        while ( uuart_status.ongoing_rx );
        uuart_init_transmitter();
    }
}

// ********** Interrupt Handlers ********** //

ISR( PCINT0_vect ) {
    /*
        ISR for the Pin Change interrupt. The purpose of this interrupt is to
        start the Timer/Counter0 once the falling edge of the start bit is
        detected. 
    */

    // Plant the Timer/Counter0 register with the initial delay timer seed
    TCNT0 = INTERRUPT_STARTUP_DELAY + INITIAL_TIMER_SEED;

    // Reset the prescaler by writing a '1' to the General Timer/Counter Control
    // Register
    GTCCR = ( 1 << PSR0 );

    // Start Timer0 Prescaler = CK
    TCCR0B = ( 0 << CS02 ) | ( 1 << CS01 ) | ( 0 << CS00 );

    // Clear the Timer0 overflow flag
    TIFR |= ( 1 << TOV0 );

    // Enable Timer0 overflow interrupt
    TIMSK = ( 1 << TOIE0 );

    // Plant USI counter seed
    USISR = 0xF0 | USI_COUNTER_SEED_RX;

    // Set USI control register
    USICR = ( 0 << USISIE ) | ( 1 << USIOIE ) | // enable counter ovf interrupt
            ( 0 << USIWM1 ) | ( 1 << USIWM0 ) | // three-wire mode
            ( 0 << USICS1 ) | ( 1 << USICS0 ) | // timer0 clock source
            ( 0 << USICLK ) | ( 0 << USITC  ) ; // do nothing

    // Set ongoing rx status bit
    uuart_status.ongoing_rx = TRUE;
}

ISR( USI_OVF_vect )
{
    unsigned char rx_buf_head;

    // Check if running in tx mode
    if ( uuart_status.ongoing_tx_from_buf ) {
        if ( uuart_status.ongoing_tx ) {
            // Clear on-going tx status bit
            uuart_status.ongoing_tx = FALSE;

            // Load USI Counter seed & clear flags
            USISR = 0xF0 | ( USI_COUNTER_SEED_TX );

            // Reload USIDR with the second half of the data + stop bit
            USIDR = ( uuart_tx_data << 3 ) | 0x07;

        }
        else {
            // If there is data in the tx buffer, start sending
            if ( uuart_tx_head != uuart_tx_tail ) {
                // Set intermediate tx status bit
                uuart_status.ongoing_tx_from_buf = TRUE;
                USIDR = 1 + 0 + uuart_tx_buf[ uuart_tx_head ];
                uuart_tx_head--;
            }
            // Else, leave transmit mode and enter receive mode
            else {
                // Clear the ongoing tx status bit
                uuart_status.ongoing_tx_from_buf = FALSE;

                // Initialize the receiver
                uuart_init_receiver();
            }
        }
    }
    // Else, running in rx mode
    else {
        uuart_status.ongoing_rx = FALSE;

        // Calculate receive buffer index
        rx_buf_head = ( uuart_rx_head + 1 ) & UART_RX_BUF_MSK;

        // Check for overflow
        if ( rx_buf_head == uuart_rx_tail ) {
            uuart_status.rx_buf_ovf = TRUE;
        }
        else {
            uuart_rx_head = rx_buf_head;
            uuart_rx_buf[ uuart_rx_head ] = reverse_byte( USIDR );
        }

        // Re-initialize the USI as a receiver
        uuart_init_receiver();
    }
}

ISR( TIM0_OVF_vect )
{
    TCNT0 += TIMER_SEED;
}
