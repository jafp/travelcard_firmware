#include <avr/wdt.h>
#include "rfid.h"

void InitRFID(void) {
	SpiInit();
	DDRD = 0x0;
}

int CardPresent(void) {
	return (PIND & (1<<Card_Present)) != 0;
}

char GetCardStatus(void) {
	SpiTransmit(0x53);
	while((PIND & (1<<Data_Ready)) == 0);
	return SpiReceive();
}

char GetCardId(unsigned char * buffer) {
	unsigned char i;
	SpiTransmit(0x55);
	for(i = 0; i < 8 ; i++) {
		if ((PIND & (1<<Card_Present)) == 0) {
			return -1;
		}
		while((PIND & (1<<Data_Ready)) == 0) {
			wdt_reset();
		}
		buffer[i] = SpiReceive();
	}
	return 0;
}