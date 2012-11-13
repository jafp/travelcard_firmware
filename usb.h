/**
 * usb.h
 */

#ifndef _USB_H_
#define _USB_H_

#include "usbdrv/usbdrv.h"

typedef usbMsgLen_t (*usbCallbackFn)(usbRequest_t *rq, uchar data[]);

void USB_InitAndConnect(void);
void USB_Poll(void);
void USB_SetCallback(usbCallbackFn fn);

#endif


