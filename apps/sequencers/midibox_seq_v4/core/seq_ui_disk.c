// $Id$
/*
 * Disk Utility page
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

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_midply.h"
#include "seq_midexp.h"
#include "seq_midimp.h"
#include "seq_midi_port.h"
#include "seq_bpm.h"

#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_song.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           6
#define ITEM_S_IMPORT          0
#define ITEM_S_EXPORT          1
#define ITEM_MF_IMPORT         2
#define ITEM_MF_EXPORT         3
#define ITEM_MF_PLAY           4
#define ITEM_ENABLE_MSD        5

#define EXPORT_NUM_OF_ITEMS    6
#define EXPORT_ITEM_MODE       0
#define EXPORT_ITEM_VAL        1
#define EXPORT_ITEM_BANK       2
#define EXPORT_ITEM_PATTERN    3
#define EXPORT_ITEM_MEASURES   4
#define EXPORT_ITEM_STEPS_P_M  5

#define SESSION_NUM_OF_ITEMS    3
#define SESSION_ITEM_PATTERN_B    0
#define SESSION_ITEM_PATTERN_E    1
#define SESSION_ITEM_PATTERN_D    2

// Session and MIDI File dialog screens
#define DIALOG_NONE               0
#define DIALOG_S_IMPORT           1
#define DIALOG_S_EXPORT           2
#define DIALOG_MF_PLAY            3
#define DIALOG_MF_IMPORT          4
#define DIALOG_MF_EXPORT          5
#define DIALOG_MF_EXPORT_FNAME    6
#define DIALOG_MF_EXPORT_FEXISTS  7
#define DIALOG_MF_EXPORT_PROGRESS 8


#define NUM_LIST_DISPLAYED_ITEMS 4
#define LIST_ENTRY_WIDTH 9



/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void MSD_EnableReq(u32 enable);

static s32 SEQ_UI_DISK_UpdateSessionDirList(void);
static s32 SEQ_UI_DISK_UpdateMfDirList(void);

static s32 DoMfExport(u8 force_overwrite);
static s32 DoSessionCopy(char *from_session, u16 from_pattern, char *to_session, u16 to_pattern, u16 num_patterns);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 menu_dialog;

static s32 dir_num_items; // contains SEQ_FILE error status if < 0
static u8 dir_view_offset = 0; // only changed once after startup
static u8 dir_selected_item = 0; // only changed once after startup
static char dir_name[12]; // directory name of device (first char is 0 if no device selected)

#define MAX_SESSION_PATTERNS SEQ_FILE_B_NUM_BANKS*64
static u16 source_pattern_begin = 0; // only changed once after startup
static u16 source_pattern_end = 0;   // only changed once after startup
static u16 destination_pattern = 0;  // only changed once after startup


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( menu_dialog ) {
  case DIALOG_S_IMPORT:
  case DIALOG_S_EXPORT:
    *gp_leds = (3 << (2*dir_selected_item));
    switch( ui_selected_item ) {
    case SESSION_ITEM_PATTERN_B:
      *gp_leds |= 0x0100;
      break;
    case SESSION_ITEM_PATTERN_E:
      *gp_leds |= 0x0200;
      break;
    case SESSION_ITEM_PATTERN_D:
      *gp_leds |= 0x0c00;
      break;
    }
    break;

  case DIALOG_MF_PLAY:
    *gp_leds = (3 << (2*dir_selected_item));
    break;

  case DIALOG_MF_IMPORT:
    *gp_leds = (3 << (2*dir_selected_item));
    break;

  case DIALOG_MF_EXPORT:
    switch( ui_selected_item ) {
    case EXPORT_ITEM_MODE:
      *gp_leds |= 0x0001;
      break;
    case EXPORT_ITEM_VAL:
      *gp_leds |= 0x0002;
      break;
    case EXPORT_ITEM_BANK:
      *gp_leds |= 0x0004;
      break;
    case EXPORT_ITEM_PATTERN:
      *gp_leds |= 0x0008;
      break;
    case EXPORT_ITEM_MEASURES:
      *gp_leds |= 0x0030;
      break;
    case EXPORT_ITEM_STEPS_P_M:
      *gp_leds |= 0x00c0;
      break;
    }
    break;

  case DIALOG_MF_EXPORT_FNAME:
  case DIALOG_MF_EXPORT_FEXISTS:
  case DIALOG_MF_EXPORT_PROGRESS:
    // no LED functions
    break;

  default: {
#if 0
    switch( ui_selected_item ) {
    case ITEM_S_IMPORT: *gp_leds |= 0x0001; break;
    case ITEM_S_EXPORT: *gp_leds |= 0x0006; break;
    case ITEM_MF_IMPORT: *gp_leds |= 0x0010; break;
    case ITEM_MF_EXPORT: *gp_leds |= 0x0060; break;
    case ITEM_MF_PLAY: *gp_leds |= 0x0080; break;
    case ITEM_ENABLE_MSD: *gp_leds |= 0x0300; break;
    }
#else
    s32 status = TASK_MSD_EnableGet();
    if( status == 1 )
      *gp_leds |= 0x0300;
#endif
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
  switch( menu_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_S_IMPORT:
    case DIALOG_S_EXPORT:
      if( encoder <= SEQ_UI_ENCODER_GP8 ) {
	if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &dir_selected_item, &dir_view_offset) )
	  SEQ_UI_DISK_UpdateSessionDirList();
	return 1;
      }

      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = SESSION_ITEM_PATTERN_B;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = SESSION_ITEM_PATTERN_E;
	break;

      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12:
	ui_selected_item = SESSION_ITEM_PATTERN_D;
	break;

      case SEQ_UI_ENCODER_GP13:
	return -1; // not mapped (yet)

      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
	// IMPORT/EXPORT only via button
	if( incrementer == 0 ) {

	  if( dir_num_items >= 1 && (dir_selected_item+dir_view_offset) < dir_num_items ) {

	    // just to ensure
	    int num_patterns = source_pattern_end - source_pattern_begin + 1;
	    if( destination_pattern + num_patterns >= MAX_SESSION_PATTERNS )
	      num_patterns = MAX_SESSION_PATTERNS - destination_pattern;

	    if( num_patterns < 1 ) {
	      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Invalid End Pattern", "selected!");
	    } else {
	      // Import/Export file
	      char session_dir[30];
	      int i;
	      char *p = (char *)&session_dir[0];
	      for(i=0; i<8; ++i) {
		char c = ui_global_dir_list[LIST_ENTRY_WIDTH*dir_selected_item + i];
		if( c != ' ' )
		  *p++ = c;
	      }
	      *p++ = 0;

	      MUTEX_SDCARD_TAKE;
	      if( menu_dialog == DIALOG_S_IMPORT ) {
		DoSessionCopy(session_dir, source_pattern_begin, seq_file_session_name, destination_pattern, num_patterns);
	      } else {
		DoSessionCopy(seq_file_session_name, source_pattern_begin, session_dir, destination_pattern, num_patterns);
	      }
	      MUTEX_SDCARD_GIVE;

#if 0
	      // we want to stay in this menu
	      ui_selected_item = 0;
	      menu_dialog = DIALOG_NONE;
#endif
	    }
	  } else {
	    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Invalid directory", "selected!");
	  }
	}
	return 1;

      case SEQ_UI_ENCODER_GP16:
	// EXIT only via button
	if( incrementer == 0 ) {
	  ui_selected_item = 0;
	  menu_dialog = DIALOG_NONE;
	}
	return 1;
      }


      switch( ui_selected_item ) {
      case SESSION_ITEM_PATTERN_B: {
	int old_offset = source_pattern_end-source_pattern_begin;
	if( old_offset < 0 )
	  old_offset = 0;
	if( SEQ_UI_Var16_Inc(&source_pattern_begin, 0, MAX_SESSION_PATTERNS-1, incrementer) ) {
	  source_pattern_end = source_pattern_begin + old_offset;
	  if( source_pattern_end >= MAX_SESSION_PATTERNS )
	    source_pattern_end = MAX_SESSION_PATTERNS - 1;
	  return 1;
	}
      }	return 0;

      case SESSION_ITEM_PATTERN_E:
	if( SEQ_UI_Var16_Inc(&source_pattern_end, source_pattern_begin, MAX_SESSION_PATTERNS-1, incrementer) ) {
	  return 1;
	}
	return 0;

      case SESSION_ITEM_PATTERN_D:
	if( SEQ_UI_Var16_Inc(&destination_pattern, 0, MAX_SESSION_PATTERNS-1, incrementer) ) {
	  return 1;
	}
	return 0;
      }

      return -1; // encoder not mapped

    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_PLAY:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9: {
	  // select Play/Stop
	  u8 value = SEQ_MIDPLY_RunModeGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) ) {
	    SEQ_MIDPLY_RunModeSet(value, 1);
	    if( value ) {
	      MUTEX_MIDIOUT_TAKE;
	      if( !SEQ_BPM_IsRunning() ) {
		// and start sequencer
		SEQ_BPM_CheckAutoMaster();
		SEQ_BPM_Start();
	      }
	      MUTEX_MIDIOUT_GIVE;

	      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Playing:", SEQ_MIDPLY_PathGet());

	    }
	    return 1;
	  }
	  return 0;
	} break;

        case SEQ_UI_ENCODER_GP10: {
	  // enable/disable loop mode
	  u8 value = SEQ_MIDPLY_LoopModeGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) >= 0 ) {
	    SEQ_MIDPLY_LoopModeSet(value);
	    return 1; // value changed
	  }
	  return 0; // no change
	} break;

        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12: {
	  // select exclusive/parallel
	  u8 value = SEQ_MIDPLY_ModeGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) ) {
	    SEQ_MIDPLY_ModeSet(value);
	    return 1;
	  }
	  return 0;
	} break;


        case SEQ_UI_ENCODER_GP13: {
	  // select MIDI port
	  u8 port_ix = SEQ_MIDI_PORT_OutIxGet(SEQ_MIDPLY_PortGet());
	  if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	    SEQ_MIDPLY_PortSet(SEQ_MIDI_PORT_OutPortGet(port_ix));
	    return 1; // value changed
	  }
	  return 0; // no change
	} break;

        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped (yet)

        case SEQ_UI_ENCODER_GP16:
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    menu_dialog = DIALOG_NONE;
	  }
	  return 1;

        default:
	  if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &dir_selected_item, &dir_view_offset) )
	    SEQ_UI_DISK_UpdateMfDirList();
      }
      return 0;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_IMPORT:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9: {
	  u8 value = SEQ_MIDIMP_ModeGet();

	  if( incrementer == 0 ) { // toggle function via button
	    if( value >= 1 )
	      incrementer = -value;
	    else
	      incrementer = 1;
	  }

	  if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) ) {
	    SEQ_MIDIMP_ModeSet(value);
	    return 1;
	  }
	  return 0;
	} break;

        case SEQ_UI_ENCODER_GP10:
        case SEQ_UI_ENCODER_GP11: {
	  // select number of layers (4/8/16)
	  u8 value = SEQ_MIDIMP_NumLayersGet();
	  u8 selection = 0;
	  if( value == 8 )
	    selection = 1;
	  if( value == 16 )
	    selection = 2;

	  if( incrementer == 0 ) { // toggle function via button
	    if( selection >= 2 )
	      incrementer = -selection;
	    else
	      incrementer = 1;
	  }

	  if( SEQ_UI_Var8_Inc(&selection, 0, 2, incrementer) ) {
	    u8 num_layers = 4;
	    if( selection == 1 )
	      num_layers = 8;
	    else if( selection == 2 )
	      num_layers = 16;
	    SEQ_MIDIMP_NumLayersSet(num_layers);
	    return 1;
	  }
	  return 0;
	} break;

        case SEQ_UI_ENCODER_GP12:
        case SEQ_UI_ENCODER_GP13: {
	  u8 value = SEQ_MIDIMP_ResolutionGet();

	  if( incrementer == 0 ) { // toggle function via button
	    if( value >= 2 )
	      incrementer = -value;
	    else
	      incrementer = 1;
	  }

	  if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) ) {
	    SEQ_MIDIMP_ResolutionSet(value);
	    return 1;
	  }
	  return 0;
	} break;

        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped (yet)

        case SEQ_UI_ENCODER_GP16:
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    menu_dialog = DIALOG_NONE;
	  }
	  return 1;

        default:
	  if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &dir_selected_item, &dir_view_offset) )
	    SEQ_UI_DISK_UpdateMfDirList();
      }


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT: {
      seq_midexp_mode_t midexp_mode = SEQ_MIDEXP_ModeGet();

      switch( encoder ) {
        case SEQ_UI_ENCODER_GP1:
          ui_selected_item = EXPORT_ITEM_MODE;
          break;

        case SEQ_UI_ENCODER_GP2:
          ui_selected_item = EXPORT_ITEM_VAL;
          break;
    
        case SEQ_UI_ENCODER_GP3:
          ui_selected_item = EXPORT_ITEM_BANK;
          break;
    
        case SEQ_UI_ENCODER_GP4:
          ui_selected_item = EXPORT_ITEM_PATTERN;
          break;
    
        case SEQ_UI_ENCODER_GP5:
        case SEQ_UI_ENCODER_GP6:
          ui_selected_item = EXPORT_ITEM_MEASURES;
          break;
    
        case SEQ_UI_ENCODER_GP7:
        case SEQ_UI_ENCODER_GP8:
          ui_selected_item = EXPORT_ITEM_STEPS_P_M;
          break;
    
        case SEQ_UI_ENCODER_GP9:
        case SEQ_UI_ENCODER_GP10:
	  // CONTINUE only via button
	  if( incrementer == 0 ) {
	    int i;

	    // initialize keypad editor
	    SEQ_UI_KeyPad_Init();
	    for(i=0; i<8; ++i)
	      dir_name[i] = ' ';

	    ui_selected_item = 0;
	    menu_dialog = DIALOG_MF_EXPORT_FNAME;
	  }
	  return 1;

        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12:
        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped

        case SEQ_UI_ENCODER_GP16:
	  // EXIT only via button
	  if( incrementer == 0 ) {
	    ui_selected_item = 0;
	    menu_dialog = DIALOG_NONE;
	  }
	  return 1;
      }
    
      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
        case EXPORT_ITEM_MODE: {
	  u8 value = (u8)SEQ_MIDEXP_ModeGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 3, incrementer) >= 0 ) {
	    SEQ_MIDEXP_ModeSet(value);
	    return 1; // value changed
	  }
	  return 0; // no change
        }

        case EXPORT_ITEM_VAL: {
	  switch( midexp_mode ) {
	    case SEQ_MIDEXP_MODE_AllGroups: {
	      return 0; // not relevant
	    }
	    case SEQ_MIDEXP_MODE_Group: {
	      if( SEQ_UI_Var8_Inc(&ui_selected_group, 0, SEQ_CORE_NUM_GROUPS-1, incrementer) >= 0 ) {
		return 1; // value changed
	      }
	      return 0; // no change
	    }
	    case SEQ_MIDEXP_MODE_Track: {
	      return SEQ_UI_GxTyInc(incrementer);
	    }
	    case SEQ_MIDEXP_MODE_Song: {
	      u8 value = (u8)SEQ_SONG_NumGet();
	      if( SEQ_UI_Var8_Inc(&value, 0, SEQ_SONG_NUM-1, incrementer) >= 0 ) {
		SEQ_SONG_Load(value);
		return 1; // value changed
	      }
	      return 0; // no change
	    }
	  }
        }
    
        case EXPORT_ITEM_BANK:
        case EXPORT_ITEM_PATTERN: {
	  if( midexp_mode != SEQ_MIDEXP_MODE_Group )
	    return -1; // not mapped

	  // change bank/pattern number
	  seq_pattern_t *pattern = &seq_pattern[ui_selected_group];
	  if( ui_selected_item == EXPORT_ITEM_PATTERN ) {
	    u8 tmp = pattern->pattern;
	    u8 max_patterns = SEQ_FILE_B_NumPatterns(pattern->bank);
	    // TODO: print error message if bank not valid (max_patterns = 0)
	    if( !max_patterns || !SEQ_UI_Var8_Inc(&tmp, 0, max_patterns-1, incrementer) ) {
	      return 0; // no change
	    }
	    pattern->pattern = tmp;
	  } else {
	    u8 tmp = pattern->bank;
	    if( !SEQ_UI_Var8_Inc(&tmp, 0, SEQ_FILE_B_NUM_BANKS-1, incrementer) ) {
	      return 0; // no change
	    }
	    pattern->bank = tmp;
	  }

	  // always switch unsynched
	  pattern->SYNCHED = 0;
	  SEQ_PATTERN_Change(ui_selected_group, *pattern, 0);
	  return 1;
        }
    
        case EXPORT_ITEM_MEASURES: {
	  u16 value = (u16)SEQ_MIDEXP_ExportMeasuresGet();
	  if( SEQ_UI_Var16_Inc(&value, 0, 9999, incrementer) >= 0 ) {
	    SEQ_MIDEXP_ExportMeasuresSet(value);
	    return 1; // value changed
	  }
	  return 0; // no change
        }
    
        case EXPORT_ITEM_STEPS_P_M: {
	  u8 value = (u8)SEQ_MIDEXP_ExportStepsPerMeasureGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) >= 0 ) {
	    SEQ_MIDEXP_ExportStepsPerMeasureSet(value);
	    return 1; // value changed
	  }
	  return 0; // no change
        }
      }

      return -1; // invalid or unsupported encoder
    }


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_FNAME: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP15: {
	  // SAVE only via button
	  if( incrementer != 0 )
	    return 0;

	  if( DoMfExport(0) == 0 ) { // don't force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    menu_dialog = DIALOG_NONE;
	  }

	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer != 0 )
	    return 0;

	  // exit keypad editor
	  ui_selected_item = 0;
	  menu_dialog = DIALOG_NONE;
	  return 1;
	} break;
      }

      return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&dir_name[0], 8);
    }
    return 0;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_FEXISTS: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP11: {
	  // YES only via button
	  if( incrementer != 0 )
	    return 0;

	  // YES: overwrite file
	  if( DoMfExport(1) == 0 ) { // force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    menu_dialog = DIALOG_NONE;
	  }
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP12: {
	  // NO only via button
	  if( incrementer != 0 )
	    return 0;

	  // NO: don't overwrite file - back to filename entry

	  ui_selected_item = 0;
	  menu_dialog = DIALOG_MF_EXPORT_FNAME;
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer != 0 )
	    return 0;

	  // exit keypad editor
	  ui_selected_item = 0;
	  menu_dialog = DIALOG_NONE;
	  return 1;
	} break;
      }

      return -1; // not mapped
    }


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_PROGRESS:
      // no encoder functions!
      return -1;


    ///////////////////////////////////////////////////////////////////////////
    default:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP1:
          ui_selected_item = ITEM_S_IMPORT;
          break;
    
        case SEQ_UI_ENCODER_GP2:
        case SEQ_UI_ENCODER_GP3:
          ui_selected_item = ITEM_S_EXPORT;
	  break;

        case SEQ_UI_ENCODER_GP4:
          return -1; // not used (yet)
	  break;

        case SEQ_UI_ENCODER_GP5:
          ui_selected_item = ITEM_MF_IMPORT;
          break;
    
        case SEQ_UI_ENCODER_GP6:
        case SEQ_UI_ENCODER_GP7:
          ui_selected_item = ITEM_MF_EXPORT;
          break;
    
        case SEQ_UI_ENCODER_GP8:
          ui_selected_item = ITEM_MF_PLAY;
          break;

        case SEQ_UI_ENCODER_GP9:
        case SEQ_UI_ENCODER_GP10:
          ui_selected_item = ITEM_ENABLE_MSD;
          break;
    
        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12:
        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
        case SEQ_UI_ENCODER_GP16:
          return -1; // not used (yet)
        
      }
    
      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
        case ITEM_S_IMPORT:
          // switch to Session Import Dialog screen
	  dir_selected_item = 0;
	  dir_view_offset = 0;
	  ui_selected_item = 0;
          menu_dialog = DIALOG_S_IMPORT;
          SEQ_UI_DISK_UpdateSessionDirList();
          return 1;
    
        case ITEM_S_EXPORT:
          // switch to Session Import Dialog screen
	  dir_selected_item = 0;
	  dir_view_offset = 0;
	  ui_selected_item = 0;
          menu_dialog = DIALOG_S_EXPORT;
          SEQ_UI_DISK_UpdateSessionDirList();
          return 1;

        case ITEM_MF_IMPORT:
          // switch to MIDI File Import Dialog screen
	  dir_selected_item = 0;
	  dir_view_offset = 0;
	  ui_selected_item = 0;
          menu_dialog = DIALOG_MF_IMPORT;
          SEQ_UI_DISK_UpdateMfDirList();
          return 1;
    
        case ITEM_MF_EXPORT:
          // switch to MIDI File Export Dialog screen
	  dir_selected_item = 0;
	  dir_view_offset = 0;
	  ui_selected_item = 0;
          menu_dialog = DIALOG_MF_EXPORT;
          return 1;

        case ITEM_MF_PLAY:
          // switch to MIDI File Play Dialog screen
	  dir_selected_item = 0;
	  dir_view_offset = 0;
	  ui_selected_item = 0;
          menu_dialog = DIALOG_MF_PLAY;
          SEQ_UI_DISK_UpdateMfDirList();
          return 1;    

        case ITEM_ENABLE_MSD:
          SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Please use", "GP Button!");
          return 1;    
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
  switch( menu_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_S_IMPORT:
    case DIALOG_S_EXPORT:
      if( depressed ) return 0; // ignore when button depressed
    
      if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {

	if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 )
	  return Encoder_Handler(button, 0);
	else {
	  if( button != SEQ_UI_BUTTON_Select )
	    dir_selected_item = button / 2;
	}
	return 1;
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_PLAY:
      if( depressed ) return 0; // ignore when button depressed
    
      if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {

	switch( button ) {
	  case SEQ_UI_BUTTON_GP9:
	    // cycle run mode
	    return Encoder_Handler(button, SEQ_MIDPLY_RunModeGet() ? -1 : 1);
	    
          case SEQ_UI_BUTTON_GP10:
	    // cycle loop mode
	    return Encoder_Handler(button, SEQ_MIDPLY_LoopModeGet() ? -1 : 1);

	  case SEQ_UI_BUTTON_GP11:
	  case SEQ_UI_BUTTON_GP12: {
	    // cycle exclusive/parallel
	    switch( SEQ_MIDPLY_ModeGet() ) {
	      case SEQ_MIDPLY_MODE_Exclusive:
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Parallel);
		break;
	      default:
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Exclusive);
		break;
	    }
	    return 1;
	  } break;

          case SEQ_UI_BUTTON_GP13:
	    // increment MIDI port
	    return Encoder_Handler(button, 1);

          case SEQ_UI_BUTTON_GP14:
          case SEQ_UI_BUTTON_GP15:
          case SEQ_UI_BUTTON_GP16:
	    return Encoder_Handler(button, 0);

          default:
	    if( button != SEQ_UI_BUTTON_Select )
	      dir_selected_item = button / 2;

	    if( dir_num_items >= 1 && (dir_selected_item+dir_view_offset) < dir_num_items ) {
	      // Play MIDI File
	      char mid_file[30];
	      int i;
	      char *p = (char *)&mid_file[0];
	      for(i=0; i<8; ++i) {
		char c = ui_global_dir_list[LIST_ENTRY_WIDTH*dir_selected_item + i];
		if( c != ' ' )
		  *p++ = c;
	      }
	      *p++ = 0;

	      char path[40];
	      sprintf(path, "/MIDI/%s.MID", mid_file);

	      MUTEX_MIDIOUT_TAKE;
	      s32 status;

	      // load file
	      status = SEQ_MIDPLY_ReadFile(path);

	      // turn on MIDI play run mode if file is valid
	      if( status >= 0 ) {
		u8 sequencer_running = SEQ_BPM_IsRunning();

		SEQ_MIDPLY_RunModeSet(1, sequencer_running);

		if( !sequencer_running ) {
		  // and start sequencer
		  SEQ_BPM_CheckAutoMaster();
		  SEQ_BPM_Start();
		}
	      } else {
		SEQ_MIDPLY_RunModeSet(0, 0);
	      }

	      MUTEX_MIDIOUT_GIVE;
	      if( status >= 0 )
		SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Playing:", SEQ_MIDPLY_PathGet());
	      else
		SEQ_UI_SDCardErrMsg(2000, status);
	    }
	}
	return 1;
      }
      break;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_IMPORT:
      if( depressed ) return 0; // ignore when button depressed
    
      if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {

	if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 )
	    return Encoder_Handler(button, 0);
	else {
	  if( button != SEQ_UI_BUTTON_Select )
	    dir_selected_item = button / 2;

	  if( dir_num_items >= 1 && (dir_selected_item+dir_view_offset) < dir_num_items ) {
	    // Import MIDI File
	    char mid_file[30];
	    int i;
	    char *p = (char *)&mid_file[0];
	    for(i=0; i<8; ++i) {
	      char c = ui_global_dir_list[LIST_ENTRY_WIDTH*dir_selected_item + i];
	      if( c != ' ' )
		*p++ = c;
	    }
	    *p++ = 0;

	    char path[40];
	    sprintf(path, "/MIDI/%s.MID", mid_file);

	    s32 status = SEQ_MIDIMP_ReadFile(path);

	    if( status >= 0 )
	      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Imported:", path);
	    else
	      SEQ_UI_SDCardErrMsg(2000, status);
	  }
	}
	return 1;
      }
      break;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT:
      if( depressed ) return 0; // ignore when button depressed
    
      if( button <= SEQ_UI_BUTTON_GP16 )
	return Encoder_Handler(button, 0); // re-use encoder handler

      switch( button ) {
        case SEQ_UI_BUTTON_Select:
        case SEQ_UI_BUTTON_Right:
          if( ++ui_selected_item >= EXPORT_NUM_OF_ITEMS )
	    ui_selected_item = 0;
    
          return 1; // value always changed
    
        case SEQ_UI_BUTTON_Left:
          if( ui_selected_item == 0 )
	    ui_selected_item = EXPORT_NUM_OF_ITEMS-1;
    
          return 1; // value always changed
    
        case SEQ_UI_BUTTON_Up:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);
    
        case SEQ_UI_BUTTON_Down:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
      }
      break;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_FNAME:
    case DIALOG_MF_EXPORT_FEXISTS:
      if( button <= SEQ_UI_BUTTON_GP16 ) {
	if( depressed ) return 0; // ignore when button depressed
	return Encoder_Handler((seq_ui_encoder_t)button, 0);
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_PROGRESS:
      // no GP button functions!
      if( button <= SEQ_UI_BUTTON_GP16 )
	return -1;

      break;


    ///////////////////////////////////////////////////////////////////////////
    default:
      if( button <= SEQ_UI_BUTTON_GP16 ) {
        s32 incrementer = 0;
    
        // GP9/10 toggles MSD enable/disable
        if( button == SEQ_UI_BUTTON_GP9 || button == SEQ_UI_BUTTON_GP10 ) {
	  u8 do_enable = TASK_MSD_EnableGet() ? 0 : 1;
	  if( depressed )
	    SEQ_UI_UnInstallDelayedActionCallback(MSD_EnableReq);
	  else {
	    if( !do_enable ) {
	      // wait a bit longer... normaly it would be better to print a warning that "unmounting via OS" is better
	      SEQ_UI_InstallDelayedActionCallback(MSD_EnableReq, 5000, do_enable);
	      SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION_R, 5001, "", "to disable MSD USB!");
	    } else {
	      SEQ_UI_InstallDelayedActionCallback(MSD_EnableReq, 2000, do_enable);
	      SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION_R, 2001, "", "to enable MSD USB!");
	    }
	  }
	  return 1;
	}

	if( depressed ) return 0; // ignore when button depressed

	// re-use encoder function
	return Encoder_Handler(button, incrementer);
      }
    
  }

  if( depressed )
    return 0; // ignore when button depressed
    
  switch( button ) {
  case SEQ_UI_BUTTON_Select:
  case SEQ_UI_BUTTON_Right:
    if( ++ui_selected_item >= NUM_OF_ITEMS )
      ui_selected_item = 0;
    return 1; // value always changed
    
  case SEQ_UI_BUTTON_Left:
    if( ui_selected_item == 0 )
      ui_selected_item = NUM_OF_ITEMS-1;
    return 1; // value always changed
    
  case SEQ_UI_BUTTON_Up:
    return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);
    
  case SEQ_UI_BUTTON_Down:
    return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

  case SEQ_UI_BUTTON_Exit:
    if( menu_dialog != DIALOG_NONE ) {
      // switch to main dialog
      menu_dialog = DIALOG_NONE;
      ui_selected_item = 0;
      return 1;
    }
    break;
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
  //    Sessions              MIDI Files      MSD USB (UMRW)                         
  // Import  Export      Import  Export  Play enabled                                


  // Session Import dialog:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Select Source Session (10 found)          Source  Destination                   
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  1:A1-1:A8 1:A1-1:A8         IMPORT EXIT

  // Session Export dialog:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Select Destination Session (10 found)     Source  Destination                   
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  1:A1-1:A8 1:A1-1:A8         EXPORT EXIT


  // MIDI Files Import dialog:
  // Select MIDI File (10 files found)       Mode Max.Layers Resolution              
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx Note      8        16th   (8 Bars)  EXIT


  // MIDI Files Play dialog:
  // Select MIDI File (10 files found)       Start Loop Playmode  Port               
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx Play   on  exclusive Def.           EXIT


  // MIDI Files Export dialog:
  // Export                Measures StepsPerM                                        
  // All Groups                 1       16   Continue                            EXIT

  // Export Group Pattern  Measures StepsPerM                                        
  // Group   G1    1:A1         1       16   Continue                            EXIT

  // Export Track          Measures StepsPerM                                        
  // Track  G1T1                1       16   Continue                            EXIT

  // Export Song           Measures                                                  
  // Song     1                 1            Continue                            EXIT

  // Continue:
  // Please enter Filename:                  /MIDI/<xxxxxxxx>.MID                    
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins   SAVE EXIT

  // File exists
  //                                         File '/MIDI/xxx.MID' already exists     
  //                                         Overwrite? YES  NO                  EXIT


  switch( menu_dialog ) {
    case DIALOG_S_IMPORT:
    case DIALOG_S_EXPORT: {
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_CursorSet(0, 0);
      if( dir_num_items < 0 ) {
	if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	  SEQ_LCD_PrintString("/SESSIONS directory not found on SD Card!");
	else
	  SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
      } else if( dir_num_items == 0 ) {
	SEQ_LCD_PrintFormattedString("No files found under /SESSIONS!");
      } else {
	if( menu_dialog == DIALOG_S_IMPORT )
	  SEQ_LCD_PrintFormattedString("Select Source Session (%d found)", dir_num_items);
	else
	  SEQ_LCD_PrintFormattedString("Select Destination Session (%d found)", dir_num_items);
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("  Source  Destination                   ");

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, dir_selected_item, dir_view_offset);

      SEQ_LCD_PrintChar(' ');

      if( ui_selected_item == SESSION_ITEM_PATTERN_B && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintFormattedString("%d:%c%d",
				     1 + (source_pattern_begin >> 6),
				     'A' + ((source_pattern_begin >> 3) & 0x7),
				     1 + (source_pattern_begin & 7));
      }

      SEQ_LCD_PrintChar('-');

      if( ui_selected_item == SESSION_ITEM_PATTERN_E && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(4);
      } else {
	SEQ_LCD_PrintFormattedString("%d:%c%d",
				     1 + (source_pattern_end >> 6),
				     'A' + ((source_pattern_end >> 3) & 0x7),
				     1 + (source_pattern_end & 7));
      }

      SEQ_LCD_PrintChar(' ');

      if( ui_selected_item == SESSION_ITEM_PATTERN_D && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(9);
      } else {
	u16 destination_pattern_end = destination_pattern + (source_pattern_end-source_pattern_begin);
	if( destination_pattern_end >= MAX_SESSION_PATTERNS )
	  destination_pattern_end = MAX_SESSION_PATTERNS-1;

	SEQ_LCD_PrintFormattedString("%d:%c%d-%d:%c%d",
				     1 + (destination_pattern >> 6),
				     'A' + ((destination_pattern >> 3) & 0x7),
				     1 + (destination_pattern & 7),
				     1 + (destination_pattern_end >> 6),
				     'A' + ((destination_pattern_end >> 3) & 0x7),
				     1 + (destination_pattern_end & 7));
      }

      SEQ_LCD_PrintSpaces(9);

      if( menu_dialog == DIALOG_S_IMPORT )
	SEQ_LCD_PrintString("IMPORT");
      else
	SEQ_LCD_PrintString("EXPORT");

      SEQ_LCD_PrintString(" EXIT");
    } break;


    case DIALOG_MF_PLAY:
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_CursorSet(0, 0);
      if( dir_num_items < 0 ) {
	if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	  SEQ_LCD_PrintString("/MIDI directory not found on SD Card!");
	else
	  SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
      } else if( dir_num_items == 0 ) {
	SEQ_LCD_PrintFormattedString("No MIDI files found under /MIDI!");
      } else {
	SEQ_LCD_PrintFormattedString("Select MIDI File (%d files found)", dir_num_items);
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(40, 0);
      if( SEQ_MIDPLY_RunModeGet() ) {
	SEQ_LCD_PrintSpaces(6);
      } else {
	SEQ_LCD_PrintString("Start ");
      }

      SEQ_LCD_PrintString("Loop Playmode  Port");
      SEQ_LCD_PrintSpaces(40-25);

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, dir_selected_item, dir_view_offset);

      if( SEQ_MIDPLY_RunModeGet() ) {
	SEQ_LCD_PrintString("STOP  ");
      } else {
	SEQ_LCD_PrintString("Play  ");
      }

      SEQ_LCD_PrintFormattedString("%s  ", SEQ_MIDPLY_LoopModeGet() ? " on": "off");


      switch( SEQ_MIDPLY_ModeGet() ) {
        case SEQ_MIDPLY_MODE_Exclusive:
	  SEQ_LCD_PrintString("exclusive ");
	  break;
        case SEQ_MIDPLY_MODE_Parallel:
	  SEQ_LCD_PrintString("parallel  ");
	  break;
        default:
	  SEQ_LCD_PrintString("????????  ");
	  break;
      }

      SEQ_LCD_PrintString((char *)SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(SEQ_MIDPLY_PortGet())));

      SEQ_LCD_PrintSpaces(11);
      SEQ_LCD_PrintString("EXIT");
      break;


    case DIALOG_MF_IMPORT: {
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_CursorSet(0, 0);
      if( dir_num_items < 0 ) {
	if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	  SEQ_LCD_PrintString("/MIDI directory not found on SD Card!");
	else
	  SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
      } else if( dir_num_items == 0 ) {
	SEQ_LCD_PrintFormattedString("No MIDI files found under /MIDI!");
      } else {
	SEQ_LCD_PrintFormattedString("Select MIDI File (%d files found)", dir_num_items);
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString("Mode Max.Layers Resolution              ");

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, dir_selected_item, dir_view_offset);

      if( SEQ_MIDIMP_ModeGet() == SEQ_MIDIMP_MODE_AllDrums )
	SEQ_LCD_PrintString("Drum ");
      else
	SEQ_LCD_PrintString("Note ");

      u8 step_resolution;
      switch( SEQ_MIDIMP_ResolutionGet() ) {
      case 1: step_resolution = 32; break;
      case 2: step_resolution = 64; break;
      default: step_resolution = 16;
      }
      SEQ_LCD_PrintSpaces(4);
      SEQ_LCD_PrintFormattedString("%2d", SEQ_MIDIMP_NumLayersGet());
      SEQ_LCD_PrintSpaces(4+1+3);

      SEQ_LCD_PrintFormattedString("%2dth", step_resolution);
      SEQ_LCD_PrintSpaces(3);

      int max_bars = SEQ_MIDIMP_MaxBarsGet();
      SEQ_LCD_PrintFormattedString("(%d Bar%s)  ", max_bars, max_bars == 1 ? "" : "s");
      SEQ_LCD_CursorSet(80-5, 1);
      SEQ_LCD_PrintString(" EXIT");
    } break;


    case DIALOG_MF_EXPORT: {
      seq_midexp_mode_t midexp_mode = SEQ_MIDEXP_ModeGet();

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintString("Export ");

      switch( midexp_mode ) {
        case SEQ_MIDEXP_MODE_AllGroups:
	  SEQ_LCD_PrintSpaces(15);
	  break;
        case SEQ_MIDEXP_MODE_Group:
	  SEQ_LCD_PrintString("Group Pattern");
	  SEQ_LCD_PrintSpaces(2);
	  break;
        case SEQ_MIDEXP_MODE_Track:
	  SEQ_LCD_PrintString("Track");
	  SEQ_LCD_PrintSpaces(10);
	  break;
        case SEQ_MIDEXP_MODE_Song:
	  SEQ_LCD_PrintString("Song");
	  SEQ_LCD_PrintSpaces(11);
	  break;
        default:
	  SEQ_LCD_PrintSpaces(15);
      }

      SEQ_LCD_PrintString("Measures ");
      if( midexp_mode == SEQ_MIDEXP_MODE_Song )
	SEQ_LCD_PrintSpaces(9);
      else
	SEQ_LCD_PrintString("StepsPerM");

      SEQ_LCD_PrintSpaces(40);


      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);
      switch( midexp_mode ) {
        case SEQ_MIDEXP_MODE_AllGroups:
	  if( ui_selected_item == EXPORT_ITEM_MODE && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(10);
	  } else {
	    SEQ_LCD_PrintString("All Groups");
	  }

	  SEQ_LCD_PrintSpaces(12);
	  break;
        case SEQ_MIDEXP_MODE_Group: {
	  u8 group = SEQ_UI_VisibleTrackGet() / SEQ_CORE_NUM_TRACKS_PER_GROUP;

	  if( ui_selected_item == EXPORT_ITEM_MODE && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(8);
	  } else {
	    SEQ_LCD_PrintFormattedString("Group   ");
	  }

	  if( ui_selected_item == EXPORT_ITEM_VAL && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(6);
	  } else {
	    SEQ_LCD_PrintFormattedString("G%d    ", (int)group+1);
	  }

	  if( ui_selected_item == EXPORT_ITEM_BANK && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(1);
	  } else {
	    SEQ_LCD_PrintFormattedString("%d", (int)seq_pattern[group].bank+1);
	  }

	  SEQ_LCD_PrintChar(':');

	  if( ui_selected_item == EXPORT_ITEM_PATTERN && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(2);
	  } else {
	    SEQ_LCD_PrintPattern(seq_pattern[group]);
	  }

	  SEQ_LCD_PrintSpaces(4);
	} break;
        case SEQ_MIDEXP_MODE_Track: {
	  u8 track = SEQ_UI_VisibleTrackGet();

	  if( ui_selected_item == EXPORT_ITEM_MODE && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(7);
	  } else {
	    SEQ_LCD_PrintString("Track  ");
	  }

	  if( ui_selected_item == EXPORT_ITEM_VAL && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(4);
	  } else {
	    SEQ_LCD_PrintFormattedString("G%dT%d",
					 (track / SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
					 (track % SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1);
	  }
	  SEQ_LCD_PrintSpaces(11);
	} break;
        case SEQ_MIDEXP_MODE_Song: {
	  if( ui_selected_item == EXPORT_ITEM_MODE && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(7);
	  } else {
	  SEQ_LCD_PrintString("Song   ");
	  }

	  if( ui_selected_item == EXPORT_ITEM_VAL && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(3);
	  } else {
	    SEQ_LCD_PrintFormattedString("%3d", SEQ_SONG_NumGet()+1);
	  }

	  SEQ_LCD_PrintSpaces(12);
	} break;
        default:
	  SEQ_LCD_PrintSpaces(22);
      }

      if( ui_selected_item == EXPORT_ITEM_MEASURES && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(6);
      } else {
	SEQ_LCD_PrintFormattedString("  %4d", (int)SEQ_MIDEXP_ExportMeasuresGet()+1);
      }
      SEQ_LCD_PrintSpaces(6);

      if( midexp_mode == SEQ_MIDEXP_MODE_Song )
	SEQ_LCD_PrintSpaces(6);
      else {
	if( ui_selected_item == EXPORT_ITEM_STEPS_P_M && ui_cursor_flash ) {
	  SEQ_LCD_PrintSpaces(3);
	} else {
	  SEQ_LCD_PrintFormattedString("%3d", (int)SEQ_MIDEXP_ExportStepsPerMeasureGet()+1);
	}
	SEQ_LCD_PrintSpaces(3);
      }

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_PrintString("Continue");
      SEQ_LCD_PrintSpaces(28);
      SEQ_LCD_PrintString("EXIT");
    } break;


    case DIALOG_MF_EXPORT_FNAME: {
      int i;

      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintString("Please enter Filename:");
      SEQ_LCD_PrintSpaces(18);

      SEQ_LCD_PrintString("/MIDI/<");
      for(i=0; i<8; ++i)
	SEQ_LCD_PrintChar(dir_name[i]);
      SEQ_LCD_PrintString(">.MID");
      SEQ_LCD_PrintSpaces(20);

      // insert flashing cursor
      if( ui_cursor_flash ) {
	SEQ_LCD_CursorSet(47 + ui_edit_name_cursor, 0);
	SEQ_LCD_PrintChar('*');
      }

      SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_PrintString("  SAVE EXIT");
    } break;


    case DIALOG_MF_EXPORT_FEXISTS: {
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintFormattedString("File '/MIDI/%s.MID' already exists!", dir_name);
      SEQ_LCD_PrintSpaces(10);

      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintString("Overwrite? YES  NO");
      SEQ_LCD_PrintSpaces(18);
      SEQ_LCD_PrintString("EXIT");
    } break;


    ///////////////////////////////////////////////////////////////////////////
    case DIALOG_MF_EXPORT_PROGRESS:
      SEQ_LCD_Clear(); // remove artifacts
      // (message print by SEQ_MIDEXP_GenerateFile())
      return 0;


    default:
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintString("   Sessions              MIDI Files      MSD USB ");
      if( TASK_MSD_EnableGet() == 1 ) {
	char str[5];
	TASK_MSD_FlagStrGet(str);
	SEQ_LCD_PrintFormattedString("(%s)", str);
      } else {
        SEQ_LCD_PrintSpaces(6);
      }
      SEQ_LCD_PrintSpaces(40-15);
    
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintString("Import  Export      Import  Export  Play");

      if( ui_selected_item == ITEM_ENABLE_MSD && ui_cursor_flash ) {
        SEQ_LCD_PrintSpaces(14);
      } else {
        s32 status = TASK_MSD_EnableGet();
        switch( status ) {
          case 0:  SEQ_LCD_PrintString(" disabled     "); break;
          case 1:  SEQ_LCD_PrintString("  enabled     "); break;
          default: SEQ_LCD_PrintString(" Not Emulated!"); break;
        }
      }
      SEQ_LCD_PrintSpaces(40-14);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_DISK_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  dir_name[0] = 0;
  menu_dialog = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function for MSD enable/disable
/////////////////////////////////////////////////////////////////////////////
static void MSD_EnableReq(u32 enable)
{
  TASK_MSD_EnableSet(enable);
  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Mass Storage via USB", enable ? "enabled!" : "disabled!");
}




/////////////////////////////////////////////////////////////////////////////
// Scan files under /SESSIONS
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_DISK_UpdateSessionDirList(void)
{
  int item;

  MUTEX_SDCARD_TAKE;
  dir_num_items = SEQ_FILE_GetDirs(SEQ_FILE_SESSION_PATH, (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
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
// Scan files under /MIDI
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_DISK_UpdateMfDirList(void)
{
  int item;

  MUTEX_SDCARD_TAKE;
  dir_num_items = SEQ_FILE_GetFiles("/MIDI", "MID", (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
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
// help function to start MIDI file export
// returns 0 on success
// returns != 0 on errors (dialog page will be changed accordingly)
/////////////////////////////////////////////////////////////////////////////
static s32 DoMfExport(u8 force_overwrite)
{
  s32 status;
  int i;
  char path[30];

  // if an error is detected, we jump back to FNAME page
  ui_selected_item = 0;
  menu_dialog = DIALOG_MF_EXPORT_FNAME;

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

  strcpy(path, "/MIDI");
  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -3;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "/MIDI directory", "doesn't exist!");
    return -4;
  }

  char mid_file[30];
  char *p = (char *)&mid_file[0];
  for(i=0; i<8; ++i) {
    char c = dir_name[i];
    if( c != ' ' )
      *p++ = c;
  }
  *p++ = 0;

  sprintf(path, "/MIDI/%s.MID", mid_file);

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_FileExists(path);
  MUTEX_SDCARD_GIVE;
	    
  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -5;
  }


  if( !force_overwrite && status == 1) {
    // file exists - ask if it should be overwritten in a special dialog page
    menu_dialog = DIALOG_MF_EXPORT_FEXISTS;
    return 1;
  }

  // select empty dialog page --- messages are print by SEQ_MIDEXP_GenerateFile()
  menu_dialog = DIALOG_MF_EXPORT_PROGRESS;
  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Exporting", path);

  if( (status=SEQ_MIDEXP_GenerateFile(path)) < 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Error during Export!", "see MIOS Terminal!");
    return -6;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Export", "successfull!");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to copy a session
// IMPORTANT: wrap this function with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE!
// returns 0 on success
// returns != 0 on errors (dialog page will be changed accordingly)
/////////////////////////////////////////////////////////////////////////////
static s32 DoSessionCopy(char *from_session, u16 from_pattern, char *to_session, u16 to_pattern, u16 num_patterns)
{
  s32 status = 0;
  int i;

  // since we need pattern memory as temporary storage:
  // store all patterns
  int group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    status = SEQ_FILE_B_PatternWrite(seq_file_session_name, seq_pattern[group].bank, seq_pattern[group].pattern, group, 1);
    if( status < 0 ) {
      SEQ_UI_SDCardErrMsg(2000, status);
      return status;
    }
  }

  for(i=0; i<num_patterns; ++i) {
    u8 s_bank = (from_pattern+i) >> 6;
    u8 s_pattern = (from_pattern+i) & 0x3f;
    u8 d_bank = (to_pattern+i) >> 6;
    u8 d_pattern = (to_pattern+i) & 0x3f;

    char msg_u[20];
    sprintf(msg_u, "Copy %-8s %d:%c%c",
	    from_session,
	    1 + s_bank,
	    'A' + (s_pattern >> 3),
	    '1' + (s_pattern & 7));

    char msg_l[20];
    sprintf(msg_l, "---> %-8s %d:%c%c",
	    to_session,
	    1 + d_bank,
	    'A' + (d_pattern >> 3),
	    '1' + (d_pattern & 7));

    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 65535, msg_u, msg_l);

    // switch to source session
    status = SEQ_FILE_B_Open(from_session, s_bank);
    if( status < 0 )
      break;

    // read pattern
    group = 0;
    status = SEQ_FILE_B_PatternRead(s_bank, s_pattern, group);

    // switch to destination session
    status = SEQ_FILE_B_Open(to_session, d_bank);
    if( status < 0 )
      break;

    // store pattern
    group = 0;
    status = SEQ_FILE_B_PatternWrite(to_session, d_bank, d_pattern, group, 0);
    if( status < 0 )
      break;
  }

  if( status >= 0 )
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Copy operations", "finished!");
  else {
    SEQ_UI_SDCardErrMsg(2000, status);

    // wait a second...
    int d;
    for(d=0; d<1000; ++d)
      MIOS32_DELAY_Wait_uS(1000);
  }

  // re-open all banks
  status = SEQ_FILE_B_LoadAllBanks(seq_file_session_name);
  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);

    // wait a second...
    int d;
    for(d=0; d<1000; ++d)
      MIOS32_DELAY_Wait_uS(1000);
  }

  // reload patterns of all groups
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    status = SEQ_FILE_B_PatternRead(seq_pattern[group].bank, seq_pattern[group].pattern, group);
    if( status < 0 )
      break;
  }

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return status;
  }

  return 0; // no error
}
