// $Id: seq_ui_pmute.c 607 2009-06-19 21:43:41Z tk $
/*
 * MIDI Monitor Page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_midi_port.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 midi_activity;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( seq_ui_button_state.SELECT_PRESSED ) {
    seq_midi_port_mon_filter_t mon_filter = SEQ_MIDI_PORT_MonFilterGet();
    if( mon_filter.MIDI_CLOCK )
      *gp_leds |= 0x000f;
    if( mon_filter.ACTIVE_SENSE )
      *gp_leds |= 0x00f0;

  } else {
    // updated by LCD handler
    *gp_leds = midi_activity;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  if( seq_ui_button_state.SELECT_PRESSED ) {
    seq_midi_port_mon_filter_t mon_filter = SEQ_MIDI_PORT_MonFilterGet();

    switch( encoder ) {
      case SEQ_UI_ENCODER_GP1:
      case SEQ_UI_ENCODER_GP2:
      case SEQ_UI_ENCODER_GP3:
      case SEQ_UI_ENCODER_GP4: {
	u8 value = mon_filter.MIDI_CLOCK;
	if( !incrementer ) // button should toggle the flag
	  incrementer = mon_filter.MIDI_CLOCK ? -1 : 1;
	if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) ) {
	  mon_filter.MIDI_CLOCK = value;
	  SEQ_MIDI_PORT_MonFilterSet(mon_filter);
	  return 1;
	}
	return 0;
      }

      case SEQ_UI_ENCODER_GP5:
      case SEQ_UI_ENCODER_GP6:
      case SEQ_UI_ENCODER_GP7:
      case SEQ_UI_ENCODER_GP8: {
	u8 value = mon_filter.ACTIVE_SENSE;
	if( !incrementer ) // button should toggle the flag
	  incrementer = mon_filter.ACTIVE_SENSE ? -1 : 1;
	if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) ) {
	  mon_filter.ACTIVE_SENSE = value;
	  SEQ_MIDI_PORT_MonFilterSet(mon_filter);
	  return 1;
	}
	return 0;
      }
	
    }
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed )
    return 0;

  if( button <= SEQ_UI_BUTTON_GP16 )
    return Encoder_Handler((seq_ui_encoder_t)button, 0);

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // USB1 USB2 USB3 USB4 IN1  IN2  IN3  IN4                           Bus1 Bus2 Bus3 
  // USB1 USB2 USB3 USB4 OUT1 OUT2 OUT3 OUT4 IIC1 IIC2 IIC3 IIC4 AOUT Bus1 Bus2 Bus3 


  //   Filter               Filter
  // MIDI Clock: on      Active Sense: on


  if( seq_ui_button_state.SELECT_PRESSED ) {
    if( high_prio )
      return 0;

    seq_midi_port_mon_filter_t mon_filter = SEQ_MIDI_PORT_MonFilterGet();

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintSpaces(2);
    SEQ_LCD_PrintString("Filter");
    SEQ_LCD_PrintSpaces(15);
    SEQ_LCD_PrintString("Filter");
    SEQ_LCD_PrintSpaces(11 + 40);


    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintFormattedString("MIDI Clock: %s", mon_filter.MIDI_CLOCK ? "on " : "off");
    SEQ_LCD_PrintSpaces(5);
    SEQ_LCD_PrintFormattedString("Active Sense: %s", mon_filter.ACTIVE_SENSE ? "on " : "off");
    SEQ_LCD_PrintSpaces(3 + 40);
  }

  ///////////////////////////////////////////////////////////////////////////
  u8 port_num = SEQ_MIDI_PORT_OutNumGet();
  if( port_num > 17 )
    port_num = 17;

  u8 port_ix;

  if( high_prio ) {
    u16 in_activity = 0;
    u16 out_activity = 0;

    for(port_ix=1; port_ix<port_num; ++port_ix) {

      SEQ_LCD_CursorSet(5*(port_ix-1), 0);
      // dirty! but only way to ensure that Bus1..3 are aligned
      if( port_ix >= 9 && port_ix <= 13 ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 port_ix_midi_in = (port_ix >= 9) ? (port_ix - 5) : port_ix;

	mios32_midi_port_t port = SEQ_MIDI_PORT_InPortGet(port_ix_midi_in);
	mios32_midi_package_t package = SEQ_MIDI_PORT_InPackageGet(port);
	if( port == 0xff ) {
	  SEQ_LCD_PrintSpaces(5);
	} else if( package.type ) {
	  SEQ_LCD_PrintEvent(package, 5);
	  in_activity |= (1 << (port_ix-1));
	} else {
	  SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(port_ix_midi_in));
	  SEQ_LCD_PrintChar(' ');
	}
      }

      SEQ_LCD_CursorSet(5*(port_ix-1), 1);
      mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(port_ix);
      mios32_midi_package_t package = SEQ_MIDI_PORT_OutPackageGet(port);
      if( port == 0xff ) {
	SEQ_LCD_PrintSpaces(5);
      } else if( package.type ) {
	SEQ_LCD_PrintEvent(package, 5);
	out_activity |= (1 << (port_ix-1));
      } else {
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(port_ix));
	SEQ_LCD_PrintChar(' ');
      }
    }

    midi_activity = in_activity | out_activity;

    return 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIDIMON_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
