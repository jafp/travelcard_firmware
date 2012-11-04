/**
 * usb.c
 */

#include "config.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h> 
#include "bits.h"
#include "usb.h"

//static usbCallbackFn callback_fn = 0;
//static uchar data_buffer[8];

void USB_InitAndConnect()
{
	uchar i;

	usbInit();
    usbDeviceDisconnect();
    
    i = 0;
    while(--i){          
        wdt_reset();
        _delay_ms(1);
    }
    
    usbDeviceConnect();
    sei();
}

void USB_SetCallback(usbCallbackFn fn)
{	
	//callback_fn = fn;
}

