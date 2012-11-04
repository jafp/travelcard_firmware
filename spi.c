
#include "spi.h"

void SpiInit(void) {
	DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK) | (1<<DD_SS);
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (0<<SPR0) | (1<<SPIE);
}

void SpiTransmit(char data) {
	EmptyBuffer();
	PORT_SPI &= (0<<SPI_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
}

unsigned char SpiReceive(void) {
	PORT_SPI &= (0<<SPI_SS);
	SPDR = 0xF5;
	while(!(SPSR & (1<<SPIF)));
	PORT_SPI |= (1<<SPI_SS);
	_delay_ms(5);
	return SPDR;
}

void EmptyBuffer(void) {
	while((PIND & (1 << 3)) != 0) {
		SpiReceive();
	}
}