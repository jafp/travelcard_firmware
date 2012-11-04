#include "spi.h"

#define Card_Present 	PD2
#define Data_Ready 		PD3

void InitRFID(void);
int CardPresent(void);
char GetCardStatus(void);
char GetCardId(unsigned char * buffer);

