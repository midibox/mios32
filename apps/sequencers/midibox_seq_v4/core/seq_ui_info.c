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
#define NUM_LIST_ITEMS         8
#define LIST_ITEM_SYSTEM       0
#define LIST_ITEM_GLOBALS      1
#define LIST_ITEM_TRACKS       2
#define LIST_ITEM_TRACK_INFO   3
#define LIST_ITEM_MIXER_MAP    4
#define LIST_ITEM_SONG         5
#define LIST_ITEM_GROOVES      6
#define LIST_ITEM_SD_CARD      7


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define LIST_ENTRY_WIDTH 9
static char list_entries[NUM_LIST_ITEMS*LIST_ENTRY_WIDTH] =
//<--------->
  "System   "
  "Globals  "
  "Tracks   "
  "TrackInfo"
  "Mixer Map"
  "Song     "
  "Grooves  "
  "SD Card  ";

static u8 list_view_offset = 0; // only changed once after startup

static s32 cpu_load_in_percent;

static u32 stopwatch_value;
static u32 stopwatch_value_max;


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
        stopwatch_value_max = 0;
        seq_midi_out_max_allocated = 0;
        seq_midi_out_dropouts = 0;
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
      case LIST_ITEM_SYSTEM: {
	char str_buffer[128];

	MUTEX_MIDIOUT_TAKE;

	DEBUG_MSG("\n");
	DEBUG_MSG("System Informations:\n");
	DEBUG_MSG("====================\n");
	DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE1 " " MIOS32_LCD_BOOT_MSG_LINE2 "\n");

	mios32_sys_time_t t = MIOS32_SYS_TimeGet();
	int hours = (t.seconds / 3600) % 24;
	int minutes = (t.seconds % 3600) / 60;
	int seconds = (t.seconds % 3600) % 60;

	DEBUG_MSG("Operating System: MIOS32\n");
	DEBUG_MSG("Board: " MIOS32_BOARD_STR "\n");
	DEBUG_MSG("Chip Family: " MIOS32_FAMILY_STR "\n");
	if( MIOS32_SYS_SerialNumberGet((char *)str_buffer) >= 0 )
	  DEBUG_MSG("Serial Number: %s\n", str_buffer);
	else
	  DEBUG_MSG("Serial Number: ?\n");
	DEBUG_MSG("Flash Memory Size: %d bytes\n", MIOS32_SYS_FlashSizeGet());
        DEBUG_MSG("RAM Size: %d bytes\n", MIOS32_SYS_RAMSizeGet());

	DEBUG_MSG("Systime: %02d:%02d:%02d\n", hours, minutes, seconds);
	DEBUG_MSG("CPU Load: %02d%%\n", cpu_load_in_percent);
	DEBUG_MSG("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
		  seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
	if( stopwatch_value_max == 0xffffffff ) {
	  DEBUG_MSG("Stopwatch: Overrun!\n");
	} else if( !stopwatch_value_max ) {
	  DEBUG_MSG("Stopwatch: no result yet\n");
	} else {
	  DEBUG_MSG("Stopwatch: %d/%d uS\n", stopwatch_value, stopwatch_value_max);
	}

#if !defined(MIOS32_FAMILY_EMULATION) && configGENERATE_RUN_TIME_STATS
        // send Run Time Stats to MIOS terminal
	DEBUG_MSG("FreeRTOS Task RunTime Stats:\n");
        FREERTOS_UTILS_RunTimeStats();
#endif

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;


      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_GLOBALS: {
	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("\n");
	DEBUG_MSG("Global Settings:\n");
	DEBUG_MSG("================\n");
	SEQ_FILE_C_Debug();

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_TRACKS: {
	char str_buffer[128];

	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("\n");
	DEBUG_MSG("Track Overview:\n");
	DEBUG_MSG("===============\n");

	DEBUG_MSG("| Track | Mode  | Layer P/T/I | Steps P/T | Length | Port  | Chn. | Muted |\n");
	DEBUG_MSG("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

	u8 track;
	for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	  seq_event_mode_t event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
	  u16 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
	  u16 num_par_layers = SEQ_PAR_NumLayersGet(track);
	  u16 num_par_steps = SEQ_PAR_NumStepsGet(track);
	  u16 num_trg_layers = SEQ_TRG_NumLayersGet(track);
	  u16 num_trg_steps = SEQ_TRG_NumStepsGet(track);
	  u16 length = (u16)SEQ_CC_Get(track, SEQ_CC_LENGTH) + 1;
	  mios32_midi_port_t midi_port = SEQ_CC_Get(track, SEQ_CC_MIDI_PORT);
	  u8 midi_chn = SEQ_CC_Get(track, SEQ_CC_MIDI_CHANNEL) + 1;

	  sprintf(str_buffer, "| G%dT%d  | %s |",
		  (track/4)+1, (track%4)+1,
		  SEQ_LAYER_GetEvntModeName(event_mode));

	  sprintf((char *)(str_buffer + strlen(str_buffer)), "   %2d/%2d/%2d  |  %3d/%3d  |   %3d  | %s%c |  %2d  |",
		  num_par_layers, num_trg_layers, num_instruments, 
		  num_par_steps, num_trg_steps,
		  length,
		  SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(midi_port)),
		  SEQ_MIDI_PORT_OutCheckAvailable(midi_port) ? ' ' : '*',
		  midi_chn);

	  if( seq_core_trk[track].state.MUTED )
	    sprintf((char *)(str_buffer + strlen(str_buffer)), "  yes  |\n");
	  else if( seq_core_trk[track].layer_muted )
	    sprintf((char *)(str_buffer + strlen(str_buffer)), " layer |\n");
	  else
	    sprintf((char *)(str_buffer + strlen(str_buffer)), "  no   |\n");

	  DEBUG_MSG(str_buffer);
	}

	DEBUG_MSG("+-------+-------+-------------+-----------+--------+-------+------+-------+\n");

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_TRACK_INFO: {
	MUTEX_MIDIOUT_TAKE;
	u8 track = SEQ_UI_VisibleTrackGet();

	DEBUG_MSG("\n");
	DEBUG_MSG("Track Parameters of G%dT%d", (track/4)+1, (track%4)+1);
	DEBUG_MSG("========================\n");

	SEQ_FILE_T_Debug(track);

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;


      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_MIXER_MAP: {
	char str_buffer[128];

	MUTEX_MIDIOUT_TAKE;
	u8 map = SEQ_MIXER_NumGet();
	int i;

	DEBUG_MSG("\n");
	DEBUG_MSG("Mixer Map #%3d\n", map+1);
	DEBUG_MSG("==============\n");

	DEBUG_MSG("|Num|Port|Chn|Prg|Vol|Pan|Rev|Cho|Mod|CC1|CC2|CC3|CC4|C1A|C2A|C3A|C4A|\n");
	DEBUG_MSG("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");

	for(i=0; i<16; ++i) {
	  sprintf(str_buffer, "|%3d|%s|", i, SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(SEQ_MIXER_Get(i, SEQ_MIXER_PAR_PORT))));

	  int par;

	  for(par=1; par<2; ++par)
	    sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par)+1);

	  for(par=2; par<12; ++par) {
	    u8 value = SEQ_MIXER_Get(i, par);
	    if( value )
	      sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", value-1);
	    else
	      sprintf((char *)(str_buffer + strlen(str_buffer)), " - |");
	  }

	  for(par=12; par<16; ++par)
	    sprintf((char *)(str_buffer + strlen(str_buffer)), "%3d|", SEQ_MIXER_Get(i, par));

	  DEBUG_MSG("%s\n", str_buffer);
	}


	DEBUG_MSG("+---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n");
	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SONG: {
	MUTEX_MIDIOUT_TAKE;
	u8 song = SEQ_SONG_NumGet();
	int i;

	DEBUG_MSG("\n");
	DEBUG_MSG("Song #%2d\n", song+1);
	DEBUG_MSG("========\n");

	DEBUG_MSG("Name: '%s'\n", seq_song_name);
	MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_song_steps[0], SEQ_SONG_NUM_STEPS*sizeof(seq_song_step_t));

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_GROOVES: {
	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("\n");
	DEBUG_MSG("Groove Templates:\n");
	DEBUG_MSG("=================\n");
	SEQ_FILE_G_Debug();

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;


      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SD_CARD: {
	char str_buffer[128];

	MUTEX_MIDIOUT_TAKE;

	DEBUG_MSG("\n");
	DEBUG_MSG("SD Card Informations\n");
	DEBUG_MSG("====================\n");

#if !defined(MIOS32_FAMILY_EMULATION)

	MUTEX_SDCARD_TAKE;
	SEQ_FILE_PrintSDCardInfos();
	MUTEX_SDCARD_GIVE;


	DEBUG_MSG("\n");
	DEBUG_MSG("Checking SD Card at application layer\n");
	DEBUG_MSG("=====================================\n");

	if( !SEQ_FILE_SDCardAvailable() ) {
	  sprintf(str_buffer, "not connected");
	} else if( !SEQ_FILE_VolumeAvailable() ) {
	  sprintf(str_buffer, "Invalid FAT");
	} else {
	  DEBUG_MSG("Deriving SD Card informations - please wait!\n");
	  MUTEX_MIDIOUT_GIVE;
	  MUTEX_SDCARD_TAKE;
	  SEQ_FILE_UpdateFreeBytes();
	  MUTEX_SDCARD_GIVE;
	  MUTEX_MIDIOUT_TAKE;

	  sprintf(str_buffer, "'%s': %u of %u MB free", 
		  SEQ_FILE_VolumeLabel(),
		  (unsigned int)(SEQ_FILE_VolumeBytesFree()/1000000),
		  (unsigned int)(SEQ_FILE_VolumeBytesTotal()/1000000));
	}
	DEBUG_MSG("SD Card: %s\n", str_buffer);
#endif

	{
	  u8 bank;
	  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
	    int num_patterns = SEQ_FILE_B_NumPatterns(bank);
	    if( num_patterns )
	      DEBUG_MSG("File MBSEQ_B%d.V4: valid (%d patterns)\n", bank+1, num_patterns);
	    else
	      DEBUG_MSG("File MBSEQ_B%d.V4: doesn't exist\n", bank+1, num_patterns);
	  }

	  int num_maps = SEQ_FILE_M_NumMaps();
	  if( num_maps )
	    DEBUG_MSG("File MBSEQ_M.V4: valid (%d mixer maps)\n", num_maps);
	  else
	    DEBUG_MSG("File MBSEQ_M.V4: doesn't exist\n");

	  int num_songs = SEQ_FILE_S_NumSongs();
	  if( num_songs )
	    DEBUG_MSG("File MBSEQ_S.V4: valid (%d songs)\n", num_songs);
	  else
	    DEBUG_MSG("File MBSEQ_S.V4: doesn't exist\n");

	  if( SEQ_FILE_G_Valid() )
	    DEBUG_MSG("File MBSEQ_G.V4: valid\n");
	  else
	    DEBUG_MSG("File MBSEQ_G.V4: doesn't exist\n");

	  if( SEQ_FILE_C_Valid() )
	    DEBUG_MSG("File MBSEQ_C.V4: valid\n");
	  else
	    DEBUG_MSG("File MBSEQ_C.V4: doesn't exist\n");

	  if( SEQ_FILE_HW_Valid() )
	    DEBUG_MSG("File MBSEQ_HW.V4: valid\n");
	  else
	    DEBUG_MSG("File MBSEQ_HW.V4: doesn't exist or hasn't been re-loaded\n");
	}

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;



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
    case LIST_ITEM_SYSTEM:
      SEQ_LCD_CursorSet(40, 0);
      if( stopwatch_value_max == 0xffffffff ) {
	SEQ_LCD_PrintFormattedString("Stopwatch: Overrun!     ");
      } else if( !stopwatch_value_max ) {
	SEQ_LCD_PrintFormattedString("Stopwatch: no result yet");
      } else {
	SEQ_LCD_PrintFormattedString("Stopwatch: %4d/%4d uS ", stopwatch_value, stopwatch_value_max);
      }
      SEQ_LCD_PrintSpaces(3);
      SEQ_LCD_PrintFormattedString("CPU Load: %02d%%", cpu_load_in_percent);

      SEQ_LCD_CursorSet(40, 1);
      if( seq_midi_out_allocated > 1000 || seq_midi_out_max_allocated > 1000 || seq_midi_out_dropouts > 1000 ) {
	SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %4d/%4d Dr: %4d",
				     seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
      } else {
	SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
				     seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
      }
      break;

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


/////////////////////////////////////////////////////////////////////////////
// Handles Idle Counter (frequently called from Background task)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INFO_Idle(void)
{
  static u32 idle_ctr = 0;
  static u32 last_seconds = 0;

  // determine the CPU load
  ++idle_ctr;
  mios32_sys_time_t t = MIOS32_SYS_TimeGet();
  if( t.seconds != last_seconds ) {
    last_seconds = t.seconds;

    // MAX_IDLE_CTR defined in mios32_config.h
    // CPU Load is printed in main menu screen
    cpu_load_in_percent = 100 - ((100 * idle_ctr) / MAX_IDLE_CTR);

#if 0
    DEBUG_MSG("Load: %d%% (Ctr: %d)\n", cpu_load_in_percent, idle_ctr);
#endif
    
    idle_ctr = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Optional stopwatch for measuring performance
// Will be displayed on menu page once stopwatch_max > 0
// Value can be reseted by pressing GP button below the max number
// Usage example: see seq_core.c
// Only one task should control the stopwatch!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INFO_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  MIOS32_STOPWATCH_Init(1); // 1 uS resolution

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets stopwatch
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INFO_StopwatchReset(void)
{
  return MIOS32_STOPWATCH_Reset();
}


/////////////////////////////////////////////////////////////////////////////
// Captures value of stopwatch and displays on screen
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_INFO_StopwatchCapture(void)
{
  u32 value = MIOS32_STOPWATCH_ValueGet();
  portENTER_CRITICAL();
  stopwatch_value = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  portEXIT_CRITICAL();

  return 0; // no error
}


