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
// Pin definitions for MBHP_CoRE_STM32 board
/////////////////////////////////////////////////////////////////////////////

#define APP_LCD_SCLK_PORT      GPIOA
#define APP_LCD_SCLK_PIN       GPIO_Pin_8

#define APP_LCD_RCLK_PORT      GPIOC
#define APP_LCD_RCLK_PIN       GPIO_Pin_9

#define APP_LCD_SER_PORT       GPIOC
#define APP_LCD_SER_PIN        GPIO_Pin_8

#define APP_LCD_E1_PORT        GPIOC
#define APP_LCD_E1_PIN         GPIO_Pin_7

#define APP_LCD_E2_PORT        GPIOC
#define APP_LCD_E2_PIN         GPIO_Pin_6

#define APP_LCD_RW_PORT        GPIOB
#define APP_LCD_RW_PIN         GPIO_Pin_2

#define APP_LCD_D7_PORT        GPIOC
#define APP_LCD_D7_PIN         GPIO_Pin_12

// should output pins to LCD (SER/E1/E2/RW) be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#define APP_LCD_OUTPUTS_OD     1

// 0: RS connected to SER input of 74HC595 shift register
// 1: RS connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V RS signal is needed)
#define APP_LCD_RS_AT_D7APOSTROPHE 0


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_SER(b)  { APP_LCD_SER_PORT->BSRR = (b) ? APP_LCD_SER_PIN : (APP_LCD_SER_PIN << 16); }
#define PIN_E1(b)   { APP_LCD_E1_PORT->BSRR  = (b) ? APP_LCD_E1_PIN  : (APP_LCD_E1_PIN << 16); }
#define PIN_E2(b)   { APP_LCD_E2_PORT->BSRR  = (b) ? APP_LCD_E2_PIN  : (APP_LCD_E2_PIN << 16); }
#define PIN_RW(b)   { APP_LCD_RW_PORT->BSRR  = (b) ? APP_LCD_RW_PIN  : (APP_LCD_RW_PIN << 16); }

#define PIN_RCLK_0  { APP_LCD_RCLK_PORT->BRR  = APP_LCD_RCLK_PIN; }
#define PIN_RCLK_1  { APP_LCD_RCLK_PORT->BSRR = APP_LCD_RCLK_PIN; }

#define PIN_SCLK_0  { APP_LCD_SCLK_PORT->BRR  = APP_LCD_SCLK_PIN; }
#define PIN_SCLK_1  { APP_LCD_SCLK_PORT->BSRR = APP_LCD_SCLK_PIN; }

#define PIN_D7_IN   (APP_LCD_D7_PORT->IDR & APP_LCD_D7_PIN)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u32 display_available = 0;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_SerWrite(u8 data, u8 rs);
void APP_LCD_SetRS(u8 rs);
void APP_LCD_Strobe(u8 value);
s32  APP_LCD_WaitUnbusy(void);


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
  PIN_RW(0);
  PIN_E1(0);
  PIN_E2(0);

  // configure push-pull pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // weak driver to reduce transients

  GPIO_InitStructure.GPIO_Pin = APP_LCD_SCLK_PIN;
  GPIO_Init(APP_LCD_SCLK_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_RCLK_PIN;
  GPIO_Init(APP_LCD_RCLK_PORT, &GPIO_InitStructure);

  // configure open-drain pins (if OD option enabled)
#if APP_LCD_OUTPUTS_OD
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
#endif
  GPIO_InitStructure.GPIO_Pin = APP_LCD_SER_PIN;
  GPIO_Init(APP_LCD_SER_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_E1_PIN;
  GPIO_Init(APP_LCD_E1_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_E2_PIN;
  GPIO_Init(APP_LCD_E2_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_RW_PIN;
  GPIO_Init(APP_LCD_RW_PORT, &GPIO_InitStructure);

  // configure "busy" input with pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = APP_LCD_D7_PIN;
  GPIO_Init(APP_LCD_D7_PORT, &GPIO_InitStructure);


  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // initialize LCD
  APP_LCD_SerWrite(0x38, 0);
  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);
  for(delay=0; delay<50000; ++delay) PIN_RW(0); // 50 mS Delay

  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);
  for(delay=0; delay<50000; ++delay) PIN_RW(0); // 50 mS Delay

  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);
  for(delay=0; delay<50000; ++delay) PIN_RW(0); // 50 mS Delay

  APP_LCD_Cmd(0x08); // Display Off
  APP_LCD_Cmd(0x0c); // Display On
  APP_LCD_Cmd(0x06); // Entry Mode
  APP_LCD_Cmd(0x01); // Clear Display
  for(delay=0; delay<50000; ++delay) PIN_RW(0); // 50 mS Delay

  // for DOG displays: perform additional display initialisation
  // this has to be explicitely enabled
  // see also $MIOS32_PATH/modules/app_lcd/dog/app_lcd.mk for "auto selection"
  // this makefile is included if environment variable "MIOS32_LCD" is set to "dog"
#ifdef APP_LCD_INIT_DOG
  APP_LCD_Cmd(0x39); // 8bit interface, switch to instruction table 1
  APP_LCD_Cmd(0x1d); // BS: 1/4, 3 line LCD
  APP_LCD_Cmd(0x50); // Booster off, set contrast C5/C4
  APP_LCD_Cmd(0x6c); // set Voltage follower and amplifier
  APP_LCD_Cmd(0x7c); // set contrast C3/C2/C1
  //  APP_LCD_Cmd(0x38); // back to instruction table 0
  // (will be done below)

  // modify cursor mapping, so that it complies with 3-line dog displays
  u8 cursor_map[] = {0x00, 0x10, 0x20, 0x30}; // offset line 0/1/2/3
  MIOS32_LCD_CursorMapSet(cursor_map);
#endif

  APP_LCD_Cmd(0x38); // experience from PIC based MIOS: without these lines
  APP_LCD_Cmd(0x0c); // the LCD won't work correctly after a second APP_LCD_Init

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // wait until LCD unbusy, exit on error (timeout)
  if( APP_LCD_WaitUnbusy() < 0 )
    return -1; // timeout

  // write data
  APP_LCD_SerWrite(data, 1); // data, rs
  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // wait until LCD unbusy, exit on error (timeout)
  if( APP_LCD_WaitUnbusy() < 0 )
    return -1; // timeout

  // write command
  APP_LCD_SerWrite(cmd, 0); // data, rs
  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);

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
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // exit with error if line is not in allowed range
  if( line >= MIOS32_LCD_MAX_MAP_LINES )
    return -1;

  // -> set cursor address
  return APP_LCD_Cmd(0x80 | (mios32_lcd_cursor_map[line] + column));
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
  s32 i;
  s32 ret;

  // send character number
  APP_LCD_Cmd(((num&7)<<3) | 0x40);

  // send 8 data bytes
  for(i=0; i<8; ++i)
    if( APP_LCD_Data(table[i]) < 0 )
      return -1; // error during sending character

  // set cursor to original position
  return APP_LCD_CursorSet(mios32_lcd_line, mios32_lcd_column);
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
  // whole function takes ca. 1.5 uS @ 72MHz
  // thats acceptable for a (C)LCD, which is normaly busy after each access for ca. 20..40 uS

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

  APP_LCD_SetRS(rs);
}


// set RS line
void APP_LCD_SetRS(u8 rs)
{
  PIN_SER(rs);

#if APP_LCD_RS_AT_D7APOSTROPHE
  // RS connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V RS signal is needed)
  // shift RS to D7' 
  // These 8 shifts take ca. 500 nS @ 72MHz, they don't really hurt
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
  PIN_SCLK_1;
  PIN_SCLK_0;
#endif
}


// used to strobe the E line
void APP_LCD_Strobe(u8 value)
{
  u32 delay;

  if( mios32_lcd_device == 0 ) {
    PIN_E1(value)
  } else {
    PIN_E2(value)
  }
}

// polls busy flag until it is 0
// disables display on timeout (-1 will be returned in this case)
s32 APP_LCD_WaitUnbusy(void)
{
  u32 poll_ctr;
  u32 delay_ctr;

  // don't wait if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // select command register (RS=0)
  APP_LCD_SetRS(0);

  // select read (will also disable output buffer of 74HC595)
  PIN_RW(1);

  // poll busy flag, timeout after 10 mS
  // each loop takes ca. 4 uS @ 72MHz, accordingly we poll 2500 times
  for(poll_ctr=2500; poll_ctr>0; --poll_ctr) {
    APP_LCD_Strobe(1);

    // due to slow slope we should wait at least for 1 uS
    for(delay_ctr=0; delay_ctr<10; ++delay_ctr)
      PIN_RW(1);

    u32 busy = PIN_D7_IN;
    APP_LCD_Strobe(0);
    if( !busy )
      break;
  }

  // deselect read (output buffers of 74HC595 enabled again)
  PIN_RW(0);

  // timeout?
  if( poll_ctr == 0 ) {
    // disable display
    display_available &= ~(1 << mios32_lcd_device);
    return -1; // timeout error
  }

  return 0; // no error
}

