// $Id$
/*
 * MIDIbox LC V2
 * Meter Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "lc_hwcfg.h"
#include "lc_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// meter variables which are sent from the host
static u8 meter_global_mode;
static u8 meter_mode[8];
static u8 meter_level[8];

// meter counters
static u8 meter_counter[8];

// the meter patterns
static u16 meter_pattern[8]; // 16 bit words for 8 meters -- global variable, used also be LC_VPOT_LEDRing_SRHandler
static u8 meter_update_req; // 8 update request flags

// patterns used for LEDrings/Meters
// note that the 12th LED (the center LED below the encoder) is set by
// LC_METERS_CheckUpdates seperately if the V-Pot pointer hasn't been received from host
static const u16 meter_patterns[16] = {
  0x0000, //   b'0000000000000000'
  0x0001, //   b'0000000000000001'
  0x0003, //   b'0000000000000011'
  0x0007, //   b'0000000000000111'
  0x000f, //   b'0000000000001111'
  0x001f, //   b'0000000000011111'
  0x003f, //   b'0000000000111111'
  0x007f, //   b'0000000001111111'
  0x00ff, //   b'0000000011111111'
  0x01ff, //   b'0000000111111111'
  0x03ff, //   b'0000001111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
};

/////////////////////////////////////////////////////////////////////////////
// This function initializes the meters handler
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_Init(u32 mode)
{
  int i;

  // clear variables
  meter_global_mode = 0;
  for(i=0; i<sizeof(meter_mode); ++i) {
    meter_mode[i] = 0;
    meter_level[i] = 0;
    meter_counter[i] = 0;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the global meter mode
/////////////////////////////////////////////////////////////////////////////
u8 LC_METERS_GlobalModeGet(void)
{
  return meter_global_mode;
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the global meter mode
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_GlobalModeSet(u8 mode)
{
  meter_global_mode = mode;
  meter_update_req = 0xff; // request update of all meters

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the meter mode
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_ModeGet(u8 meter)
{
  return meter_mode[meter];
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the meter mode
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_ModeSet(u8 meter, u8 mode)
{
  meter_mode[meter] = mode;
  MIOS32_IRQ_Disable();
  meter_update_req |= (1 << meter); // request update of meter
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the meter level
/////////////////////////////////////////////////////////////////////////////
u8 LC_METERS_LevelGet(u8 meter)
{
  return meter_level[meter];
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the meter level
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_LevelSet(u8 meter, u8 level)
{
  // coding:
  // meter: channel to be address (0 thru 7)
  // level: lever meter:
  //       0 thru C: level meter 0%..100% (overload not cleared!)
  //       E       : set overload
  //       F       : clear overload

  // our own coding in METER_LEVEL[meter_number] array:
  // O000llll
  // O: overload flag
  // llll: 1 thru D - lever meter 0%..100% (+1)

  MIOS32_IRQ_Disable();

  if( level == 0x0e ) {         // set overload flag
    // meter_level[meter] |= 0x80;
    meter_level[meter] = 0x8d;  // set overload flag and set level to highest value (seems to be better with Logic Audio)
  } else if (level == 0x0f ) {  // clear overload flag
    meter_level[meter] &= 0x7f;
  } else {                      // set value, dont touch overload flag
    meter_level[meter] = (meter_level[meter] & 0x80) | ((level & 0x0f) + 1);
  }

  meter_update_req |= (1 << meter); // request update of meter

  LC_LCD_Update_Meter(meter); // request update on LCD

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from Timer() in main.c every 20 ms (see Init())
//
// handles the meter counters. For the best show effect we don't decrement
// all meter levels at once every 300 ms, but seperately
// 	
// every single counter will be reloaded by lc_mproc.inc::LC_MPROC_Received_D0
// to 15 (== 300 ms / 20 ms), the levels will be decremented when this
// value reaches zero
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_Timer(void)
{
  u8 meter;
  u8 level;

  for(meter=0; meter<8; ++meter) {
    
    // decrement counter, continue once counter is zero
    if( !--meter_counter[meter] ) {
      meter_counter[meter] = 15; // preload counter again

      // decrement meter level if != 0
      level = meter_level[meter];
      if( level & 0x7f ) {
	meter_level[meter] = (level & 0x80) | ((level & 0x7f) - 1);
	MIOS32_IRQ_Disable();
	meter_update_req |= (1 << meter); // request update
	MIOS32_IRQ_Enable();
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from Tick() in main.c
// it updates the meters patterns for which an update has been requested
/////////////////////////////////////////////////////////////////////////////
s32 LC_METERS_CheckUpdates(void)
{
  u8 meter;
  u8 meter_select;
  u8 center_led;
  u8 level;

  for(meter=0, meter_select=0x01; meter<8; ++meter) {

    // check if update has been requested
    if( meter_update_req & meter_select ) {
      MIOS32_IRQ_Disable();
      meter_update_req &= ~meter_select; // clear request flag
      MIOS32_IRQ_Enable();

      // set LED pattern
      level = meter_level[meter];
      center_led = (level & (1 << 7)) ? 1 : 0;
      meter_pattern[meter] = meter_patterns[level & 0x0f] | (center_led ? (1 << (12-1)) : 0);
    }

    meter_select <<= 1; // shift left the select bit
  }

  return 0; // no error
}

// note: the meter patterns are output within LC_VPOT_LEDRing_SRHandler() !!!
// meter_pattern[] is a global variable

