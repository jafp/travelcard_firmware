
#include "config.h"
#include <avr/io.h>
#include <util/delay.h>

#define PORT_SPI	PORTB
#define DDR_SPI		DDRB
#define DD_MOSI		DDB5 
#define DD_MISO		DDB6 
#define DD_SCK 		DDB7
#define DD_SS 		DDB4
#define SPI_SS 		PB4

void SpiInit(void);
void SpiTransmit(char data);
unsigned char SpiReceive(void);
void EmptyBuffer(void);