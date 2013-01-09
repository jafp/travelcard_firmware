
#ifndef _COMMON_H_
#define _COMMON_H_

/**
 * For port and pin names
 */
#include <avr/io.h>
 
/**
 * For bit manipulation helper macros
 */
#include "bits.h"

#define BTN_PORT	PIND
#define BTN_PIN		PD0

#define RED_PIN		PC3
#define YELLOW_PIN	PC2
#define GREEN_PIN	PC1
#define SPEAKER_PIN	PC0

#define RED_ON 		clr(PORTC, RED_PIN)
#define RED_OFF		set(PORTC, RED_PIN)
#define YELLOW_ON	clr(PORTC, YELLOW_PIN)
#define YELLOW_OFF	set(PORTC, YELLOW_PIN)
#define GREEN_ON	clr(PORTC, GREEN_PIN);
#define GREEN_OFF	set(PORTC, GREEN_PIN);
#define SPEAKER_ON	set(PORTC, SPEAKER_PIN);
#define SPEAKER_OFF	clr(PORTC, SPEAKER_PIN);

/**
 * USB Command codes
 */
#define CMD_ECHO				0
#define CMD_RESPONSE			3
#define CMD_KEEP_ALIVE			4

/**
 * USB Command Acknowledge Code
 */
#define CMD_RESPONSE_ACK		0xE5

/**
 * Response codes
 */
#define RESP_ERROR				1
#define RESP_CARD_NOT_FOUND		2
#define RESP_INSUFFICIENT_FUNDS	3
#define RESP_TOO_LATE_CHECK_OUT	4
#define RESP_INVALID_CARD		5
#define RESP_CHECKED_IN			6
#define RESP_CHECKED_OUT		7
#define RESP_OK 				8

#endif
