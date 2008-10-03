// $Id$
/*
 * Application specific GLCD driver for ST7637
 * Referenced from MIOS32_LCD routines
 *
 * Some of the code has been taken over and adapted from Raisonance Circle OS
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

void APP_LCD_CtrlLinesWrite(GPIO_TypeDef* GPIOx, u32 CtrlPins, BitAction BitVal);
void APP_LCD_DataLinesWrite(GPIO_TypeDef* GPIOx, u32 PortVal);
void APP_LCD_DataLinesConfig(u8 output);
void APP_LCD_CheckLCDStatus(void);
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

  // Enable GPIO clock for LCD
  RCC_APB2PeriphClockCmd(GPIO_LCD_CTRL_PERIPH, ENABLE);
  RCC_APB2PeriphClockCmd(GPIO_LCD_D_PERIPH, ENABLE);
  RCC_APB2PeriphClockCmd(GPIO_LCD_CS_PERIPH, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  // backlight pin (could also be dimmed via PWM)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // backlight on
  GPIO_SetBits(GPIOB, GPIO_Pin_7);

  // control lines
  GPIO_InitStructure.GPIO_Pin   =  LCD_CTRL_PINS;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOx_CTRL_LCD, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   =  CtrlPin_CS;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOx_CS_LCD, &GPIO_InitStructure);

  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RS,  Bit_SET );    /* RS = 1   */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD,  Bit_SET );    /* RD = 1   */
  APP_LCD_CtrlLinesWrite( GPIOx_CS_LCD,   CtrlPin_CS,  Bit_SET );    /* CS = 1   */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR,  Bit_SET );    /* WR = 1   */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RST, Bit_RESET );  /* RST = 0  */

  // Apply hardware reset
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RST, Bit_SET );    /* RST = 1  */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RST, Bit_RESET );  /* RST = 0  */
  for(delay=0; delay<0x500; ++delay);

  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RST, Bit_SET );    /* RST = 1  */
  for(delay=0; delay<0x500; ++delay);

  // default mode is output
  APP_LCD_DataLinesConfig(1);

  APP_LCD_CheckLCDStatus();

  APP_LCD_Cmd(ST7637_SWRESET);

  //-----------disable autoread + Manual read once ----------------------------
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

  APP_LCD_Cmd( 0xB7 );                // Glass direction
  APP_LCD_Data( 0xC0 );               //

  APP_LCD_Cmd( ST7637_ANASET );       // Analog circuit setting 0xd0
  APP_LCD_Data( 0x1D );               //

  APP_LCD_Cmd( 0xB4 );                // PTL mode set
  APP_LCD_Data( 0x18 );               // power normal mode
  APP_LCD_Cmd( ST7637_INVOFF );       // Display Inversion OFF 0x20

  APP_LCD_Cmd( 0x2A );                // column range
  APP_LCD_Data( 0x04 );               //
  APP_LCD_Data( 0x83 );               //

  APP_LCD_Cmd( 0x2B );                // raw range
  APP_LCD_Data( 0x04 );               //
  APP_LCD_Data( 0x83 );               //


  APP_LCD_Cmd( ST7637_COLMOD );       // Color mode = 65k 0x3A
  APP_LCD_Data( 0x05 );               //

  APP_LCD_Cmd( ST7637_MADCTR );       // Memory Access Control 0x36
  APP_LCD_Data( V12_MADCTRVAL );

  APP_LCD_Cmd( ST7637_DUTYSET );      // Duty = 132 duty 0xb0
  APP_LCD_Data( 0x7F );

  APP_LCD_Cmd( 0x29 );                // Display ON
  APP_LCD_Cmd( 0xF9 );                // Gamma
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
  // Configure Data lines as Output
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RS, Bit_SET );       /* RS = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_SET );       /* RD = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CS_LCD,   CtrlPin_CS, Bit_RESET );     /* CS = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR, Bit_RESET );     /* WR = 0 */

  // Write data to the LCD
  APP_LCD_DataLinesWrite( GPIOx_D_LCD,(u32)data );
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR, Bit_SET );       /* WR = 1 */

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // Configure Data lines as Output
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RS, Bit_RESET );     /* RS = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_SET );       /* RD = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CS_LCD,   CtrlPin_CS, Bit_RESET );     /* CS = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR, Bit_RESET );     /* WR = 0 */

  // Write data to the LCD
  APP_LCD_DataLinesWrite( GPIOx_D_LCD, (u32)cmd );
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR, Bit_SET );       /* WR = 1 */

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
  lcd_bcolour = ((g&0x07)<<13) | (g>>5) | ((b>>3)<<8) | ((r>>3)<<3);
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
  lcd_fcolour = ((g&0x07)<<13) | (g>>5) | ((b>>3)<<8) | ((r>>3)<<3);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ST7637 specific code
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Write to control lines
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_CtrlLinesWrite(GPIO_TypeDef* GPIOx, u32 CtrlPins, BitAction BitVal)
{
  // Set or Reset the control line
  GPIO_WriteBit(GPIOx, CtrlPins, BitVal);
}

/////////////////////////////////////////////////////////////////////////////
// Write to data lines
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_DataLinesWrite(GPIO_TypeDef* GPIOx, u32 PortVal)
{
  // Write only the lowest 8 bits!
  GPIOx->ODR = ( (GPIOx->ODR) & 0xFF00 ) | (u8)PortVal;
}

/////////////////////////////////////////////////////////////////////////////
// Set data lines to input (0) or output (1)
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_DataLinesConfig(u8 output)
{
  GPIO_InitTypeDef             GPIO_InitStructure;
	   
  GPIO_InitStructure.GPIO_Pin   =  LCD_DATA_PINS;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  if( output ) {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  } else{
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  }

  GPIO_Init(GPIOx_D_LCD, &GPIO_InitStructure);
}

/////////////////////////////////////////////////////////////////////////////
// Check whether LCD is busy or not
/////////////////////////////////////////////////////////////////////////////
void APP_LCD_CheckLCDStatus(void)
{
  unsigned char ID1;
  unsigned char ID2;
  unsigned char ID3;

  APP_LCD_Cmd(ST7637_RDDID);

  // Configure Data lines as Input
  APP_LCD_DataLinesConfig(0);

  // Start the LCD send data sequence
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RS, Bit_RESET );     /* RS = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_RESET );     /* RD = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CS_LCD,   CtrlPin_CS, Bit_RESET );     /* CS = 0 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_WR, Bit_SET );       /* WR = 1 */

  // Read data to the LCD
  GPIO_ReadInputData( GPIOx_D_LCD );
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_SET );       /* RD = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_RESET );     /* RD = 0 */

  ID1 = GPIO_ReadInputData(GPIOx_D_LCD);

  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_SET );       /* RD = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_RESET );     /* RD = 0 */

  ID2 = GPIO_ReadInputData(GPIOx_D_LCD);

  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_SET );       /* RD = 1 */
  APP_LCD_CtrlLinesWrite( GPIOx_CTRL_LCD, CtrlPin_RD, Bit_RESET );     /* RD = 0 */

  ID3 = GPIO_ReadInputData(GPIOx_D_LCD);

  APP_LCD_DataLinesConfig(1);
}

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
