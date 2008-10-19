// $Id$
/*
 * Application specific GLCD driver for T6963 with vertical screen (-> 64x240)
 * Referenced from MIOS32_LCD routines
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

#include <glcd_font.h>

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

#define APP_LCD_RD_N_PORT      GPIOC
#define APP_LCD_RD_N_PIN       GPIO_Pin_7

#define APP_LCD_WR_N_PORT      GPIOB
#define APP_LCD_WR_N_PIN       GPIO_Pin_2

#define APP_LCD_D7_PORT        GPIOC
#define APP_LCD_D7_PIN         GPIO_Pin_12


// should output pins to LCD (SER/RD_N/WR_N) be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#define APP_LCD_OUTPUTS_OD     1

// 0: CD connected to SER input of 74HC595 shift register
// 1: CD connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V CD signal is needed)
#define APP_LCD_CS_AT_D7APOSTROPHE 0


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_SER(b)  { APP_LCD_SER_PORT->BSRR  = (b) ? APP_LCD_SER_PIN  : (APP_LCD_SER_PIN << 16); }
#define PIN_RD_N(b) { APP_LCD_RD_N_PORT->BSRR = (b) ? APP_LCD_RD_N_PIN : (APP_LCD_RD_N_PIN << 16); }
#define PIN_WR_N(b) { APP_LCD_WR_N_PORT->BSRR = (b) ? APP_LCD_WR_N_PIN : (APP_LCD_WR_N_PIN << 16); }

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

static u32 display_available = 0;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void APP_LCD_SerWrite(u8 data, u8 cd);
static void APP_LCD_SetCD(u8 cd);


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
  PIN_WR_N(1);
  PIN_RD_N(1);

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

  GPIO_InitStructure.GPIO_Pin = APP_LCD_RD_N_PIN;
  GPIO_Init(APP_LCD_RD_N_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_WR_N_PIN;
  GPIO_Init(APP_LCD_WR_N_PORT, &GPIO_InitStructure);

  // configure "busy" input with pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = APP_LCD_D7_PIN;
  GPIO_Init(APP_LCD_D7_PORT, &GPIO_InitStructure);


  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // initialize LCD
  for(delay=0; delay<50000; ++delay) PIN_WR_N(1); // 50 mS Delay

  // set graphic home address to 0x0000
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x42);

  // set it again, Sam
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x42);

  // set graphic area to 0x20 bytes per line
  APP_LCD_Data(0x20);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x43);

  // set mode (AND mode, CG ROM)
  APP_LCD_Cmd(0x82);

  // set display mode (graphic, no text)
  APP_LCD_Cmd(0x98);

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // write data
  APP_LCD_SerWrite(data, 0); // data, cd

  u32 delay_ctr;
  for(delay_ctr=0; delay_ctr<2; ++delay_ctr) PIN_WR_N(0);
  for(delay_ctr=0; delay_ctr<2; ++delay_ctr) PIN_WR_N(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // write command
  APP_LCD_SerWrite(cmd, 1); // data, cd

  u32 delay_ctr;
  for(delay_ctr=0; delay_ctr<2; ++delay_ctr) PIN_WR_N(0);
  for(delay_ctr=0; delay_ctr<75; ++delay_ctr) PIN_WR_N(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  s32 error = 0;
  u8 x, y;

  // set Y=0, X=0
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Cmd(0x24);

  // clear 32*64 bytes
  for(y=0; y<64; ++y) {
    for(x=0; x<32; ++x) {
      error |= APP_LCD_Data(0x00);
      error |= APP_LCD_Cmd(0xc0); // write and increment
    }
  }

  // set X=0, Y=0
  error |= MIOS32_LCD_CursorSet(0, 0);

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // nothing to do for GLCD driver

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  // nothing to do for GLCD driver

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Prints a single character
// IN: character in <c>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(char c)
{
  int x, y, line;

  // font not initialized yet!
  if( mios32_lcd_font == NULL )
    return -1;

  u8 y_lines = (mios32_lcd_font_height>>3);

  // transfer character to screen buffer
  for(line=0; line<y_lines; ++line) {

    // calculate pointer to character line
    u8 *font_ptr = mios32_lcd_font + line * mios32_lcd_font_offset + y_lines * mios32_lcd_font_offset * (size_t)c + (size_t)mios32_lcd_font_x0;

    for(x=0; x<mios32_lcd_font_width; ++x) {
      u8 x_pos = mios32_lcd_x + x;
      u8 y_pos = (mios32_lcd_y>>3) + line;

      // set address pointer
      u16 addr = 32*x_pos + ((240/8)-1-y_pos);
      APP_LCD_Data(addr & 0xff);
      APP_LCD_Data(addr >> 8);
      APP_LCD_Cmd(0x24);

      // write and increment
      APP_LCD_Data(*font_ptr++);
      APP_LCD_Cmd(0xc0);
    }
  }

  // increment cursor
  mios32_lcd_column += 1;
  mios32_lcd_x += mios32_lcd_font_width;

  return 0; // no error
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
static void APP_LCD_SerWrite(u8 data, u8 cd)
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

  APP_LCD_SetCD(cd);
}


// set CD line
static void APP_LCD_SetCD(u8 cd)
{
  PIN_SER(cd);

#if APP_LCD_CD_AT_D7APOSTROPHE
  // CD connected to D7' output of the 74HC595 register (only required if no open drain mode is used, and a 5V CD signal is needed)
  // shift CD to D7' 
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
