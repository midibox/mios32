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
#include <string.h>

#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_midi_port.h"
#include "seq_label.h"
#include "seq_cc_labels.h"
#include "seq_file.h"
#include "seq_file_t.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// Note/Chord/CC
#define NUM_OF_ITEMS_NORMAL 8
#define ITEM_GXTY           0
#define ITEM_EVENT_MODE     1
#define ITEM_MIDI_PORT      2
#define ITEM_MIDI_CHANNEL   3
#define ITEM_EDIT_NAME      4
#define ITEM_LAYER_SELECT   5
#define ITEM_LAYER_CONTROL  6
#define ITEM_LAYER_PAR      7

// Drum
#define NUM_OF_ITEMS_DRUM   11
//#define ITEM_GXTY           0
//#define ITEM_EVENT_MODE     1
//#define ITEM_MIDI_PORT      2
//#define ITEM_MIDI_CHANNEL   3
//#define ITEM_EDIT_NAME      4
#define ITEM_LAYER_A_SELECT 5
#define ITEM_LAYER_B_SELECT 6
#define ITEM_DRUM_SELECT    7
#define ITEM_DRUM_NOTE      8
#define ITEM_DRUM_VEL_N     9
#define ITEM_DRUM_VEL_A     10

#define IMPORT_NUM_OF_ITEMS    5
#define IMPORT_ITEM_NAME       0
#define IMPORT_ITEM_CHN        1
#define IMPORT_ITEM_MAPS       2
#define IMPORT_ITEM_CFG        3
#define IMPORT_ITEM_STEPS      4


// Preset dialog screens
#define PR_DIALOG_NONE          0
#define PR_DIALOG_EDIT_LABEL    1
#define PR_DIALOG_PRESETS       2
#define PR_DIALOG_EXPORT_FNAME  3
#define PR_DIALOG_EXPORT_FEXISTS 4
#define PR_DIALOG_IMPORT        5

#define NUM_LIST_DISPLAYED_ITEMS 4
#define LIST_ENTRY_WIDTH 9


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  seq_event_mode_t event_mode;  // event mode as forwarded to SEQ_CC_MIDI_EVENT_MODE
  u8               par_layers;  // number of parameter layers
  u16              par_steps;   // number of steps per parameter layer
  u8               trg_layers;  // number of trigger layers
  u16              trg_steps;   // number of steps per trigger layer
  u8               instruments; // number of instruments per track
} layer_config_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 GetLayerConfig(u8 track);
static s32 CopyPreset(u8 track, u8 config);
static void InitReq(u32 dummy);

static s32 SEQ_UI_TRKEVNT_UpdateDirList(void);

static s32 DoExport(u8 force_overwrite);
static s32 DoImport(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_layer_config;
static u8 selected_layer_config_track;

static u8 edit_cc_number;

static const layer_config_t layer_config[] = {
  //      mode           par_layers  par_steps  trg_layers  trg_steps  instruments
  { SEQ_EVENT_MODE_Note,    16,          64,        8,          64,      1 },
  { SEQ_EVENT_MODE_Note,     8,         128,        8,         128,      1 },
  { SEQ_EVENT_MODE_Note,     4,         256,        8,         256,      1 },
  { SEQ_EVENT_MODE_Chord,   16,          64,        8,          64,      1 },
  { SEQ_EVENT_MODE_Chord,    8,         128,        8,         128,      1 },
  { SEQ_EVENT_MODE_Chord,    4,         256,        8,         256,      1 },
  { SEQ_EVENT_MODE_CC,      16,          64,        8,          64,      1 },
  { SEQ_EVENT_MODE_CC,       8,         128,        8,         128,      1 },
  { SEQ_EVENT_MODE_CC,       4,         256,        8,         256,      1 },
  { SEQ_EVENT_MODE_Drum,     1,          64,        2,          64,     16 },
  { SEQ_EVENT_MODE_Drum,     2,          32,        1,         128,     16 },
  { SEQ_EVENT_MODE_Drum,     1,         128,        2,         128,      8 },
  { SEQ_EVENT_MODE_Drum,     2,          64,        1,         256,      8 },
  { SEQ_EVENT_MODE_Drum,     1,          64,        1,          64,     16 },
  { SEQ_EVENT_MODE_Drum,     1,         128,        1,         128,      8 },
  { SEQ_EVENT_MODE_Drum,     1,         256,        1,         256,      4 }
};


static u8 pr_dialog;

static s32 dir_num_items; // contains SEQ_FILE error status if < 0
static u8 dir_view_offset = 0; // only changed once after startup
static char dir_name[12]; // directory name of device (first char is 0 if no device selected)

static seq_file_t_import_flags_t import_flags;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  switch( pr_dialog ) {
    case PR_DIALOG_EDIT_LABEL:
      // no LED functions yet
      break;

    case PR_DIALOG_PRESETS:
      *gp_leds = (3 << (2*ui_selected_item));
      break;

    case PR_DIALOG_EXPORT_FNAME:
    case PR_DIALOG_EXPORT_FEXISTS:
      // no LED functions yet
      break;

    case PR_DIALOG_IMPORT:
      switch( ui_selected_item ) {
        case IMPORT_ITEM_NAME:
	  *gp_leds |= 0x0100;
	  break;
        case IMPORT_ITEM_CHN:
	  *gp_leds |= 0x0200;
	  break;
        case IMPORT_ITEM_MAPS:
	  *gp_leds |= 0x0400;
	  break;
        case IMPORT_ITEM_CFG:
	  *gp_leds |= 0x0800;
	  break;
        case IMPORT_ITEM_STEPS:
	  *gp_leds |= 0x1000;
	  break;
      }
      break;

    default:
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
	  case ITEM_EDIT_NAME: *gp_leds = 0x0080; break;
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
	  case ITEM_EDIT_NAME: *gp_leds = 0x0080; break;
	  case ITEM_LAYER_SELECT: *gp_leds = 0x0100; break;
	  case ITEM_LAYER_CONTROL: *gp_leds = 0x0200; break;
	  case ITEM_LAYER_PAR: *gp_leds = 0x1c00; break;
	}
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
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL:
      if( encoder <= SEQ_UI_ENCODER_GP16 ) {
	switch( encoder ) {
	  case SEQ_UI_ENCODER_GP15: { // select preset
	    int pos;

	    if( event_mode == SEQ_EVENT_MODE_Drum ) {
	      if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_drum, 0, SEQ_LABEL_NumPresetsDrum()-1, incrementer) ) {
		SEQ_LABEL_CopyPresetDrum(ui_edit_preset_num_drum, (char *)&seq_core_trk[visible_track].name[5*ui_selected_instrument]);
		for(pos=4, ui_edit_name_cursor=pos; pos>=0; --pos)
		  if( seq_core_trk[visible_track].name[5*ui_selected_instrument + pos] == ' ' )
		    ui_edit_name_cursor = pos;
		  else
		    break;
		return 1;
	      }
	      return 0;
	    } else {
	      if( ui_edit_name_cursor < 5 ) { // select category preset
		if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_category, 0, SEQ_LABEL_NumPresetsCategory()-1, incrementer) ) {
		  SEQ_LABEL_CopyPresetCategory(ui_edit_preset_num_category, (char *)&seq_core_trk[visible_track].name[0]);
		  for(pos=4, ui_edit_name_cursor=pos; pos>=0; --pos)
		    if( seq_core_trk[visible_track].name[pos] == ' ' )
		      ui_edit_name_cursor = pos;
		    else
		      break;
		  return 1;
		}
		return 0;
	      } else { // select label preset
		if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_label, 0, SEQ_LABEL_NumPresets()-1, incrementer) ) {
		  SEQ_LABEL_CopyPreset(ui_edit_preset_num_label, (char *)&seq_core_trk[visible_track].name[5]);
		  for(pos=19, ui_edit_name_cursor=pos; pos>=5; --pos)
		    if( seq_core_trk[visible_track].name[pos] == ' ' )
		      ui_edit_name_cursor = pos;
		    else
		      break;
		  return 1;
		}
		return 0;
	      }
	    }
	  } break;

	  case SEQ_UI_ENCODER_GP16: // exit keypad editor
	    // EXIT only via button
	    if( incrementer == 0 ) {
	      ui_selected_item = 0;
	      pr_dialog = PR_DIALOG_NONE;
	    }
	    return 1;
	}

	if( event_mode == SEQ_EVENT_MODE_Drum )
	  return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_core_trk[visible_track].name[ui_selected_instrument*5], 5);
	else
	  return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_core_trk[visible_track].name, 20);
      }

      return -1; // encoder not mapped


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_PRESETS:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
	  return SEQ_UI_GxTyInc(incrementer);

        case SEQ_UI_ENCODER_GP10:
	  return -1; // not mapped

        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12: {
	  int i;

	  // only via button
	  if( incrementer != 0 )
	    return 0; 

	  // initialize keypad editor
	  SEQ_UI_KeyPad_Init();
	  for(i=0; i<8; ++i)
	    dir_name[i] = ' ';

	  ui_selected_item = 0;
	  pr_dialog = PR_DIALOG_EXPORT_FNAME;

	  return 1;
	}

        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped

        case SEQ_UI_ENCODER_GP16:
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }
	  return 1;

        default:
	  if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &dir_view_offset) )
	    SEQ_UI_TRKEVNT_UpdateDirList();
      }
      break;


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_IMPORT: {
      if( encoder <= SEQ_UI_ENCODER_GP8 )
	return -1; // not mapped

      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
          ui_selected_item = IMPORT_ITEM_NAME;
          break;

        case SEQ_UI_ENCODER_GP10:
          ui_selected_item = IMPORT_ITEM_CHN;
          break;
    
        case SEQ_UI_ENCODER_GP11:
          ui_selected_item = IMPORT_ITEM_MAPS;
          break;
    
        case SEQ_UI_ENCODER_GP12:
          ui_selected_item = IMPORT_ITEM_CFG;
          break;
    
        case SEQ_UI_ENCODER_GP13:
          ui_selected_item = IMPORT_ITEM_STEPS;
          break;
    
        case SEQ_UI_ENCODER_GP14:
	  // not mapped
	  return -1;

        case SEQ_UI_ENCODER_GP15:
	  // IMPORT only via button
	  if( incrementer == 0 ) {
	    if( DoImport() == 0 ) {
	      // exit page
	      ui_selected_item = 0;
	      pr_dialog = PR_DIALOG_NONE;
	    }
	  }
	  return 1;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }
	  return 1;
	} break;
      }

      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
        case IMPORT_ITEM_NAME:
	  if( !incrementer ) incrementer = import_flags.NAME ? -1 : 1;
	  import_flags.NAME = incrementer > 0 ? 1 : 0;
	  break;

        case IMPORT_ITEM_CHN:
	  if( !incrementer ) incrementer = import_flags.CHN ? -1 : 1;
	  import_flags.CHN = incrementer > 0 ? 1 : 0;
	  break;

        case IMPORT_ITEM_MAPS:
	  if( !incrementer ) incrementer = import_flags.MAPS ? -1 : 1;
	  import_flags.MAPS = incrementer > 0 ? 1 : 0;
	  break;

        case IMPORT_ITEM_CFG:
	  if( !incrementer ) incrementer = import_flags.CFG ? -1 : 1;
	  import_flags.CFG = incrementer > 0 ? 1 : 0;
	  break;

        case IMPORT_ITEM_STEPS:
	  if( !incrementer ) incrementer = import_flags.STEPS ? -1 : 1;
	  import_flags.STEPS = incrementer > 0 ? 1 : 0;
	  break;
      }

      return -1; // invalid or unsupported encoder
    }


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EXPORT_FNAME: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP15: {
	  int i;

	  // SAVE only via button
	  if( incrementer != 0 )
	    return 0;

	  if( DoExport(0) == 0 ) { // don't force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }

	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }
	  return 1;
	} break;
      }

      return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&dir_name[0], 8);
    }


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EXPORT_FEXISTS: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP11: {
	  // YES only via button
	  if( incrementer != 0 )
	    return 0;

	  // YES: overwrite file
	  if( DoExport(1) == 0 ) { // force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP12: {
	  // NO only via button
	  if( incrementer != 0 )
	    return 0;

	  // NO: don't overwrite file - back to filename entry

	  ui_selected_item = 0;
	  pr_dialog = PR_DIALOG_EXPORT_FNAME;
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_NONE;
	  }
	  return 1;
	} break;
      }

      return -1; // not mapped
    }


    ///////////////////////////////////////////////////////////////////////////
    default:

      if( encoder >= SEQ_UI_ENCODER_GP9 && encoder <= SEQ_UI_ENCODER_GP16 ) {
	if( selected_layer_config != GetLayerConfig(visible_track) ) {
	  // ENC function disabled so long pattern hasn't been initialized
	  return -1;
	}

	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  switch( encoder ) {
	    case SEQ_UI_ENCODER_GP9:
	      ui_selected_item = ITEM_LAYER_A_SELECT;
	      break;

	    case SEQ_UI_ENCODER_GP10:
	      ui_selected_item = ITEM_LAYER_B_SELECT;
	      break;

	    case SEQ_UI_ENCODER_GP11:
	      ui_selected_item = ITEM_DRUM_SELECT;
	      break;

	    case SEQ_UI_ENCODER_GP12:
	      ui_selected_item = ITEM_DRUM_NOTE;
	      break;

	    case SEQ_UI_ENCODER_GP13:
	      ui_selected_item = ITEM_DRUM_VEL_N;
	      break;

	    case SEQ_UI_ENCODER_GP14:
	      ui_selected_item = ITEM_DRUM_VEL_A;
	      break;

	    case SEQ_UI_ENCODER_GP15:
	      // change to preset page (only via button)
	      if( incrementer == 0 ) {
		ui_selected_item = 0;
		pr_dialog = PR_DIALOG_PRESETS;
		SEQ_UI_TRKEVNT_UpdateDirList();
	      }
	      return 1;

	    case SEQ_UI_ENCODER_GP16:
	      return -1; // EXIT not mapped to encoder
	  }
	} else {
	  switch( encoder ) {
	    case SEQ_UI_ENCODER_GP9:
	      ui_selected_item = ITEM_LAYER_SELECT;
	      break;

	    case SEQ_UI_ENCODER_GP10:
	      ui_selected_item = ITEM_LAYER_CONTROL;
	      break;

	    case SEQ_UI_ENCODER_GP11:
	    case SEQ_UI_ENCODER_GP12:
	    case SEQ_UI_ENCODER_GP13:
	      // CC number selection now has to be confirmed with GP button
	      if( ui_selected_item != ITEM_LAYER_PAR ) {
		edit_cc_number = SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_par_layer);
		ui_selected_item = ITEM_LAYER_PAR;
		SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Please confirm CC", "with GP button!");
	      } else if( incrementer == 0 ) {
		if( edit_cc_number != SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_par_layer) ) {
		  SEQ_CC_Set(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_par_layer, edit_cc_number);
		  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "CC number", "has been changed.");
		}
	      }
	      break;

	    case SEQ_UI_ENCODER_GP14:
	    case SEQ_UI_ENCODER_GP15:
	      // change to preset page (only via button)
	      if( incrementer == 0 ) {
		ui_selected_item = 0;
		pr_dialog = PR_DIALOG_PRESETS;
		SEQ_UI_TRKEVNT_UpdateDirList();
	      }
	      return 1;

	    case SEQ_UI_ENCODER_GP16:
	      return -1; // not mapped to encoder
	  }
	}
      }

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
	  ui_selected_item = ITEM_EDIT_NAME;
	  break;
      }

      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
        case ITEM_GXTY: return SEQ_UI_GxTyInc(incrementer);
        case ITEM_EVENT_MODE: {
	  u8 max_layer_config = (sizeof(layer_config)/sizeof(layer_config_t)) - 1;
	  return SEQ_UI_Var8_Inc(&selected_layer_config, 0, max_layer_config, incrementer);
	} break;

        case ITEM_MIDI_PORT: {
	  mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
	  u8 port_ix = SEQ_MIDI_PORT_OutIxGet(port);
	  if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) ) {
	    mios32_midi_port_t new_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	    SEQ_UI_CC_Set(SEQ_CC_MIDI_PORT, new_port);
	    return 1; // value changed
	  }
	  return 0; // value not changed
	} break;

        case ITEM_MIDI_CHANNEL: return SEQ_UI_CC_Inc(SEQ_CC_MIDI_CHANNEL, 0, 15, incrementer);

        case ITEM_EDIT_NAME:
	  // switch to keypad editor if button has been pressed
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    pr_dialog = PR_DIALOG_EDIT_LABEL;
	    SEQ_UI_KeyPad_Init();
	  }
	  return 1;
      }

      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	switch( ui_selected_item ) {
	case ITEM_DRUM_SELECT: {
	  u8 num_drums = layer_config[selected_layer_config].instruments;
	  return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_drums-1, incrementer);
	} break;
	case ITEM_LAYER_A_SELECT: {
	  if( SEQ_UI_CC_Inc(SEQ_CC_PAR_ASG_DRUM_LAYER_A, 0, SEQ_PAR_NUM_TYPES-1, incrementer) ) {
	    SEQ_LAYER_CopyParLayerPreset(visible_track, 0);
	    return 1;
	  }
	  return 0;
	} break;
	case ITEM_LAYER_B_SELECT: {
	  if( SEQ_UI_CC_Inc(SEQ_CC_PAR_ASG_DRUM_LAYER_B, 0, SEQ_PAR_NUM_TYPES-1, incrementer) ) {
	    SEQ_LAYER_CopyParLayerPreset(visible_track, 1);
	    return 1;
	  }
	  return 0;
	} break;
	case ITEM_DRUM_NOTE:     return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + ui_selected_instrument, 0, 127, incrementer);
	case ITEM_DRUM_VEL_N:    return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + ui_selected_instrument, 0, 127, incrementer);
	case ITEM_DRUM_VEL_A:    return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_C1 + ui_selected_instrument, 0, 127, incrementer);
	}
      } else {
	switch( ui_selected_item ) {
	  case ITEM_LAYER_SELECT:  return SEQ_UI_Var8_Inc(&ui_selected_par_layer, 0, 15, incrementer);
	  case ITEM_LAYER_PAR:
	    if( event_mode == SEQ_EVENT_MODE_Drum ) {
	      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + ui_selected_par_layer, 0, 127, incrementer);
	    } else {
	      // CC number selection now has to be confirmed with GP button
	      return SEQ_UI_Var8_Inc(&edit_cc_number, 0, 127, incrementer);
	    }
	    
	  case ITEM_LAYER_CONTROL: {
	    // TODO: has to be done for all selected tracks
	    if( SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + ui_selected_par_layer, 0, SEQ_PAR_NUM_TYPES-1, incrementer) ) {
	      SEQ_LAYER_CopyParLayerPreset(visible_track, ui_selected_par_layer);
	      return 1;
	    }
	    return 0;
	  } break;
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
  u8 event_mode = layer_config[selected_layer_config].event_mode;
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL:
      if( depressed ) return 0; // ignore when button depressed

      if( button <= SEQ_UI_BUTTON_GP16 )
	return Encoder_Handler(button, 0); // re-use encoder handler

      return -1; // button not mapped


    case PR_DIALOG_PRESETS:
      if( depressed ) return 0; // ignore when button depressed

      if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 )
	return Encoder_Handler(button, 0); // re-use encoder handler

      if( button <= SEQ_UI_BUTTON_GP8 || button == SEQ_UI_BUTTON_Select ) {
	if( button != SEQ_UI_BUTTON_Select )
	  ui_selected_item = button / 2;

	if( dir_num_items >= 1 && (ui_selected_item+dir_view_offset) < dir_num_items ) {
	  // get filename
	  int i;
	  char *p = (char *)&dir_name[0];
	  for(i=0; i<8; ++i) {
	    char c = ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item + i];
	    if( c != ' ' )
	      *p++ = c;
	  }
	  *p++ = 0;
	  // switch to import page
	  ui_selected_item = 0;
	  pr_dialog = PR_DIALOG_IMPORT;
	}
      }
      return 1;


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_IMPORT:
      if( depressed ) return 0; // ignore when button depressed

      if( button <= SEQ_UI_BUTTON_GP16 )
	return Encoder_Handler(button, 0); // re-use encoder handler

      switch( button ) {
        case SEQ_UI_BUTTON_Select:
        case SEQ_UI_BUTTON_Right:
          if( ++ui_selected_item >= IMPORT_NUM_OF_ITEMS )
	    ui_selected_item = 0;
    
          return 1; // value always changed
    
        case SEQ_UI_BUTTON_Left:
          if( ui_selected_item == 0 )
	    ui_selected_item = IMPORT_NUM_OF_ITEMS-1;
    
          return 1; // value always changed
    
        case SEQ_UI_BUTTON_Up:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);
    
        case SEQ_UI_BUTTON_Down:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
      }

      return -1; // invalid or unsupported button


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EXPORT_FNAME:
    case PR_DIALOG_EXPORT_FEXISTS: {
      if( depressed ) return 0; // ignore when button depressed

      return Encoder_Handler((seq_ui_encoder_t)button, 0);
    }


    ///////////////////////////////////////////////////////////////////////////
    default:
      if( button == SEQ_UI_BUTTON_GP16 ) {
	if( depressed )
	  SEQ_UI_UnInstallDelayedActionCallback(InitReq);
	else {
	  SEQ_UI_InstallDelayedActionCallback(InitReq, 2000, 0);
	  SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION_R, 2001, "", "to initialize Track!");
	}
	return 1; // value has been changed
      }

      if( depressed ) return 0; // ignore when button depressed

      if( button <= SEQ_UI_BUTTON_GP15 )
	return Encoder_Handler((seq_ui_encoder_t)button, 0);

      switch( button ) {
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
  // Trk. Type Steps/ParL/TrgL Port Chn. EditLayer  controls                         
  // G1T1 Note  256   4     8  IIC2  12  Name  D    Prob                PRESETS  INIT

  // Trk. Type Steps/ParL/TrgL Port Chn. EditLayer  controls                         
  // G1T1 Note  256   4     8  IIC2  12  Name  D    CC #001 (ModWheel)  PRESETS  INIT

  // Track Type "Note", Chord" and "CC":
  // Note: Parameter Layer A/B/C statically assigned to Note Number/Velocity/Length
  //       Parameter Layer D..P can be mapped to
  //       - additional Notes (e.g. for poly recording)
  //       - Base note (directly forwarded to Transposer, e.g. for Force-to-Scale or Chords)
  //       - CC (Number 000..127 assignable, GM controller name will be displayed)
  //       - Pitch Bender
  //       - Loopback (Number 000..127 assignable, name will be displayed)
  //       - Delay (0..95 ticks @384 ppqn)
  //       - Probability (100%..0%)
  //       - "Roll/Flam Effect" (a selection of up to 128 different effects selectable for each step separately)
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
  // Trk. Type StepsP/T  Drums Port Chn. EditLayA LayB  Drum Note VelN VelA PRE-    
  // G1T1 Drum  (64/2*64) 16   USB1  10  NamePrb. ----   BD   C-1  100  127 SETS INIT

  // Trk. Type StepsP/T  Drums Port Chn. EditLayA LayB  Drum Note VelN VelA PRE-         
  // G1T1 Drum  (2*64/256) 8   USB1  12  NameVel. Len.   SD   D-1  ---  --- SETS INIT


  // Track Type "Drums":
  //    1 or 2 parameter layers for each trigger layer (drum instrument).
  //    If parameters layers consist of more than 1024 steps in total, steps > 1024 will be mirrored
  //    assignable to Velocity/Gatelength/Delay/Probability/Roll/Flam only
  //    (no CC, no Pitch Bender, no Loopback, as they don't work note-wise, but 
  //     only channel wise
  //     just send them from a different track)

  // Each drum instrument provides 4 constant parameters for:
  //   - Note Number
  //   - MIDI Channel (1-16) for all drum instruments (TODO: optionally allow to use a "local" channel, edit parameter right of VelA)
  //   - Velocity Normal (if not taken from Parameter Layer)
  //   - Velocity Accented (if not taken from Parameter Layer)


  // Available Layer Constraints (Partitioning for 1024 bytes Par. memory, 2048 bits Trg. memory)
  //    - 16 Parameter Layers with 64 steps and 2*16 Trigger Layers A-P with 64 steps taken for Gate and Accent
  //    - 2*16 Parameter Layer with 32 steps and 16 Trigger Layers A-P with 128 steps taken for Gate
  //    - 8 Parameter Layer with 128 steps and 2*8 Trigger Layers A-P with 128 steps taken for Gate and Accent
  //    - 2*8 Parameter Layer with 64 steps and 8 Trigger Layers A-P with 256 steps taken for Gate



  // "Edit Name" layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Please enter Track Category for G1T1    <xxxxx-xxxxxxxxxxxxxxx>                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset DONE

  // "Edit Drum Name" layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Please enter Drum Label for G1T1- 1:C-1 <xxxxx>                                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset DONE


  // Preset dialogs:
  // Load Preset File (10 files found)       Trk.      SAVE AS                       
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx G1T1     NEW PRESET                 EXIT

  // Import selections:
  // Importing /PRESETS/xxxxx.V4T to G1T1    Name Chn. Maps Cfg. Steps
  // Please select track sections:           yes  yes  yes  yes   yes     IMPORT EXIT

  // "Save as new preset"
  // Please enter Filename:                  /PRESETS/<xxxxxxxx>.V4T                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins   SAVE EXIT

  // File exists
  //                                         File '/PRESETS/xxx.V4T' already exists     
  //                                         Overwrite? YES  NO                  EXIT




  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // if track has changed, switch layer config:
  if( visible_track != selected_layer_config_track ) {
    selected_layer_config_track = visible_track;
    selected_layer_config = GetLayerConfig(selected_layer_config_track);
  }

  u8 event_mode = layer_config[selected_layer_config].event_mode;


  switch( pr_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_EDIT_LABEL: {
      int i;

      SEQ_LCD_CursorSet(0, 0);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintString("Please enter Drum Label for ");
	SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	SEQ_LCD_PrintFormattedString("-%2d:", ui_selected_instrument + 1);
	SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument));
	SEQ_LCD_PrintSpaces(1);

	SEQ_LCD_PrintChar('<');
	for(i=0; i<5; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[5*ui_selected_instrument + i]);
	SEQ_LCD_PrintChar('>');
	SEQ_LCD_PrintSpaces(33);

	// insert flashing cursor
	if( ui_cursor_flash ) {
	  if( ui_edit_name_cursor >= 5 ) // correct cursor position if it is outside the 5 char label space
	    ui_edit_name_cursor = 0;
	  SEQ_LCD_CursorSet(41 + ui_edit_name_cursor, 0);
	  SEQ_LCD_PrintChar('*');
	}
      } else {
	if( ui_edit_name_cursor < 5 ) {
	  SEQ_LCD_PrintString("Please enter Track Category for ");
	  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	  SEQ_LCD_PrintSpaces(4);
	} else {
	  SEQ_LCD_PrintString("Please enter Track Label for ");
	  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
	  SEQ_LCD_PrintSpaces(7);
	}
	SEQ_LCD_PrintChar('<');
	for(i=0; i<5; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[i]);
	SEQ_LCD_PrintChar('-');
	for(i=5; i<20; ++i)
	  SEQ_LCD_PrintChar(seq_core_trk[visible_track].name[i]);
	SEQ_LCD_PrintChar('>');
	SEQ_LCD_PrintSpaces(17);
      }


      // insert flashing cursor
      if( ui_cursor_flash ) {
	SEQ_LCD_CursorSet(40 + ((ui_edit_name_cursor < 5) ? 1 : 2) + ui_edit_name_cursor, 0);
	SEQ_LCD_PrintChar('*');
      }

      SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_PrintString("Preset DONE");
    } break;


    ///////////////////////////////////////////////////////////////////////////
    case PR_DIALOG_PRESETS: {
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_CursorSet(0, 0);
      if( dir_num_items < 0 ) {
	if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	  SEQ_LCD_PrintString("/PRESETS directory not found on SD Card!");
	else
	  SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
      } else if( dir_num_items == 0 ) {
	SEQ_LCD_PrintFormattedString("No files found under /PRESETS!");
      } else {
	SEQ_LCD_PrintFormattedString("Select Preset File (%d files found)", dir_num_items);
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("Trk.      SAVE AS");
      SEQ_LCD_PrintSpaces(40-23);

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, dir_view_offset);

      SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
      SEQ_LCD_PrintString("     NEW PRESET                 EXIT");
    } break;


    case PR_DIALOG_IMPORT: {
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintFormattedString("Importing /PRESETS/%s.V4T to ", dir_name);
      SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
      SEQ_LCD_PrintSpaces(10);

      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("Name Chn. Maps Cfg. Steps");
      SEQ_LCD_PrintSpaces(15);

      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintFormattedString("Please select track sections:");
      SEQ_LCD_PrintSpaces(11);

      if( ui_selected_item == IMPORT_ITEM_NAME && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString(import_flags.NAME ? "yes" : " no");
      }
      SEQ_LCD_PrintSpaces(2);

      if( ui_selected_item == IMPORT_ITEM_CHN && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString(import_flags.CHN ? "yes" : " no");
      }
      SEQ_LCD_PrintSpaces(2);

      if( ui_selected_item == IMPORT_ITEM_MAPS && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString(import_flags.MAPS ? "yes" : " no");
      }
      SEQ_LCD_PrintSpaces(2);

      if( ui_selected_item == IMPORT_ITEM_CFG && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString(import_flags.CFG ? "yes" : " no");
      }
      SEQ_LCD_PrintSpaces(2);

      if( ui_selected_item == IMPORT_ITEM_STEPS && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(3);
      } else {
	SEQ_LCD_PrintString(import_flags.STEPS ? "yes" : " no");
      }

      SEQ_LCD_PrintSpaces(6);

      SEQ_LCD_PrintString("IMPORT EXIT");
    } break;


    case PR_DIALOG_EXPORT_FNAME: {
      int i;

      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintString("Please enter Filename:");
      SEQ_LCD_PrintSpaces(18);

      SEQ_LCD_PrintString("/PRESETS/<");
      for(i=0; i<8; ++i)
	SEQ_LCD_PrintChar(dir_name[i]);
      SEQ_LCD_PrintString(">.V4T");
      SEQ_LCD_PrintSpaces(20);

      // insert flashing cursor
      if( ui_cursor_flash ) {
	SEQ_LCD_CursorSet(50 + ui_edit_name_cursor, 0);
	SEQ_LCD_PrintChar('*');
      }

      SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_PrintString("  SAVE EXIT");
    } break;


    case PR_DIALOG_EXPORT_FEXISTS: {
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintFormattedString("File '/PRESETS/%s.V4T' already exists!", dir_name);
      SEQ_LCD_PrintSpaces(10);

      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintString("Overwrite? YES  NO");
      SEQ_LCD_PrintSpaces(18);
      SEQ_LCD_PrintString("EXIT");
    } break;



    ///////////////////////////////////////////////////////////////////////////
    default:

      SEQ_LCD_CursorSet(0, 0);

      SEQ_LCD_PrintString("Trk. Type ");
      SEQ_LCD_PrintString((event_mode == SEQ_EVENT_MODE_Drum) ? "StepsP/T  Drums " : "Steps/ParL/TrgL ");
      SEQ_LCD_PrintString("Port Chn. ");

      if( ui_selected_item == ITEM_EDIT_NAME && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintFormattedString("Edit");
      }

      if( selected_layer_config != GetLayerConfig(visible_track) ) {
	SEQ_LCD_PrintString("    Please initialize the track         ");
      } else if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintString("LayA ");
	SEQ_LCD_PrintString((layer_config[selected_layer_config].par_layers >= 2) ? "LayB " : "     ");
	SEQ_LCD_PrintString(" Drum Note VelN VelA PRE-    ");
      } else {
	SEQ_LCD_PrintString("Layer  controls                         ");
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
	SEQ_LCD_PrintString((char *)SEQ_LAYER_GetEvntModeName(event_mode));

	layer_config_t *lc = (layer_config_t *)&layer_config[selected_layer_config];
	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  SEQ_LCD_PrintChar(' ');
	  SEQ_LCD_PrintChar('(');
	  SEQ_LCD_PrintSpaces(14); // for easier handling
	  SEQ_LCD_CursorSet(12, 1);
	  
	  if( lc->par_layers > 1 )
	    SEQ_LCD_PrintFormattedString("%d*", lc->par_layers);
	  SEQ_LCD_PrintFormattedString("%d/", lc->par_steps);
	  if( lc->trg_layers > 1 )
	    SEQ_LCD_PrintFormattedString("%d*", lc->trg_layers);
	  SEQ_LCD_PrintFormattedString("%d", lc->trg_steps);
	  
	  SEQ_LCD_PrintFormattedString(") %d", lc->instruments);
	  SEQ_LCD_CursorSet(26, 1);
	} else {
	  SEQ_LCD_PrintFormattedString("  %3d %3d  %3d  ", lc->par_steps, lc->par_layers, lc->trg_layers);
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
      if( ui_selected_item == ITEM_EDIT_NAME && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintFormattedString("Name");
      }

      ///////////////////////////////////////////////////////////////////////////
      if( selected_layer_config != GetLayerConfig(visible_track) ) {
	SEQ_LCD_PrintString(" ---------------------------------> ");
	if( (ui_cursor_flash_overrun_ctr & 1) ) {
	  SEQ_LCD_PrintSpaces(4);
	} else {
	  SEQ_LCD_PrintString("INIT");
	}

        ///////////////////////////////////////////////////////////////////////////
      } else if( event_mode == SEQ_EVENT_MODE_Drum ) {
	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_LAYER_A_SELECT && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, 0));
	}

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_LAYER_B_SELECT && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  if( layer_config[selected_layer_config].par_layers >= 2 ) {
	    SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, 1));
	  } else {
	    if( ui_selected_item == ITEM_LAYER_B_SELECT )
	      SEQ_LCD_PrintString("---- "); // not a bug, but a feature - highlight, that layer not configurable
	    else
	      SEQ_LCD_PrintSpaces(5);
	  }
	}
	SEQ_LCD_PrintSpaces(1);

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_DRUM_SELECT && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
	}

	/////////////////////////////////////////////////////////////////////////
	SEQ_LCD_PrintSpaces(1);
	if( ui_selected_item == ITEM_DRUM_NOTE && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(3);
	} else {
	  SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument));
	}
	SEQ_LCD_PrintSpaces(1);

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_DRUM_VEL_N && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_instrument));
	}

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_DRUM_VEL_A && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_C1 + ui_selected_instrument));
	}

	SEQ_LCD_PrintString("SETS INIT");
      } else {
	/////////////////////////////////////////////////////////////////////////
	SEQ_LCD_PrintSpaces(2);
	if( ui_selected_item == ITEM_LAYER_SELECT && ui_cursor_flash ) {
	  SEQ_LCD_PrintChar(' ');
	} else {
	  SEQ_LCD_PrintChar('A' + ui_selected_par_layer);
	}
	SEQ_LCD_PrintSpaces(4);

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_LAYER_CONTROL && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, ui_selected_par_layer));
	}

	/////////////////////////////////////////////////////////////////////////
	if( ui_selected_item == ITEM_LAYER_PAR && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(15);
	} else {
	  switch( SEQ_PAR_AssignmentGet(visible_track, ui_selected_par_layer) ) {
            case SEQ_PAR_Type_CC: {
	      mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
	      u8 current_value = SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_par_layer);
	      u8 edit_value = ui_selected_item == ITEM_LAYER_PAR ? edit_cc_number : current_value;
	      SEQ_LCD_PrintFormattedString("%03d%c(%s) ", 
					   edit_value,
					   (current_value != edit_value) ? '!' : ' ',
					   SEQ_CC_LABELS_Get(port, edit_value));
	    } break;

            default:
	      SEQ_LCD_PrintSpaces(15);
	  }
	}

	SEQ_LCD_PrintString("PRESETS  INIT");
      }
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

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  selected_layer_config_track = SEQ_UI_VisibleTrackGet();
  selected_layer_config = GetLayerConfig(selected_layer_config_track);

  // initialize edit label vars (name is modified directly, not via ui_edit_name!)
  ui_edit_name_cursor = 0;
  ui_edit_preset_num_category = 0;
  ui_edit_preset_num_label = 0;

  dir_name[0] = 0;
  pr_dialog = 0;

  import_flags.ALL = 0xff;

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

  layer_config_t *lc = (layer_config_t *)&layer_config[selected_layer_config];

  // partitionate layers and clear all steps
  SEQ_PAR_TrackInit(track, lc->par_steps, lc->par_layers, lc->instruments);
  SEQ_TRG_TrackInit(track, lc->trg_steps, lc->trg_layers, lc->instruments);

#if 0
  // confusing if assignments are not changed under such a condition
  u8 init_assignments = lc->event_mode != SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
#else
  u8 init_assignments = 1;
#endif

  SEQ_CC_Set(track, SEQ_CC_MIDI_EVENT_MODE, lc->event_mode);

  // BEGIN TMP
  if( lc->event_mode == SEQ_EVENT_MODE_Drum ) {
    int i;

    for(i=0; i<16; ++i)
      SEQ_LABEL_CopyPresetDrum(i, (char *)&seq_core_trk[track].name[5*i]);
  } else {
    memset((char *)seq_core_trk[track].name, ' ', 80);
  }
  // END TMP

  u8 only_layers = 0;
  u8 all_triggers_cleared = 0;
  return SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);
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
  u8 num_instruments = SEQ_PAR_NumInstrumentsGet(track);

  for(i=0; i<(sizeof(layer_config)/sizeof(layer_config_t)); ++i) {
    layer_config_t *lc = (layer_config_t *)&layer_config[i];
    if( lc->event_mode == event_mode &&
	lc->par_layers == par_layers &&	lc->par_steps == par_steps &&
	lc->trg_layers == trg_layers &&	lc->trg_steps == trg_steps &&
	lc->instruments == num_instruments ) {
      return i;
    }
  }

  return 0; // not found, return index to first config
}

/////////////////////////////////////////////////////////////////////////////
// help function for Init button function
/////////////////////////////////////////////////////////////////////////////
static void InitReq(u32 dummy)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // TODO: copy preset for all selected tracks!
  CopyPreset(visible_track, selected_layer_config);

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Track has been", "initialized!");
}


/////////////////////////////////////////////////////////////////////////////
// help function to scan presets directory
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_TRKEVNT_UpdateDirList(void)
{
  int item;

  MUTEX_SDCARD_TAKE;
  dir_num_items = SEQ_FILE_GetFiles("/PRESETS", "V4T", (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
  MUTEX_SDCARD_GIVE;

  if( dir_num_items < 0 )
    item = 0;
  else if( dir_num_items < NUM_LIST_DISPLAYED_ITEMS )
    item = dir_num_items;
  else
    item = NUM_LIST_DISPLAYED_ITEMS;

  while( item < NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to start Track preset file export
// returns 0 on success
// returns != 0 on errors (dialog page will be changed accordingly)
/////////////////////////////////////////////////////////////////////////////
static s32 DoExport(u8 force_overwrite)
{
  s32 status;
  int i;
  char path[20];

  // if an error is detected, we jump back to FNAME page
  ui_selected_item = 0;
  pr_dialog = PR_DIALOG_EXPORT_FNAME;

  u8 filename_valid = 1;
  for(i=0; i<8; ++i)
    if( dir_name[i] == '.' || dir_name[i] == '?' || dir_name[i] == ',' || dir_name[i] == '!' )
      filename_valid = 0;

  if( !filename_valid ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Filename not valid!", "(remove . ? , !)");
    return -1;
  }

  if( dir_name[0] == ' ') {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Please enter", "Filename!");
    return -2;
  }

  strcpy(path, "/PRESETS");
  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_MakeDir(path); // create directory if it doesn't exist
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -3;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "/PRESETS directory", "cannot be created!");
    return -4;
  }

  char v4t_file[20];
  char *p = (char *)&v4t_file[0];
  for(i=0; i<8; ++i) {
    char c = dir_name[i];
    if( c != ' ' )
      *p++ = c;
  }
  *p++ = 0;

  sprintf(path, "/PRESETS/%s.v4t", v4t_file);

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_FileExists(path);
  MUTEX_SDCARD_GIVE;
	    
  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -5;
  }


  if( !force_overwrite && status == 1) {
    // file exists - ask if it should be overwritten in a special dialog page
    pr_dialog = PR_DIALOG_EXPORT_FEXISTS;
    return 1;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Exporting", path);

  MUTEX_SDCARD_TAKE;
  status=SEQ_FILE_T_Write(path, SEQ_UI_VisibleTrackGet());
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Error during Export!", "see MIOS Terminal!");
    return -6;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Export", "successfull!");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to start Track preset file export
// returns 0 on success
// returns != 0 on errors (dialog page will be changed accordingly)
/////////////////////////////////////////////////////////////////////////////
static s32 DoImport(void)
{
  s32 status;
  char path[20];
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  sprintf(path, "/PRESETS/%s.V4T", dir_name);

  // mute track to avoid random effects while loading the file
  MIOS32_IRQ_Disable(); // this operation should be atomic!
  u8 muted = seq_core_trk[visible_track].state.MUTED;
  if( !muted )
    seq_core_trk[visible_track].state.MUTED = 1;
  MIOS32_IRQ_Enable();

  // read file
  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_T_Read(path, visible_track, import_flags);
  MUTEX_SDCARD_GIVE;

  // unmute track if it wasn't muted before
  MIOS32_IRQ_Disable(); // this operation should be atomic!
  if( !muted )
    seq_core_trk[visible_track].state.MUTED = 0;
  MIOS32_IRQ_Enable();

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
  } else {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, path, "imported!");

    // fix layer config
    selected_layer_config_track = visible_track;
    selected_layer_config = GetLayerConfig(selected_layer_config_track);
  }

  return 0; // no error
}
