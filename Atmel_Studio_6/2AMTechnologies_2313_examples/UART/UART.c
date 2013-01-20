/*
 * UART.c
 *
 *	Author: 2AM Technologies
 *	https://2amtechnolgies.com
 *
 *	This project shows you how to get set up the UART up and running
 *	on your 2313.
 *
 *	Pinout: (for the PDIP/SOIC package)
 *	RX: pin 2 (PD0)
 *	TX: pin 3 (PD1)
 *	GND: pin 10 
 *
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 8000000 // in Herz
// At 1 MHz, UART doesn't work too well. 
// You should set the fuse to at least run at 8 MHz!

// You should define F_CPU BEFORE you include util/delay.h otherwise
// util/delay.h will supply a default value for F_CPU!
#include <util/delay.h>
// Also, a very important bit of information (located in the delay.h source) is
// "The maximal possible delay is 262.14 ms / F_CPU in MHz."
// At 8 MHz, thats a max of 32 ms! (Don't call _delay_ms(33 or higher!))

// Define our baud rate
#define USART_BAUDRATE 9600 // kbps
// Calculate the baud scale from our desired baud rate.
// This makes sure the baud rate is the same no matter what our 2313 is running at.
// This equation can be found on the data sheet.
#define BAUD_SCALE (((F_CPU / (USART_BAUDRATE * 16UL))) -1 )

// We will put all of our set-up into a function we can call in the main program
void uartInit() {
	// Configure TX/RX
	UCSRB |= (1 << RXEN) | (1 << TXEN); // Turn on T/R circuitry
	UCSRC |= (1 << USBS) | (1 << UCSZ0) | (1 << UCSZ1); // Set to 8 data bits, 1 stop bit, no parity
	
	// Set the TX/RX baud rate
	UBRRH = (unsigned char)(BAUD_SCALE >> 8);
	UBRRL = (unsigned char) BAUD_SCALE;
	
	// Turn on Interrupts for TX/RX
	UCSRB |= (1 << RXCIE);
	// Enable global interrupts
	sei();
	
}


// Define some things here we will use in both main() and the interrupt
//
// We'll blink some LEDs on PORTB just on send/receive just for fun!
// If you don't have any LED's hooked up, it doesn't matter.
uint8_t TX_LED  = PINB0; // pin 12
uint8_t RX_LED = PINB1; // pin 13
// If you confuse port numbers and pin numbers, you're gonna have a bad time.
//
// For the STK500, you have to set the pin to 1 for OFF, and 0 for ON
// You might need to switch this depending on your set up
uint8_t LED_ON = 0;
uint8_t LED_OFF = 1;
//
// US-ASCII Stuff
char asciiA = 0x41; // Ascii hex code for A
char asciiNL = 0x0a; // Ascii hex code for New Line
char asciiCR = 0x0d; // Ascii hex code for Carriage Return
char asciiNBSP = 0x20; // Ascii hex code for a space
//
int main(void)
{
	// Initialize the UART
	uartInit();
	
	
	// Data Direction Register B (DDRB)
	DDRB = 0x00; // Start as all inputs. 
	DDRB |= (1 << TX_LED); // Set LED1 as an output
	DDRB |= (1 << RX_LED); // Set LED 2 as an output
	// We could have also just done this as DDRB = 0x03;
	PORTB = (LED_OFF << TX_LED) | (LED_OFF << RX_LED);
	
	
	// Print out the (US Ascii) characters from A to Z
	// once on start up
	for (int i=0; i <26; i++) {
		// We're transmitting; turn our led on!
		PORTB = (LED_ON << TX_LED) | (LED_OFF << RX_LED);
		// Setting UDR = "hex code" will send out that hex code over the TX pin
		UDR = asciiA + i;
		UDR = asciiNBSP;
		// We don't want to flood UDR. 
		// If we send too much too fast, we could miss some bytes.
		_delay_ms(5);
		// We're done transmitting; turn our led off!
		PORTB = (LED_OFF << TX_LED) | (LED_OFF << RX_LED);
	}
	UDR = asciiNL;		
		
    while(1)
    {
        // Leave the program running so we can still trigger the interrupt 
    }
}


/*
 * UART (Receive) Interrupt 
 */
ISR(USART_RX_vect){
	// The interrupt has been triggered!
	// Flash our RX_LED !
	PORTB = (LED_OFF << TX_LED) | (LED_ON << RX_LED);
	// using "variable" = UDR is will "read" from the RX pin 
	// and store it in that variable.
	char receivedByte = UDR;
	_delay_ms(32); // The long delay is really only for the LEDs
	PORTB = (LED_OFF << TX_LED) | (LED_OFF << RX_LED);
	
	// Add 1 to the character received
	char nextByte = receivedByte + 0x01;
	// Send the next byte. 
	// (If the user sends "A", then this will return "B", etc...
	PORTB = (LED_ON << TX_LED) | (LED_OFF << RX_LED);
	UDR = nextByte;
	UDR = asciiNL;
	_delay_ms(32); // The long delay is really only for the LEDs
	PORTB = (LED_OFF << TX_LED) | (LED_OFF << RX_LED);
}