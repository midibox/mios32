// $Id$
/*
 * Application specific GLCD driver for ST7637
 * Referenced from MIOS32_LCD routines
 *
 * Some of the code has been taken over and adapted from Raisonance Circle OS
 * It has been dramatically speed-optimized! (ca. 8 times faster :-)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 * and
 *  Copyright (C) 2007 RAISONANCE S.A.S.
 *
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
#include "app_lcd_st7637.h"


/////////////////////////////////////////////////////////////////////////////
// Pin definitions for STM32 Primer board
/////////////////////////////////////////////////////////////////////////////

#define APP_LCD_D_PINS         (GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7)
#define APP_LCD_D_PORT         GPIOC

#define APP_LCD_BL_PORT        GPIOB
#define APP_LCD_BL_PIN         GPIO_Pin_7

#define APP_LCD_RS_PORT        GPIOC
#define APP_LCD_RS_PIN         GPIO_Pin_8

#define APP_LCD_RD_PORT        GPIOC
#define APP_LCD_RD_PIN         GPIO_Pin_9

#define APP_LCD_WR_PORT        GPIOC
#define APP_LCD_WR_PIN         GPIO_Pin_10

#define APP_LCD_CS_PORT        GPIOC
#define APP_LCD_CS_PIN         GPIO_Pin_11

#define APP_LCD_RST_PORT       GPIOC
#define APP_LCD_RST_PIN        GPIO_Pin_12


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_BL(b)   { APP_LCD_BL_PORT->BSRR  = (b) ? APP_LCD_BL_PIN  : (APP_LCD_BL_PIN << 16); }
#define PIN_RS(b)   { APP_LCD_RS_PORT->BSRR  = (b) ? APP_LCD_RS_PIN  : (APP_LCD_RS_PIN << 16); }
#define PIN_RD(b)   { APP_LCD_RD_PORT->BSRR  = (b) ? APP_LCD_RD_PIN  : (APP_LCD_RD_PIN << 16); }
#define PIN_WR(b)   { APP_LCD_WR_PORT->BSRR  = (b) ? APP_LCD_WR_PIN  : (APP_LCD_WR_PIN << 16); }
#define PIN_CS(b)   { APP_LCD_CS_PORT->BSRR  = (b) ? APP_LCD_CS_PIN  : (APP_LCD_CS_PIN << 16); }
#define PIN_RST(b)  { APP_LCD_RST_PORT->BSRR = (b) ? APP_LCD_RST_PIN : (APP_LCD_RST_PIN << 16); }


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u16 lcd_bcolour;
u16 lcd_fcolour;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

void APP_LCD_SetRect_For_Cmd(s16 x, s16 y, s16 width, s16 height);



/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  s32 delay;

  // notify that this is a GLCD type display
  mios32_lcd_type = MIOS32_LCD_TYPE_GLCD;

  // set initial font and colours
  MIOS32_LCD_FontInit(GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xff, 0xff, 0xff);
  MIOS32_LCD_FColourSet(0x00, 0x00, 0x00);

  // control lines
  PIN_BL(1);
  PIN_RS(1);
  PIN_RD(1);
  PIN_CS(1);
  PIN_WR(1);
  PIN_RST(1);

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_BL_PIN;
  GPIO_Init(APP_LCD_BL_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_RS_PIN;
  GPIO_Init(APP_LCD_RS_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_RD_PIN;
  GPIO_Init(APP_LCD_RD_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_WR_PIN;
  GPIO_Init(APP_LCD_WR_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_CS_PIN;
  GPIO_Init(APP_LCD_CS_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = APP_LCD_RST_PIN;
  GPIO_Init(APP_LCD_RST_PORT, &GPIO_InitStructure);

  // Apply hardware reset
  PIN_RST(0);
  for(delay=0; delay<0x500; ++delay);

  PIN_RST(1);
  for(delay=0; delay<0x500; ++delay);

  // configure data pins as outputs
  GPIO_InitStructure.GPIO_Pin  = APP_LCD_D_PINS;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(APP_LCD_D_PORT, &GPIO_InitStructure);

  //----------- software reset ------------------------------------------------
  APP_LCD_Cmd(ST7637_SWRESET);

  //----------- disable autoread + Manual read once ---------------------------
  APP_LCD_Cmd( ST7637_AUTOLOADSET );  // Auto Load Set 0xD7
  APP_LCD_Data( 0xBF );               // Auto Load Disable

  APP_LCD_Cmd( ST7637_EPCTIN );       // EE Read/write mode 0xE0
  APP_LCD_Data( 0x00 );               // Set read mode

  APP_LCD_Cmd( ST7637_EPMRD );        // Read active 0xE3
  APP_LCD_Cmd( ST7637_EPCTOUT );      // Cancel control 0xE1

  //---------------------------------- Sleep OUT ------------------------------
  APP_LCD_Cmd( ST7637_DISPOFF );      // display off 0x28
  APP_LCD_Cmd( ST7637_SLPOUT );       // Sleep Out 0x11

  //--------------------------------Vop setting--------------------------------
  APP_LCD_Cmd( ST7637_VOPSET );       // Set Vop by initial Module 0xC0
  APP_LCD_Data( 0xFB );               // Vop = 13.64
  APP_LCD_Data( 0x00 );               // base on Module

  //----------------------------Set Register-----------------------------------
  APP_LCD_Cmd( ST7637_BIASSEL );      // Bias select 0xC3
  APP_LCD_Data( 0x00 );               // 1/12 Bias, base on Module

  APP_LCD_Cmd( ST7637_BSTBMPXSEL );   // Setting Booster times 0xC4
  APP_LCD_Data( 0x05 );               // Booster X 8

  APP_LCD_Cmd( ST7637_BSTEFFSEL );    // Booster eff 0xC5
  APP_LCD_Data( 0x11 );               // BE = 0x01 (Level 2)

  APP_LCD_Cmd( ST7637_VGSORCSEL );    // Vg with booster x2 control 0xcb
  APP_LCD_Data( 0x01 );               // Vg from Vdd2

  APP_LCD_Cmd( ST7637_ID1SET );       // ID1 = 00 0xcc
  APP_LCD_Data( 0x00 );               //

  APP_LCD_Cmd( ST7637_ID3SET );       // ID3 = 00 0xce
  APP_LCD_Data( 0x00 );               //

  APP_LCD_Cmd( ST7637_COMSCANDIR );   // Glass direction
  APP_LCD_Data( 0xC0 );               //

  APP_LCD_Cmd( ST7637_ANASET );       // Analog circuit setting 0xd0
  APP_LCD_Data( 0x1D );               //

  APP_LCD_Cmd( ST7637_PTLMOD );       // PTL mode set
  APP_LCD_Data( 0x18 );               // power normal mode
  APP_LCD_Cmd( ST7637_INVOFF );       // Display Inversion OFF 0x20

  APP_LCD_Cmd( ST7637_CASET );        // column range
  APP_LCD_Data( 0x04 );               //
  APP_LCD_Data( 0x83 );               //

  APP_LCD_Cmd( ST7637_RASET );        // raw range
  APP_LCD_Data( 0x04 );               //
  APP_LCD_Data( 0x83 );               //


  APP_LCD_Cmd( ST7637_COLMOD );       // Color mode = 65k 0x3A
  APP_LCD_Data( 0x05 );               //

  APP_LCD_Cmd( ST7637_MADCTR );       // Memory Access Control 0x36
  APP_LCD_Data( V12_MADCTRVAL );

  APP_LCD_Cmd( ST7637_DUTYSET );      // Duty = 132 duty 0xb0
  APP_LCD_Data( 0x7F );

  APP_LCD_Cmd( ST7637_DISPON );       // Display ON
  APP_LCD_Cmd( ST7637_FRAMESET );     // Gamma
  APP_LCD_Data( 0x00 );               //
  APP_LCD_Data( 0x03 );               //
  APP_LCD_Data( 0x05 );               //
  APP_LCD_Data( 0x07 );               //
  APP_LCD_Data( 0x09 );               //
  APP_LCD_Data( 0x0B );               //
  APP_LCD_Data( 0x0D );               //
  APP_LCD_Data( 0x0F );               //
  APP_LCD_Data( 0x11 );               //
  APP_LCD_Data( 0x13 );               //
  APP_LCD_Data( 0x15 );               //
  APP_LCD_Data( 0x17 );               //
  APP_LCD_Data( 0x19 );               //
  APP_LCD_Data( 0x1B );               //
  APP_LCD_Data( 0x1D );               //
  APP_LCD_Data( 0x1F );               //

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

  // Configure Data lines as Output
  PIN_RS(1);
  PIN_RD(1);
  PIN_CS(0);
  PIN_WR(0);

  // Write data to the LCD
  APP_LCD_D_PORT->ODR = (APP_LCD_D_PORT->ODR & 0xff00) | (u32)data;
  PIN_WR(0);
#if 1
  PIN_WR(1);
#else
  for(delay=0; delay<1; ++delay) PIN_WR(1); // memo: fine for 72 MHz, increase delay if display output doesn't work @ higher speeds
#endif

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

  // Configure Data lines as Output
  PIN_RS(0);
  PIN_RD(1);
  PIN_CS(0);
  PIN_WR(0);

  // Write data to the LCD
  APP_LCD_D_PORT->ODR = (APP_LCD_D_PORT->ODR & 0xff00) | (u32)cmd;
  PIN_WR(0);
  for(delay=0; delay<10; ++delay) PIN_WR(1); // memo: fine for 72 MHz, increase delay if display output doesn't work @ higher speeds

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  int x, y;

  // clear whole 128x128 screen with background colour

  // ST7637 specific function to set view
  APP_LCD_SetRect_For_Cmd(0, 0, 128, 128);

  // Send command to write data on the LCD screen.
  APP_LCD_Cmd(ST7637_RAMWR);

  for(x=0; x<128; ++x) {
    for(y=0; y<127; ++y) {
      APP_LCD_Data(lcd_bcolour & 0xff);
      APP_LCD_Data(lcd_bcolour >> 8);
    }
  }

  // set cursor to initial position
  APP_LCD_CursorSet(0, 0);

  return 0; // no error
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
// Prints a single character
// IN: character in <c>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(char c)
{
  int x, y, lines;

  // font not initialized yet!
  if( mios32_lcd_font == NULL )
    return -1;

  u8 y_lines = (mios32_lcd_font_height>>3);

  for(lines=0; lines<y_lines; ++lines) {

    // calculate pointer to character line
    u8 *font_ptr = mios32_lcd_font + lines * mios32_lcd_font_offset + y_lines * mios32_lcd_font_offset * (size_t)c + (size_t)mios32_lcd_font_x0;

    // ST7637 specific function to set view
    APP_LCD_SetRect_For_Cmd(mios32_lcd_x, 128-mios32_lcd_y-8*(lines+1), mios32_lcd_font_width, 8);

    // Send command to write data on the LCD screen.
    APP_LCD_Cmd(ST7637_RAMWR);

    // transfer character
    for(x=0; x<mios32_lcd_font_width; ++x) {
      u8 b = *font_ptr++;
      for(y=0; y<8; ++y) {
	if( b & 0x80 ) {
	  APP_LCD_Data(lcd_fcolour & 0xff);
	  APP_LCD_Data(lcd_fcolour >> 8);
	} else {
	  APP_LCD_Data(lcd_bcolour & 0xff);
	  APP_LCD_Data(lcd_bcolour >> 8);
	}
	b <<= 1;
      }
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
  // coding: G2G1G0B4 B3B2B1B0 R4R3R2R1 R0G5G4G3
  lcd_bcolour = ((g&0x07)<<13) | ((g&0x38)>>3) | ((b&0x1f)<<8) | ((r&0x1f)<<3);
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b values
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u8 r, u8 g, u8 b)
{
  // coding: G2G1G0B4 B3B2B1B0 R4R3R2R1 R0G5G4G3
  lcd_fcolour = ((g&0x07)<<13) | ((g&0x38)>>3) | ((b&0x1f)<<8) | ((r&0x1f)<<3);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ST7637 specific code
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Define the rectangle for the next command to be applied
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_SetRect_For_Cmd(s16 x, s16 y, s16 width, s16 height)
{
  APP_LCD_Cmd(ST7637_CASET);
  APP_LCD_Data(y);
  APP_LCD_Data(y + height - 1);

  APP_LCD_Cmd(ST7637_RASET);
  APP_LCD_Data(x + 4);
  APP_LCD_Data(x + 4 + width - 1);
}
