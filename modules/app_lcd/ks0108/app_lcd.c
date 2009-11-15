// $Id$
/*
 * Application specific GLCD driver for KS0108 with horizontal screen (-> 240x64)
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
// Driver specific pin definitions for MBHP_CoRE_STM32 board
/////////////////////////////////////////////////////////////////////////////

#define APP_LCD_CS0_PORT       GPIOC       // J5C.A8
#define APP_LCD_CS0_PIN        GPIO_Pin_4
#define APP_LCD_CS1_PORT       GPIOC       // J5C.A9
#define APP_LCD_CS1_PIN        GPIO_Pin_5
#define APP_LCD_CS2_PORT       GPIOB       // J5C.A10
#define APP_LCD_CS2_PIN        GPIO_Pin_0
#define APP_LCD_CS3_PORT       GPIOB       // J5C.A11
#define APP_LCD_CS3_PIN        GPIO_Pin_1


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#if APP_LCD_CS_INVERTED
# define PIN_CS0_0   { APP_LCD_CS0_PORT->BSRR = APP_LCD_CS0_PIN; }
# define PIN_CS0_1   { APP_LCD_CS0_PORT->BRR  = APP_LCD_CS0_PIN; }
# define PIN_CS1_0   { APP_LCD_CS1_PORT->BSRR = APP_LCD_CS1_PIN; }
# define PIN_CS1_1   { APP_LCD_CS1_PORT->BRR  = APP_LCD_CS1_PIN; }
# define PIN_CS2_0   { APP_LCD_CS2_PORT->BSRR = APP_LCD_CS2_PIN; }
# define PIN_CS2_1   { APP_LCD_CS2_PORT->BRR  = APP_LCD_CS2_PIN; }
# define PIN_CS3_0   { APP_LCD_CS3_PORT->BSRR = APP_LCD_CS3_PIN; }
# define PIN_CS3_1   { APP_LCD_CS3_PORT->BRR  = APP_LCD_CS3_PIN; }
#else
# define PIN_CS0_0   { APP_LCD_CS0_PORT->BRR  = APP_LCD_CS0_PIN; }
# define PIN_CS0_1   { APP_LCD_CS0_PORT->BSRR = APP_LCD_CS0_PIN; }
# define PIN_CS1_0   { APP_LCD_CS1_PORT->BRR  = APP_LCD_CS1_PIN; }
# define PIN_CS1_1   { APP_LCD_CS1_PORT->BSRR = APP_LCD_CS1_PIN; }
# define PIN_CS2_0   { APP_LCD_CS2_PORT->BRR  = APP_LCD_CS2_PIN; }
# define PIN_CS2_1   { APP_LCD_CS2_PORT->BSRR = APP_LCD_CS2_PIN; }
# define PIN_CS3_0   { APP_LCD_CS3_PORT->BRR  = APP_LCD_CS3_PIN; }
# define PIN_CS3_1   { APP_LCD_CS3_PORT->BSRR = APP_LCD_CS3_PIN; }
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void APP_LCD_SetCS(u8 all);



/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE) < 0 )
    return -2; // failed to initialize J15

  // configure CS pins
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // weak driver to reduce transients
#if APP_LCD_OUTPUT_MODE
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif

  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS0_PIN;
  GPIO_Init(APP_LCD_CS0_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS1_PIN;
  GPIO_Init(APP_LCD_CS1_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS2_PIN;
  GPIO_Init(APP_LCD_CS2_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS3_PIN;
  GPIO_Init(APP_LCD_CS3_PORT, &GPIO_InitStructure);

  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // set LCD type
  mios32_lcd_type = MIOS32_LCD_TYPE_GLCD;

  // initialize LCD
#ifdef MIOS32_DONT_USE_DELAY
  u32 delay;
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  // "Display On" command
  APP_LCD_Cmd(0x3e + 1);

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // determine chip select line(s)
  APP_LCD_SetCS(0); // select display depending on current X position

  // wait until LCD unbusy, exit on error (timeout)
  if( MIOS32_BOARD_J15_PollUnbusy(mios32_lcd_device, 2500) < 0 ) {
    // disable display
    display_available &= ~(1 << mios32_lcd_device);
    return -2; // timeout
  }

  // write data
  MIOS32_BOARD_J15_DataSet(data);
  MIOS32_BOARD_J15_RS_Set(1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);

  // increment graphical cursor
  // if end of display segment reached: set X position of all segments to 0
  if( (++mios32_lcd_x & 0x3f) == 0x00 )
    return APP_LCD_Cmd(0x40);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // determine chip select line(s)
  APP_LCD_SetCS(0); // select display depending on current X position

  // wait until LCD unbusy, exit on error (timeout)
  if( MIOS32_BOARD_J15_PollUnbusy(mios32_lcd_device, 2500) < 0 ) {
    // disable display
    display_available &= ~(1 << mios32_lcd_device);
    return -2; // timeout
  }

  // select all displays
  APP_LCD_SetCS(1);

  // write command
  MIOS32_BOARD_J15_DataSet(cmd);
  MIOS32_BOARD_J15_RS_Set(0);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);

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

  // use default font
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  for(y=0; y<(APP_LCD_HEIGHT/8); ++y) {
    error |= MIOS32_LCD_CursorSet(0, y);
    for(x=0; x<APP_LCD_WIDTH; ++x)
      error |= APP_LCD_Data(0x00);
  }

  // set Y0=0
  error |= APP_LCD_Cmd(0xc0 + 0);

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
  error |= APP_LCD_Cmd(0x40 | (x & 0x3f));

  // set Y position
  error |= APP_LCD_Cmd(0xb8 | ((y>>3) & 0x7));

  return error;
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
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb)
{
  return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  if( x >= bitmap.width || y >= bitmap.height )
    return -1; // pixel is outside bitmap

  u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
  u8 mask = 1 << (y % 8);

  *pixel &= ~mask;
  if( colour )
    *pixel |= mask;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  int line;
  int y_lines = (bitmap.height >> 3);

  for(line=0; line<y_lines; ++line) {

    // calculate pointer to bitmap line
    u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

    // set graphical cursor after second line has reached
    if( line > 0 ) {
      mios32_lcd_x -= bitmap.width;
      mios32_lcd_y += 8;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

    // transfer bitmap
    int x;
    for(x=0; x<bitmap.width; ++x)
      APP_LCD_Data(*memory_ptr++);
  }

  // fix graphical cursor if more than one line has been print
  if( y_lines >= 1 ) {
    mios32_lcd_y = mios32_lcd_y - (bitmap.height-8);
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Help Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// set CS line depending on X cursor position
// if "all" flag is set, commands are sent to all segments
static void APP_LCD_SetCS(u8 all)
{
  if( all ) {
    // set all chip select lines
    PIN_CS0_1;
    PIN_CS1_1;
    PIN_CS2_1;
    PIN_CS3_1;
  } else {
    // set only one chip select line depending on X pos   
    PIN_CS0_0;
    PIN_CS1_0;
    PIN_CS2_0;
    PIN_CS3_0;
    switch( (mios32_lcd_x >> 6) & 0x3 ) {
      case 0: PIN_CS0_1; break;
      case 1: PIN_CS1_1; break;
      case 2: PIN_CS2_1; break;
      case 3: PIN_CS3_1; break;
    }
  }
}
