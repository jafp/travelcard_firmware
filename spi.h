
#ifndef _SPI_H_
#define _SPI_H_

#include "config.h"
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#define PORT_SPI	PORTB
#define DDR_SPI		DDRB
#define DD_MOSI		DDB5 
#define DD_MISO		DDB6 
#define DD_SCK 		DDB7
#define DD_SS 		DDB4
#define SPI_SS 		PB4

void SPI_Init(void);
void SPI_Transmit(uint8_t data);
uint8_t SPI_Receive(void);

#endif
