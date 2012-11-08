/**
 * usbtest/main.c
 */

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv/usbdrv.h"
#include "bits.h"
#include "lcd.h"
#include "usb.h"
#include "rfid.h"

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
#define CMD_IDENTIFER			1
#define CMD_RESPONSE			3
#define CMD_KEEP_ALIVE			4

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

/**
 * Enum of possible terminal states
 */
enum terminal_state { starting, idle, scanning, processing, info, no_connection };

/**
 * Counter variables for the keep alive counter.
 */
// static unsigned int keep_alive_i = 0;
// static unsigned int keep_alive_j = 0;

/**
 * Card identifier buffer.
 */
static uint8_t card_id[8];

/**
 * USB reply buffer
 */
static uchar reply_buffer[8];

/**
 * Response code from server.
 */
static uchar response = 0;

/**
 * First step in state flag
 */
static uchar first_step = 1;

/**
 * The current terminal state.
 */
static enum terminal_state state = starting;

/** 
 * Variables used during check-in and out.
 * Both are 16-bit unsigned integers.
 */

/**
 * Card balance
 */
static unsigned int balance;

/**
 * Journey price
 */
static unsigned int price;

/**
 * Catch-all ISR
 */
ISR(BADISR_vect, ISR_NOBLOCK)
{

}

/**
 * USB polling ISR
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
 */
inline void delay(uint8_t ms) 
{
	while (ms--) 
	{
		_delay_ms(1);
		wdt_reset();
	}
}

/**
 * 
 */
inline void speakerBeep(uint8_t ms) 
{
	SPEAKER_ON;
	delay(ms);
	SPEAKER_OFF;
}

/**
 * Notify that we're still alive!
 */
static void isAlive(void)
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
 * USB request handler
 */
usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    uchar len = 0;

    if (data[1] == CMD_ECHO)
    {	
    	reply_buffer[0] = data[2];
    	reply_buffer[1] = data[3];
    	len = 2;
    }
    if (data[1] == CMD_IDENTIFER)
    {
    	reply_buffer[0] = ID;
    	len = 1;
    }
    if (data[1] == CMD_RESPONSE)
    {
    	len = USB_NO_MSG;
    }
    if (data[1] == CMD_KEEP_ALIVE)
    {
    	isAlive();
    	len = 0;
    }

    usbMsgPtr = reply_buffer;
    return len;
}

/**
 * USB request data handler
 */
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	response = data[0];

	if ( response == RESP_CHECKED_IN || response == RESP_CHECKED_OUT || response == RESP_INSUFFICIENT_FUNDS) 
	{
		balance = data[2] | (data[1] << 8);
	}

	if ( response == RESP_CHECKED_OUT || response == RESP_INSUFFICIENT_FUNDS )
	{
		price = data[4] | (data[3] << 8); 
	}

	return 1;
}

/**
 * Prints the given message on the given line.
 * The line are cleared before print.
 */
static void setStatus(const char * msg, uchar line)
{
	LCD_GotoXY(0, line);
	LCD_PutString("                ");
	LCD_GotoXY(0, line);
	LCD_PutString(msg);
}

/**
 * Creates a string like "Saldo: [current balance]"
 */
static void makeBalance(char *buffer)
{
	sprintf(buffer, "Saldo %d kr", balance);
}

/**
 * Hardware setup routines.
 */
static void setup(void)
{
	wdt_enable(WDTO_1S);

	DDRC |= (1 << RED_PIN) | (1 << YELLOW_PIN) | (1 << GREEN_PIN);
	PORTC |= 0x0E;

	TCCR1B |= (1 << WGM12);
    TIMSK |= (1 << OCIE1A);
    OCR1A = 46874; // 256 prescaler --> 1 Hz

	TCCR0 |= (1 << WGM01);
	OCR0 = 45; // 1024 prescaler --> 250 Hz
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
 */
int __attribute__((noreturn)) main(void)
{
	// Utility counter variables
	unsigned int i = 0;
	unsigned char j = 0;
	// Buffer used when generating output for the display
	char buf[16];

	// Perform setup of registers and peripherals
	setup();

	// Indicate setup done
	//RED_ON;

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
				setStatus("Starter...", 0);
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
				//RED_OFF;
				YELLOW_ON;

				if (first_step)
				{
					first_step = 0;

					setStatus("Scan her!", 0);
					setStatus("", 1);
				}
				
				if (RFID_IsCardPresent())
				{
					first_step = 1;
					state = scanning;
				}

				break;
			}
			case scanning:
			{
				RED_ON;
				if (first_step)
				{
					first_step = 0;
					setStatus("Arbejder...", 0);
				}

				// XXX: These two call may cause problems sometimes.
				// The usbSetInterrupt should not block, but maybe it does.
				// Maybe the SPI transfer hangs, and causes the Watchdog timer to perform a reset.
				uint8_t n = RFID_GetCardId(card_id);
				if (n == -1 || card_id[0] != 0x86) 
				{
					state = info;
					response = RESP_INVALID_CARD;
					first_step = 1;
				}
				else
				{
					if (!usbInterruptIsReady())
					{
						wdt_reset();
					}

					usbSetInterrupt(card_id, 8);
					first_step = 1;
					state = processing;
				}
				break;
			}
			case processing:
			{
				RED_OFF;
				if (response != 0)
				{
					if (response == RESP_CHECKED_IN) 
					{
						speakerBeep(100);
					}
					else if (response == RESP_CHECKED_OUT)
					{
						speakerBeep(100);
						delay(50);
						speakerBeep(100);	
					}
					else 
					{
						speakerBeep(255);
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

				i++;
				if (i == 50000)
				{
					j++;
					if (j == 10)
					{
						state = info;
						response = 99;
						first_step = 1;
					}
				}
				
				break;
			}
			case info:
			{
				YELLOW_OFF;
				GREEN_ON;

				switch (response)
				{
					case RESP_CHECKED_IN:
					{
						setStatus("Check ind", 0);
						makeBalance(buf);
						setStatus(buf, 1);

						break;
					}
					case RESP_CHECKED_OUT:
					{
						sprintf(buf, "Check ud   %d kr", price);
						setStatus(buf, 0);
						makeBalance(buf);
						setStatus(buf, 1);

						break;
					}
					case RESP_INSUFFICIENT_FUNDS:
					{
						setStatus("Saldo for lav", 0);
						makeBalance(buf);
						setStatus(buf, 1);

						break;
					}
					case RESP_CARD_NOT_FOUND:
					case RESP_INVALID_CARD:
					{
						setStatus("Ugyldigt kort", 0);
						setStatus("", 1);

						break;
					}
					case RESP_TOO_LATE_CHECK_OUT:
					{
						setStatus("For sent checkud", 0);
						setStatus("", 1);

						break;
					}
					case RESP_OK:
					{
						setStatus("OK", 0);
						setStatus("", 1);

						break;
					}
					default: 
					{
						setStatus("SYSTEMFEJL", 0);
						setStatus("", 1);

						break;
					}
				}

				// Stay here until the customer removes the card
				while ( RFID_IsCardPresent() ) 
				{
					wdt_reset();
				}

				GREEN_OFF;
				state = idle;
				response = 0;
				first_step = 1;
				
				for (i = 0; i < 150; i++) 
				{
					wdt_reset();
					_delay_ms(10);
				}

				break;
			}
			case no_connection:
			{
				if (first_step)
				{
					first_step = 0;
					
					RED_ON;
					setStatus("Ude af drift", 0);
					setStatus("", 1);
				}

				break;
			}
		}
	}	
}