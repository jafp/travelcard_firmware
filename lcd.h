//                        LCD.H                          2011-05-07 AgF

// Instructions:
// Connect LCD display to the ports and pins defined below.
// Call lcd_init() first with the number of columns on the display.

#include <avr/interrupt.h>

//////////////////////////////////////////////////////////////////////
//
//            Port and pin definitions
//
//////////////////////////////////////////////////////////////////////

// Define which port the LCD display is connected to:
#define  LCD_PORT       PORTA
#define  LCD_DIRECTION  DDRA

// Define control and data connections
#define  LCD_RD     1             // RD connected to bit 1 on port
#define  LCD_RS     2             // RS connected to bit 2 on port
#define  LCD_ENABLE 3             // EN connected to bit 3 on port
#define  LCD_DATA4  4             // D4 - D7 connected to bit 4 - 7 on port


//////////////////////////////////////////////////////////////////////
//
//            Function prototypes:
//
//////////////////////////////////////////////////////////////////////

#ifdef __cplusplus  // use C-style linking when compiled as C++
extern "C" {
#endif

// Initialize the LCD controller. Set number of columns.
void LCD_Init(unsigned char lcd_columns);

// Clear the LCD display
void LCD_Clear(void);

// Set the LCD display position.
// x = column, starting at 0. y = row, starting at 0.
void LCD_GotoXY(unsigned char x, unsigned char y);

// Write a character to the LCD
void LCD_PutChar(char c);

// Write a zero-terminated ASCII string to the LCD
void LCD_PutString(const char *str);

// Delay. Approximately 200 microseconds at 10 MHz
void LCD_delay(void) __attribute__ ((noinline));

// Long delay. Approximately 26 miliseconds at 10 MHz
void LCD_long_delay(void) __attribute__ ((noinline));

#ifdef __cplusplus
}
#endif
