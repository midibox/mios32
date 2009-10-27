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

#include "seq_lcd.h"
#include "seq_ui.h"
#include "tasks.h"

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_midply.h"
#include "seq_midexp.h"
#include "seq_midimp.h"

#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_song.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           5
#define ITEM_BACKUP            0
#define ITEM_ENABLE_MSD        1
#define ITEM_MF_IMPORT         2
#define ITEM_MF_EXPORT         3
#define ITEM_MF_PLAY           4

#define EXPORT_NUM_OF_ITEMS    6
#define EXPORT_ITEM_MODE       0
#define EXPORT_ITEM_VAL        1
#define EXPORT_ITEM_BANK       2
#define EXPORT_ITEM_PATTERN    3
#define EXPORT_ITEM_MEASURES   4
#define EXPORT_ITEM_STEPS_P_M  5


// MIDI File dialog screens
#define MF_DIALOG_NONE          0
#define MF_DIALOG_PLAY          1
#define MF_DIALOG_IMPORT        2
#define MF_DIALOG_EXPORT        3
#define MF_DIALOG_EXPORT_FNAME  4
#define MF_DIALOG_EXPORT_FEXISTS 5
#define MF_DIALOG_EXPORT_PROGRESS 6


#define NUM_LIST_DISPLAYED_ITEMS 4
#define LIST_ENTRY_WIDTH 9



/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void BackupReq(u32 dummy);
static void FormatReq(u32 dummy);
static void MSD_EnableReq(u32 enable);

static s32 SEQ_UI_DISK_UpdateDirList(void);

static s32 DoExport(u8 force_overwrite);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 mf_dialog;

static s32 dir_num_items; // contains SEQ_FILE error status if < 0
static u8 dir_view_offset = 0; // only changed once after startup
static char dir_name[12]; // directory name of device (first char is 0 if no device selected)


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( mf_dialog ) {
    case MF_DIALOG_PLAY:
      *gp_leds = (3 << (2*ui_selected_item));
      break;

    case MF_DIALOG_IMPORT:
      // no LED functions yet
      break;

    case MF_DIALOG_EXPORT:
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

    case MF_DIALOG_EXPORT_FNAME:
    case MF_DIALOG_EXPORT_FEXISTS:
    case MF_DIALOG_EXPORT_PROGRESS:
      // no LED functions
      break;

    default:
      switch( ui_selected_item ) {
        case ITEM_BACKUP:
          if( SEQ_FILE_FormattingRequired() )
	    *gp_leds |= 0x00ff;
	  else
	    *gp_leds |= 0x0003;
	  break;
        case ITEM_ENABLE_MSD: *gp_leds |= 0x0300; break;
        case ITEM_MF_IMPORT: *gp_leds |= 0x1000; break;
        case ITEM_MF_EXPORT: *gp_leds |= 0x6000; break;
        case ITEM_MF_PLAY: *gp_leds |= 0x8000; break;
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
  switch( mf_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_PLAY:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9: {
	  // select Play/Stop
	  u8 value = SEQ_MIDPLY_RunModeGet();;
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
	  u8 value = SEQ_MIDPLY_ModeGet();;
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
	    mf_dialog = MF_DIALOG_NONE;
	  }
	  return 1;

        default:
	  if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &dir_view_offset) )
	    SEQ_UI_DISK_UpdateDirList();
      }
      break;


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_IMPORT:
      // no encoder functions yet
      break;


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT: {
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
	    mf_dialog = MF_DIALOG_EXPORT_FNAME;
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
	    mf_dialog = MF_DIALOG_NONE;
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
	  SEQ_PATTERN_Change(ui_selected_group, *pattern);
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
    case MF_DIALOG_EXPORT_FNAME: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP15: {
	  int i;

	  // SAVE only via button
	  if( incrementer != 0 )
	    return 0;

	  if( DoExport(0) == 0 ) { // don't force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    mf_dialog = MF_DIALOG_NONE;
	  }

	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer != 0 )
	    return 0;

	  // exit keypad editor
	  ui_selected_item = 0;
	  mf_dialog = MF_DIALOG_NONE;
	  return 1;
	} break;
      }

      return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&dir_name[0], 8);
    }


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT_FEXISTS: {
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP11: {
	  // YES only via button
	  if( incrementer != 0 )
	    return 0;

	  // YES: overwrite file
	  if( DoExport(1) == 0 ) { // force overwrite
	    // exit keypad editor
	    ui_selected_item = 0;
	    mf_dialog = MF_DIALOG_NONE;
	  }
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP12: {
	  // NO only via button
	  if( incrementer != 0 )
	    return 0;

	  // NO: don't overwrite file - back to filename entry

	  ui_selected_item = 0;
	  mf_dialog = MF_DIALOG_EXPORT_FNAME;
	  return 1;
	} break;

        case SEQ_UI_ENCODER_GP16: {
	  // EXIT only via button
	  if( incrementer != 0 )
	    return 0;

	  // exit keypad editor
	  ui_selected_item = 0;
	  mf_dialog = MF_DIALOG_NONE;
	  return 1;
	} break;
      }

      return -1; // not mapped
    }


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT_PROGRESS:
      // no encoder functions!
      return -1;


    ///////////////////////////////////////////////////////////////////////////
    default:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP1:
        case SEQ_UI_ENCODER_GP2:
          ui_selected_item = ITEM_BACKUP;
          break;
    
        case SEQ_UI_ENCODER_GP3:
        case SEQ_UI_ENCODER_GP4:
        case SEQ_UI_ENCODER_GP5:
        case SEQ_UI_ENCODER_GP6:
        case SEQ_UI_ENCODER_GP7:
        case SEQ_UI_ENCODER_GP8:
          if( SEQ_FILE_FormattingRequired() )
	    ui_selected_item = ITEM_BACKUP;
          else
	    return -1; // not used (yet)
          break;
    
        case SEQ_UI_ENCODER_GP9:
        case SEQ_UI_ENCODER_GP10:
          ui_selected_item = ITEM_ENABLE_MSD;
          break;
    
        case SEQ_UI_ENCODER_GP11:
        case SEQ_UI_ENCODER_GP12:
          return -1; // not used (yet)
    
        case SEQ_UI_ENCODER_GP13:
          ui_selected_item = ITEM_MF_IMPORT;
          break;
    
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
          ui_selected_item = ITEM_MF_EXPORT;
          break;
    
        case SEQ_UI_ENCODER_GP16:
          ui_selected_item = ITEM_MF_PLAY;
          break;
    
      }
    
      // for GP encoders and Datawheel
      switch( ui_selected_item ) {
        case ITEM_BACKUP: {
          if( SEQ_FILE_FormattingRequired() )
	    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Use GP Button", "to start FORMAT!");
          else
	    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Use GP Button", "to start Backup!");
          return 1;
        };
    
        case ITEM_ENABLE_MSD:
          SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Please use", "GP Button!");
          return 1;
    
        case ITEM_MF_IMPORT:
#if 0
          // switch to MIDI File Import Dialog screen
	  ui_selected_item = 0;
          mf_dialog = MF_DIALOG_IMPORT;
          SEQ_UI_DISK_UpdateDirList();
#else
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Not implemented", "yet!");
#endif
          return 1;
    
        case ITEM_MF_EXPORT:
          // switch to MIDI File Export Dialog screen
	  ui_selected_item = 0;
          mf_dialog = MF_DIALOG_EXPORT;
          return 1;

        case ITEM_MF_PLAY:
          // switch to MIDI File Play Dialog screen
	  ui_selected_item = 0;
          mf_dialog = MF_DIALOG_PLAY;
          SEQ_UI_DISK_UpdateDirList();
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
  switch( mf_dialog ) {
    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_PLAY:
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
	      ui_selected_item = button / 2;

	    if( dir_num_items >= 1 && (ui_selected_item+dir_view_offset) < dir_num_items ) {
	      // Play MIDI File
	      char mid_file[20];
	      memcpy((char *)mid_file, (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item], 8);
	      memcpy((char *)mid_file+8, ".mid", 5);
	      char path[40];
	      sprintf(path, "midi/%s", mid_file);

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
      }
      return 1;


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_IMPORT:
      if( depressed ) return 0; // ignore when button depressed
    
      // no button functions yet
      break;


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT:
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

      return -1; // invalid or unsupported button


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT_FNAME:
    case MF_DIALOG_EXPORT_FEXISTS: {
      if( depressed ) return 0; // ignore when button depressed

      return Encoder_Handler((seq_ui_encoder_t)button, 0);
    }


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT_PROGRESS:
      // no button functions!
      return -1;


    ///////////////////////////////////////////////////////////////////////////
    default:
      if( button <= SEQ_UI_BUTTON_GP16 ) {
        s32 incrementer = 0;
    
        // GP1-8 to start formatting
        if( SEQ_FILE_FormattingRequired() ) {
          if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP8 ) {
	    if( depressed )
	      SEQ_UI_UnInstallDelayedActionCallback(FormatReq);
	    else {
	      SEQ_UI_InstallDelayedActionCallback(FormatReq, 5000, 0);
	      SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION, 5001, "", "to FORMAT Files!");
	    }
	    return 1;
          }
        } else {
          // GP1/2 start backup
          if( button == SEQ_UI_BUTTON_GP1 || button == SEQ_UI_BUTTON_GP2 ) {
	    if( depressed )
	      SEQ_UI_UnInstallDelayedActionCallback(BackupReq);
	    else {
	      SEQ_UI_InstallDelayedActionCallback(BackupReq, 5000, 0);
	      SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION, 5001, "", "to start Backup");
	    }
	    return 1;
          }
        }
    
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
    
      if( depressed ) return 0; // ignore when button depressed
    
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
  // ---------------> FORMAT <---------------  MSD USB
  // ---------------> FILES! <--------------- disabled

  //   Create                                  MSD USB
  //   Backup                                 disabled

  //   Create                                  MSD USB                MIDI Files    
  //   Backup                                 disabled           Import  Export  Play

  //                                           MSD USB (UMRW)         MIDI Files    
  //                                           enabled           Import  Export  Play


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
  // Please enter Filename:                  /midi/<xxxxxxxx>.mid                    
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins   SAVE EXIT

  // File exists
  //                                         File '/midi/xxx.mid' already exists                                             
  //                                         Overwrite? YES  NO                  EXIT


  switch( mf_dialog ) {
    case MF_DIALOG_PLAY:
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_CursorSet(0, 0);
      if( dir_num_items < 0 ) {
	if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	  SEQ_LCD_PrintString("/midi directory not found on SD Card!");
	else
	  SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
      } else if( dir_num_items == 0 ) {
	SEQ_LCD_PrintFormattedString("No MIDI files found under /midi!");
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

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, dir_view_offset);

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

    case MF_DIALOG_EXPORT: {
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


    case MF_DIALOG_EXPORT_FNAME: {
      int i;

      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintString("Please enter Filename:");
      SEQ_LCD_PrintSpaces(18);

      SEQ_LCD_PrintString("/midi/<");
      for(i=0; i<8; ++i)
	SEQ_LCD_PrintChar(dir_name[i]);
      SEQ_LCD_PrintString(">.mid");
      SEQ_LCD_PrintSpaces(20);

      // insert flashing cursor
      if( ui_cursor_flash ) {
	SEQ_LCD_CursorSet(47 + ui_edit_name_cursor, 0);
	SEQ_LCD_PrintChar('*');
      }

      SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_PrintString("  SAVE EXIT");
    } break;


    case MF_DIALOG_EXPORT_FEXISTS: {
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintFormattedString("File '/midi/%s.mid' already exists!", dir_name);
      SEQ_LCD_PrintSpaces(10);

      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintSpaces(40);

      SEQ_LCD_PrintString("Overwrite? YES  NO");
      SEQ_LCD_PrintSpaces(18);
      SEQ_LCD_PrintString("EXIT");
    } break;


    ///////////////////////////////////////////////////////////////////////////
    case MF_DIALOG_EXPORT_PROGRESS:
      SEQ_LCD_Clear(); // remove artifacts
      // (message print by SEQ_MIDEXP_GenerateFile())
      return 0;


    default:
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 0);
      if( SEQ_FILE_FormattingRequired() ) {
        SEQ_LCD_PrintString("---------------> ");
        if( ui_selected_item == ITEM_BACKUP && ui_cursor_flash ) {
          SEQ_LCD_PrintSpaces(6);
        } else {
          SEQ_LCD_PrintString("FORMAT");
        }
        SEQ_LCD_PrintString(" <---------------");
      } else {
        if( ui_selected_item == ITEM_BACKUP && ui_cursor_flash ) {
          SEQ_LCD_PrintSpaces(8);
        } else {
          SEQ_LCD_PrintString("  Create");
        }
        SEQ_LCD_PrintSpaces(32);
      }
    
      SEQ_LCD_PrintString("  MSD USB ");
      if( TASK_MSD_EnableGet() == 1 ) {
        char str[5];
        TASK_MSD_FlagStrGet(str);
        SEQ_LCD_PrintFormattedString("(%s)", str);
      } else {
        SEQ_LCD_PrintSpaces(6);
      }
      SEQ_LCD_PrintString("         MIDI Files     ");
    
      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);
      if( SEQ_FILE_FormattingRequired() ) {
        SEQ_LCD_PrintString("---------------> ");
        if( ui_selected_item == ITEM_BACKUP && ui_cursor_flash ) {
          SEQ_LCD_PrintSpaces(6);
        } else {
          SEQ_LCD_PrintString("FILES!");
        }
        SEQ_LCD_PrintString(" <---------------");
      } else {
        if( ui_selected_item == ITEM_BACKUP && ui_cursor_flash ) {
          SEQ_LCD_PrintSpaces(8);
        } else {
          SEQ_LCD_PrintString("  Backup");
        }
        SEQ_LCD_PrintSpaces(32);
      }
    
      ///////////////////////////////////////////////////////////////////////////
    
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
      SEQ_LCD_PrintSpaces(6);
    
      SEQ_LCD_PrintString("Import  Export  Play");
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

  mf_dialog = 0;

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
// help function for Backup button function
/////////////////////////////////////////////////////////////////////////////
static void BackupReq(u32 dummy)
{
  // backup handled by low-priority task in app.c
  // messages print in seq_ui.c so long request is active
  seq_ui_backup_req = 1;
}

/////////////////////////////////////////////////////////////////////////////
// help function for Format button function
/////////////////////////////////////////////////////////////////////////////
void FormatReq(u32 dummy)
{
  // formatting handled by low-priority task in app.c
  // messages print in seq_ui.c so long request is active
  seq_ui_format_req = 1;
}



static s32 SEQ_UI_DISK_UpdateDirList(void)
{
  int item;

  MUTEX_SDCARD_TAKE;
  dir_num_items = SEQ_FILE_GetFiles("midi/", "mid", (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
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
static s32 DoExport(u8 force_overwrite)
{
  s32 status;
  int i;
  char path[20];

  // if an error is detected, we jump back to FNAME page
  ui_selected_item = 0;
  mf_dialog = MF_DIALOG_EXPORT_FNAME;

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

  strcpy(path, "midi/");
  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -3;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "/midi directory", "doesn't exist!");
    return -4;
  }

  dir_name[8] = 0;
  sprintf(path, "midi/%s.mid", dir_name);

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_FileExists(path);
  MUTEX_SDCARD_GIVE;
	    
  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -5;
  }


  if( !force_overwrite && status == 1) {
    // file exists - ask if it should be overwritten in a special dialog page
    mf_dialog = MF_DIALOG_EXPORT_FEXISTS;
    return 1;
  }

  // select empty dialog page --- messages are print by SEQ_MIDEXP_GenerateFile()
  mf_dialog = MF_DIALOG_EXPORT_PROGRESS;
  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Exporting", path);

  if( (status=SEQ_MIDEXP_GenerateFile(path)) < 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Error during Export!", "see MIOS Terminal!");
    return -6;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Export", "successfull!");

  return 0; // no error
}
