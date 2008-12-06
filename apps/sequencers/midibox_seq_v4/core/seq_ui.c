// $Id$
/*
 * User Interface Routines
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

#if DEFAULT_SRM_ENABLED
#include <blm8x8.h>
#endif

#include "seq_ui.h"
#include "seq_lcd.h"
#include "seq_led.h"
#include "seq_midi.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 seq_ui_display_update_req;
u8 seq_ui_display_init_req;

u8 ui_selected_group;
u8 ui_selected_tracks;
u8 ui_selected_par_layer;
u8 ui_selected_trg_layer;
u8 ui_selected_step_view;
u8 ui_selected_step;

u16 ui_gp_leds;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Init(u32 mode)
{
  // init selection variables
  ui_selected_group = 0;
  ui_selected_tracks = (1 << 0);
  ui_selected_par_layer = 0;
  ui_selected_trg_layer = 0;
  ui_selected_step_view = 0;
  ui_selected_step = 0;

  // visible GP pattern
  ui_gp_leds = 0x0000;

  // request display initialisation
  seq_ui_display_init_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Dedicated button functions
// Mapped to physical buttons in SEQ_UI_Button_Handler()
// Will also be mapped to MIDI keys later (for MIDI remote function)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_Button_GP(s32 depressed, u32 gp)
{
  if( depressed ) return -1; // ignore when button depressed

  // toggle trigger layer
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 step = gp + ui_selected_step_view*16;
  trg_layer_value[visible_track][ui_selected_trg_layer][step>>3] ^= (1 << (step&7));

  return 0; // no error
}

static s32 SEQ_UI_Button_Left(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  if( played_step ) // tmp.
    --played_step;

  return 0; // no error
}

static s32 SEQ_UI_Button_Right(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  if( ++played_step >= SEQ_CORE_NUM_STEPS ) // tmp.
    played_step = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Scrub(s32 depressed)
{
#if 0
  if( depressed ) return -1; // ignore when button depressed
#else
  // Sending SysEx Stream test
  u8 buffer[7] = { 0xf0, 0x11, 0x22, 0x33, 0x44, depressed ? 0x00 : 0x55, 0xf7 };

  MIOS32_MIDI_SendSysEx(DEFAULT, buffer, 7);
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Metronome(s32 depressed)
{
#if 0
  if( depressed ) return -1; // ignore when button depressed
#else
  // Sending MIDI Note test
  if( depressed )
    MIOS32_MIDI_SendNoteOff(DEFAULT, 0x90, 0x3c, 0x00);
  else
    MIOS32_MIDI_SendNoteOn(DEFAULT, 0x90, 0x3c, 0x7f);
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Stop(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if sequencer running: stop it
  // if sequencer already stopped: reset song position
  if( seq_core_state_run )
    SEQ_CORE_Stop(0);
  else
    SEQ_CORE_Reset();

  return 0; // no error
}

static s32 SEQ_UI_Button_Pause(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // pause sequencer
  SEQ_CORE_Pause(0);

  return 0; // no error
}

static s32 SEQ_UI_Button_Play(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // start sequencer
  SEQ_CORE_Start(0);

  return 0; // no error
}

static s32 SEQ_UI_Button_Rew(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Fwd(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F1(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F2(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F3(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F4(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Utility(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Copy(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Paste(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Clear(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Menu(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Select(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Exit(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Edit(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Mute(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Pattern(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Song(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Solo(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Fast(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_All(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_StepView(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  ui_selected_step_view = ui_selected_step_view ? 0 : 1;

  return 0; // no error
}

static s32 SEQ_UI_Button_TapTempo(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_Track(s32 depressed, u32 track)
{
  if( depressed ) return -1; // ignore when button depressed

  if( track >= 4 ) return -2; // max. 4 track buttons

  ui_selected_tracks = (1 << track); // TODO: multi-selections!

  return 0; // no error
}

static s32 SEQ_UI_Button_Group(s32 depressed, u32 group)
{
  if( depressed ) return -1; // ignore when button depressed

  if( group >= 4 ) return -2; // max. 4 group buttons

  ui_selected_group = group;

  return 0; // no error
}

static s32 SEQ_UI_Button_ParLayer(s32 depressed, u32 par_layer)
{
  if( depressed ) return -1; // ignore when button depressed

  if( par_layer >= 3 ) return -2; // max. 3 parlayer buttons

  ui_selected_par_layer = par_layer;

  return 0; // no error
}

static s32 SEQ_UI_Button_TrgLayer(s32 depressed, u32 trg_layer)
{
  if( depressed ) return -1; // ignore when button depressed

  if( trg_layer >= 3 ) return -2; // max. 3 trglayer buttons

  ui_selected_trg_layer = trg_layer;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Button handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value)
{
  switch( pin ) {
#if BUTTON_GP1
    case BUTTON_GP1:   SEQ_UI_Button_GP(pin_value, 0); break;
#endif
#if BUTTON_GP2
    case BUTTON_GP2:   SEQ_UI_Button_GP(pin_value, 1); break;
#endif
#if BUTTON_GP3
    case BUTTON_GP3:   SEQ_UI_Button_GP(pin_value, 2); break;
#endif
#if BUTTON_GP4
    case BUTTON_GP4:   SEQ_UI_Button_GP(pin_value, 3); break;
#endif
#if BUTTON_GP5
    case BUTTON_GP5:   SEQ_UI_Button_GP(pin_value, 4); break;
#endif
#if BUTTON_GP6
    case BUTTON_GP6:   SEQ_UI_Button_GP(pin_value, 5); break;
#endif
#if BUTTON_GP7
    case BUTTON_GP7:   SEQ_UI_Button_GP(pin_value, 6); break;
#endif
#if BUTTON_GP8
    case BUTTON_GP8:   SEQ_UI_Button_GP(pin_value, 7); break;
#endif
#if BUTTON_GP9
    case BUTTON_GP9:   SEQ_UI_Button_GP(pin_value, 8); break;
#endif
#if BUTTON_GP10
    case BUTTON_GP10:  SEQ_UI_Button_GP(pin_value, 9); break;
#endif
#if BUTTON_GP11
    case BUTTON_GP11:  SEQ_UI_Button_GP(pin_value, 10); break;
#endif
#if BUTTON_GP12
    case BUTTON_GP12:  SEQ_UI_Button_GP(pin_value, 11); break;
#endif
#if BUTTON_GP13
    case BUTTON_GP13:  SEQ_UI_Button_GP(pin_value, 12); break;
#endif
#if BUTTON_GP14
    case BUTTON_GP14:  SEQ_UI_Button_GP(pin_value, 13); break;
#endif
#if BUTTON_GP15
    case BUTTON_GP15:  SEQ_UI_Button_GP(pin_value, 14); break;
#endif
#if BUTTON_GP16
    case BUTTON_GP16:  SEQ_UI_Button_GP(pin_value, 15); break;
#endif

#if BUTTON_LEFT
    case BUTTON_LEFT:  SEQ_UI_Button_Left(pin_value); break;
#endif
#if BUTTON_RIGHT
    case BUTTON_RIGHT: SEQ_UI_Button_Right(pin_value); break;
#endif

#if BUTTON_SCRUB
    case BUTTON_SCRUB: SEQ_UI_Button_Scrub(pin_value); break;
#endif
#if BUTTON_METRONOME
    case BUTTON_METRONOME: SEQ_UI_Button_Metronome(pin_value); break;
#endif

#if BUTTON_STOP
    case BUTTON_STOP:  SEQ_UI_Button_Stop(pin_value); break;
#endif
#if BUTTON_PAUSE
    case BUTTON_PAUSE: SEQ_UI_Button_Pause(pin_value); break;
#endif
#if BUTTON_PLAY
    case BUTTON_PLAY:  SEQ_UI_Button_Play(pin_value); break;
#endif
#if BUTTON_REW
    case BUTTON_REW:   SEQ_UI_Button_Rew(pin_value); break;
#endif
#if BUTTON_FWD
    case BUTTON_FWD:   SEQ_UI_Button_Fwd(pin_value); break;
#endif

#if BUTTON_F1
    case BUTTON_F1:    SEQ_UI_Button_F1(pin_value); break;
#endif
#if BUTTON_F2
    case BUTTON_F2:    SEQ_UI_Button_F2(pin_value); break;
#endif
#if BUTTON_F3
    case BUTTON_F3:    SEQ_UI_Button_F3(pin_value); break;
#endif
#if BUTTON_F4
    case BUTTON_F4:    SEQ_UI_Button_F4(pin_value); break;
#endif

#if BUTTON_UTILITY
    case BUTTON_UTILITY: SEQ_UI_Button_Utility(pin_value); break;
#endif
#if BUTTON_COPY
    case BUTTON_COPY:  SEQ_UI_Button_Copy(pin_value); break;
#endif
#if BUTTON_PASTE
    case BUTTON_PASTE: SEQ_UI_Button_Paste(pin_value); break;
#endif
#if BUTTON_CLEAR
    case BUTTON_CLEAR: SEQ_UI_Button_Clear(pin_value); break;
#endif

#if BUTTON_MENU
    case BUTTON_MENU:  SEQ_UI_Button_Menu(pin_value); break;
#endif
#if BUTTON_SELECT
    case BUTTON_SELECT:SEQ_UI_Button_Select(pin_value); break;
#endif
#if BUTTON_EXIT
    case BUTTON_EXIT:  SEQ_UI_Button_Exit(pin_value); break;
#endif

#if BUTTON_TRACK1
    case BUTTON_TRACK1: SEQ_UI_Button_Track(pin_value, 0); break;
#endif
#if BUTTON_TRACK2
    case BUTTON_TRACK2: SEQ_UI_Button_Track(pin_value, 1); break;
#endif
#if BUTTON_TRACK3
    case BUTTON_TRACK3: SEQ_UI_Button_Track(pin_value, 2); break;
#endif
#if BUTTON_TRACK4
    case BUTTON_TRACK4: SEQ_UI_Button_Track(pin_value, 3); break;
#endif

#if BUTTON_PAR_LAYER_A
    case BUTTON_PAR_LAYER_A: SEQ_UI_Button_ParLayer(pin_value, 0); break;
#endif
#if BUTTON_PAR_LAYER_B
    case BUTTON_PAR_LAYER_B: SEQ_UI_Button_ParLayer(pin_value, 1); break;
#endif
#if BUTTON_PAR_LAYER_C
    case BUTTON_PAR_LAYER_C: SEQ_UI_Button_ParLayer(pin_value, 2); break;
#endif

#if BUTTON_EDIT
    case BUTTON_EDIT:   SEQ_UI_Button_Edit(pin_value); break;
#endif
#if BUTTON_MUTE
    case BUTTON_MUTE:   SEQ_UI_Button_Mute(pin_value); break;
#endif
#if BUTTON_PATTERN
    case BUTTON_PATTERN:SEQ_UI_Button_Pattern(pin_value); break;
#endif
#if BUTTON_SONG
    case BUTTON_SONG:   SEQ_UI_Button_Song(pin_value); break;
#endif

#if BUTTON_SOLO
    case BUTTON_SOLO:   SEQ_UI_Button_Solo(pin_value); break;
#endif
#if BUTTON_FAST
    case BUTTON_FAST:   SEQ_UI_Button_Fast(pin_value); break;
#endif
#if BUTTON_ALL
    case BUTTON_ALL:    SEQ_UI_Button_All(pin_value); break;
#endif

#if BUTTON_GROUP1
    case BUTTON_GROUP1: SEQ_UI_Button_Group(pin_value, 0); break;
#endif
#if BUTTON_GROUP2
    case BUTTON_GROUP2: SEQ_UI_Button_Group(pin_value, 1); break;
#endif
#if BUTTON_GROUP3
    case BUTTON_GROUP3: SEQ_UI_Button_Group(pin_value, 2); break;
#endif
#if BUTTON_GROUP4
    case BUTTON_GROUP4: SEQ_UI_Button_Group(pin_value, 3); break;
#endif

#if BUTTON_TRG_LAYER_A
    case BUTTON_TRG_LAYER_A: SEQ_UI_Button_TrgLayer(pin_value, 0); break;
#endif
#if BUTTON_TRG_LAYER_B
    case BUTTON_TRG_LAYER_B: SEQ_UI_Button_TrgLayer(pin_value, 1); break;
#endif
#if BUTTON_TRG_LAYER_C
    case BUTTON_TRG_LAYER_C: SEQ_UI_Button_TrgLayer(pin_value, 2); break;
#endif

#if BUTTON_STEP_VIEW
    case BUTTON_STEP_VIEW: SEQ_UI_Button_StepView(pin_value); break;
#endif

#if BUTTON_TAP_TEMPO
    case BUTTON_TAP_TEMPO:   SEQ_UI_Button_TapTempo(pin_value); break;
#endif

    default:
      return -1; // button function not mapped to physical button
  }

  // request display update
  seq_ui_display_update_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Encoder handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer)
{
  if( encoder > 16 )
    return -1; // encoder doesn't exist

  // limit incrementer
  if( incrementer > 3 )
    incrementer = 3;
  else if( incrementer < -3 )
    incrementer = -3;

  if( encoder == 0 ) {
    s32 value = played_step + incrementer;
    if( value < 0 )
      value = 0;
    if( value > SEQ_CORE_NUM_STEPS-1 )
      value = SEQ_CORE_NUM_STEPS-1;
    played_step = (u8)value;
  } else {
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
  }

  // request display update
  seq_ui_display_update_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Update LCD messages
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Handler(void)
{
  if( seq_ui_display_init_req ) {
    seq_ui_display_init_req = 0; // clear request

    // clear both LCDs
    int i;
    for(i=0; i<2; ++i) {
      MIOS32_LCD_DeviceSet(i);
      SEQ_LCD_Clear();
    }

    // initialise charset
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBARS);

    // request display update
    seq_ui_display_update_req = 1;
  }

  if( seq_ui_display_update_req ) {
    seq_ui_display_update_req = 0; // clear request

    char tmp[128];
    u8 step;
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
  }



  // for debugging
#if 0
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintFormattedString("%5d  ", seq_midi_queue_size);
#endif

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Update all LEDs
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // track LEDs
  SEQ_LED_PinSet(LED_TRACK1, (ui_selected_tracks & (1 << 0)) ? 1 : 0);
  SEQ_LED_PinSet(LED_TRACK2, (ui_selected_tracks & (1 << 1)) ? 1 : 0);
  SEQ_LED_PinSet(LED_TRACK3, (ui_selected_tracks & (1 << 2)) ? 1 : 0);
  SEQ_LED_PinSet(LED_TRACK4, (ui_selected_tracks & (1 << 3)) ? 1 : 0);
  
  // parameter layer LEDs
  SEQ_LED_PinSet(LED_PAR_LAYER_A, (ui_selected_par_layer == 0) ? 1 : 0);
  SEQ_LED_PinSet(LED_PAR_LAYER_B, (ui_selected_par_layer == 1) ? 1 : 0);
  SEQ_LED_PinSet(LED_PAR_LAYER_C, (ui_selected_par_layer == 2) ? 1 : 0);
  
  // group LEDs
  SEQ_LED_PinSet(LED_GROUP1, (ui_selected_group == 0) ? 1 : 0);
  SEQ_LED_PinSet(LED_GROUP2, (ui_selected_group == 1) ? 1 : 0);
  SEQ_LED_PinSet(LED_GROUP3, (ui_selected_group == 2) ? 1 : 0);
  SEQ_LED_PinSet(LED_GROUP4, (ui_selected_group == 3) ? 1 : 0);
  
  // trigger layer LEDs
  SEQ_LED_PinSet(LED_TRG_LAYER_A, (ui_selected_trg_layer == 0) ? 1 : 0);
  SEQ_LED_PinSet(LED_TRG_LAYER_B, (ui_selected_trg_layer == 1) ? 1 : 0);
  SEQ_LED_PinSet(LED_TRG_LAYER_C, (ui_selected_trg_layer == 2) ? 1 : 0);
  
  // remaining LEDs
  SEQ_LED_PinSet(LED_EDIT, 1);
  SEQ_LED_PinSet(LED_MUTE, 0);
  SEQ_LED_PinSet(LED_PATTERN, 0);
  SEQ_LED_PinSet(LED_SONG, 0);
  
  SEQ_LED_PinSet(LED_SOLO, 0);
  SEQ_LED_PinSet(LED_FAST, 0);
  SEQ_LED_PinSet(LED_ALL, 0);
  
  SEQ_LED_PinSet(LED_PLAY, seq_core_state_run);
  SEQ_LED_PinSet(LED_STOP, !seq_core_state_run);
  SEQ_LED_PinSet(LED_PAUSE, seq_core_state_pause);
  
  SEQ_LED_PinSet(LED_STEP_1_16, (ui_selected_step_view == 0) ? 1 : 0);
  SEQ_LED_PinSet(LED_STEP_17_32, (ui_selected_step_view == 1) ? 1 : 0); // will be obsolete in MBSEQ V4

  // write GP LED values into ui_gp_leds variable
  // will be transfered to DOUT registers in SEQ_UI_LED_Handler_Periodic
  ui_gp_leds =
    (trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+1] << 8) |
     trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+0];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// updates high-prio LED functions (GP LEDs and Beat LED)
// Called before SRIO update
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler_Periodic()
{
  // GP LEDs are only updated when ui_gp_leds or pos_marker_mask has changed
  static u16 prev_ui_gp_leds = 0x0000;
  static u16 prev_pos_marker_mask = 0x0000;


  // beat LED: tmp. for demo w/o real sequencer
  SEQ_LED_PinSet(LED_BEAT, ((played_step & 3) == 0) ? 1 : 0);

  // for song position marker (supports 16 LEDs, check for selected step view)
  u16 pos_marker_mask = 0x0000;
  if( (played_step >> 4) == ui_selected_step_view )
    pos_marker_mask = 1 << (played_step & 0xf);

  // exit of pattern hasn't changed
  if( prev_ui_gp_leds == ui_gp_leds && prev_pos_marker_mask == pos_marker_mask )
    return 0;
  prev_ui_gp_leds = ui_gp_leds;
  prev_pos_marker_mask = pos_marker_mask;

  // transfer to GP LEDs

#ifdef DEFAULT_GP_DOUT_SR_L
# ifdef DEFAULT_GP_DOUT_SR_L2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L-1, (ui_gp_leds >> 0) & 0xff);
# else
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L-1, ((ui_gp_leds ^ pos_marker_mask) >> 0) & 0xff);
# endif
#endif
#ifdef DEFAULT_GP_DOUT_SR_R
# ifdef DEFAULT_GP_DOUT_SR_R2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R-1, (ui_gp_leds >> 8) & 0xff);
#else
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R-1, ((ui_gp_leds ^ pos_marker_mask) >> 8) & 0xff);
#endif
#endif

#ifdef DEFAULT_GP_DOUT_SR_L2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L2-1, (pos_marker_mask >> 0) & 0xff);
#endif
#ifdef DEFAULT_GP_DOUT_SR_R2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R2-1, (pos_marker_mask >> 8) & 0xff);
#endif

#if DEFAULT_SRM_ENABLED && DEFAULT_SRM_DOUT_M_MAPPING == 1
  // for wilba's frontpanel

  // BLM8X8 DOUT -> GP LED mapping
  // 0 = 15,16	1 = 13,14	2 = 11,12	3 = 9,10
  // 4 = 1,2	5 = 3,4		6 = 5,6		7 = 7,8

  // bit 7: first green (i.e. GP1-G)
  // bit 6: first red (i.e. GP1-R)
  // bit 5: second green (i.e. GP2-G)
  // bit 4: second red (i.e. GP2-R)

  // this mapping routine takes ca. 5 uS
  // since it's only executed when ui_gp_leds or gp_mask has changed, it doesn't really hurt

  u16 modified_gp_leds = ui_gp_leds;
#if 1
  // extra: red LED is lit exclusively for higher contrast
  modified_gp_leds &= ~pos_marker_mask;
#endif

  int sr;
  const u8 blm8x8_sr_map[8] = {4, 5, 6, 7, 3, 2, 1, 0};
  u16 gp_mask = 1 << 0;
  for(sr=0; sr<8; ++sr) {
    u8 pattern = 0;

    if( modified_gp_leds & gp_mask )
      pattern |= 0x80;
    if( pos_marker_mask & gp_mask )
      pattern |= 0x40;
    gp_mask <<= 1;
    if( modified_gp_leds & gp_mask )
      pattern |= 0x20;
    if( pos_marker_mask & gp_mask )
      pattern |= 0x10;
    gp_mask <<= 1;

    u8 mapped_sr = blm8x8_sr_map[sr];
    blm8x8_led_row[mapped_sr] = (blm8x8_led_row[mapped_sr] & 0x0f) | pattern;
  }
#endif

}


/////////////////////////////////////////////////////////////////////////////
// Returns the currently visible track
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_UI_VisibleTrackGet(void)
{
  u8 offset = 0;

  if( ui_selected_tracks & (1 << 3) )
    offset = 3;
  if( ui_selected_tracks & (1 << 2) )
    offset = 2;
  if( ui_selected_tracks & (1 << 1) )
    offset = 1;
  if( ui_selected_tracks & (1 << 0) )
    offset = 0;

  return 4*ui_selected_group + offset;
}

