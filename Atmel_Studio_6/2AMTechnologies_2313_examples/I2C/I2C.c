/*
 * I2C.c
 *
 *	Author: 2AM Technologies
 *	https://2amtechnolgies.com
 *
 *	This project shows you how to get set up I2C
 *	on your 2313. (aka Two Wire Interface (TWI) )
 *
 *	Pinout: (for the PDIP/SOIC package)
 *	SCL: pin 19 (PB7)
 *	SDA: pin 17 (PB5)
 *
 */ 


#include <avr/io.h>
#define F_CPU = 8000000UL; // Define before importing util/delay.h
#include <util/delay.h>;

// Useful defines
#define SYS_CLK 1000.0 // Typically 1000.0 or 4000.0
#define T2_TWI ((SYS_CLK *4700) /1000000) +1 // >4,7us
#define T4_TWI ((SYS_CLK *4000) /1000000) +1 // >4,0us
// These are 2313-specific
#define TWI_DDR DDRB
#define TWI_PORT PORTB
#define TWI_PIN PINB
#define TWI_SDA_PIN PINB5
#define TWI_SCL_PIN PINB7


// Set up a function to initialize the hardware
void twiInit(void) {
	
	// Enable SDA/SCL and set high as released
	TWI_PORT |= ( 1 << TWI_SDA_PIN);
	TWI_PORT |= ( 1 << TWI_SCL_PIN);
	
	// Set the Data direction to output on SDA/SCL
	DDRB = (1 << PINB5) | (1 << PINB7);
	TWI_DDR |= (1 << TWI_SDA_PIN);
	TWI_DDR |= (1 << TWI_SCL_PIN);
	
	// Pre-load data register with "released level" data
	USIDR = 0xFF;
	
	
	USICR =	(0 << USISIE) | (0 << USIOIE) | // Disable interrupts
			(1 << USIWM1) | (0 << USIWM0) | // Set the USI to Two-Wire mode
			(1 << USICS1) | (0 << USICS0) | // Set software Strobe as a counter 
			(1 << USICLK) |	(0 << USITC);	
	
	// Clear flags and reset counter
	USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC) |
			(0 << USICNT0);
	
}

// A function to transfer data two/from the hardware. 
// We don't use this directly, but the read/write functions do

uint8_t dataTransfer(uint8_t TEMP) {
	
	USISR = TEMP; // Set USISR according to the data
	
	// Prepare the clocking
	TEMP =	( 0 << USISIE ) | ( 0 << USIOIE ) | // Disable interrupts
			( 1 << USIWM1 ) | ( 0 << USIWM0 ) | // Set USI to Two-Wire Mode
			( 1 << USICS1 ) | ( 0 << USICS0 ) | // Set software Strobe as a counter
			( 1 << USICLK ) | ( 1 << USITC );	// Last part here toggles the Clock Port!
	
	do
	{
		
		__builtin_avr_delay_cycles(T2_TWI); //
		USICR = TEMP; // Send the new config which toggles the clock port (generates positive SCL edge)
		while(!(TWI_PIN & (1 << TWI_SCL_PIN))); // Wait for SCL to go high
		__builtin_avr_delay_cycles(T4_TWI); 
		USICR = TEMP; // Generate a negative SCL edge
		
	} while ( ! (USISR & (1 << USIOIF))); // Makes sure there is a complete transfer
	
	__builtin_avr_delay_cycles(T2_TWI);
	TEMP = USIDR; // Read the data
	USIDR = 0xff; // Release SDA
	TWI_DDR |= (1 << TWI_SDA_PIN); // Enable SDA as output
	
	return TEMP; // Return our data
}


// Here are two variables that will be used in the read/write functions.
// Putting it one place means less places to mis-type it :-)

// This sets the register to clear any flags and set USI
// to shift 8 bits (i.e. count 16 clock edges)
uint8_t tempUSISR_8bit =	(1 << USISIF ) | (1 << USIOIF ) |
							(1 << USIPF  ) | (1 << USIDC )	|
							(0x0 << USICNT0);

// This sets the register to clear any flags and set USI
// to shift 1 bit (i.e. count 2 clock edges)
uint8_t tempUSISR_1bit =	(1 << USISIF ) | (1 << USIOIF ) |
							(1 << USIPF  ) | (1 << USIDC )	|
							(0xe << USICNT0);
							
// A function to do a write
uint8_t twiWrite(uint8_t data) {
	
	TWI_PORT &= ~(1 << TWI_SCL_PIN); // Pull SCA low
	USIDR = data; // Set up the data
	dataTransfer(tempUSISR_8bit); // Transfer the data
	
	// Verify (N)ACK from slave
	TWI_DDR &= ~(1 << TWI_SDA_PIN); // Enable SDA as input
	uint8_t success = dataTransfer(tempUSISR_1bit) & (1 << 0) ;
	return success;
}

// A function to do a read with an ACK
uint8_t twiRead_ACK(void) {
	uint8_t data;
	TWI_DDR &= ~(1 << TWI_SDA_PIN); // Enable SDA as input
	data = dataTransfer(tempUSISR_8bit); // Read the data
	
	USIDR = 0x00; // Load up an ACK
	dataTransfer(tempUSISR_1bit); // Send the ACK
	
	return data; // Return our data
}

// A function to do a read with an NACK
uint8_t twiRead_NACK(void) {
	uint8_t data;
	TWI_DDR &= ~(1 << TWI_SDA_PIN); // Enable SDA as input
	data = dataTransfer(tempUSISR_8bit); // Read the data
	
	USIDR = 0xff; // Load up an NACK
	dataTransfer(tempUSISR_1bit); // Send the NACK
	
	return data; // Return our data
}

// A function to send an I2C start condition
void twiStart(void) {
	TWI_PORT |=  (1 << TWI_SCL_PIN); // Release SCL
	while( !(TWI_PORT & (1 << TWI_SCL_PIN))); // Wait for SCL to go high
	__builtin_avr_delay_cycles(T4_TWI);
	TWI_PORT &= ~(1 << TWI_SDA_PIN); // Pull SDA low
	__builtin_avr_delay_cycles(T4_TWI);
	TWI_PORT &= ~(1 << TWI_SCL_PIN); // Pull SCL low
	TWI_PORT |= (1 << TWI_SDA_PIN); // Release SDA
	
}

// A function to send an I2C stop condition
void twiStop(void) {
	
	TWI_PORT &= ~(1 << TWI_SDA_PIN); // Pull SDA low
	TWI_PORT |= (1 << TWI_SCL_PIN); // Release SCL
	while(!(TWI_PIN & (1 << TWI_SCL_PIN))); // Wait for SCL to go high
	__builtin_avr_delay_cycles(T4_TWI);
	TWI_PORT |= (1 << TWI_SDA_PIN); // Release SDA
	__builtin_avr_delay_cycles(T2_TWI);

}


int main(void)
{
	// We now have all of the functions to "talk" via I2C.
	// There is no example here, but it will generally go
	// as followed (with 0x00 bytes as place holders...)
	
	// Initialize
	twiInit();
	
	// A Write
	twiStart(); // Send a start condition
	twiWrite(0x00); // Send the device ID (write mode address)
	twiWrite(0x00); // Send the Register to write on the device
	twiWrite(0x00); // Send the data to write to the register
	twiStop(); // send a stop condition
	
	// A single Read
	twiStart(); // Send a start condition
	twiWrite(0x00); // Send the device ID (write mode address)
	twiWrite(0x00); // Send the Register to read on the device
	twiStart(); // Send ANOTHER start condition
	twiWrite(0x00 + 0x01); // Send device address (read mode address; usuall write mode adddress +1)
	uint8_t dataByte = twiRead_NACK(); // Read one byte (with a NACK to indicate that's all we want)
	twiStop(); // send a stop condition
	
	// A multiple-register Read
	int readLength = 10;
	uint8_t data[readLength];
	twiStart(); // Send a start condition
	twiWrite(0x00); // Send the device ID (write mode address)
	twiWrite(0x00); // Send the Register to read on the device
	twiStart(); // Send ANOTHER start condition
	twiWrite(0x00 + 0x01); // Send device address (read mode address; usuall write mode adddress +1)
	for (int i=0; i < readLength - 1; i++) { // Only read readLength - 1
		data[i] = twiRead_ACK();
	}
	data[readLength-1] = twiRead_NACK(); //Read last byte with a NACK
	twiStop(); // send a stop condition
	
	
    while(1)
    {
        // Do something cool here with your data!
		// We will have an example up soon of this + UART working with a BMP085 breakout board
    }
}