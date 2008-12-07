// $Id$
/*
 * Edit page
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
#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_midi.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"


/////////////////////////////////////////////////////////////////////////////
// Local GP LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_LED_Handler(u16 *gp_leds, u16 *gp_leds_flashing)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  *gp_leds =
    (trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+1] << 8) |
    trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+0];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local GP button callback function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_Button_Handler(u32 pin, s32 depressed)
{
  // toggle trigger layer
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 step = pin + ui_selected_step_view*16;
  trg_layer_value[visible_track][ui_selected_trg_layer][step>>3] ^= (1 << (step&7));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local GP encoder callback function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_Encoder_Handler(u32 encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 step = encoder-1 + ui_selected_step_view*16;

  // select step
  ui_selected_step = step;

  // add to absolute value
  s32 value = (s32)par_layer_value[visible_track][ui_selected_par_layer][step] + incrementer;
  if( value < 0 )
    value = 0;
  else if( value >= 128 )
    value = 127;

  // take over if changed
  if( (u8)value != par_layer_value[visible_track][ui_selected_par_layer][step] ) {
    par_layer_value[visible_track][ui_selected_par_layer][step] = (u8)value;
    // here we could do more on changes - important for mixer function (event only sent on changes)
  }

  // (de)activate gate depending on value
  if( value )
    trg_layer_value[visible_track][0][step>>3] |= (1 << (step&7));
  else
    trg_layer_value[visible_track][0][step>>3] &= ~(1 << (step&7));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  SEQ_LCD_PrintSpaces(2);

  SEQ_LCD_PrintParLayer(ui_selected_par_layer);
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintFormattedString("Chn%2d", midi_channel+1);
  MIOS32_LCD_PrintChar('/');
  SEQ_LCD_PrintMIDIPort(midi_port);
  SEQ_LCD_PrintSpaces(1);

  SEQ_LCD_PrintTrgLayer(ui_selected_trg_layer);

  SEQ_LCD_PrintStepView(ui_selected_step_view);


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_CursorSet(0, 0);

  MIOS32_LCD_PrintFormattedString("Step");
  SEQ_LCD_PrintSelectedStep(ui_selected_step, 15);
  MIOS32_LCD_PrintChar(':');

  SEQ_LCD_PrintNote(par_layer_value[visible_track][0][ui_selected_step]);
  MIOS32_LCD_PrintChar((char)par_layer_value[visible_track][1][ui_selected_step] >> 4);
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintFormattedString("Vel:%3d", par_layer_value[visible_track][1][ui_selected_step]);
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintFormattedString("Len:");
  SEQ_LCD_PrintGatelength(par_layer_value[visible_track][2][ui_selected_step]);
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintFormattedString("G-a-r--");

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 1);

  u8 step;
  for(step=0; step<16; ++step) {
    // 9th step reached: switch to second LCD
    if( step == 8 ) {
      MIOS32_LCD_DeviceSet(1);
      MIOS32_LCD_CursorSet(0, 1);
    }

    u8 visible_step = step + 16*ui_selected_step_view;
    u8 note = par_layer_value[visible_track][0][visible_step];
    u8 vel = par_layer_value[visible_track][1][visible_step];
    u8 len = par_layer_value[visible_track][2][visible_step];
    u8 gate = trg_layer_value[visible_track][0][visible_step>>3] & (1 << (visible_step&7));

    switch( ui_selected_par_layer ) {
      case 0: // Note
      case 1: // Velocity

	if( gate ) {
	  SEQ_LCD_PrintNote(note);
	  MIOS32_LCD_PrintChar((char)vel >> 4);
	} else {
        MIOS32_LCD_PrintFormattedString("----");
	}
	break;

      case 2: // Gatelength
        // TODO: print length like on real hardware (length bars)
        SEQ_LCD_PrintGatelength(len);
        break;
      default:
        MIOS32_LCD_PrintFormattedString("????");
    }

    MIOS32_LCD_PrintChar(
        (visible_step == ui_selected_step) ? '<' 
	: ((visible_step == ui_selected_step-1) ? '>' : ' '));
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallGPButtonCallback(GP_Button_Handler);
  SEQ_UI_InstallGPEncoderCallback(GP_Encoder_Handler);
  SEQ_UI_InstallGPLEDCallback(GP_LED_Handler);
  SEQ_UI_InstallGPLCDCallback(LCD_Handler);

  // initialise charset
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBARS);

  return 0; // no error
}
