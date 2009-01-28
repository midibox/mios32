// $Id$
/*
 * Track event page
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
#include "seq_cc.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// Note/Chord/CC
#define NUM_OF_ITEMS_NORMAL 6
#define ITEM_GXTY           0
#define ITEM_EVENT_MODE     1
#define ITEM_MIDI_PORT      2
#define ITEM_MIDI_CHANNEL   3
#define ITEM_LAYER_SELECT   4
#define ITEM_LAYER_CONTROL  5
#define ITEM_LAYER_PAR      6

// Drum
#define NUM_OF_ITEMS_DRUM   11
#define ITEM_GXTY           0
#define ITEM_EVENT_MODE     1
#define ITEM_MIDI_PORT      2
#define ITEM_MIDI_CHANNEL   3
#define ITEM_MIDI_CHANNEL_LOCAL 4
#define ITEM_LAYER_A_SELECT 5
#define ITEM_LAYER_B_SELECT 6
#define ITEM_DRUM_SELECT    7
#define ITEM_DRUM_NOTE      8
#define ITEM_DRUM_VEL_N     9
#define ITEM_DRUM_VEL_A     10



/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  seq_event_mode_t event_mode;  // event mode as forwarded to SEQ_CC_MIDI_EVENT_MODE
  u8               par_layers;  // number of parameter layers
  u16              par_steps;   // number of steps per parameter layer
  u8               trg_layers;  // number of trigger layers
  u16              trg_steps;   // number of steps per trigger layer
  u8               drum_with_accent; // drum track with accent triggers
} layer_config_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 GetLayerConfig(u8 track);
static s32 CopyPreset(u8 track, u8 config);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_layer_config;

static layer_config_t layer_config[] = {
  //      mode           par_layers  par_steps  trg_layers  trg_steps  drum w/ accent
  { SEQ_EVENT_MODE_Note,     4,         256,        8,         256,      0 },
  { SEQ_EVENT_MODE_Note,     8,         128,        8,         128,      0 },
  { SEQ_EVENT_MODE_Note,    16,          64,        8,          64,      0 },
  { SEQ_EVENT_MODE_Chord,    4,         256,        8,         256,      0 },
  { SEQ_EVENT_MODE_Chord,    8,         128,        8,         128,      0 },
  { SEQ_EVENT_MODE_Chord,   16,          64,        8,          64,      0 },
  { SEQ_EVENT_MODE_CC,       4,         256,        8,         256,      0 },
  { SEQ_EVENT_MODE_CC,       8,         128,        8,         128,      0 },
  { SEQ_EVENT_MODE_CC,      16,          64,        8,          64,      0 },
  { SEQ_EVENT_MODE_Drum,     2,          64,     2*16,          64,      1 },
  { SEQ_EVENT_MODE_Drum,     2,          64,       16,         128,      0 },
  { SEQ_EVENT_MODE_Drum,     2,         128,      2*8,         128,      1 },
  { SEQ_EVENT_MODE_Drum,     2,         128,        8,         256,      0 }
};

static u8 selected_layer;
static u8 selected_drum;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) { // if flashing flag active: no LED flag set
    // flash INIT LED if current preset doesn't match with old preset
    // this notifies the user, that he should press the "INIT" button
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    if( selected_layer_config != GetLayerConfig(visible_track) )
      *gp_leds = 0x8000;

    return 0;
  }

  if( layer_config[selected_layer_config].event_mode == SEQ_EVENT_MODE_Drum ) {
    switch( ui_selected_item ) {
      case ITEM_GXTY: *gp_leds = 0x0001; break;
      case ITEM_EVENT_MODE: *gp_leds = 0x001e; break;
      case ITEM_MIDI_PORT: *gp_leds = 0x0020; break;
      case ITEM_MIDI_CHANNEL: *gp_leds = 0x0040; break;
      case ITEM_MIDI_CHANNEL_LOCAL: *gp_leds = 0x0080; break;
      case ITEM_LAYER_A_SELECT: *gp_leds = 0x0100; break;
      case ITEM_LAYER_B_SELECT: *gp_leds = 0x0200; break;
      case ITEM_DRUM_SELECT: *gp_leds = 0x0400; break;
      case ITEM_DRUM_NOTE: *gp_leds = 0x0800; break;
      case ITEM_DRUM_VEL_N: *gp_leds = 0x1000; break;
      case ITEM_DRUM_VEL_A: *gp_leds = 0x2000; break;
    }
  } else {
    switch( ui_selected_item ) {
      case ITEM_GXTY: *gp_leds = 0x0001; break;
      case ITEM_EVENT_MODE: *gp_leds = 0x001e; break;
      case ITEM_MIDI_PORT: *gp_leds = 0x0020; break;
      case ITEM_MIDI_CHANNEL: *gp_leds = 0x0040; break;
      case ITEM_LAYER_SELECT: *gp_leds = 0x0100; break;
      case ITEM_LAYER_CONTROL: *gp_leds = 0x0200; break;
      case ITEM_LAYER_PAR: *gp_leds = 0x3c00; break;
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
  u8 event_mode = layer_config[selected_layer_config].event_mode;

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_EVENT_MODE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_MIDI_PORT;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      break;

    case SEQ_UI_ENCODER_GP8:
      if( event_mode == SEQ_EVENT_MODE_Drum )
	ui_selected_item = ITEM_MIDI_CHANNEL_LOCAL;
      else
	return -1;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_LAYER_A_SELECT : ITEM_LAYER_SELECT;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_LAYER_B_SELECT : ITEM_LAYER_CONTROL;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_SELECT : ITEM_LAYER_PAR;
      break;

    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_NOTE : ITEM_LAYER_PAR;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_VEL_N : ITEM_LAYER_PAR;
      break;

    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_VEL_A : ITEM_LAYER_PAR;
      break;

    case SEQ_UI_ENCODER_GP15:
      return -1; // not mapped to encoder

    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped to encoder
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_EVENT_MODE: {
      u8 max_layer_config = (sizeof(layer_config)/sizeof(layer_config_t)) - 1;
      return SEQ_UI_Var8_Inc(&selected_layer_config, 0, max_layer_config, incrementer);
    } break;
    case ITEM_MIDI_PORT: {
      u8 visible_track = SEQ_UI_VisibleTrackGet();
      mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) ) {
	mios32_midi_port_t new_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	SEQ_UI_CC_Set(SEQ_CC_MIDI_PORT, new_port);
	return 1; // value changed
      }
      return 0; // value not changed
    } break;
    case ITEM_MIDI_CHANNEL:  return SEQ_UI_CC_Inc(SEQ_CC_MIDI_CHANNEL, 0, 15, incrementer);
  }

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    switch( ui_selected_item ) {
      case ITEM_DRUM_SELECT:   return SEQ_UI_Var8_Inc(&selected_drum, 0, 15, incrementer);
      case ITEM_DRUM_NOTE:     return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + selected_drum, 0, 127, incrementer);
      case ITEM_DRUM_VEL_N:    return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + selected_drum, 0, 127, incrementer);
      case ITEM_DRUM_VEL_A:    return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_C1 + selected_drum, 0, 127, incrementer);
    }
  } else {
    switch( ui_selected_item ) {
      case ITEM_LAYER_SELECT:  return SEQ_UI_Var8_Inc(&selected_layer, 0, 15, incrementer);
      case ITEM_LAYER_CONTROL: return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + selected_layer, 0, 127, incrementer);
      case ITEM_LAYER_PAR:     return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + selected_layer, 0, 127, incrementer);
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
  if( depressed ) return 0; // ignore when button depressed

  u8 event_mode = layer_config[selected_layer_config].event_mode;

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 1;

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_EVENT_MODE;
      return 1;

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_MIDI_PORT;
      return 1;

    case SEQ_UI_BUTTON_GP7:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      return 1;

    case SEQ_UI_BUTTON_GP8:
      if( event_mode == SEQ_EVENT_MODE_Drum )
	ui_selected_item = ITEM_MIDI_CHANNEL_LOCAL;
      else
	return -1;
      return 1;

    case SEQ_UI_BUTTON_GP9:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_LAYER_A_SELECT : ITEM_LAYER_SELECT;
      return 1;

    case SEQ_UI_BUTTON_GP10:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_LAYER_B_SELECT : ITEM_LAYER_CONTROL;
      return 1;

    case SEQ_UI_BUTTON_GP11:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_SELECT : ITEM_LAYER_CONTROL;
      return 1;

    case SEQ_UI_BUTTON_GP12:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_NOTE : ITEM_LAYER_CONTROL;
      return 1;

    case SEQ_UI_BUTTON_GP13:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_VEL_N : ITEM_LAYER_CONTROL;
      return 1;

    case SEQ_UI_BUTTON_GP14:
      ui_selected_item = (event_mode == SEQ_EVENT_MODE_Drum) ? ITEM_DRUM_VEL_A : ITEM_LAYER_CONTROL;
      return 1;

    case SEQ_UI_BUTTON_GP15:
      // TODO
      return 1;

    case SEQ_UI_BUTTON_GP16:
      // ui_selected_item = ITEM_PRESET;
      // TODO: copy preset for all selected tracks!
      CopyPreset(SEQ_UI_VisibleTrackGet(), selected_layer_config);
      return 1; // value has been changed


    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	if( ++ui_selected_item >= NUM_OF_ITEMS_DRUM )
	  ui_selected_item = 0;
      } else {
	if( ++ui_selected_item >= NUM_OF_ITEMS_NORMAL )
	  ui_selected_item = 0;
      }

      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	if( ui_selected_item == 0 )
	  ui_selected_item = NUM_OF_ITEMS_DRUM-1;
      } else {
	if( ui_selected_item == 0 )
	  ui_selected_item = NUM_OF_ITEMS_NORMAL-1;
      }
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Type Steps/ParL/TrgL Port Chn.     Layer  controls                Edit      
  // G1T1 Note  256   4     8  IIC2  12        D    Prob                    Name INIT

  // Trk. Type Steps/ParL/TrgL Port Chn.     Layer  controls                Edit      
  // G1T1 Note  256   4     8  IIC2  12        D    CC #001 ModWheel        Name INIT

  // Track Type "Note", Chord" and "CC":
  // Note: Parameter Layer A/B/C statically assigned to Note Number/Velocity/Length
  //       Parameter Layer D..P can be mapped to
  //       - additional Notes (e.g. for poly recording)
  //       - Base note (directly forwarded to Transposer, e.g. for Force-to-Scale or Chords)
  //       - CC (Number 000..127 assignable, GM controller name will be displayed)
  //       - Pitch Bender
  //       - Loopback (Number 000..127 assignable, name will be displayed)
  //       - Delay (+/- 96 ticks @384 ppqn)
  //       - Probability (100%..0%)
  // Chord: same like Note, but predefined Chords will be played
  // CC: Layer A/B/C play CCs (or other parameters) as well - no note number, no velocity, no length

  // Available Layer Constraints (Partitioning for 1024 bytes Par. memory, 2048 bits Trg. memory)
  //    - 256 Steps with  4 Parameter Layers A-D and 8 Trigger Layers A-H
  //    - 128 Steps with  8 Parameter Layers A-H and 8 Trigger Layers A-H
  //    -  64 Steps with 16 Parameter Layers A-P and 8 Trigger Layers A-H


  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Type St/ParL St/TrgL Port Chn. Glb LayA LayB  Drum Note VelN VelA Edit     
  // G1T1 Drum 64/2   128/2*16 USB1  10  yes Dly. Prb.   BD   C-1  100  127 Name INIT

  // Trk. Type St/ParL St/TrgL Port Chn. Glb LayA LayB  Drum Note VelN VelA Edit     
  // G1T1 Drum 64/2   256/16   USB1  12   no Vel. Len.   SD   D-1  ---  --- Name INIT


  // Track Type "Drums":
  //    2 parameter layers for each trigger layer (drum instrument).
  //    If a trigger layer consists of 128 steps, the parameters will be mirrored (looped each 64 steps)
  //    assignable to Velocity/Gatelength/Delay/Probability only
  //    or to special parameter for drum mode: "Roll/Flam Effect" (a selection of up 
  //    to 128 different effects selectable for each step separately)
  //    (no CC, no Pitch Bender, no Loopback, as they don't work note-wise, but 
  //     only channel wise
  //     just send them from a different track)

  // Each drum instrument provides 4 constant parameters for:
  //   - Note Number
  //   - MIDI Channel (1-16) - optionally, we can select a "local" channel definition for the instrument
  //   - Velocity Normal (if not taken from Parameter Layer)
  //   - Velocity Accented (if not taken from Parameter Layer)


  // Available Layer Constraints (Partitioning for 1024 bytes Par. memory, 2048 bits Trg. memory)
  //    - 2 Parameter Layer with 64 steps and 2*16 Trigger Layers A-P with 64 steps taken for Gate and Accent
  //    - 2 Parameter Layer with 64 steps and 16 Trigger Layers A-P with 128 steps taken for Gate
  //    - 2 Parameter Layer with 64 steps and 2*8 Trigger Layers A-P with 128 steps taken for Gate and Accent
  //    - 2 Parameter Layer with 64 steps and 8 Trigger Layers A-P with 256 steps taken for Gate

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = layer_config[selected_layer_config].event_mode;

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintString("Trk. Type St/ParL St/TrgL Port Chn. Glb Layer  controls                Edit     ");
    SEQ_LCD_PrintString("LayA ");
    SEQ_LCD_PrintString("LayB ");
    SEQ_LCD_PrintString(" Drum Note VelN VelA Edit     ");
  } else {
    SEQ_LCD_PrintString("Trk. Type Steps/ParL/TrgL Port Chn.     Layer  controls                Edit     ");
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_EVENT_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(21);
  } else {
    const char event_mode_str[4][6] = { "Note ", "Chord", " CC  ", "Drum " };
    SEQ_LCD_PrintString(event_mode_str[event_mode]);

    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintFormattedString(layer_config[selected_layer_config].par_steps >= 100 ? "%3d/%1d  %3d/" : "%2d/%1d   %3d/",
				   layer_config[selected_layer_config].par_steps, 
				   layer_config[selected_layer_config].par_layers,
				   layer_config[selected_layer_config].trg_steps);
      if( layer_config[selected_layer_config].drum_with_accent ) {
	SEQ_LCD_PrintFormattedString("2*%2d ", layer_config[selected_layer_config].trg_layers);
      } else {
	SEQ_LCD_PrintFormattedString("%2d   ", layer_config[selected_layer_config].trg_layers);
      }
    } else {
      SEQ_LCD_PrintFormattedString("  %3d %3d  %3d  ", 
				   layer_config[selected_layer_config].par_steps, 
				   layer_config[selected_layer_config].par_layers,
				   layer_config[selected_layer_config].trg_layers);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
  if( ui_selected_item == ITEM_MIDI_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintMIDIOutPort(port);
    SEQ_LCD_PrintChar(SEQ_MIDI_PORT_OutCheckAvailable(port) ? ' ' : '*');
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MIDI_CHANNEL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString("%3d  ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_CHANNEL_LOCAL && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintFormattedString(" no ");
    }

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LAYER_A_SELECT && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintFormattedString("Dly. ");
    }

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LAYER_B_SELECT && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(6);
    } else {
      SEQ_LCD_PrintFormattedString("Par.  ");
    }

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_DRUM_SELECT && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintTrackDrum(visible_track, selected_drum, (char *)seq_core_trk[visible_track].name);
    }

    /////////////////////////////////////////////////////////////////////////
    SEQ_LCD_PrintSpaces(1);
    if( ui_selected_item == ITEM_DRUM_NOTE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + selected_drum));
    }
    SEQ_LCD_PrintSpaces(1);

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_DRUM_VEL_N && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + selected_drum));
    }

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_DRUM_VEL_A && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_C1 + selected_drum));
    }
  } else {
    /////////////////////////////////////////////////////////////////////////
    SEQ_LCD_PrintSpaces(4);

    /////////////////////////////////////////////////////////////////////////
    SEQ_LCD_PrintSpaces(2);
    if( ui_selected_item == ITEM_LAYER_SELECT && ui_cursor_flash ) {
      SEQ_LCD_PrintChar(' ');
    } else {
      SEQ_LCD_PrintChar('A' + selected_layer);
    }
    SEQ_LCD_PrintSpaces(4);

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LAYER_CONTROL && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintFormattedString("CC  #"); // TODO
    }

    /////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LAYER_PAR && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(19);
    } else {
      SEQ_LCD_PrintFormattedString("%03d ModWheel       ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + selected_layer));
    }
  }

  /////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("Name ");

  /////////////////////////////////////////////////////////////////////////
  if( selected_layer_config != GetLayerConfig(visible_track) && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString("INIT");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKEVNT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  selected_layer_config = GetLayerConfig(SEQ_UI_VisibleTrackGet());

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copies preset
/////////////////////////////////////////////////////////////////////////////
static s32 CopyPreset(u8 track, u8 config)
{
  // exit if invalid config
  if( config >= (sizeof(layer_config)/sizeof(layer_config_t)) )
    return -1; // invalid config

  // partitionate layers and clear all steps
  SEQ_PAR_TrackInit(track, layer_config[config].par_steps, layer_config[config].par_layers);
  SEQ_TRG_TrackInit(track, layer_config[config].trg_steps, layer_config[config].trg_layers);

  SEQ_CC_Set(track, SEQ_CC_MIDI_EVENT_MODE, layer_config[config].event_mode);

  u8 only_layers = 0;
  u8 all_triggers_cleared = 0;
  u8 drum_with_accent = layer_config[config].drum_with_accent;
  return SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, drum_with_accent);
}


/////////////////////////////////////////////////////////////////////////////
// Determines current layer config
/////////////////////////////////////////////////////////////////////////////
static s32 GetLayerConfig(u8 track)
{
  int i;

  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  u8 par_layers = SEQ_PAR_NumLayersGet(track);
  u16 par_steps = SEQ_PAR_NumStepsGet(track);
  u8 trg_layers = SEQ_TRG_NumLayersGet(track);
  u16 trg_steps = SEQ_TRG_NumStepsGet(track);

  for(i=0; i<(sizeof(layer_config)/sizeof(layer_config_t)); ++i) {
    layer_config_t *lc = (layer_config_t *)&layer_config[i];
    if( lc->event_mode == event_mode &&
	lc->par_layers == par_layers &&	lc->par_steps == par_steps &&
	lc->trg_layers == trg_layers &&	lc->trg_steps == trg_steps ) {
      return i;
    }
  }

  return 0; // not found, return index to first config
}

