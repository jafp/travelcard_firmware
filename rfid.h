
#ifndef _RFID_H_
#define _RFID_H_

#include <stdint.h>
#include "spi.h"

#define RFID_CARD_PRESENT 	PD2
#define RFID_DATA_READY 	PD3

/**
 * RFID command for optaining the card status
 */
#define RFID_CMD_STATUS		0x53

/**
 * RFID command for optaining the card id
 */
#define RFID_CMD_ID			0x55

/**
 * RFID acknowledge response
 */
#define RFID_ACK_RESP		0x86


void RFID_Init(void);
uint8_t RFID_IsCardPresent(void);
uint8_t RFID_GetCardStatus(void);
uint8_t RFID_GetCardId(uint8_t * buffer);

#endif
