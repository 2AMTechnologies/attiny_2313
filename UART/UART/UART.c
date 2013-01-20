/*
 * UART.c
 *
 *  Author: 2AM Technologies
 *	https://2amtechnolgies.com
 *
 * This project shows you how to get set up the UART up and running
 * on your 2313.
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000 // in Herz
// You should define F_CPU BEFORE you include util/delay.h otherwise
// util/delay.h will supply a default value for F_CPU!
#include <util/delay.h>
// Also, a very important bit of information (locate in delay.h) is
// The maximal possible delay is 262.14 ms / F_CPU in MHz.

// Define our baud rate
#define USART_BAUDRATE 9600
// Calculate the baud scale from our desired baud rate
#define BAUD_SCALE (((F_CPU / (USART_BAUDRATE * 16UL))) -1 )

// We will put all of our set-up into a function we can call in the main program
void uartInit() {
	// Configure TX/RX
	UCSRB |= (1 << RXEN) | (1 << TXEN); // Turn on T/R circuitry
	UCSRC |= (1 << USBS) | (1 << UCSZ0) | (1 << UCSZ1); // Set to 8bits, 1 stop bit, no parity
	
	// Set the TX/RX baud rate
	UBRRH = (unsigned char)(BAUD_SCALE >> 8);
	UBRRL = (unsigned char) BAUD_SCALE;
	
	// Turn on Interrupts for TX/RX
	UCSRB |= (1 << RXCIE);
	sei();
	
}

int main(void)
{
	// Initialize the UART
	uartInit();
	
	// Print out the (US Ascii) characters from A to Z
	char asciiA = 0x41;
	for (int i=0; i <26; i++) {
		UDR = asciiA + i;
		// We don't want to flood UDR. 
		// If we send too much too fast, we could miss some bytes.
		_delay_ms(2);
	}		
		
    while(1)
    {
        // Leave the program running so we can still trigger the interrupt 
    }
}


/*
 * UART Interrupt 
 */
ISR(USART_RX_vect){
	PORTB |= (1 << 2);
	// The interrupt has been triggered, store the byte
	char receivedByte = UDR;
	// Add 1 to it
	char nextByte = receivedByte + 0x01;
	// Send the next byte. (If the user sends "A", then this will return "B", etc...
	UDR = nextByte;
	PORTB &= ~( 1 << 2);
}