// $Id$
/*
 * Sequencer Demo
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

#include "seq_demo.h"
#include "seq_lcd.h"
#include "srio_mapping.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned INIT_REQ:1;    // request display re-initialisation
    unsigned UPDATE_REQ:1;  // request display update
  };
} display_t;

display_t display;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
u8 get_visible_track(void);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define NUM_STEPS 32
#define NUM_TRACKS 16
u8 par_layer_value[NUM_TRACKS][3][NUM_STEPS];
u8 trg_layer_value[NUM_TRACKS][3][NUM_STEPS/8];

u8 selected_group = 0;
u8 selected_tracks = (1 << 0);
u8 selected_par_layer = 0;
u8 selected_trg_layer = 0;
u8 selected_step_view = 0;
u8 selected_step = 0;
u8 midi_channel = 0;
u8 midi_port = 0;
u8 played_step = 0;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 Init(u32 mode)
{
  int i, track, step;

  // init parameter layer values
  for(track=0; track<NUM_TRACKS; ++track) {
    for(step=0; step<NUM_STEPS; ++step) {
      par_layer_value[track][0][step] = 0x3c; // note C-3
      par_layer_value[track][1][step] = 100;  // velocity
      par_layer_value[track][2][step] = 17;   // gatelength 75%
    }

    // init trigger layer values
    for(i=0; i<NUM_STEPS/8; ++i) {
      trg_layer_value[track][0][i] = track == 0 ? 0x11 : 0x00; // gate
      trg_layer_value[track][1][i] = 0x00; // accent
      trg_layer_value[track][2][i] = 0x00; // roll
    }
  }

  // request display initialisation
  display.INIT_REQ = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
#if 0
  // for debugging
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 0);
  printf("Pin %3d = %d", pin, pin_value);
#else

#if 0
  // send MIDI event for debugging
  MIOS32_MIDI_SendNoteOn(MIOS32_MIDI_PORT_USB0, 0, pin, pin_value ? 0x00 : 0x7f);
#endif

  // NOTE: for MIDI remote all functions have to be outsourced later!
  // TODO: add #ifdef statements for the case that pins are disabled

  s32 gp = -1; // notifies that GP button has been pressed
  switch( pin ) {
    case BUTTON_GP1:  gp =  0; break;
    case BUTTON_GP2:  gp =  1; break;
    case BUTTON_GP3:  gp =  2; break;
    case BUTTON_GP4:  gp =  3; break;
    case BUTTON_GP5:  gp =  4; break;
    case BUTTON_GP6:  gp =  5; break;
    case BUTTON_GP7:  gp =  6; break;
    case BUTTON_GP8:  gp =  7; break;
    case BUTTON_GP9:  gp =  8; break;
    case BUTTON_GP10: gp =  9; break;
    case BUTTON_GP11: gp = 10; break;
    case BUTTON_GP12: gp = 11; break;
    case BUTTON_GP13: gp = 12; break;
    case BUTTON_GP14: gp = 13; break;
    case BUTTON_GP15: gp = 14; break;
    case BUTTON_GP16: gp = 15; break;

    case BUTTON_LEFT:
      if( pin_value ) return; // ignore when button depressed
      if( played_step ) // tmp.
        --played_step;
      break;
    case BUTTON_RIGHT:
      if( pin_value ) return; // ignore when button depressed
      if( ++played_step >= NUM_STEPS ) // tmp.
        played_step = 0;
      break;

    case BUTTON_SCRUB:
      break;
    case BUTTON_METRONOME:
      break;

    case BUTTON_STOP:
      break;
    case BUTTON_PAUSE:
      break;
    case BUTTON_PLAY:
      break;
    case BUTTON_REW:
      break;
    case BUTTON_FWD:
      break;

    case BUTTON_F1:
      break;
    case BUTTON_F2:
      break;
    case BUTTON_F3:
      break;
    case BUTTON_F4:
      break;

    case BUTTON_MENU:
      break;
    case BUTTON_SELECT:
      break;
    case BUTTON_EXIT:
      break;

    case BUTTON_TRACK1:
      if( pin_value ) return; // ignore when button depressed
      selected_tracks = (1 << 0); // TODO: multi-selections!
      break;
    case BUTTON_TRACK2:
      if( pin_value ) return; // ignore when button depressed
      selected_tracks = (1 << 1); // TODO: multi-selections!
      break;
    case BUTTON_TRACK3:
      if( pin_value ) return; // ignore when button depressed
      selected_tracks = (1 << 2); // TODO: multi-selections!
      break;
    case BUTTON_TRACK4:
      if( pin_value ) return; // ignore when button depressed
      selected_tracks = (1 << 3); // TODO: multi-selections!
      break;

    case BUTTON_PAR_LAYER_A:
      if( pin_value ) return; // ignore when button depressed
      selected_par_layer = 0;
      break;
    case BUTTON_PAR_LAYER_B:
      if( pin_value ) return; // ignore when button depressed
      selected_par_layer = 1;
      break;
    case BUTTON_PAR_LAYER_C:
      if( pin_value ) return; // ignore when button depressed
      selected_par_layer = 2;
      break;

    case BUTTON_EDIT:
      break;
    case BUTTON_MUTE:
      break;
    case BUTTON_PATTERN:
      break;
    case BUTTON_SONG:
      break;

    case BUTTON_SOLO:
      break;
    case BUTTON_FAST:
      break;
    case BUTTON_ALL:
      break;

    case BUTTON_GROUP1:
      if( pin_value ) return; // ignore when button depressed
      selected_group = 0;
      break;
    case BUTTON_GROUP2:
      if( pin_value ) return; // ignore when button depressed
      selected_group = 1;
      break;
    case BUTTON_GROUP3:
      if( pin_value ) return; // ignore when button depressed
      selected_group = 2;
      break;
    case BUTTON_GROUP4:
      if( pin_value ) return; // ignore when button depressed
      selected_group = 3;
      break;

    case BUTTON_TRG_LAYER_A:
      if( pin_value ) return; // ignore when button depressed
      selected_trg_layer = 0;
      break;
    case BUTTON_TRG_LAYER_B:
      if( pin_value ) return; // ignore when button depressed
      selected_trg_layer = 1;
      break;
    case BUTTON_TRG_LAYER_C:
      if( pin_value ) return; // ignore when button depressed
      selected_trg_layer = 2;
      break;

    case BUTTON_STEP_VIEW:
      selected_step_view = selected_step_view ? 0 : 1;
      break;

    case BUTTON_TAP_TEMPO:
      break;
  }

  // GP button pressed?
  if( gp >= 0 ) {

    // GP buttons
    if( pin_value ) return; // ignore when button depressed

    // toggle trigger layer
    u8 visible_track = get_visible_track();
    u8 step = gp + selected_step_view*16;
    trg_layer_value[visible_track][selected_trg_layer][step>>3] ^= (1 << (step&7));
  }

  // request display update
  display.UPDATE_REQ = 1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  if( encoder > 16 )
    return; // encoder doesn't exist

#if 0
  // send MIDI event for debugging
  MIOS32_MIDI_SendCC(MIOS32_MIDI_PORT_USB0, 0, encoder, incrementer & 0x7f);
#endif

  // limit incrementer
  if( incrementer > 3 )
    incrementer = 3;
  else if( incrementer < -3 )
    incrementer = -3;

  if( encoder == 0 ) {
    s32 value = played_step + incrementer;
    if( value < 0 )
      value = 0;
    if( value > NUM_STEPS-1 )
      value = NUM_STEPS-1;
    played_step = (u8)value;
  } else {
    u8 visible_track = get_visible_track();
    u8 step = encoder-1 + selected_step_view*16;

    // select step
    selected_step = step;

    // add to absolute value
    s32 value = (s32)par_layer_value[visible_track][selected_par_layer][step] + incrementer;
    if( value < 0 )
      value = 0;
    else if( value >= 128 )
      value = 127;

    // take over if changed
    if( (u8)value != par_layer_value[visible_track][selected_par_layer][step] ) {
      par_layer_value[visible_track][selected_par_layer][step] = (u8)value;
      // here we could do more on changes - important for mixer function (event only sent on changes)
    }

    // (de)activate gate depending on value
    if( value )
      trg_layer_value[visible_track][0][step>>3] |= (1 << (step&7));
    else
      trg_layer_value[visible_track][0][step>>3] &= ~(1 << (step&7));
  }

  // request display update
  display.UPDATE_REQ = 1;
}


/////////////////////////////////////////////////////////////////////////////
// temporary help function to get the visible track
// (could be outsourced somewhere else later)
/////////////////////////////////////////////////////////////////////////////
u8 get_visible_track(void)
{
  u8 offset = 0;

  if( selected_tracks & (1 << 3) )
    offset = 3;
  if( selected_tracks & (1 << 2) )
    offset = 2;
  if( selected_tracks & (1 << 1) )
    offset = 1;
  if( selected_tracks & (1 << 0) )
    offset = 0;

  return 4*selected_group + offset;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle the state of all LEDs (allows to measure the execution speed with a scope)
  MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

  if( display.INIT_REQ ) {
    display.INIT_REQ = 0; // clear request

    int i;

    // clear both LCDs
    for(i=0; i<2; ++i) {
      MIOS32_LCD_DeviceSet(i);
      SEQ_LCD_Clear();
    }

    // initialise charset
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBARS);

    // request display update
    display.UPDATE_REQ = 1;
  }

  if( display.UPDATE_REQ ) {
    display.UPDATE_REQ = 0; // clear request

    char tmp[128];
    u8 step;
    u8 visible_track = get_visible_track();
  
    ///////////////////////////////////////////////////////////////////////////
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_CursorSet(0, 0);
  
    SEQ_LCD_PrintGxTy(selected_group, selected_tracks);
    SEQ_LCD_PrintSpaces(2);
  
    SEQ_LCD_PrintParLayer(selected_par_layer);
    SEQ_LCD_PrintSpaces(1);
  
    printf("Chn%2d", midi_channel);
    MIOS32_LCD_PrintChar('/');
    SEQ_LCD_PrintMIDIPort(midi_port);
    SEQ_LCD_PrintSpaces(1);
  
    SEQ_LCD_PrintTrgLayer(selected_trg_layer);
  
    SEQ_LCD_PrintStepView(selected_step_view);
  
  
    ///////////////////////////////////////////////////////////////////////////
    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_CursorSet(0, 0);
  
    printf("Step");
    SEQ_LCD_PrintSelectedStep(selected_step, 15);
    MIOS32_LCD_PrintChar(':');
  
    SEQ_LCD_PrintNote(par_layer_value[visible_track][0][selected_step]);
    MIOS32_LCD_PrintChar((char)par_layer_value[visible_track][1][selected_step] >> 4);
    SEQ_LCD_PrintSpaces(1);
  
    printf("Vel:%3d", par_layer_value[visible_track][1][selected_step]);
    SEQ_LCD_PrintSpaces(1);

    printf("Len:");
    SEQ_LCD_PrintGatelength(par_layer_value[visible_track][2][selected_step]);
    SEQ_LCD_PrintSpaces(1);
  
    printf("G-a-r--");
  
    ///////////////////////////////////////////////////////////////////////////
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_CursorSet(0, 1);
  
    for(step=0; step<16; ++step) {
      // 9th step reached: switch to second LCD
      if( step == 8 ) {
        MIOS32_LCD_DeviceSet(1);
        MIOS32_LCD_CursorSet(0, 1);
      }
  
      u8 visible_step = step + 16*selected_step_view;
      u8 note = par_layer_value[visible_track][0][visible_step];
      u8 vel = par_layer_value[visible_track][1][visible_step];
      u8 len = par_layer_value[visible_track][2][visible_step];
      u8 gate = trg_layer_value[visible_track][0][visible_step>>3] & (1 << (visible_step&7));
  
      switch( selected_par_layer ) {
        case 0: // Note
        case 1: // Velocity
  
  	if( gate ) {
  	  SEQ_LCD_PrintNote(note);
  	  MIOS32_LCD_PrintChar((char)vel >> 4);
  	} else {
	  printf("----");
  	}
  	break;
  
        case 2: // Gatelength
	  // TODO: print length like on real hardware (length bars)
	  SEQ_LCD_PrintGatelength(len);
	  break;
        default:
	  printf("????");
      }
  
      MIOS32_LCD_PrintChar(
          (visible_step == selected_step) ? '<' 
  	: ((visible_step == selected_step-1) ? '>' : ' '));
    }
  
    ///////////////////////////////////////////////////////////////////////////
  
  
    // GP LEDs
    // TODO: invert if no dual colour LEDs defined
  #ifdef DEFAULT_GP_DOUT_SR_L
    MIOS32_DOUT_SRSet(DEFAULT_GP_DOUT_SR_L-1, trg_layer_value[visible_track][selected_trg_layer][2*selected_step_view+0]);
  #endif
  #ifdef DEFAULT_GP_DOUT_SR_R
    MIOS32_DOUT_SRSet(DEFAULT_GP_DOUT_SR_R-1, trg_layer_value[visible_track][selected_trg_layer][2*selected_step_view+1]);
  #endif
    // TODO: check for selected step view!
  #ifdef DEFAULT_GP_DOUT_SR_L2
    MIOS32_DOUT_SRSet(DEFAULT_GP_DOUT_SR_L2-1, played_step < 8 ? (1 << (played_step&7)) : 0);
  #endif
  #ifdef DEFAULT_GP_DOUT_SR_R2
    MIOS32_DOUT_SRSet(DEFAULT_GP_DOUT_SR_R2-1, played_step >= 8 ? (1 << (played_step&7)) : 0);
  #endif
  
    // beat LED: tmp. for demo w/o real sequencer
    MIOS32_DOUT_PinSet(LED_BEAT, ((played_step & 3) == 0) ? 1 : 0);
  
    // track LEDs
    MIOS32_DOUT_PinSet(LED_TRACK1, (selected_tracks & (1 << 0)) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_TRACK2, (selected_tracks & (1 << 1)) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_TRACK3, (selected_tracks & (1 << 2)) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_TRACK4, (selected_tracks & (1 << 3)) ? 1 : 0);
  
    // parameter layer LEDs
    MIOS32_DOUT_PinSet(LED_PAR_LAYER_A, (selected_par_layer == 0) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_PAR_LAYER_B, (selected_par_layer == 1) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_PAR_LAYER_C, (selected_par_layer == 2) ? 1 : 0);
  
    // group LEDs
    MIOS32_DOUT_PinSet(LED_GROUP1, (selected_group == 0) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_GROUP2, (selected_group == 1) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_GROUP3, (selected_group == 2) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_GROUP4, (selected_group == 3) ? 1 : 0);
  
    // trigger layer LEDs
    MIOS32_DOUT_PinSet(LED_TRG_LAYER_A, (selected_trg_layer == 0) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_TRG_LAYER_B, (selected_trg_layer == 1) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_TRG_LAYER_C, (selected_trg_layer == 2) ? 1 : 0);
  
    // remaining LEDs
    MIOS32_DOUT_PinSet(LED_EDIT, 1);
    MIOS32_DOUT_PinSet(LED_MUTE, 0);
    MIOS32_DOUT_PinSet(LED_PATTERN, 0);
    MIOS32_DOUT_PinSet(LED_SONG, 0);
  
    MIOS32_DOUT_PinSet(LED_SOLO, 0);
    MIOS32_DOUT_PinSet(LED_FAST, 0);
    MIOS32_DOUT_PinSet(LED_ALL, 0);
  
    MIOS32_DOUT_PinSet(LED_PLAY, 1);
    MIOS32_DOUT_PinSet(LED_STOP, 0);
    MIOS32_DOUT_PinSet(LED_PAUSE, 0);
  
    MIOS32_DOUT_PinSet(LED_STEP_1_16, (selected_step_view == 0) ? 1 : 0);
    MIOS32_DOUT_PinSet(LED_STEP_17_32, (selected_step_view == 1) ? 1 : 0); // will be obsolete in MBSEQ V4
  }
}
