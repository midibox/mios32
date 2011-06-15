// $Id$
/*
 * MIDI Force-To-Scale
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
#include "app.h"

#include "scale.h"

/////////////////////////////////////////////////////////////////////////////
// Local Defines
/////////////////////////////////////////////////////////////////////////////
// if 0: any MIDI channel will be converted
// if 1..16: a selected MIDI channel will be converted
#define DEFAULT_MIDI_CHANNEL 0


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////
static u8 selected_scale;
static u8 selected_root;
static volatile u8 display_update;

static u8 last_played_note[128];


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // init last played note array
  int i;
  for(i=0; i<128; ++i)
    last_played_note[i] = 0x80;

  // initial scale/root key
  SCALE_Init(0);
  selected_scale = 2; // Harmonic Minor
  selected_root = 0; // C

  // request initial LCD update
  display_update = 1;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  const char root_name[12*2] = "C C#D D#E F F#G G#A A#B ";

  // init LCD
  MIOS32_LCD_Clear();

  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( display_update ) {
      display_update = 0;

      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("Root: ");
      char *selected_root_name = (char *)&root_name[2*selected_root];
      MIOS32_LCD_PrintChar(*selected_root_name++);
      MIOS32_LCD_PrintChar(*selected_root_name);

      MIOS32_LCD_CursorSet(0, 1);
      if( selected_scale == 0 ) {
	MIOS32_LCD_PrintString("No Scale            ");
      } else {
	MIOS32_LCD_PrintString(SCALE_NameGet(selected_scale-1));
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // Note On?
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
#if DEFAULT_MIDI_CHANNEL
    if( midi_package.chn != (DEFAULT_MIDI_CHANNEL-1) )
      return;
#endif

    // using the played note as index for "last played" array
    u8 note_ix = midi_package.note;

    // get scaled note
    if( selected_scale )
      SCALE_Note(&midi_package, selected_scale-1, selected_root);

    // same note already played from another key? then send note off and play it again
    int i;
    u8 *last_played_note_ptr = (u8 *)&last_played_note[0];
    for(i=0; i<128; ++i, ++last_played_note_ptr)
      if( *last_played_note_ptr == midi_package.note ) {
	mios32_midi_package_t p = midi_package;
	p.velocity = 0;

	// forward Note Off event to MIDI ports
	MIOS32_MIDI_SendPackage(USB0, p);
	MIOS32_MIDI_SendPackage(UART0, p);
	MIOS32_MIDI_SendPackage(UART1, p);

	*last_played_note_ptr = 0x80;
      }

    // store note
    last_played_note[note_ix] = midi_package.note;

    // forward Note On event to MIDI ports
    MIOS32_MIDI_SendPackage(USB0, midi_package);
    MIOS32_MIDI_SendPackage(UART0, midi_package);
    MIOS32_MIDI_SendPackage(UART1, midi_package);
    return;
  }

  // Note Off?
  if( midi_package.type == NoteOff ||
      (midi_package.type == NoteOn && midi_package.velocity == 0) ) {
#if DEFAULT_MIDI_CHANNEL
    if( midi_package.chn != (DEFAULT_MIDI_CHANNEL-1) )
      return;
#endif

    // using the played note as index for "last played" array
    u8 note_ix = midi_package.note;

    // map to last played note
    if( last_played_note[note_ix] < 0x80 ) {
      midi_package.note = last_played_note[note_ix];
      last_played_note[note_ix] = 0x80;

      // forward to MIDI ports
      MIOS32_MIDI_SendPackage(USB0, midi_package);
      MIOS32_MIDI_SendPackage(UART0, midi_package);
      MIOS32_MIDI_SendPackage(UART1, midi_package);
    }

    return;    
  }

  // CC?
  if( midi_package.type == CC ) {
#if DEFAULT_MIDI_CHANNEL
    if( midi_package.chn != (DEFAULT_MIDI_CHANNEL-1) )
      return;
#endif

    switch( midi_package.cc_number ) {
    case 16:
      if( midi_package.value <= SCALE_NumGet() ) {
	selected_scale = midi_package.value;
	display_update = 1;
      }
      break;
    case 17:
      if( midi_package.value < 12 ) {
	selected_root = midi_package.value;
	display_update = 1;
      }
      break;
    }

    return;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  // exit if button has been depressed
  if( pin_value )
    return;

  // inc/dec root/scale
  // functions are assigned to two DINs for higher flexibility with existing control surfaces
  switch( pin ) {
  case 0:
  case 4:
    if( selected_scale < (SCALE_NumGet()+1) )
      ++selected_scale;
    else
      selected_scale = 0;
    break;

  case 1:
  case 5:
    if( selected_scale > 0 )
      --selected_scale;
    else
      selected_scale = SCALE_NumGet();
    break;

  case 2:
  case 6:
    if( selected_root < 12 )
      ++selected_root;
    else
      selected_root = 0;
    break;

  case 3:
  case 7:
    if( selected_root > 0 )
      --selected_root;
    else
      selected_root = 11;
    break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
