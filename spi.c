
#include <avr/wdt.h>
#include "spi.h"

static void EmptyBuffer(void) 
{
	while((PIND & (1 << 3)) != 0) 
	{
		SPI_Receive();
	}
}

void SPI_Init(void) 
{
	DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (0<<SPR0);
}

void SPI_Transmit(uint8_t data) 
{
	EmptyBuffer();
	PORT_SPI &= (0<<SPI_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF))) 
	{
		// XXX: We get stuck here sometimes!!
		//wdt_reset();
	}
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
}

uint8_t SPI_Receive(void) 
{
	PORT_SPI &= (0<<SPI_SS);
	SPDR = 0xF5;
	while(!(SPSR & (1<<SPIF)))
	{
		// XXX: We get stuck here sometimes!!
		//wdt_reset();
	}
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
	return SPDR;
}
