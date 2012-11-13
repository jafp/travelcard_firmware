
#ifndef _BITS_H_
#define _BITS_H_

#define set(port, pin) port |= (1 << pin)
#define clr(port, pin) port &= ~(1 << pin)
#define tgl(port, pin) port ^= (1 << pin)  
#define is_set(port, pin) ((port & (1 << pin)) != 0)

#endif
