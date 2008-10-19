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

#define APP_LCD_CS0_PORT       GPIOC       // J5C.A8
#define APP_LCD_CS0_PIN        GPIO_Pin_4
#define APP_LCD_CS1_PORT       GPIOC       // J5C.A9
#define APP_LCD_CS1_PIN        GPIO_Pin_5
#define APP_LCD_CS2_PORT       GPIOB       // J5C.A10
#define APP_LCD_CS2_PIN        GPIO_Pin_0
#define APP_LCD_CS3_PORT       GPIOB       // J5C.A11
#define APP_LCD_CS3_PIN        GPIO_Pin_1


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

static void APP_LCD_SerWrite(u8 data, u8 rs);
static void APP_LCD_SetRS(u8 rs);
static void APP_LCD_Strobe(u8 value);
static s32  APP_LCD_WaitUnbusy(void);
static void APP_LCD_SetCS(u8 all);



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

  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS0_PIN;
  GPIO_Init(APP_LCD_CS0_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS1_PIN;
  GPIO_Init(APP_LCD_CS1_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS2_PIN;
  GPIO_Init(APP_LCD_CS2_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = APP_LCD_CS3_PIN;
  GPIO_Init(APP_LCD_CS3_PORT, &GPIO_InitStructure);

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
  for(delay=0; delay<50000; ++delay) PIN_RW(0); // 50 mS Delay

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
  // wait until LCD unbusy, exit on error (timeout)
  if( APP_LCD_WaitUnbusy() < 0 )
    return -1; // timeout

  // determine chip select line(s)
  APP_LCD_SetCS(0); // select display depending on current X position

  // write data
  APP_LCD_SerWrite(data, 1); // data, rs
  APP_LCD_Strobe(1);
  APP_LCD_Strobe(0);

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
  // wait until LCD unbusy, exit on error (timeout)
  if( APP_LCD_WaitUnbusy() < 0 )
    return -1; // timeout

  // determine chip select line(s)
  APP_LCD_SetCS(1); // select all displays

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
  s32 error = 0;
  u8 x, y;

  for(y=0; y<8; ++y) {
    error |= MIOS32_LCD_CursorSet(0, y);
    for(x=0; x<240; ++x)
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

// writes to serial register
static void APP_LCD_SerWrite(u8 data, u8 rs)
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
static void APP_LCD_SetRS(u8 rs)
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
static void APP_LCD_Strobe(u8 value)
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
static s32 APP_LCD_WaitUnbusy(void)
{
  u32 poll_ctr;
  u32 delay_ctr;

  // don't wait if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // determine chip select line(s)
  APP_LCD_SetCS(0); // select display depending on current X position

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
