// $Id: app_lcd.c 2179 2015-06-10 18:36:14Z hawkeye $
/*
 * Application specific OLED driver for up to 8 * SSD1322 (every display
 * provides a resolution of 256x64)
 * Referenced from MIOS32_LCD routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
// Local defines
/////////////////////////////////////////////////////////////////////////////

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef APP_LCD_OUTPUT_MODE
#define APP_LCD_OUTPUT_MODE  0
#endif

// for LPC17 only: should J10 be used for CS lines
// This option is nice if no J15 shiftregister is connected to a LPCXPRESSO.
// This shiftregister is available on the MBHP_CORE_LPC17 module
#ifndef APP_LCD_USE_J10_FOR_CS
#define APP_LCD_USE_J10_FOR_CS 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Instruction Setting
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Column_Address(unsigned char start_addr, unsigned char end_addr)
{
  APP_LCD_Cmd(0x15);						// Set Column Address
  APP_LCD_Data(start_addr);						//   Default => 0x00
  APP_LCD_Data(end_addr);						//   Default => 0x77
}


void Set_Row_Address(unsigned char start_addr, unsigned char end_addr)
{
  APP_LCD_Cmd(0x75);						// Set Row Address
  APP_LCD_Data(start_addr);						//   Default => 0x00
  APP_LCD_Data(end_addr);						//   Default => 0x7F
}

void Set_Write_RAM()
{
  APP_LCD_Cmd(0x5C);						// Enable MCU to Write into RAM
}

void Set_Read_RAM()
{
  APP_LCD_Cmd(0x5D);						// Enable MCU to Read from RAM
}

void Set_Remap_Format(unsigned char format)
{
  APP_LCD_Cmd(0xA0);						// Set Re-Map / Dual COM Line Mode
  APP_LCD_Data(format);							// Default => 0x40, Horizontal Address Increment, Column Address 0 Mapped to SEG0, Disable Nibble Remap
  // Scan from COM0 to COM[N-1], Disable COM Split Odd Even
  APP_LCD_Data(0x11);							// Default => 0x01 (Disable Dual COM Mode)
}

void Set_Start_Line(unsigned char line)
{
  APP_LCD_Cmd(0xA1);						// Set Vertical Scroll by RAM
  APP_LCD_Data(line);							// Default => 0x00
}

void Set_Display_Offset(unsigned char offset)
{
  APP_LCD_Cmd(0xA2);						// Set Vertical Scroll by Row
  APP_LCD_Data(offset);							// Default => 0x00
}

void Set_Display_Mode(unsigned char mode)
{
  APP_LCD_Cmd(0xA4|mode);					// Set Display Mode
  //   Default => 0xA4
  //     0xA4 (0x00) => Entire Display Off, All Pixels Turn Off
  //     0xA5 (0x01) => Entire Display On, All Pixels Turn On at GS Level 15
  //     0xA6 (0x02) => Normal Display
  //     0xA7 (0x03) => Inverse Display
}

void Set_Partial_Display_On(unsigned char start_row, unsigned char end_row)
{
  APP_LCD_Cmd(0xA8);
  APP_LCD_Data(start_row);
  APP_LCD_Data(end_row);
}

void Set_Partial_Display_Off()
{
  APP_LCD_Cmd(0xA9);
}

void Set_Function_Selection(unsigned char function)
{
  APP_LCD_Cmd(0xAB);						// Function Selection
  APP_LCD_Data(function);						//   Default => 0x01, Enable Internal VDD Regulator
}

void Set_Display_On()
{
  APP_LCD_Cmd(0xAF);
}

void Set_Display_Off()
{
  APP_LCD_Cmd(0xAE);
}

void Set_Phase_Length(unsigned char length)
{
  APP_LCD_Cmd(0xB1);						// Phase 1 (Reset) & Phase 2 (Pre-Charge) Period Adjustment
  APP_LCD_Data(length);							//   Default => 0x74 (7 Display Clocks [Phase 2] / 9 Display Clocks [Phase 1])
  //     D[3:0] => Phase 1 Period in 5~31 Display Clocks
  //     D[7:4] => Phase 2 Period in 3~15 Display Clocks
}

void Set_Display_Clock(unsigned char divider)
{
  APP_LCD_Cmd(0xB3);						// Set Display Clock Divider / Oscillator Frequency
  APP_LCD_Data(divider);						//   Default => 0xD0, A[3:0] => Display Clock Divider, A[7:4] => Oscillator Frequency
}

void Set_Display_Enhancement_A(unsigned char vsl, unsigned char quality)
{
  APP_LCD_Cmd(0xB4);						// Display Enhancement
  APP_LCD_Data(0xA0|vsl);						//   Default => 0xA2, 0xA0 (0x00) => Enable External VSL, 0xA2 (0x02) => Enable Internal VSL (Kept VSL Pin N.C.)
  APP_LCD_Data(0x05|quality);					//   Default => 0xB5, 0xB5 (0xB0) => Normal, 0xFD (0xF8) => Enhance Low Gray Scale Display Quality
}

void Set_GPIO(unsigned char gpio)
{
  APP_LCD_Cmd(0xB5);						// General Purpose IO
  APP_LCD_Data(gpio);							//   Default => 0x0A (GPIO Pins output Low Level.)
}

void Set_Precharge_Period(unsigned char period)
{
  APP_LCD_Cmd(0xB6);						// Set Second Pre-Charge Period
  APP_LCD_Data(period);							//   Default => 0x08 (8 Display Clocks)
}

void Set_Precharge_Voltage(unsigned char voltage)
{
  APP_LCD_Cmd(0xBB);						// Set Pre-Charge Voltage Level
  APP_LCD_Data(voltage);						//   Default => 0x17 (0.50*VCC)
}

void Set_VCOMH(unsigned char voltage_level)
{
  APP_LCD_Cmd(0xBE);						// Set COM Deselect Voltage Level
  APP_LCD_Data(voltage_level);					//   Default => 0x04 (0.80*VCC)
}

void Set_Contrast_Current(unsigned char contrast)
{
  APP_LCD_Cmd(0xC1);						// Set Contrast Current
  APP_LCD_Data(contrast);						//   Default => 0x7F
}

void Set_Master_Current(unsigned char master_current)
{
  APP_LCD_Cmd(0xC7);						// Master Contrast Current Control
  APP_LCD_Data(master_current);					//   Default => 0x0f (Maximum)
}

void Set_Multiplex_Ratio(unsigned char ratio)
{
  APP_LCD_Cmd(0xCA);						// Set Multiplex Ratio
  APP_LCD_Data(ratio);							//   Default => 0x7F (1/128 Duty)
}

void Set_Display_Enhancement_B(unsigned char enhancement)
{
  APP_LCD_Cmd(0xD1);						// Display Enhancement
  APP_LCD_Data(0x82|enhancement);				//   Default => 0xA2, 0x82 (0x00) => Reserved, 0xA2 (0x20) => Normal
  APP_LCD_Data(0x20);
}

void Set_Command_Lock(unsigned char lock)
{
  APP_LCD_Cmd(0xFD);						// Set Command Lock
  APP_LCD_Data(0x12|lock);						//   Default => 0x12
  //     0x12 => Driver IC interface is unlocked from entering command.
  //     0x16 => All Commands are locked except 0xFD.
}

void Set_Gray_Scale_Table()
{
  APP_LCD_Cmd(0xB8);						// Set Gray Scale Table

  APP_LCD_Data(0x00);						//   Gray Scale Level 1
  APP_LCD_Data(0x05);//  APP_LCD_Data(0x28);						//   Gray Scale Level 2
  APP_LCD_Data(0x10);						//   Gray Scale Level 3
  APP_LCD_Data(0x15);						//   Gray Scale Level 4
  APP_LCD_Data(0x20);						//   Gray Scale Level 5
  APP_LCD_Data(0x30);						//   Gray Scale Level 6
  APP_LCD_Data(0x40);						//   Gray Scale Level 7
  APP_LCD_Data(0x50);						//   Gray Scale Level 8
  APP_LCD_Data(0x60);						//   Gray Scale Level 9
  APP_LCD_Data(0x70);						//   Gray Scale Level 10
  APP_LCD_Data(0x80);						//   Gray Scale Level 11
  APP_LCD_Data(0x90);						//   Gray Scale Level 12
  APP_LCD_Data(0xA0);						//   Gray Scale Level 13
  APP_LCD_Data(0xB0);						//   Gray Scale Level 14
  APP_LCD_Data(0xC0);						//   Gray Scale Level 15

  /*APP_LCD_Data(0x00);						//   Gray Scale Level 1
  APP_LCD_Data(0x05);//  APP_LCD_Data(0x28);						//   Gray Scale Level 2
  APP_LCD_Data(0x31);						//   Gray Scale Level 3
  APP_LCD_Data(0x43);						//   Gray Scale Level 4
  APP_LCD_Data(0x4D);						//   Gray Scale Level 5
  APP_LCD_Data(0x56);						//   Gray Scale Level 6
  APP_LCD_Data(0x60);						//   Gray Scale Level 7
  APP_LCD_Data(0x68);						//   Gray Scale Level 8
  APP_LCD_Data(0x72);						//   Gray Scale Level 9
  APP_LCD_Data(0x7C);						//   Gray Scale Level 10
  APP_LCD_Data(0x86);						//   Gray Scale Level 11
  APP_LCD_Data(0x91);						//   Gray Scale Level 12
  APP_LCD_Data(0x9B);						//   Gray Scale Level 13
  APP_LCD_Data(0xA6);						//   Gray Scale Level 14
  APP_LCD_Data(0xB4);						//   Gray Scale Level 15
  */

  APP_LCD_Cmd(0x00);						// Enable Gray Scale Table
}

/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE) < 0 )
    return -2; // failed to initialize J15

#if APP_LCD_USE_J10_FOR_CS
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, APP_LCD_OUTPUT_MODE ? MIOS32_BOARD_PIN_MODE_OUTPUT_OD : MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#endif

  // set LCD type
  mios32_lcd_parameters.lcd_type = MIOS32_LCD_TYPE_GLCD_CUSTOM;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_X;
  mios32_lcd_parameters.width = APP_LCD_WIDTH;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_Y;
  mios32_lcd_parameters.height = APP_LCD_HEIGHT;
  mios32_lcd_parameters.colour_depth = APP_LCD_COLOUR_DEPTH;

  u16 ctr;
  for (ctr=0; ctr<300; ++ctr)
    MIOS32_DELAY_Wait_uS(1000);

  // Initialize LCD (old code)
  /*
  APP_LCD_Cmd(0xfd); // Unlock 
  APP_LCD_Data(0x12);

  APP_LCD_Cmd(0xae | 0); // Set_Display_Off

  APP_LCD_Cmd(0x15); // Set_Column_Address
  APP_LCD_Data(0x1c);  
  APP_LCD_Data(0x5b); 

  APP_LCD_Cmd(0x75); // Set_Row_Address
  APP_LCD_Data(0x00); 
  APP_LCD_Data(0x3f); 

  APP_LCD_Cmd(0xca); // Set_Multiplex_Ratio
  APP_LCD_Data(0x3f); 

  APP_LCD_Cmd(0xa0); // Set_Remap_Format
  APP_LCD_Data(0x14); 
  APP_LCD_Data(0x11); 

  APP_LCD_Cmd(0xb3); // Set_Display_Clock
  APP_LCD_Data(0);
  APP_LCD_Data(12);

  APP_LCD_Cmd(0xc1); // Set_Contrast_Current
  APP_LCD_Data(0xff);

  APP_LCD_Cmd(0xc7); // Set_Master_Current
  APP_LCD_Data(0x0f);

  APP_LCD_Cmd(0xb9); // Set_Linear_Gray_Scale_Table();
  APP_LCD_Cmd(0x00); // OLD  enable gray scale table

  APP_LCD_Cmd(0xb1); // Set_Phase_Length(0xE2);
  APP_LCD_Data(0x56); 

  APP_LCD_Cmd(0xbb); // Set_Precharge_Voltage(0x1F);
  APP_LCD_Data(0x0); 

  APP_LCD_Cmd(0xb6); // Set_Precharge_Period(0x08);
  APP_LCD_Data(0x08);

  APP_LCD_Cmd(0xbe); // Set_VCOMH(0x07);
  APP_LCD_Data(0x00); 

  APP_LCD_Cmd(0xa4 | 0x02); // OLD Set_Display_Mode(0x02);

  APP_LCD_Clear();

  APP_LCD_Cmd(0xae | 1); // Set_Display_On_Off(0x01);
   */

  // Initialize display (NHD 3.12 datasheet)
  Set_Command_Lock(0x12); // Unlock Basic Commands (0x12/0x16)
  Set_Display_Off();// Display Off (0x00/0x01)
  Set_Column_Address(0x1C,0x5B);
  Set_Row_Address(0x00,0x3F);
  Set_Display_Clock(0x91);// Set Clock as 80 Frames/Sec
  Set_Multiplex_Ratio(0x3F);// 1/64 Duty (0x0F~0x3F)
  Set_Display_Offset(0x00);// Shift Mapping RAM Counter (0x00~0x3F)
  Set_Start_Line(0x00);// Set Mapping RAM Display Start Line (0x00~0x7F)
  Set_Remap_Format(0x14);// Set Horizontal Address Increment//     Column Address 0 Mapped to SEG0//     Disable Nibble Remap//     Scan from COM[N-1] to COM0//     Disable COM Split Odd Even//Enable Dual COM Line Mode
  Set_GPIO(0x00);// Disable GPIO Pins Input
  Set_Function_Selection(0x01);// Enable Internal VDD Regulator
  Set_Display_Enhancement_A(0xA0,0xFD);// Enable External VSL 
  Set_Contrast_Current(0x9F);// Set Segment OutputCurrent
  Set_Master_Current(0x0F);// Set Scale Factor of Segment Output Current Control
  Set_Gray_Scale_Table();// Set Pulse Width for Gray Scale Table
  // Set_Linear_Gray_Scale_Table();//set default linear gray scale table
  Set_Phase_Length(0xE2);// Set Phase 1 as 5 Clocks & Phase 2 as 14 Clocks
  Set_Display_Enhancement_B(0x20);// Enhance Driving Scheme Capability (0x00/0x20)
  Set_Precharge_Voltage(0x1F);// Set Pre-Charge Voltage Level as 0.60*VCC
  Set_Precharge_Period(0x08);// Set Second Pre-Charge Period as 8 Clocks
  Set_VCOMH(0x07);// Set Common Pins Deselect Voltage Level as 0.86*VCC
  Set_Display_Mode(0x02);// Normal Display Mode (0x00/0x01/0x02/0x03)
  // Set_Partial_Display(0x01,0x00,0x00);// Disable Partial Display
  Set_Display_On();


  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
#if 0  // TODO
  // select LCD depending on current cursor position
  // THIS PART COULD BE CHANGED TO ARRANGE THE 8 DISPLAYS ON ANOTHER WAY
  u8 cs = mios32_lcd_y / APP_LCD_HEIGHT;

  if( cs >= 8 )
    return -1; // invalid CS line
#endif

  u8 cs=0;

  // chip select and DC
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(~(1 << cs));
#else
  MIOS32_BOARD_J15_DataSet(~(1 << cs));
#endif
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

  // send data
  MIOS32_BOARD_J15_SerDataShift(data);

  // increment graphical cursor
  ++mios32_lcd_x;

#if 0
  // if end of display segment reached: set X position of all segments to 0
  if( (mios32_lcd_x % APP_LCD_WIDTH) == 0 ) {
    APP_LCD_Cmd(0x75); // set X=0
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
  }
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
  // select all LCDs
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(0x00);
#else
  MIOS32_BOARD_J15_DataSet(0x00);
#endif
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

  MIOS32_BOARD_J15_SerDataShift(cmd);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  u8 i, j;

  for (j=0; j<64; j++)
  {
    APP_LCD_Cmd(0x15);
    APP_LCD_Data(0+0x1c);

    APP_LCD_Cmd(0x75);
    APP_LCD_Data(j);

    APP_LCD_Cmd(0x5c);

    for (i=0; i<64; i++)
    {
       APP_LCD_Data(0);
       APP_LCD_Data(0);
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
  return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  s32 error = 0;

  error |= APP_LCD_Cmd(0x75); // Row
  error |= APP_LCD_Data(y);

  error |= APP_LCD_Cmd(0x15); // Column
  error |= APP_LCD_Data((x / 4) + 0x1c);

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
  // not implemented yet
  return -1;

#if 0
  int line;
  int y_lines = (bitmap.height >> 3);

  for(line=0; line<y_lines; ++line) {
    // select the view
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    APP_LCD_Cmd(0x5c); // Write RAM

    // calculate pointer to bitmap line
    u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

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
#endif

  return 0; // no error
}
