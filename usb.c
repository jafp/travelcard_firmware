/*--------------------------------------------------------

usb.c

This file contains functions related to setting of the USB 
library. Rest of the USB related functions required by VUSB
library is implemented in main.c

Version:    1
Author:     Jacob Pedersen
Company:    IHK
Date:       2012-12-01

--------------------------------------------------------*/

#include "config.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h> 
 
#include "usb.h"
#include "usbdrv/usbdrv.h"

/**
 * Initialize the USB library and connect to the host
 */
void 
USB_InitAndConnect()
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

