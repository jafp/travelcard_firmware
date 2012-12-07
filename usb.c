/**
 * usb.c
 */

#include "config.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h> 
 
#include "usb.h"
#include "usbdrv/usbdrv.h"

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

