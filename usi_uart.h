#ifndef USI_UART_H
#define USI_UART_H

// ********* System characterization ********* //
#define SYSTEM_CLOCK        8000000L
#define BAUD_RATE           9600L
#define TIMER_PRESCALER     8

// ********** Buffer sizes ********** //
#define UART_RX_BUF_SZ      4
#define UART_TX_BUF_SZ      4

#define UART_RX_BUF_MSK ( UART_RX_BUF_SZ - 1 )
#if ( UART_RX_BUF_SZ & UART_RX_BUF_MSK )
    #error RX buffer size is not a power of 2
#endif

#define UART_TX_BUF_MSK ( UART_TX_BUF_SZ - 1 )
#if ( UART_TX_BUF_SZ & UART_TX_BUF_MSK )
    #error TX buffer size is not a power of 2
#endif

// ********** Frame timing ********** //
#define CYCLES_PER_BIT ( SYSTEM_CLOCK / ( BAUD_RATE * TIMER_PRESCALER ) )
#define INTERRUPT_STARTUP_DELAY ( 0x11 / TIMER_PRESCALER )

#define USI_COUNTER_MAX     16
#define DATA_BITS           8
#define START_BIT           1
#define STOP_BIT            1
#define HALF_FRAME          5
#define USI_COUNTER_SEED_TX ( USI_COUNTER_MAX - HALF_FRAME )

#if ( ( CYCLES_PER_BIT * 3 / 2 ) > 256 )
    #define INITIAL_TIMER_SEED ( 256 - CYCLES_PER_BIT * 0.5 )
    #define USI_COUNTER_SEED_RX ( USI_COUNTER_MAX - ( START_BIT + DATA_BITS ) )
#else
    #define INITIAL_TIMER_SEED ( 256 - CYCLES_PER_BIT * 1.5 )
    #define USI_COUNTER_SEED_RX ( USI_COUNTER_MAX - DATA_BITS )
#endif
#define TIMER_SEED ( 256 - CYCLES_PER_BIT )

// ********** General defines ********** //
#define TRUE 1
#define FALSE 0

// ********** Prototypes ********** //

unsigned char   reverse_byte( unsigned char );
void            uuart_flush_buffers( void );

void            uuart_init_receiver( void );
unsigned char   uuart_rx_byte( void );
unsigned char   uuart_data_in_rx_buffer( void );

void            uuart_init_transmitter( void );
void            uuart_tx_byte( unsigned char );

#endif
