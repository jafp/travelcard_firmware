/*--------------------------------------------------------

rfid.c

This file contains functions related to the RFID card 
reader. This includes functions for reading the status
of a card, and reading the card serial number.

Version: 	1
Author: 	Jacob Pedersen
Company:	IHK
Date:		2012-12-01

--------------------------------------------------------*/

#include "config.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv/usbdrv.h"
#include "common.h"
#include "lcd.h"
#include "usb.h"
#include "rfid.h"
#include "test.h"
#include "lang.h"


/**
 * Forward declaration of setStatus
 */
static void setStatus(const char *, uint8_t);

/**
 * Struct with information about responses from the server
 */
struct response {
	/**
	 * The response code
	 */
	uint8_t code;

	/**
	 * The balance received from the server
	 */
	uint16_t balance;

	/**
	 * The price received by the server
	 */
	uint16_t price;

};

/**
 * Struct with information about the processing of a card scan.
 */
struct transaction {
	/**
	 * The card ID read 
	 */
	uint8_t card_id[8];

	/**
	 * The latest command requested by the server
	 */
	uint8_t command;

	/**
	 * The latest response
	 */
	struct response response;

};

/**
 * Enum of possible terminal states
 */
enum terminal_state_t { starting, idle, scanning, processing, info, no_connection };

/**
 * Flag indicating if we should use the buzzer.
 */
static uint8_t use_buzzer;

/**
 * USB reply buffer
 */
static uint8_t reply_buffer[8];

/**
 * First step in state flag
 */
static uint8_t first_step = 1;

/**
 * The current terminal state.
 */
static enum terminal_state_t state = starting;

/**
 * The current transaction / customer interaction
 */
static struct transaction current;

/**
 * Buffer holding the data to echo back
 */
uint8_t echo_buffer[8];

/**
 * Flag for indicating echo data ready
 */
volatile uint8_t echo_ready = 0;


/**
 * USB polling ISR. This interrupt get called at regular intervals
 * to keep the USB connection alive.
 */
ISR(TIMER0_COMP_vect, ISR_NOBLOCK)
{
	usbPoll();
}

/**
 * Keep alive timer
 */
ISR(TIMER1_COMPA_vect, ISR_NOBLOCK)
{
	state = no_connection;
	first_step = 1;
}

/**
 * Helper function to generate a delay.
 * The watchdog timer is kept busy in the meanwhile.
 *
 * @param ms Length of the delay in milliseconds
 */
inline void 
delay(uint8_t ms) 
{
	while (ms--) 
	{
		_delay_ms(1);
		wdt_reset();
	}
}

/**
 * Beeps the speaker in the given amount of time
 *
 * @param Length of beep in milliseconds
 */
inline void 
speakerBeep(uint8_t ms) 
{
	SPEAKER_ON;
	delay(ms);
	SPEAKER_OFF;
}

/**
 * Notify that we're still alive!
 */
static void 
isAlive(void)
{
	unsigned char i = SREG;
	cli();

	TCNT1 = 0x0000;
	SREG = i;

	if (state == no_connection)
	{
		state = starting;
		first_step = 1;
	}
}

/**
 * USB request handler. Get called by the USB library every
 * time a request is made to the device. 
 *
 * @param data The USB request packet
 */
usbMsgLen_t 
usbFunctionSetup(uint8_t data[8])
{
    uint8_t len = 0;
    current.command = data[1];

    if (data[1] == CMD_ECHO)
    {	
    	len = USB_NO_MSG;

    }
    else if (data[1] == CMD_RESPONSE)
    {
    	len = USB_NO_MSG;
    }
    else if (data[1] == CMD_KEEP_ALIVE)
    {
    	isAlive();
    	len = 0;
    } 

    usbMsgPtr = reply_buffer;
    return len;
}

/**
 * USB request data handler. This method is called directly from the 
 * USB library.
 *
 * @param data An array of unsigned 8-bit integers
 * @param len The length of the data array	
 */
USB_PUBLIC uint8_t 
usbFunctionWrite(uint8_t *data, uint8_t len)
{
	current.response.code = data[0];

	if (current.command == CMD_RESPONSE)
	{
		if ( current.response.code == RESP_CHECKED_IN 
			|| current.response.code == RESP_CHECKED_OUT 
			|| current.response.code == RESP_INSUFFICIENT_FUNDS) 
		{
			current.response.balance = data[2] | (data[1] << 8);
		}

		if ( current.response.code == RESP_CHECKED_OUT 
			|| current.response.code == RESP_INSUFFICIENT_FUNDS )
		{
			current.response.price = data[4] | (data[3] << 8); 
		}
	}
	else if (current.command == CMD_ECHO)
	{
		memcpy(echo_buffer, data, len);
		echo_ready = 1;
	}

	return 1;
}

/**
 * Prints the given message on the given line.
 * The line are cleared before print.
 * 
 * @param msg The string to print
 * @param msg The line number
 */
static void 
setStatus(const char * msg, uint8_t line)
{
	LCD_GotoXY(0, line);
	LCD_PutString("                ");
	LCD_GotoXY(0, line);
	LCD_PutString(msg);
}

/**
 * @return Non-zero if the black button is pressed.
 */
static uint8_t
isButtonPressed(void)
{
	return (BTN_PORT & (1 << BTN_PIN)) == 0;
}

/**
 * Creates a string like "Saldo: [current balance] kr"
 *
 * @param buffer The buffer we write the string to
 */
static void 
makeBalance(char *buffer)
{
	sprintf(buffer, L_BALANCE, current.response.balance);
}

/**
 * Hardware setup peripherals
 */
static void 
setup(void)
{
	wdt_enable(WDTO_1S);

	use_buzzer = 1;

	DDRC |= (1 << RED_PIN) | (1 << YELLOW_PIN) | (1 << GREEN_PIN) | (1 << SPEAKER_PIN);
	PORTC |= 0x0E;

	TCCR1B |= (1 << WGM12);
    TIMSK |= (1 << OCIE1A);
    OCR1A = 65400; // 256 prescaler --> 1 Hz

	TCCR0 |= (1 << WGM01);
	OCR0 = 255; // 1024 prescaler --> 250 Hz
	TIMSK |= (1 << OCIE0);

	LCD_Init(16);
	LCD_Clear();
	LCD_GotoXY(0, 0);

	USB_InitAndConnect();

	// TIMER1: 256 prescaler
	TCCR1B |= (1 << CS12); 

	// TIMER0: 1024 prescaler
	TCCR0 |=  (1 << CS02) | (1 << CS00); 

	RFID_Init();
}

/**
 * The main function with the main loop and state machine. 
 * This function should never return, so we tell the compiler that
 * by annotating the function with a noreturn-attribute.
 */
int __attribute__((noreturn)) 
main(void)
{
	// Utility counter variables
	unsigned int i = 0;
	unsigned char j = 0;
	// Buffer used when generating output for the display
	char buf[16];

	// Perform setup of registers and peripherals
	setup();

	// Indicate setup done
	GREEN_ON;

	if (isButtonPressed())
	{
		wdt_disable();
		test();

		for (;;);
	}
	else
	{
		// Main loop and state machine
		for (;;)
		{
			wdt_reset();

			switch (state)
			{
				case starting:
				{
					// Start-up phase.
					// Do nothing in a bunch of clock cycles
					setStatus(L_STARTING, 0);
					setStatus("", 1);

					for (i = 0; i < 100; i++) 
					{
						wdt_reset();
						_delay_ms(10);
					}
					state = idle;
					first_step = 1;

					break;
				}
				case idle:
				{
					GREEN_ON;

					if (first_step)
					{
						first_step = 0;

						setStatus(L_SCAN_HERE, 0);
						setStatus("", 1);
					}
					
					if (RFID_IsCardPresent())
					{
						GREEN_OFF;
						first_step = 1;
						state = scanning;
					}

					break;
				}
				case scanning:
				{
					if (first_step)
					{
						first_step = 0;
						setStatus(L_WORKING, 0);
					}

					uint8_t n = RFID_GetCardId(current.card_id);
					if (n == -1) 
					{
						state = info;
						current.response.code = RESP_INVALID_CARD;
						first_step = 1;
					}
					else
					{
						if (!usbInterruptIsReady())
						{
							wdt_reset();
						}
						usbSetInterrupt(current.card_id, 8);

						first_step = 1;
						state = processing;
					}
					break;
				}
				case processing:
				{
					if (current.response.code != 0)
					{
						if (use_buzzer)
						{
							if (current.response.code == RESP_CHECKED_IN) 
							{
								speakerBeep(100);
							}
							else if (current.response.code == RESP_CHECKED_OUT)
							{
								speakerBeep(100);
								delay(50);
								speakerBeep(100);	
							}
							else 
							{
								speakerBeep(255);
							}
						}

						first_step = 1;
						state = info;
						
						break;
					}

					if (first_step)
					{
						i = j = 0;
						first_step = 0;
					}

					// When we have been in this state in 500.000 ticks,
					// we change the state to info, an set the response to error
					i++;
					if (i == 50000)
					{
						j++;
						if (j == 10)
						{
							state = info;
							current.response.code = 99;
							first_step = 1;
						}
					}
					
					break;
				}
				case info:
				{
					switch (current.response.code)
					{
						case RESP_CHECKED_IN:
						{
							setStatus(L_CHECK_IN, 0);
							makeBalance(buf);
							setStatus(buf, 1);

							break;
						}
						case RESP_CHECKED_OUT:
						{
							sprintf(buf, L_CHECK_OUT, current.response.price);
							setStatus(buf, 0);
							makeBalance(buf);
							setStatus(buf, 1);

							break;
						}
						case RESP_INSUFFICIENT_FUNDS:
						{
							setStatus(L_INSUFFICIENT_FUNDS, 0);
							makeBalance(buf);
							setStatus(buf, 1);

							break;
						}
						case RESP_CARD_NOT_FOUND:
						case RESP_INVALID_CARD:
						{
							setStatus(L_INVALID_CARD, 0);
							setStatus("", 1);

							break;
						}
						case RESP_TOO_LATE_CHECK_OUT:
						{
							setStatus(L_CHECK_OUT_TOO_LATE, 0);
							setStatus(L_LATE_CHECK_OUT_FEE, 1);

							break;
						}
						case RESP_OK:
						{
							setStatus(L_OK, 0);
							setStatus("", 1);

							break;
						}
						default: 
						{
							setStatus(L_SYSTEM_ERROR, 0);
							setStatus("", 1);

							break;
						}
					}

					// Stay here until the customer removes the card
					while ( RFID_IsCardPresent() ) 
					{
						wdt_reset();
					}

					state = idle;
					first_step = 1;

					current.response.code = 0;
					memset(current.card_id, 0, 8);
					
					for (i = 0; i < 150; i++) 
					{
						wdt_reset();
						_delay_ms(10);
					}

					break;
				}
				case no_connection:
				{
					GREEN_OFF;

					if (first_step)
					{
						first_step = 0;

						setStatus(L_OUT_OF_ORDER, 0);
						setStatus("", 1);
					}

					break;
				}
			}
		}
	}	
}