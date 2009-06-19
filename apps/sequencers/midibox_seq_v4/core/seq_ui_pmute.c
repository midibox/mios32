// $Id$
/*
 * Port Mute Page
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

#include "seq_midi_router.h"
#include "seq_midi_port.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 latched_mute;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  u8 port_num = SEQ_MIDI_PORT_OutNumGet();
  if( port_num > 16 )
    port_num = 16;

  u8 port_ix;

  if( !ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED ) {
    *gp_leds = latched_mute;
  } else {
    for(port_ix=0; port_ix<port_num; ++port_ix) {
      mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(port_ix);
      if( SEQ_MIDI_PORT_OutMuteGet(port) )
	*gp_leds |= (1 << port_ix);
    }
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    if( seq_ui_button_state.SELECT_PRESSED ) {
      // select button pressed: indirect MUTED flag modification (taken over when select button depressed)
      u16 mask = 1 << encoder;
      if( incrementer > 0 || (incrementer == 0 && !(latched_mute & mask)) )
	latched_mute |= mask;
      else
	latched_mute &= ~mask;
    } else {
      // select button not pressed: direct MUTED flag modification
      u8 port_num = SEQ_MIDI_PORT_OutNumGet();
      u8 port_ix = encoder;

      if( encoder >= port_num )
	return -1;

      mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(port_ix);

      if( incrementer == 0 )
	SEQ_MIDI_PORT_OutMuteSet(port, SEQ_MIDI_PORT_OutMuteGet(port) ? 0 : 1);
      else
	SEQ_MIDI_PORT_OutMuteSet(port, (incrementer > 0) ? 1 : 0);
    }

    return 1; // value changed
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    if( depressed ) return 0; // ignore when button depressed

    // re-using encoder routine
    return Encoder_Handler(button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
      portENTER_CRITICAL();
      if( depressed ) {
	// select button released: take over latched mutes
	u8 port_num = SEQ_MIDI_PORT_OutNumGet();
	u8 port_ix;
	for(port_ix=0; port_ix<port_num; ++port_ix) {
	  mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  SEQ_MIDI_PORT_OutMuteSet(port, (latched_mute & (1 << port_ix)) ? 1 : 0);
	}
      } else {
	// select pressed: init latched mutes which will be taken over once SELECT button released
	latched_mute = 0;
	u8 port_num = SEQ_MIDI_PORT_OutNumGet();
	u8 port_ix;
	for(port_ix=0; port_ix<port_num; ++port_ix) {
	  mios32_midi_port_t port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  if( SEQ_MIDI_PORT_OutMuteGet(port) )
	    latched_mute |= (1 << port_ix);
	}
      }

      portEXIT_CRITICAL();
      return 1;
  }

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
  // Def. USB1 USB2 USB3 USB4 OUT1 OUT2 OUT3 OUT4 IIC1 IIC2 IIC3 IIC4 AOUT Bus1 Bus2 
  //   o    o    o    o    o    o    o    o    o    o    o    o    o    o    o    o  

  ///////////////////////////////////////////////////////////////////////////
  u8 port_num = SEQ_MIDI_PORT_OutNumGet();
  if( port_num > 16 )
    port_num = 16;

  u8 port_ix;

  for(port_ix=0; port_ix<port_num; ++port_ix) {
    SEQ_LCD_CursorSet(5*port_ix, 0);
    if( ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED )
      SEQ_LCD_PrintSpaces(5);
    else
      SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(port_ix));
    SEQ_LCD_PrintChar(' ');

    SEQ_LCD_CursorSet(5*port_ix, 1);
    SEQ_LCD_PrintSpaces(2);
    if( seq_ui_button_state.SELECT_PRESSED )
      SEQ_LCD_PrintChar((latched_mute & (1 << port_ix)) ? '*' : 'o');
    else
      SEQ_LCD_PrintChar(SEQ_MIDI_PORT_OutMuteGet(SEQ_MIDI_PORT_OutPortGet(port_ix)) ? '*' : 'o');
    SEQ_LCD_PrintSpaces(2);
  }

  if( port_num < 16 ) {
    u8 blank_slots = port_num-port_ix;
    SEQ_LCD_CursorSet(5*port_ix, 0);
    SEQ_LCD_PrintSpaces(5*blank_slots);
    SEQ_LCD_CursorSet(5*port_ix, 1);
    SEQ_LCD_PrintSpaces(5*blank_slots);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PMUTE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  latched_mute = 0;

  return 0; // no error
}
