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
#include "seq_midply.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           2
#define ITEM_BACKUP            0
#define ITEM_ENABLE_MSD        1
#define ITEM_MF_PLAY           2
#define ITEM_MF_IMPORT         3
#define ITEM_MF_EXPORT         4


// MIDI File dialog screens
#define MF_DIALOG_NONE          0
#define MF_DIALOG_PLAY          1
#define MF_DIALOG_IMPORT        2
#define MF_DIALOG_EXPORT        3


#define NUM_LIST_DISPLAYED_ITEMS 4
#define LIST_ENTRY_WIDTH 9



/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void BackupReq(u32 dummy);
static void FormatReq(u32 dummy);


static s32 SEQ_UI_DISK_UpdateDirList(void);


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
    case MF_DIALOG_EXPORT:
      // no LED functions yet
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
        case ITEM_MF_PLAY: *gp_leds |= 0x2000; break;
        case ITEM_MF_IMPORT: *gp_leds |= 0x6000; break;
        case ITEM_MF_EXPORT: *gp_leds |= 0x8000; break;
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
    case MF_DIALOG_PLAY:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP9:
        case SEQ_UI_ENCODER_GP10: {
	  // select off/exclusive/parallel
	  u8 value = SEQ_MIDPLY_ModeGet();;
	  if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) ) {
	    SEQ_MIDPLY_ModeSet(value);
	    return 1;
	  }
	  return 0;
	} break;


        case SEQ_UI_ENCODER_GP11: {
	  // enable/disable loop mode
	  u8 value = SEQ_MIDPLY_LoopModeGet();
	  if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) >= 0 ) {
	    SEQ_MIDPLY_LoopModeSet(value);
	    return 1; // value changed
	  }
	  return 0; // no change
	} break;

        case SEQ_UI_ENCODER_GP12: {
	  // select MIDI port
	  u8 port_ix = SEQ_MIDI_PORT_OutIxGet(SEQ_MIDPLY_PortGet());
	  if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	    SEQ_MIDPLY_PortSet(SEQ_MIDI_PORT_OutPortGet(port_ix));
	    return 1; // value changed
	  }
	  return 0; // no change
	} break;

        case SEQ_UI_ENCODER_GP13:
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
	  return -1; // not mapped (yet)

        case SEQ_UI_ENCODER_GP16:
	  // EXIT only via button
	  if( incrementer == 0 )
	    mf_dialog = MF_DIALOG_NONE;
	  return 1;

        default:
	  if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &dir_view_offset) )
	    SEQ_UI_DISK_UpdateDirList();
      }
      break;

    case MF_DIALOG_IMPORT:
    case MF_DIALOG_EXPORT:
      // no encoder functions yet
      break;

    default:
      switch( encoder ) {
        case SEQ_UI_ENCODER_GP1:
        case SEQ_UI_ENCODER_GP2:
          ui_selected_item = ITEM_BACKUP;
          break;
    
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
          ui_selected_item = ITEM_MF_PLAY;
          break;
    
        case SEQ_UI_ENCODER_GP14:
        case SEQ_UI_ENCODER_GP15:
          ui_selected_item = ITEM_MF_IMPORT;
          break;
    
        case SEQ_UI_ENCODER_GP16:
          ui_selected_item = ITEM_MF_EXPORT;
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
          TASK_MSD_EnableSet((incrementer > 0) ? 1 : 0);
          SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Mass Storage via USB", (incrementer > 0) ? "enabled!" : "disabled!");
    
          return 1;
    
        case ITEM_MF_PLAY:
          // switch to MIDI File Play Dialog screen
          mf_dialog = MF_DIALOG_PLAY;
	  ui_selected_item = 0;
          SEQ_UI_DISK_UpdateDirList();
          return 1;
    
        case ITEM_MF_IMPORT:
#if 0
          // switch to MIDI File Import Dialog screen
          mf_dialog = MF_DIALOG_IMPORT;
	  ui_selected_item = 0;
          SEQ_UI_DISK_UpdateDirList();
#else
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Not implemented", "yet!");
#endif
          return 1;
    
        case ITEM_MF_EXPORT:
#if 0
          // switch to MIDI File Export Dialog screen
          mf_dialog = MF_DIALOG_EXPORT;
	  ui_selected_item = 0;
          SEQ_UI_DISK_UpdateDirList();
#else
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Not implemented", "yet!");
#endif
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
    case MF_DIALOG_PLAY:
      if( depressed ) return 0; // ignore when button depressed
    
      if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {

	switch( button ) {
	  case SEQ_UI_BUTTON_GP9:
	  case SEQ_UI_BUTTON_GP10: {
	    // cycle off/exclusive/parallel
	    switch( SEQ_MIDPLY_ModeGet() ) {
	      case SEQ_MIDPLY_MODE_Off:
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Exclusive);
		break;
	      case SEQ_MIDPLY_MODE_Exclusive:
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Parallel);
		break;
	      case SEQ_MIDPLY_MODE_Parallel:
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Off);
		break;
	    }
	    return 1;
	  } break;

          case SEQ_UI_BUTTON_GP11:
	    // cycle loop mode
	    return Encoder_Handler(button, SEQ_MIDPLY_LoopModeGet() ? -1 : 1);

          case SEQ_UI_BUTTON_GP12:
	    // increment MIDI port
	    return Encoder_Handler(button, 1);

          case SEQ_UI_BUTTON_GP13:
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

	      // turn on MIDI play mode if not enabled yet
	      if( SEQ_MIDPLY_ModeGet() == SEQ_MIDPLY_MODE_Off )
		SEQ_MIDPLY_ModeSet(SEQ_MIDPLY_MODE_Exclusive);

	      MUTEX_MIDIOUT_TAKE;
	      // always restart sequencer
	      SEQ_BPM_CheckAutoMaster();
	      SEQ_BPM_Start();

	      s32 status = SEQ_MIDPLY_PlayFile(path);
	      MUTEX_MIDIOUT_GIVE;
	      if( status >= 0 )
		SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Playing:", mid_file);
	      else
		SEQ_UI_SDCardErrMsg(2000, status);
	    }
	}
      }
      return 1;

    case MF_DIALOG_IMPORT:
    case MF_DIALOG_EXPORT:
      if( depressed ) return 0; // ignore when button depressed
    
      // no button functions yet
      break;

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
	      SEQ_UI_InstallDelayedActionCallback(BackupReq, 2000, 0);
	      SEQ_UI_Msg(SEQ_UI_MSG_DELAYED_ACTION, 2001, "", "to start Backup");
	    }
	    return 1;
          }
        }
    
        if( depressed ) return 0; // ignore when button depressed
    
        // GP9/10 toggles MSD enable/disable
        if( button == SEQ_UI_BUTTON_GP9 || button == SEQ_UI_BUTTON_GP10 )
          incrementer = (TASK_MSD_EnableGet() > 0) ? -1 : 1;
    
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
  //   Backup                                 disabled           Play  Import  Export

  //                                           MSD USB (UMRW)         MIDI Files    
  //                                           enabled           Play  Import  Export


  // Select MIDI File (10 files found)       Playmode   Loop Port
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx exclusive  on   Def.                EXIT


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
      SEQ_LCD_PrintString("Playmode   Loop Port");
      SEQ_LCD_PrintSpaces(40-10);

      ///////////////////////////////////////////////////////////////////////////
      SEQ_LCD_CursorSet(0, 1);

      SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, dir_view_offset);

      switch( SEQ_MIDPLY_ModeGet() ) {
        case SEQ_MIDPLY_MODE_Exclusive:
	  SEQ_LCD_PrintString("exclusive");
	  break;
        case SEQ_MIDPLY_MODE_Parallel:
	  SEQ_LCD_PrintString("parallel ");
	  break;
        default:
	  SEQ_LCD_PrintString("disabled ");
	  break;
      }

      SEQ_LCD_PrintFormattedString("  %s  ", SEQ_MIDPLY_LoopModeGet() ? " on": "off");

      SEQ_LCD_PrintString((char *)SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(SEQ_MIDPLY_PortGet())));

      SEQ_LCD_PrintSpaces(16);
      SEQ_LCD_PrintString("EXIT");
      break;

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
    
      SEQ_LCD_PrintString("Play  Import  Export");
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
