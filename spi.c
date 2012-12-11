/*--------------------------------------------------------

spi.c

This file contains functions related to SPI communication.

Version: 	1
Author: 	Jacob Pedersen
Company:	IHK
Date:		2012-12-01

--------------------------------------------------------*/

#include <avr/wdt.h>
#include "spi.h"

/**
 * Function used to empty possibly waiting data on the SPI
 * lines.
 */
static void 
EmptyBuffer(void) 
{
	while((PIND & (1 << 3)) != 0) 
	{
		SPI_Receive();
	}
}

/**
 * Initializes the SPI module.
 * Set-ups the data direction register and SPI control register.
 */
void 
SPI_Init(void) 
{
	/*
	 * MOSI 	Output
	 * SCK 		Output
	 * SS 		Output
	 */
 	DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);

	/*
	 * SPE 		SPI Enable
	 * MSTR		SPI Master Mode
	 * SPR1		f_osc / 64
	 */
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);
}

/**
 * Transmits the given data. A delay of 5 milliseconds
 * is hardcoded in the the of the function.
 * 
 * @param data The data to send
 */
void 
SPI_Transmit(uint8_t data) 
{
	EmptyBuffer();
	PORT_SPI &= (0<<SPI_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)))
	{
		// Do nothing
	}
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
}

/**
 * Receive a byte on the SPI connection.
 *
 * @return The received byte
 */
uint8_t 
SPI_Receive(void) 
{
	PORT_SPI &= (0<<SPI_SS);
	SPDR = 0xF5;
	while(!(SPSR & (1<<SPIF)))
	{
		// Do nothing
	}
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
	return SPDR;
}
