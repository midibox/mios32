// $Id$
/*
 * Application specific GLCD driver for up to 8 * PCD8544 (every display 
 * provides a resolution of 84x48)
 * Referenced from MIOS32_LCD routines
 *
 * This display can mostly be found in Nokia mobile phones
 *
 * This driver allows to drive up to 8 of them, every display is connected
 * to a dedicated chip select line at port B. They can be addressed with
 * following (graphical) cursor positions:
 * 
 * 
 * CS at PortB.0     CS at PortB.1     CS at PortB.2
 * +--------------+  +--------------+  +--------------+
 * |              |  |              |  |              |
 * | X =   0.. 83 |  | X =  84..167 |  | X = 168..251 |  
 * | Y =   0..  5 |  | Y =   0..  5 |  | Y =   0..  5 |
 * |              |  |              |  |              |
 * +--------------+  +--------------+  +--------------+  
 *
 * CS at PortB.3     CS at PortB.4     CS at PortB.5
 * +--------------+  +--------------+  +--------------+
 * |              |  |              |  |              |
 * | X =   0.. 83 |  | X =  84..167 |  | X = 168..251 |  
 * | Y =   6.. 11 |  | Y =   6.. 11 |  | Y =   6.. 11 |
 * |              |  |              |  |              |
 * +--------------+  +--------------+  +--------------+  
 *
 * CS at PortB.6     CS at PortB.7   
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0.. 83 |  | X =  84..167 |
 * | Y =  12.. 17 |  | Y =  12.. 17 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * The arrangement can be modified below the USER_LCD_Data_CS and USER_LCD_GCursorSet label
 *
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

#define APP_LCD_LCDSCLK_PORT   GPIOC
#define APP_LCD_LCDSCLK_PIN    GPIO_Pin_7

#define APP_LCD_SDA_PORT       GPIOB
#define APP_LCD_SDA_PIN        GPIO_Pin_2

#define APP_LCD_D7_PORT        GPIOC
#define APP_LCD_D7_PIN         GPIO_Pin_12


// should output pins to LCD (SER/E1/E2/RW) be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#define APP_LCD_OUTPUTS_OD     1
// MEMO: PCD8544 works at 3.3V, level shifting (and open drain mode) not required
// TODO: try this out later


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_SER(b)  { APP_LCD_SER_PORT->BSRR = (b) ? APP_LCD_SER_PIN : (APP_LCD_SER_PIN << 16); }
#define PIN_SDA(b)  { APP_LCD_SDA_PORT->BSRR  = (b) ? APP_LCD_SDA_PIN  : (APP_LCD_SDA_PIN << 16); }

#define PIN_LCDSCLK_0  { APP_LCD_LCDSCLK_PORT->BRR  = APP_LCD_LCDSCLK_PIN; }
#define PIN_LCDSCLK_1  { APP_LCD_LCDSCLK_PORT->BSRR = APP_LCD_LCDSCLK_PIN; }

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

static void APP_LCD_SerCSWrite(u8 data, u8 dc);
static void APP_LCD_SerLCDWrite(u8 data);


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
  PIN_LCDSCLK_0;
  PIN_SDA(0);

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

  GPIO_InitStructure.GPIO_Pin = APP_LCD_LCDSCLK_PIN;
  GPIO_Init(APP_LCD_LCDSCLK_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = APP_LCD_SDA_PIN;
  GPIO_Init(APP_LCD_SDA_PORT, &GPIO_InitStructure);

  // configure "busy" input with pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = APP_LCD_D7_PIN;
  GPIO_Init(APP_LCD_D7_PORT, &GPIO_InitStructure);


  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // initialize LCD
  for(delay=0; delay<50000; ++delay) PIN_SDA(0); // 50 mS Delay

  // initialisation sequence based on PCD8544 datasheet
  APP_LCD_Cmd(0x21); // PD=0 and V=0, select extended instruction set (H=1 mode)
  APP_LCD_Cmd(0x90); // Vop is set to a + 16 x b[V]
  APP_LCD_Cmd(0x20); // PD=0 and V=0, select normal instruction set (H=1 mode)
  APP_LCD_Cmd(0x0c); // enter normal mode (D=1 and E=0)

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // select LCD depending on current cursor position
  // THIS PART COULD BE CHANGED TO ARRANGE THE 8 DISPLAYS ON ANOTHER WAY
  u8 line = 0;
  if( mios32_lcd_y >= 2*6*8 )
    line = 2;
  else if( mios32_lcd_y >= 1*6*8 )
    line = 1;

  u8 row = 0;
  if( mios32_lcd_x >= 2*84 )
    row = 2;
  else if( mios32_lcd_x >= 1*84 )
    row = 1;

  u8 cs = 3*line + row;

  if( cs >= 8 )
    return -1; // invalid CS line

  APP_LCD_SerCSWrite(~(1 << cs), 1); // CS, dc
  //  APP_LCD_SerCSWrite(~0x02, 1); // CS, dc

  // send data
  APP_LCD_SerLCDWrite(data);

  // increment graphical cursor
  ++mios32_lcd_x;
  // if end of display segment reached: set X position of all segments to 0
  if( (mios32_lcd_x % 84) == 0 )
    return APP_LCD_Cmd(0x80); // set X=0

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // select all LCDs
  APP_LCD_SerCSWrite(0x00, 0); // CS, dc

  // send command
  APP_LCD_SerLCDWrite(cmd);

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

  // send data
  for(y=0; y<6; ++y) {
    error |= MIOS32_LCD_CursorSet(0, y);

    // select all LCDs
    APP_LCD_SerCSWrite(0x00, 1); // CS, dc

    for(x=0; x<84; ++x)
      APP_LCD_SerLCDWrite(0x00);
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
  s32 error = 0;

#if 0
  // exit with error if line is not in allowed range
  if( line >= MIOS32_LCD_MAX_MAP_LINES )
    return -1;

  // exit if mapped line >= 8 (not supported by KS0108)
  u8 mapped_line = mios32_lcd_cursor_map[line];
  if( mapped_line >= 8 )
    return -2;

  // ...

  return error;
#else
  // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
  return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  s32 error = 0;

  // set X position
  error |= APP_LCD_Cmd(0x80 | (x % 84));

  // set Y position
  error |= APP_LCD_Cmd(0x40 | ((y>>3) % 6));

  return error;
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

  for(line=0; line<y_lines; ++line) {

    // calculate pointer to character line
    u8 *font_ptr = mios32_lcd_font + line * mios32_lcd_font_offset + y_lines * mios32_lcd_font_offset * (size_t)c + (size_t)mios32_lcd_font_x0;

    // set graphical cursor after second line has reached
    if( line > 0 ) {
      mios32_lcd_x -= mios32_lcd_font_width;
      mios32_lcd_y += 8;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

    // transfer character
    for(x=0; x<mios32_lcd_font_width; ++x)
      APP_LCD_Data(*font_ptr++);
  }

  // fix graphical cursor if more than one line has been print
  if( y_lines >= 1 ) {
    mios32_lcd_y = mios32_lcd_y - (mios32_lcd_font_height-8);
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
  }

  // increment cursor
  mios32_lcd_column += 1;

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

// 74HC595 register update - the CS lines are connected to the 8 outputs
static void APP_LCD_SerCSWrite(u8 data, u8 dc)
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

  // set DC line
  PIN_SER(dc);

  PIN_RCLK_1;
}

// serial transfer to LCD
static void APP_LCD_SerLCDWrite(u8 data)
{
  // shift in 8bit data
  // PCD8544 datasheet specifies a setup/hold time of 100 nS

  PIN_SDA(data & 0x80); // D7
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x40); // D6
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x20); // D5
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x10); // D4
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x08); // D3
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x04); // D2
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x02); // D1
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
  PIN_SDA(data & 0x01); // D0
  PIN_LCDSCLK_0; // setup delay
  PIN_LCDSCLK_1;
}
