// $Id$
/*
 * Application specific CLCD driver for HD44780 (or compatible) character LCDs
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Pin definitions (tmp. for STM32 Primer board)
/////////////////////////////////////////////////////////////////////////////

// Note: on Primer, PinA.0 is also connected to wakeup button - it has to be triggered fast to avoid a driver conflict (LCD won't be initialized correctly in this case)
// Thats acceptable for this "evaluation solution"
#define APP_LCD_SER_PORT       GPIOA
#define APP_LCD_SER_PIN        GPIO_Pin_0

#define APP_LCD_SCLK_PORT      GPIOA
#define APP_LCD_SCLK_PIN       GPIO_Pin_1

#define APP_LCD_RCLK_PORT      GPIOA
#define APP_LCD_RCLK_PIN       GPIO_Pin_2

#define APP_LCD_E1_PORT        GPIOA
#define APP_LCD_E1_PIN         GPIO_Pin_3

#define APP_LCD_RW_PORT        GPIOA
#define APP_LCD_RW_PIN         GPIO_Pin_4


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////
#define PIN_SER(b)  { if( b ) APP_LCD_SER_PORT->BSRR = APP_LCD_SER_PIN; else APP_LCD_SER_PORT->BRR = APP_LCD_SER_PIN; }

#define PIN_E1_0    { APP_LCD_E1_PORT->BRR  = APP_LCD_E1_PIN; }
#define PIN_E1_1    { APP_LCD_E1_PORT->BSRR = APP_LCD_E1_PIN; }

#define PIN_RCLK_0  { APP_LCD_RCLK_PORT->BRR  = APP_LCD_RCLK_PIN; }
#define PIN_RCLK_1  { APP_LCD_RCLK_PORT->BSRR = APP_LCD_RCLK_PIN; }

#define PIN_SCLK_0  { APP_LCD_SCLK_PORT->BRR  = APP_LCD_SCLK_PIN; }
#define PIN_SCLK_1  { APP_LCD_SCLK_PORT->BSRR = APP_LCD_SCLK_PIN; }

#define PIN_RW_0    { APP_LCD_RCLK_PORT->BRR  = APP_LCD_RCLK_PIN; }
#define PIN_RW_1    { APP_LCD_RCLK_PORT->BSRR = APP_LCD_RCLK_PIN; }


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_SerWrite(u8 data, u8 rs);
void APP_LCD_Strobe(void);


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  u32 delay;

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  PIN_SCLK_0;
  PIN_RCLK_0;
  PIN_E1_0;
  PIN_RW_0;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Pin = APP_LCD_SCLK_PIN;
  GPIO_Init(APP_LCD_SCLK_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_RCLK_PIN;
  GPIO_Init(APP_LCD_RCLK_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_SER_PIN;
  GPIO_Init(APP_LCD_SER_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_E1_PIN;
  GPIO_Init(APP_LCD_E1_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_RW_PIN;
  GPIO_Init(APP_LCD_RW_PORT, &GPIO_InitStructure);

  // wait for ca. 100 mS
  for(delay=0; delay<1000000; ++delay) PIN_RW_0; // each loop takes ca. 100 nS

  // initialize LCD
  APP_LCD_SerWrite(0x38, 0);
  APP_LCD_Strobe();
  for(delay=0; delay<50000; ++delay) PIN_RW_0; // 50 mS Delay

  APP_LCD_Strobe();
  for(delay=0; delay<50000; ++delay) PIN_RW_0; // 50 mS Delay

  APP_LCD_Strobe();
  for(delay=0; delay<50000; ++delay) PIN_RW_0; // 50 mS Delay

  APP_LCD_Cmd(0x08); // Display Off
  APP_LCD_Cmd(0x0c); // Display On
  APP_LCD_Cmd(0x06); // Entry Mode
  APP_LCD_Cmd(0x01); // Clear Display
  for(delay=0; delay<50000; ++delay) PIN_RW_0; // 50 mS Delay

  APP_LCD_Cmd(0x38); // experience from PIC based MIOS: without these lines
  APP_LCD_Cmd(0x0c); // the LCD won't work correctly after a second APP_LCD_Init

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  u32 delay;
  u8 busy;

  // write data
  APP_LCD_SerWrite(data, 1); // data, rs
  APP_LCD_Strobe();

  for(delay=0; delay<1000; ++delay) PIN_RW_0; // 50 uS Delay

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  u32 delay;

  APP_LCD_SerWrite(cmd, 0); // data, rs
  APP_LCD_Strobe();

  for(delay=0; delay<1000; ++delay) PIN_RW_0; // 50 uS Delay

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  // -> send clear command
  return APP_LCD_Cmd(0x01);
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <line> and <column>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 line, u16 column)
{
  // -> set cursor address
  return APP_LCD_Cmd(0x80 | (line*0x40 + column));
}


/////////////////////////////////////////////////////////////////////////////
// Prints a single character
// IN: character in <c>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(char c)
{
  // -> send as data byte
  return APP_LCD_Data(c);
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  // TODO

  return -1; // not implemented yet
}


/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b values
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u8 r, u8 g, u8 b)
{
  return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b values
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u8 r, u8 g, u8 b)
{
  return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Help Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// writes to serial register
void APP_LCD_SerWrite(u8 data, u8 rs)
{
  // shift in 8bit data
  PIN_SER(data & 0x80); // D7
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x40); // D6
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x20); // D5
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x10); // D4
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x08); // D3
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x04); // D2
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x02); // D1
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x01); // D0
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;

  // transfer to output register
  PIN_RCLK_1;
  PIN_RCLK_1;
  PIN_RCLK_1;
  PIN_RCLK_0;

  // shift RS to Q8
  PIN_SER(rs);
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
  PIN_SCLK_1;
  PIN_SCLK_0;
}


// strobes the E line
void APP_LCD_Strobe(void)
{
  u32 delay;

  if( mios32_lcd_device == 0 ) {
    for(delay=0; delay<10; ++delay)
      PIN_E1_1; // generate pulse length of 1 US
    PIN_E1_0;
  } else {
    // TODO
  }
}

