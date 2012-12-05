
#include <string.h>
#include <avr/wdt.h>
#include "rfid.h"

void RFID_Init(void) 
{
	SPI_Init();
	DDRD = 0x0;
}

uint8_t RFID_IsCardPresent(void) 
{
	return (PIND & (1 << RFID_CARD_PRESENT));
}

uint8_t RFID_GetCardStatus(void) 
{
	SPI_Transmit(RFID_CMD_STATUS);
	while((PIND & (1 << RFID_DATA_READY)) == 0);
	return SPI_Receive();
}

uint8_t RFID_GetCardId(uint8_t * buffer) 
{
	uint8_t i = 8;
	memset(buffer, 0, sizeof(buffer));

	SPI_Transmit(RFID_CMD_ID);

	while(!(PIND & (1<<RFID_DATA_READY)));
	uint8_t status = SPI_Receive();

	if (status != 0x86) {
		return -1;
	}

	/**
	 * The bytes of the card ID is received en little endian order thus
	 * on the server side we are using big endian order (because of the ByteBuffer).
	 */
	while (--i) 
	{
		if ((PIND & (1<<RFID_CARD_PRESENT)) == 0) 
		{
			return -1;
		}
		while((PIND & (1<<RFID_DATA_READY)) == 0) 
		{
			wdt_reset();
		}
		buffer[i] = SPI_Receive();
	}
	return 0;
}