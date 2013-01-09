/*--------------------------------------------------------

test.c

This file contains unit testing of the hardware.

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
#include "test.h"
#include "common.h"
#include "lcd.h"
#include "rfid.h"

#define BTN_PRESSED ( (BTN_PORT & (1 << BTN_PIN)) == 0 )

/**
 * Possible test states
 */
enum test_state_t { button, display, usb, buzzer, rfid };

/**
 * echo_buffer from main.c
 */
extern uint8_t echo_buffer[8];

/**
 * echo_ready from main.c
 */
extern volatile uint8_t echo_ready;

/**
 * Current test state
 */
static enum test_state_t state = button;

static void 
waitForPressAndRelease(void)
{
	while (!BTN_PRESSED);
	while (BTN_PRESSED);
}

static void 
delay_long(uint8_t c)
{
	while (c--)
	{
		_delay_ms(10);
	}
}

static void 
printOnLine(const char * str, uint8_t line)
{
	LCD_GotoXY(0, line);
	LCD_PutString("              ");
	LCD_GotoXY(0, line);
	LCD_PutString(str);
}

static void 
printState(const char * str)
{
	printOnLine(str, 0);
}

static void 
printCounterLineTwo(uint8_t i)
{
	char buf[16];
	sprintf(buf, "i: %d", i);
	printOnLine(buf, 1);
}

void
test(void)
{
	uint8_t i, r;
	uint8_t card[8];
	char buf[16];

	GREEN_ON;

	// Wait for the user to release the button
	while (BTN_PRESSED);

	printState("Press to start");
	waitForPressAndRelease();

	for (;;)
	{
		switch (state)
		{
			case button:
			{
				printState("Test: button");
				printOnLine("", 0);

				delay_long(100);
				printState("Press 10 times");

				i = 0;
				for (; i < 10; i++)
				{
					printCounterLineTwo(i);
					waitForPressAndRelease();
				}

				printCounterLineTwo(i + 1);	

				printOnLine("OK", 0);
				printOnLine("Press for next", 1);

				waitForPressAndRelease();
				state = display;

				break;
			}
			case display:
			{
				printState("Test: display");
				printOnLine("", 1);
				
				delay_long(100);

				printOnLine("OK", 0);
				printOnLine("Press for next", 1);
				waitForPressAndRelease();
				state = usb;

				break;
			}
			case usb: 
			{
				i = 0;

				printState("Test: usb");
				printOnLine("Echo 100 bytes", 1);

				delay_long(100);
				printOnLine("Press to skip", 1);

				for (;;)
				{	
					while (echo_ready == 0)
					{
						if (BTN_PRESSED)
						{
							break;
						}
					}
					if (BTN_PRESSED)
					{
						break;
					}

					echo_ready = 0;

					while (!usbInterruptIsReady())
					{
						wdt_reset();
					}
					usbSetInterrupt(echo_buffer, 8);
					i++;

					if (i == 100) 
					{
						break;
					}
				}	
				
				printOnLine("OK", 0);
				printOnLine("Press for next", 1);
				waitForPressAndRelease();

				state = buzzer;

				break;
			}
			case buzzer: 
			{
				printState("Test: buzzer");
				printOnLine("", 1);

				delay_long(100);

				SPEAKER_ON;
				delay_long(1);
				SPEAKER_OFF;

				delay_long(50);

				SPEAKER_ON;
				delay_long(4);
				SPEAKER_OFF;

				delay_long(50);

				SPEAKER_ON;
				delay_long(8);
				SPEAKER_OFF;

				delay_long(50);

				SPEAKER_ON;
				delay_long(16);
				SPEAKER_OFF;

				delay_long(50);

				SPEAKER_ON;
				delay_long(32);
				SPEAKER_OFF;

				delay_long(100);

				printOnLine("OK", 0);
				printOnLine("Press for next", 1);

				waitForPressAndRelease();
				state = rfid;

				break;
			}
			case rfid: 
			{
				printState("Test: rfid");
				printOnLine("Scan card", 1);

				while (!RFID_IsCardPresent());
				_delay_ms(100);

				r = RFID_GetCardId(card);
				if (r == -1) 
				{
					printOnLine("FAILURE!", 1);
				}
				else
				{
					sprintf(buf, "0x%x%x%x%x%x%x%x%x", card[7], card[6], card[5], card[4], card[3], card[2], card[1], card[0]);
					printOnLine(buf, 1);
				}

				delay_long(255);
				printOnLine("OK", 0);
				printOnLine("Press for next", 1);

				waitForPressAndRelease();
				state = button;

				break;
			}
		}
	}

}