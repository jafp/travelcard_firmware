/**
 * usbtest/main.c
 */

#include "config.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "bits.h"
#include "lcd.h"
#include "usb.h"
#include "rfid.h"

#define RED_PIN		PC3
#define YELLOW_PIN	PC2
#define GREEN_PIN	PC1
#define SPEAKER_PIN	PC0
#define LEDS		PORTC
#define RED 		RED_PIN
#define YELLOW 		YELLOW_PIN
#define GREEN 		GREEN_PIN

#define RED_ON 		clr(PORTC, RED_PIN)
#define RED_OFF		set(PORTC, RED_PIN)
#define YELLOW_ON	clr(PORTC, YELLOW_PIN)
#define YELLOW_OFF	set(PORTC, YELLOW_PIN)
#define GREEN_ON	clr(PORTC, GREEN_PIN);
#define GREEN_OFF	set(PORTC, GREEN_PIN);


/**
 * USB Command codes
 */
#define CMD_ECHO				0
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
static uchar card_id[8];

/**
 * USB reply buffer
 */
static uchar reply_buffer[8];

/**
 * Response code from server.
 */
static uchar response = 0;

/**
 * Terminal "is-alive" flag
 */
static uchar is_alive = 0;

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
	if (is_alive == 0)
	{
		state = no_connection;
		first_step = 1;
		//GREEN_ON;
	}
	else
	{
		//GREEN_OFF;
		if (state == no_connection)
		{
			state = idle;
		}
		first_step = 1;
		is_alive = 0;
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
    if (data[1] == CMD_RESPONSE)
    {
    	len = USB_NO_MSG;
    }
    if (data[1] == CMD_KEEP_ALIVE)
    {
    	is_alive = 1;
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
static void set_status_msg(const char * msg, uchar line)
{
	LCD_GotoXY(0, line);
	LCD_PutString("                ");
	LCD_GotoXY(0, line);
	LCD_PutString(msg);
}

/**
 * Creates a string like "Saldo: [current balance]"
 */
static void make_balance_str(char *buffer)
{
	sprintf(buffer, "Saldo %d kr", balance);
}

/**
 * Ticks the keep alive timer. 
 */
// static void keep_alive_tick(void)
// {
// 	keep_alive_i++;
// 	if (keep_alive_i == 1000)
// 	{
// 		keep_alive_j++;
// 		keep_alive_i = 0;

// 		if (keep_alive_j == 500)
// 		{
// 			keep_alive_j = 0;

// 			if (is_alive == 0)
// 			{
// 				state = no_connection;
// 				first_step = 1;
// 			}
// 			else
// 			{
// 				if (state == no_connection)
// 				{
// 					state = idle;
// 				}

// 				first_step = 1;
// 				is_alive = 0;
// 			}
// 		}
// 	}
// }

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
    OCR1A = 23000; //35156; // (12000000 / 1024) / (1/2) roughly 0.5 Hz 

	TCCR0 |= (1 << WGM01);
	OCR0 = 45; // (12000000÷1024)÷(1÷0.004)−1 = 45.6 --- 250 Hz
	TIMSK |= (1 << OCIE0);

	LCD_Init(16);
	LCD_Clear();
	LCD_GotoXY(0, 0);

	USB_InitAndConnect();

	TCCR1B |= (1 << CS12) | (1 << CS10); // 1024 prescaler
	TCCR0 |=  (1 << CS02) | (1 << CS00); // 1024 prescaler

	InitRFID();
}

/**
 * The main function with the main loop and state machine.
 */
int __attribute__((noreturn)) main(void)
{
	unsigned int i = 0;
	unsigned char j = 0;
	char buf[32];

	setup();

	RED_ON;

	for (;;)
	{
		wdt_reset();
		// keep_alive_tick();

		switch (state)
		{
			case starting:
			{
				// Start-up phase.
				// Do nothing in a bunch of clock cycles
				set_status_msg("Starter...", 0);
				set_status_msg("", 1);

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
				RED_OFF;
				YELLOW_ON;

				if (first_step)
				{
					first_step = 0;

					set_status_msg("Scan her!", 0);
					set_status_msg("", 1);
				}
				
				if (CardPresent())
				{
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
					set_status_msg("Arbejder...", 0);
				}

				// XXX: These two call may cause problems sometimes.
				// The usbSetInterrupt should not block, but maybe it does.
				// Maybe the SPI transfer hangs, and causes the Watchdog timer to perform a reset.
				char n = GetCardId(card_id);
				if (n == -1 || card_id[0] != 0x86) 
				{
					state = info;
					response = RESP_INVALID_CARD;
					first_step = 1;
				}
				else
				{
					usbSetInterrupt(card_id, 8);
					first_step = 1;
					state = processing;
				}
				break;
			}
			case processing:
			{
				
				if (response != 0)
				{
					set(PORTC, 0);
					_delay_ms(100);
					clr(PORTC, 0);

					first_step = 1;
					state = info;
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

				clr(LEDS, GREEN);

				switch (response)
				{
					case RESP_CHECKED_IN:
					{
						set_status_msg("Check ind", 0);
						make_balance_str(buf);
						set_status_msg(buf, 1);

						break;
					}
					case RESP_CHECKED_OUT:
					{
						sprintf(buf, "Check ud   %d kr", price);
						set_status_msg(buf, 0);
						make_balance_str(buf);
						set_status_msg(buf, 1);

						break;
					}
					case RESP_INSUFFICIENT_FUNDS:
					{
						set_status_msg("Saldo for lav", 0);
						make_balance_str(buf);
						set_status_msg(buf, 1);

						break;
					}
					case RESP_CARD_NOT_FOUND:
					case RESP_INVALID_CARD:
					{
						set_status_msg("Ugyldigt kort", 0);
						set_status_msg("", 1);

						break;
					}
					case RESP_TOO_LATE_CHECK_OUT:
					{
						set_status_msg("For sent checkud", 0);
						set_status_msg("", 1);

						break;
					}
					default: 
					{
						set_status_msg("SYSTEMFEJL", 0);
						set_status_msg("", 1);

						break;
					}
				}

				// Stay here until the customer removes the card
				while ( CardPresent() ) 
				{
					wdt_reset();
				}

				set(LEDS, GREEN);

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
				//YELLOW_OFF;

				if (first_step)
				{
					first_step = 0;
					
					RED_ON;
					set_status_msg("Ude af drift", 0);
					set_status_msg("", 1);
				}

				break;
			}
		}
	}	
}