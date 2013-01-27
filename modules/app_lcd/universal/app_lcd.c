// $Id$
/*
 * Universal LCD driver which supports various character and graphical LCDs
 *
 * See mios32_lcd_type_t in mios32_lcd.h for list of LCDs
 *
 * The LCD type will be fetched from the Bootloader Info section after reset.
 * It can be changed with the bootloader update application.
 *
 * Optionally it's also possible to change the type during runtime with
 * MIOS32_LCD_TypeSet (e.g. after it has been loaded from a config file from
 * SD Card)
 *
 * This driver has to be selected with MIOS32_LCD environment variable set
 * to "universal"
 *
 * Please only add drivers which are not resource hungry (e.g. they shouldn't
 * allocate RAM for buffering) - otherwise all applications would be affected!
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
#include <string.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
// the number of LCDs is currently only limited by the size of the display_available variable!
#define MAX_LCDS 64


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static unsigned long long display_available = 0;
static u8 lcd_testmode = 0;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Sets the CS line of a KS0108 LCD depending on X cursor position
// if "all" flag is set, commands are sent to all segments
/////////////////////////////////////////////////////////////////////////////
static s32 APP_LCD_KS0108_CS_Set(u8 all)
{
  // determine polarity of CS pins
  u8 level_active = (mios32_lcd_parameters.lcd_type == MIOS32_LCD_TYPE_GLCD_KS0108_INVCS) ? 0 : 1;
  u8 level_nonactive = level_active ? 0 : 1;

  int cs;
  // STM32: the 4 CS pins are available at J5C
  // LPC17: the 4 CS pins are available at J28
#if defined(MIOS32_FAMILY_STM32F10x)
  if( all ) {
    // set all chip select lines
    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J5_PinSet(cs+8, level_active);
  } else {
    // set only one chip select line depending on X pos   
    u8 sel_cs = (mios32_lcd_x >> 6) & 0x3;

    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J5_PinSet(cs+8, (cs == sel_cs) ? level_active : level_nonactive);
  }
#elif defined(MIOS32_FAMILY_LPC17xx)
  if( all ) {
    // set all chip select lines
    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J28_PinSet(cs, level_active);
  } else {
    // set only one chip select line depending on X pos   
    u8 sel_cs = (mios32_lcd_x >> 6) & 0x3;

    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J28_PinSet(cs, (cs == sel_cs) ? level_active : level_nonactive);
  }
#else
# warning "KS0108 CS pins not adapted for this MIOS32_FAMILY"
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets the CS line of a serial GLCDs depending on mios32_lcd_device
// if "all" flag is set, commands are sent to all segments
/////////////////////////////////////////////////////////////////////////////
static s32 APP_LCD_SERGLCD_CS_Set(u8 value, u8 all)
{
  int num_additional_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y - 8;
  if( num_additional_lcds >= (MAX_LCDS-8) )
    num_additional_lcds = (MAX_LCDS-8);

  // Note: assume that CS lines are low-active!
  if( all ) {
    MIOS32_BOARD_J15_DataSet(value ? 0x00 : 0xff);

    if( num_additional_lcds <= 4 ) {
      int i;
      for(i=0; i<num_additional_lcds; ++i)
	MIOS32_BOARD_J28_PinSet(i, value ? 0 : 1);
    } else {
      int num_shifts = num_additional_lcds / 8;
      if( num_additional_lcds % 8 )
	++num_shifts;

      // shift data
      int i;
      for(i=num_shifts-1; i>=0; --i) {
	MIOS32_BOARD_J28_SerDataShift(value ? 0x00 : 0xff);
      }

      // pulse RC (J28.WS)
      MIOS32_BOARD_J28_PinSet(2, 0);
      MIOS32_BOARD_J28_PinSet(2, 1);
    }
  } else {
    u32 mask = value ? ~(1 << mios32_lcd_device) : 0xffffffff;

    MIOS32_BOARD_J15_DataSet(mask);

    if( num_additional_lcds <= 4 ) {
      int i;
      for(i=0; i<num_additional_lcds; ++i)
	MIOS32_BOARD_J28_PinSet(i, (mask >> (8+i)) & 1);
    } else {
      int num_shifts = num_additional_lcds / 8;
      if( num_additional_lcds % 8 )
	++num_shifts;

      int selected_lcd = mios32_lcd_device - 8;
      int selected_lcd_sr = selected_lcd / 8;
      u8 selected_lcd_mask = value ? ~(1 << (selected_lcd % 8)) : 0xff;

      // shift data
      int i;
      for(i=num_shifts-1; i>=0; --i) {
	u8 data = (i == selected_lcd_sr) ? selected_lcd_mask : 0xff;
	MIOS32_BOARD_J28_SerDataShift(data);
      }

      // pulse RC (J28.WS)
      MIOS32_BOARD_J28_PinSet(2, 0);
      MIOS32_BOARD_J28_PinSet(2, 1);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets the E (enable) line depending on mios32_lcd_device
/////////////////////////////////////////////////////////////////////////////
static s32 APP_LCD_E_Set(u8 value)
{
  if( mios32_lcd_device < 2 ) {
    return MIOS32_BOARD_J15_E_Set(mios32_lcd_device, value);
  }

  int num_additional_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y - 2;
  if( num_additional_lcds < 0 )
    return -2; // E line not configured

  if( num_additional_lcds <= 4 ) {
    // the J28:SDA/SC/WS/RC are used as dedicated E pins
    MIOS32_BOARD_J28_PinSet(mios32_lcd_device - 2, value);
  } else {
    if( num_additional_lcds >= (MAX_LCDS-2) )
      num_additional_lcds = MAX_LCDS-2; // saturate
    int num_shifts = num_additional_lcds / 8;
    if( num_additional_lcds % 8 )
      ++num_shifts;

    int selected_lcd = mios32_lcd_device - 2;
    int selected_lcd_sr = selected_lcd / 8;
    u8 selected_lcd_mask = value ? (1 << (selected_lcd % 8)) : 0;

    // shift data
    int i;
    for(i=num_shifts-1; i>=0; --i) {
      u8 data = (i == selected_lcd_sr) ? selected_lcd_mask : 0;
      MIOS32_BOARD_J28_SerDataShift(data);
    }

    // pulse RC (J28.WS)
    MIOS32_BOARD_J28_PinSet(2, 0);
    MIOS32_BOARD_J28_PinSet(2, 1);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Polls for unbusy depending on mios32_lcd_device
/////////////////////////////////////////////////////////////////////////////
static s32 APP_LCD_PollUnbusy(u32 time_out)
{
  if( mios32_lcd_device < 2 ) {
    return MIOS32_BOARD_J15_PollUnbusy(mios32_lcd_device, time_out);
  }

  if( mios32_lcd_device >= MAX_LCDS )
    return -1; // LCD not supported

  u32 poll_ctr;
  u32 delay_ctr;

  // select command register (RS=0)
  MIOS32_BOARD_J15_RS_Set(0);

  // enable pull-up
  MIOS32_BOARD_J15_D7InPullUpEnable(1);

  // select read (will also disable output buffer of 74HC595)
  MIOS32_BOARD_J15_RW_Set(1);

  // check if E pin is available
  if( APP_LCD_E_Set(1) < 0 )
    return -1; // LCD port not available

  // poll busy flag, timeout after 10 mS
  // each loop takes ca. 4 uS @ 72MHz, this has to be considered when defining the time_out value
  for(poll_ctr=time_out; poll_ctr>0; --poll_ctr) {
    APP_LCD_E_Set(1);

    // due to slow slope we should wait at least for 1 uS
    for(delay_ctr=0; delay_ctr<10; ++delay_ctr)
      MIOS32_BOARD_J15_RW_Set(1);

    u32 busy = MIOS32_BOARD_J15_GetD7In();
    APP_LCD_E_Set(0);
    if( !busy )
      break;
  }

  // disable pull-up
  MIOS32_BOARD_J15_D7InPullUpEnable(0);

  // deselect read (output buffers of 74HC595 enabled again)
  MIOS32_BOARD_J15_RW_Set(0);

  // timeout?
  if( poll_ctr == 0 )
    return -2; // timeout error

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( mios32_lcd_device >= MAX_LCDS )
    return -2; // unsupported LCD device number

  // enable display by default
  display_available |= (1ULL << mios32_lcd_device);

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: {
    // 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
    if( MIOS32_BOARD_J15_PortInit(1) < 0 )
      return -2; // failed to initialize J15

    // configure CS pins
    // STM32: the 4 CS pins are available at J5C
    // LPC17: the 4 CS pins are available at J28
#if defined(MIOS32_FAMILY_STM32F10x)
    int cs;
    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J5_PinInit(cs + 8, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#elif defined(MIOS32_FAMILY_LPC17xx)
    int cs;
    for(cs=0; cs<4; ++cs)
      MIOS32_BOARD_J28_PinInit(cs, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#else
# warning "KS0108 CS pins not adapted for this MIOS32_FAMILY"
#endif

    // "Display On" command
    APP_LCD_Cmd(0x3e + 1);
  } break;

  case MIOS32_LCD_TYPE_GLCD_DOG: {
    // all OLEDs will be initialized at once by activating all CS lines!
    if( mios32_lcd_device == 0 ) {
      // DOGM128 works at 3.3V, level shifting (and open drain mode) not required
      if( MIOS32_BOARD_J15_PortInit(0) < 0 )
	return -2; // failed to initialize J15

      display_available |= 0xff;

      int num_additional_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y - 8;
      if( num_additional_lcds >= 4 )
	num_additional_lcds = 4;

      int i;
      for(i=0; i<num_additional_lcds; ++i) {
	MIOS32_BOARD_J28_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
	display_available |= (1 << (8+i));
      }

      // initialisation sequence based on EA-DOGL/M datasheet
  
      APP_LCD_Cmd(0x40); //2 - Display start line = 0
      APP_LCD_Cmd(0xA1); //8 - ADC Normal mode = 0 
      APP_LCD_Cmd(0xC0); //15 - COMS normal = 1/65  duty
      APP_LCD_Cmd(0xA6); //9 - Display  = normal  
      APP_LCD_Cmd(0xA2); //11 - 1/65 duty 1/9 bias for 65x132 display
      APP_LCD_Cmd(0x2F); //16  - Power control set = B.,R,F all ON
      APP_LCD_Cmd(0xF8); //20-1 - select Booster ratio set
      APP_LCD_Cmd(0x00); //20-2 - Booster ratio register (must be preceeded by 20-1)
      APP_LCD_Cmd(0x27); //17 - VO volt reg set 
      APP_LCD_Cmd(0x81); //18-1 - Elect vol control - contrast
#if 0
      APP_LCD_Cmd(0x16); //18-2 - Contrast level dec 22	
#else
      APP_LCD_Cmd(0x10); //18-2 - Contrast level dec 16
#endif
      APP_LCD_Cmd(0xAC); //19-1 - Static Indicator - set off
      APP_LCD_Cmd(0x00); //19-2 - No Indicator
      APP_LCD_Cmd(0xAF); //20 - Display ON
    }

  } break;

  case MIOS32_LCD_TYPE_GLCD_SSD1306: {
    // all OLEDs will be initialized at once by activating all CS lines!
    if( mios32_lcd_device == 0 ) {
      // the OLED works at 3.3V, level shifting (and open drain mode) not required
      if( MIOS32_BOARD_J15_PortInit(0) < 0 )
	return -2; // failed to initialize J15

      display_available |= 0xff;

      int num_additional_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y - 8;
      if( num_additional_lcds >= 4 )
	num_additional_lcds = 4;

      int i;
      for(i=0; i<num_additional_lcds; ++i) {
	MIOS32_BOARD_J28_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
	display_available |= (1 << (8+i));
      }


      // wait 500 mS to ensure that the reset is released
      for(i=0; i<500; ++i)
	MIOS32_DELAY_Wait_uS(1000);


      // initialize LCDs
      APP_LCD_Cmd(0xa8); // Set MUX Ratio
      APP_LCD_Cmd(0x3f);

      APP_LCD_Cmd(0xd3); // Set Display Offset
      APP_LCD_Cmd(0x00);

      APP_LCD_Cmd(0x40); // Set Display Start Line

      APP_LCD_Cmd(0xa0); // Set Segment re-map

      APP_LCD_Cmd(0xc0); // Set COM Output Scan Direction

      APP_LCD_Cmd(0xda); // Set COM Pins hardware configuration
      APP_LCD_Cmd(0x12);

      APP_LCD_Cmd(0x81); // Set Contrast Control
      APP_LCD_Cmd(0x7f); // middle

      APP_LCD_Cmd(0xa4); // Disable Entiere Display On

      APP_LCD_Cmd(0xa6); // Set Normal Display

      APP_LCD_Cmd(0xd5); // Set OSC Frequency
      APP_LCD_Cmd(0x80);

      APP_LCD_Cmd(0x8d); // Enable charge pump regulator
      APP_LCD_Cmd(0x14);

      APP_LCD_Cmd(0xaf); // Display On

      APP_LCD_Cmd(0x20); // Enable Page mode
      APP_LCD_Cmd(0x02);
    }
  } break;

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default: {
    // 0: J15 pins are configured in Push Pull Mode (3.3V)
    // 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
    if( mios32_lcd_parameters.lcd_type == MIOS32_LCD_TYPE_CLCD_DOG ) {
      // DOG CLCD works at 3.3V, level shifting (and open drain mode) not required
      if( MIOS32_BOARD_J15_PortInit(0) < 0 )
	return -2; // failed to initialize J15
    } else {
      if( MIOS32_BOARD_J15_PortInit(1) < 0 )
	return -2; // failed to initialize J15
    }

    // init extension port J28?
    int num_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y;
    if( num_lcds >= 2 ) {
      if( num_lcds <= 6 ) {
	// the J28:SDA/SC/WS/RC are used as dedicated E pins
	int e;
	for(e=0; e<4; ++e)
	  MIOS32_BOARD_J28_PinInit(e, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
      } else {
	MIOS32_BOARD_J28_PinInit(0, MIOS32_BOARD_PIN_MODE_OUTPUT_PP); // J28.SDA (used to shift out data)
	MIOS32_BOARD_J28_PinInit(1, MIOS32_BOARD_PIN_MODE_OUTPUT_PP); // J28.SC (used as clock)
	MIOS32_BOARD_J28_PinInit(2, MIOS32_BOARD_PIN_MODE_OUTPUT_PP); // J28.WS (used as strobe)
	MIOS32_BOARD_J28_PinSet(2, 1); // RC set to 1 by default
      }
    }

    // initialize LCD
    MIOS32_BOARD_J15_DataSet(0x38);
    MIOS32_BOARD_J15_RS_Set(0);
    MIOS32_BOARD_J15_RW_Set(0);
    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);
    MIOS32_DELAY_Wait_uS(5000); // according to the hitachi datasheet, this command takes 37 uS - take 1 mS to be at the secure side

    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);
    MIOS32_DELAY_Wait_uS(500); // and now only 500 uS anymore

    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);
    MIOS32_DELAY_Wait_uS(500);

    APP_LCD_Cmd(0x08); // Display Off

    // display still available?
    // if not, we can already break here!
    if( !(display_available & (1ULL << mios32_lcd_device)) )
      return -1; // display not available

    APP_LCD_Cmd(0x0c); // Display On
    APP_LCD_Cmd(0x06); // Entry Mode
    APP_LCD_Cmd(0x01); // Clear Display

    // for DOG displays: perform additional display initialisation
    if( mios32_lcd_parameters.lcd_type == MIOS32_LCD_TYPE_CLCD_DOG ) {
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
    }

    APP_LCD_Cmd(0x38); // experience from PIC based MIOS: without these lines
    APP_LCD_Cmd(0x0c); // the LCD won't work correctly after a second APP_LCD_Init
  }
  }

  return (display_available & (1ULL << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  // check if if display already has been disabled
  if( !(display_available & (1ULL << mios32_lcd_device)) )
    return -1;

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: {

    // due to historical reasons currently only one device provided, it's spreaded over multiple CS lines
    if( mios32_lcd_device != 0 )
      return -1;

    // abort if max. width reached
    if( mios32_lcd_x >= mios32_lcd_parameters.width )
      return -1;

    // determine chip select line(s)
    APP_LCD_KS0108_CS_Set(0); // select display depending on current X position

    // wait until LCD unbusy, exit on error (timeout)
    if( APP_LCD_PollUnbusy(2500) < 0 ) {
      // disable display
      display_available &= ~(1ULL << mios32_lcd_device);
      return -2; // timeout
    }

    // write data
    MIOS32_BOARD_J15_DataSet(data);
    MIOS32_BOARD_J15_RS_Set(1);
    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);

    // increment graphical cursor
    // if end of display segment reached: set X position of all segments to 0
    if( (++mios32_lcd_x & 0x3f) == 0x00 )
      return APP_LCD_Cmd(0x40);

    return 0; // no error
  } break;

  case MIOS32_LCD_TYPE_GLCD_DOG: {
    // chip select and DC
    APP_LCD_SERGLCD_CS_Set(1, 0);
    MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control A0

    // send data
    MIOS32_BOARD_J15_SerDataShift(data);

    // increment graphical cursor
    ++mios32_lcd_x;

    // if end of display segment reached: set X position of all segments to 0
    if( (mios32_lcd_x % mios32_lcd_parameters.width) == 0 ) {
      APP_LCD_Cmd(0x10); // Set upper nibble to 0
      return APP_LCD_Cmd(0x00); // Set lower nibble to 0
    }

    return 0; // no error
  } break;

  case MIOS32_LCD_TYPE_GLCD_SSD1306: {
    // chip select and DC
    APP_LCD_SERGLCD_CS_Set(1, 0);
    MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

    // send data
    MIOS32_BOARD_J15_SerDataShift(data);

    // increment graphical cursor
    ++mios32_lcd_x;

    // if end of display segment reached: set X position of all segments to 0
    if( (mios32_lcd_x % mios32_lcd_parameters.width) == 0 ) {
      APP_LCD_Cmd(0x00); // set X=0
      APP_LCD_Cmd(0x10);
    }

    return 0; // no error
  } break;

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default: {
    // wait until LCD unbusy, exit on error (timeout)
    if( APP_LCD_PollUnbusy(2500) < 0 ) {
      // disable display
      display_available &= ~(1ULL << mios32_lcd_device);
      return -2; // timeout
    }

    // write data
    MIOS32_BOARD_J15_DataSet(data);
    MIOS32_BOARD_J15_RS_Set(1);
    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);

    return 0; // no error
  }
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  // check if if display already has been disabled
  if( !(display_available & (1ULL << mios32_lcd_device)) )
    return -1;

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: {

    // due to historical reasons currently only one device provided, it's spreaded over multiple CS lines
    if( mios32_lcd_device != 0 )
      return -1;

    // determine chip select line(s)
    APP_LCD_KS0108_CS_Set(0); // select display depending on current X position

    // wait until LCD unbusy, exit on error (timeout)
    if( APP_LCD_PollUnbusy(2500) < 0 ) {
      // disable display
      display_available &= ~(1ULL << mios32_lcd_device);
      return -2; // timeout
    }

    // select all displays
    APP_LCD_KS0108_CS_Set(1);

    // write command
    MIOS32_BOARD_J15_DataSet(cmd);
    MIOS32_BOARD_J15_RS_Set(0);
    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);

    return 0; // no error
  } break;

  case MIOS32_LCD_TYPE_GLCD_DOG: {
    // select all LCDs
    APP_LCD_SERGLCD_CS_Set(1, 1);
    MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control A0

    // send command
    MIOS32_BOARD_J15_SerDataShift(cmd);

    return 0; // no error
  } break;

  case MIOS32_LCD_TYPE_GLCD_SSD1306: {
    // select all LCDs
    APP_LCD_SERGLCD_CS_Set(1, 1);
    MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

    MIOS32_BOARD_J15_SerDataShift(cmd);
  } break;

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default: {
    // wait until LCD unbusy, exit on error (timeout)
    if( APP_LCD_PollUnbusy(2500) < 0 ) {
      // disable display
      display_available &= ~(1ULL << mios32_lcd_device);
      return -2; // timeout
    }

    // write command
    MIOS32_BOARD_J15_DataSet(cmd);
    MIOS32_BOARD_J15_RS_Set(0);
    APP_LCD_E_Set(1);
    APP_LCD_E_Set(0);

    return 0; // no error
  }
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: {
    s32 error = 0;
    int x, y;

    // use default font
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

    for(y=0; y<(mios32_lcd_parameters.height/8); ++y) {
      error |= MIOS32_LCD_CursorSet(0, y);
      for(x=0; x<mios32_lcd_parameters.width; ++x)
	error |= APP_LCD_Data(0x00);
    }

    // set Y0=0
    error |= APP_LCD_Cmd(0xc0 + 0);

    // set X=0, Y=0
    error |= MIOS32_LCD_CursorSet(0, 0);

    return error;
  } break;

  case MIOS32_LCD_TYPE_GLCD_DOG: {
    s32 error = 0;
    u8 x, y;

    // use default font
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

    // send data
    for(y=0; y<(mios32_lcd_parameters.height/8); ++y) {
      error |= MIOS32_LCD_CursorSet(0, y);

      // select all LCDs
      APP_LCD_SERGLCD_CS_Set(1, 1);
      MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

      for(x=0; x<mios32_lcd_parameters.width; ++x)
	MIOS32_BOARD_J15_SerDataShift(0x00);
    }

    // set X=0, Y=0
    error |= MIOS32_LCD_CursorSet(0, 0);

    return error;
  } break;

  case MIOS32_LCD_TYPE_GLCD_SSD1306: {
    s32 error = 0;
    u8 x, y;

    // use default font
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

    // send data
    for(y=0; y<mios32_lcd_parameters.height/8; ++y) {
      error |= MIOS32_LCD_CursorSet(0, y);

      // select all LCDs
      APP_LCD_SERGLCD_CS_Set(1, 1);
      MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

      for(x=0; x<mios32_lcd_parameters.width; ++x)
	MIOS32_BOARD_J15_SerDataShift(0x00);
    }

    // set X=0, Y=0
    error |= MIOS32_LCD_CursorSet(0, 0);

    return error;
  } break;

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default:
    // -> send clear command
    return APP_LCD_Cmd(0x01);
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS:
  case MIOS32_LCD_TYPE_GLCD_DOG:
  case MIOS32_LCD_TYPE_GLCD_SSD1306:
    // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
    return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default: {
    // exit with error if line is not in allowed range
    if( line >= MIOS32_LCD_MAX_MAP_LINES )
      return -1;

    // -> set cursor address
    return APP_LCD_Cmd(0x80 | (mios32_lcd_cursor_map[line] + column));
  }
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: {
    s32 error = 0;

    // set X position
    error |= APP_LCD_Cmd(0x40 | (x & 0x3f));

    // set Y position
    error |= APP_LCD_Cmd(0xb8 | ((y>>3) & 0x7));

    return error;
  } break;

  case MIOS32_LCD_TYPE_GLCD_DOG: {
    s32 error = 0;
  
    // set X position
    error |= APP_LCD_Cmd(0x10 | (((x % mios32_lcd_parameters.width) >> 4) & 0x0f));   // First send MSB nibble
    error |= APP_LCD_Cmd(0x00 | ((x % mios32_lcd_parameters.width) & 0x0f)); // Then send LSB nibble

    // set Y position
    error |= APP_LCD_Cmd(0xb0 | ((y>>3) % (mios32_lcd_parameters.height/8)));

    return error;
  } break;

  case MIOS32_LCD_TYPE_GLCD_SSD1306: {
    s32 error = 0;

    // set X position
    error |= APP_LCD_Cmd(0x00 | (x & 0xf));
    error |= APP_LCD_Cmd(0x10 | ((x>>4) & 0xf));

    // set Y position
    error |= APP_LCD_Cmd(0xb0 | ((y>>3) & 7));

    return error;
  } break;
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  switch( mios32_lcd_parameters.lcd_type ) {
  case MIOS32_LCD_TYPE_GLCD_KS0108:
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS:
  case MIOS32_LCD_TYPE_GLCD_DOG:
  case MIOS32_LCD_TYPE_GLCD_SSD1306:
    // TODO
    break;

  case MIOS32_LCD_TYPE_CLCD:
  case MIOS32_LCD_TYPE_CLCD_DOG:
  default: {
    s32 i;

    // send character number
    APP_LCD_Cmd(((num&7)<<3) | 0x40);

    // send 8 data bytes
    for(i=0; i<8; ++i)
      if( APP_LCD_Data(table[i]) < 0 )
	return -1; // error during sending character

    // set cursor to original position
    return APP_LCD_CursorSet(mios32_lcd_column, mios32_lcd_line);
  }
  }

  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb)
{
  return -3; // not supported
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

  // all GLCDs support the same bitmap scrambling
  u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
  u8 mask = 1 << (y % 8);

  *pixel &= ~mask;
  if( colour )
    *pixel |= mask;

  return -3; // not supported
}



/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  if( lcd_testmode )
    return -1; // direct access disabled in testmode

  if( !MIOS32_LCD_TypeIsGLCD() )
    return -1; // no GLCD

  // all GLCDs support the same bitmap scrambling
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

    // transfer character
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
// Terminal Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  testlcdpin:     type this command to get further informations about this testmode.");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  int input_len = strlen(input);

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcasecmp(parameter, "testlcdpin") == 0 ) {
      char *arg;
      int pin_number = -1;
      int level = -1;
      
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcasecmp(arg, "rs") == 0 )
	  pin_number = 1;
	else if( strcasecmp(arg, "e1") == 0 )
	  pin_number = 2;
	else if( strcasecmp(arg, "e2") == 0 )
	  pin_number = 3;
	else if( strcasecmp(arg, "rw") == 0 )
	  pin_number = 4;
	else if( strcasecmp(arg, "d0") == 0 )
	  pin_number = 5;
	else if( strcasecmp(arg, "d1") == 0 )
	  pin_number = 6;
	else if( strcasecmp(arg, "d2") == 0 )
	  pin_number = 7;
	else if( strcasecmp(arg, "d3") == 0 )
	  pin_number = 8;
	else if( strcasecmp(arg, "d4") == 0 )
	  pin_number = 9;
	else if( strcasecmp(arg, "d5") == 0 )
	  pin_number = 10;
	else if( strcasecmp(arg, "d6") == 0 )
	  pin_number = 11;
	else if( strcasecmp(arg, "d7") == 0 )
	  pin_number = 12;
	else if( strcasecmp(arg, "reset") == 0 ) {
	  pin_number = 0;
	  level = 0; // dummy
	}
      }
      
      if( pin_number < 0 ) {
	out("Please specifiy valid LCD pin name: rs, e1, e2, rw, d0, d1, ... d7\n");
      }

      if( (arg = strtok_r(NULL, separators, &brkt)) )
	level = get_dec(arg);
	
      if( level != 0 && level != 1 ) {
	out("Please specifiy valid logic level for LCD pin: 0 or 1\n");
      }

      if( pin_number >= 0 && level >= 0 ) {
	lcd_testmode = 1;

	// clear all pins
	MIOS32_BOARD_J15_PortInit(0);
	
	switch( pin_number ) {
	case 1: {
	  MIOS32_BOARD_J15_RS_Set(level);
	  out("J15A.RS and J15B.RS set to ca. %s", level ? "3.3V" : "0V");
	} break;

	case 2: {
	  MIOS32_BOARD_J15_E_Set(0, level);
	  out("J15A.E set to ca. %s", level ? "3.3V" : "0V");
	} break;

	case 3: {
	  MIOS32_BOARD_J15_E_Set(1, level);
	  out("J15B.E set to ca. %s", level ? "3.3V" : "0V");
	} break;

	case 4: {
	  MIOS32_BOARD_J15_RW_Set(level);
	  out("J15A.RW and J15B.RW set to ca. %s", level ? "3.3V" : "0V");
	} break;

	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12: {
	  u8 d_pin = pin_number-5;
	  MIOS32_BOARD_J15_DataSet(level ? (1 << d_pin) : 0x00);
	  out("J15A.D%d and J15B.D%d set to ca. %s", d_pin, d_pin, level ? "5V (resp. 3.3V)" : "0V");

	  int d7_in = MIOS32_BOARD_J15_GetD7In();
	  if( d7_in < 0 ) {
	    out("ERROR: LCD driver not enabled?!?");
	  } else if( d_pin == 7 && level == 1 && d7_in == 1 ) {
	    out("D7 input pin correctly shows logic-1");
	  } else if( d7_in == 0 ) {
	    out("D7 input pin correctly shows logic-0");
	  } else {
	    out("ERROR: D7 input pin shows unexpected logic-%d level!", d7_in);
	    out("Please check the D7 feedback line on your core module!");
	  }

	} break;

	default:
	  lcd_testmode = 0;
	  u8 prev_dev = mios32_lcd_device;

	  mios32_lcd_device = 0;
	  APP_LCD_Init(0);
	  MIOS32_LCD_PrintString("READY.");
	  mios32_lcd_device = 1;
	  APP_LCD_Init(0);
	  MIOS32_LCD_PrintString("READY.");

	  mios32_lcd_device = prev_dev;

	  out("LCD Testmode is disabled now.");
	  out("It makes sense to reset the application, so that the LCD screen will be re-initialized!");
	}

      } else {
	out("Following commands are supported:\n");
	out("testlcdpin rs 0  -> sets J15(AB):RS to ca. 0V");
	out("testlcdpin rs 1  -> sets J15(AB):RS to ca. 3.3V");
	out("testlcdpin e1 0  -> sets J15A:E to ca. 0V");
	out("testlcdpin e1 1  -> sets J15A:E to ca. 3.3V");
	out("testlcdpin e2 0  -> sets J15B:E to ca. 0V");
	out("testlcdpin e2 1  -> sets J15B:E to ca. 3.3V");
	out("testlcdpin rw 0  -> sets J15(AB):RW to ca. 0V");
	out("testlcdpin rw 1  -> sets J15(AB):RW to ca. 3.3V");
	out("testlcdpin d0 0  -> sets J15(AB):D0 to ca. 0V");
	out("testlcdpin d0 1  -> sets J15(AB):D0 to ca. 5V (resp. 3.3V)");
	out("testlcdpin d1 0  -> sets J15(AB):D1 to ca. 0V");
	out("testlcdpin d1 1  -> sets J15(AB):D1 to ca. 5V (resp. 3.3V)");
	out("testlcdpin d...  -> same for J15(AB):D2, D3, D4, D5, D6, D7");
	out("testlcdpin reset -> re-initializes LCD modules so that they can be used again.");
	out("The testmode is currently %s", lcd_testmode ? "enabled" : "disabled");
      }
      return 1; // command taken
    }
  }

  // restore input line (replace NUL characters by spaces)
  int i;
  char *input_ptr = input;
  for(i=0; i<input_len; ++i, ++input_ptr)
    if( !*input_ptr )
      *input_ptr = ' ';

  return 0; // command not taken
}


/////////////////////////////////////////////////////////////////////////////
// Prints the current configuration
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalPrintConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("lcd_type: 0x%02x - %s\n",
      mios32_lcd_parameters.lcd_type,
      MIOS32_LCD_LcdTypeName(mios32_lcd_parameters.lcd_type)
      ? MIOS32_LCD_LcdTypeName(mios32_lcd_parameters.lcd_type)
      : "unknown");
  out("lcd_num_x: %d\n", mios32_lcd_parameters.num_x);
  out("lcd_num_y: %d\n", mios32_lcd_parameters.num_y);
  out("lcd_width: %d\n", mios32_lcd_parameters.width);
  out("lcd_height: %d\n", mios32_lcd_parameters.height);

  return 0; // no error
}
