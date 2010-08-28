// $Id$
/*
 * "About this MIDIbox" (Info page)
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

#include <seq_midi_out.h>

#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_terminal.h"

#include "seq_core.h"
#include "seq_song.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_mixer.h"
#include "seq_midi_port.h"

#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_s.h"
#include "seq_file_m.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"
#include "seq_file_hw.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           4
#define ITEM_LIST1             0
#define ITEM_LIST2             1
#define ITEM_LIST3             2
#define ITEM_LIST4             3


#define NUM_LIST_DISPLAYED_ITEMS NUM_OF_ITEMS
#define NUM_LIST_ITEMS         10
#define LIST_ITEM_SYSTEM       0
#define LIST_ITEM_GLOBALS      1
#define LIST_ITEM_CONFIG       2
#define LIST_ITEM_TRACKS       3
#define LIST_ITEM_TRACK_INFO   4
#define LIST_ITEM_MIXER_MAP    5
#define LIST_ITEM_SONG         6
#define LIST_ITEM_GROOVES      7
#define LIST_ITEM_SD_CARD      8
#define LIST_ITEM_NETWORK      9


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define LIST_ENTRY_WIDTH 9
static char list_entries[NUM_LIST_ITEMS*LIST_ENTRY_WIDTH] =
//<--------->
  "System   "
  "Globals  "
  "Config   "
  "Tracks   "
  "TrackInfo"
  "Mixer Map"
  "Song     "
  "Grooves  "
  "SD Card  "
  "Network  ";

static u8 list_view_offset = 0; // only changed once after startup


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_UI_INFO_UpdateList(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = (3 << (2*ui_selected_item));

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
  if( encoder >= SEQ_UI_ENCODER_GP9 && encoder <= SEQ_UI_ENCODER_GP16 ) {
    switch( ui_selected_item + list_view_offset ) {
      case LIST_ITEM_SYSTEM:
	SEQ_STATISTICS_Reset();
	break;
    }
    return 1;
  }

  // any other encoder changes the list view
  if( SEQ_UI_SelectListItem(incrementer, NUM_LIST_ITEMS, NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &list_view_offset) )
    SEQ_UI_INFO_UpdateList();

  return 1;
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

  if( button <= SEQ_UI_BUTTON_GP8 || button == SEQ_UI_BUTTON_Select ) {
    if( button != SEQ_UI_BUTTON_Select )
      ui_selected_item = button / 2;

    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 10000, "Sending Informations", "to MIOS Terminal!");

    switch( ui_selected_item + list_view_offset ) {

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SYSTEM:
	SEQ_TERMINAL_PrintSystem(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_GLOBALS:
	SEQ_TERMINAL_PrintGlobalConfig(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_CONFIG:
	SEQ_TERMINAL_PrintSessionConfig(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_TRACKS:
	SEQ_TERMINAL_PrintTracks(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_TRACK_INFO:
	SEQ_TERMINAL_PrintTrack(DEBUG_MSG, SEQ_UI_VisibleTrackGet());
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_MIXER_MAP:
	SEQ_TERMINAL_PrintCurrentMixerMap(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SONG:
	SEQ_TERMINAL_PrintCurrentSong(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_GROOVES:
	SEQ_TERMINAL_PrintGrooveTemplates(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SD_CARD:
	SEQ_TERMINAL_PrintSdCardInfo(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_NETWORK:
	SEQ_TERMINAL_PrintNetworkInfo(DEBUG_MSG);
	break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      default:
	DEBUG_MSG("No informations available.");
    }

    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Sent Informations", "to MIOS Terminal!");

    return 1;
  }

  if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 ) {
    // re-using encoder handler
    return Encoder_Handler(button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
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
  if( high_prio ) {
    SEQ_LCD_CursorSet(32, 0);
    mios32_sys_time_t t = MIOS32_SYS_TimeGet();
    int hours = (t.seconds / 3600) % 24;
    int minutes = (t.seconds % 3600) / 60;
    int seconds = (t.seconds % 3600) % 60;
    SEQ_LCD_PrintFormattedString("%02d:%02d:%02d", hours, minutes, seconds);
    return 0;
  }


  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // About this MIDIbox:             00:00:00Stopwatch:   100/  300 uS  CPU Load: 40%
  //   System   Globals    Tracks  TrackInfo>MIDI Scheduler: Alloc   0/  0 Drops:   0


  // Select MIDI Device with GP Button:      10 Devices found under /sysex           
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx>



  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("About this MIDIbox:             ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, NUM_LIST_ITEMS, NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, list_view_offset);

  ///////////////////////////////////////////////////////////////////////////
  switch( ui_selected_item + list_view_offset ) {
    case LIST_ITEM_SYSTEM: {
      u32 stopwatch_value_max = SEQ_STATISTICS_StopwatchGetValueMax();
      u32 stopwatch_value = SEQ_STATISTICS_StopwatchGetValue();

      SEQ_LCD_CursorSet(40, 0);
      if( stopwatch_value_max == 0xffffffff ) {
	SEQ_LCD_PrintFormattedString("Stopwatch: Overrun!     ");
      } else if( !stopwatch_value_max ) {
	SEQ_LCD_PrintFormattedString("Stopwatch: no result yet");
      } else {
	SEQ_LCD_PrintFormattedString("Stopwatch: %4d/%4d uS ", stopwatch_value, stopwatch_value_max);
      }
      SEQ_LCD_PrintSpaces(3);
      SEQ_LCD_PrintFormattedString("CPU Load: %02d%%", SEQ_STATISTICS_CurrentCPULoad());

      SEQ_LCD_CursorSet(40, 1);
      if( seq_midi_out_allocated > 1000 || seq_midi_out_max_allocated > 1000 || seq_midi_out_dropouts > 1000 ) {
	SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %4d/%4d Dr: %4d",
				     seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
      } else {
	SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
				     seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
      }
    } break;

    default:
      SEQ_LCD_CursorSet(40, 0);
      //                  <---------------------------------------->
      SEQ_LCD_PrintString("Press SELECT to send Informations       ");
      SEQ_LCD_CursorSet(40, 1);
      SEQ_LCD_PrintString("to MIOS Terminal                        ");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INFO_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  SEQ_UI_INFO_UpdateList();

  return 0; // no error
}


static s32 SEQ_UI_INFO_UpdateList(void)
{
  int item;

  for(item=0; item<NUM_LIST_DISPLAYED_ITEMS && item<NUM_LIST_ITEMS; ++item) {
    int i;

    char *list_item = (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*item];
    char *list_entry = (char *)&list_entries[LIST_ENTRY_WIDTH*(item+list_view_offset)];
    memcpy(list_item, list_entry, LIST_ENTRY_WIDTH);
    for(i=LIST_ENTRY_WIDTH-1; i>=0; --i)
      if( list_item[i] == ' ' )
	list_item[i] = 0;
      else
	break;
  }

  while( item < NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}
