// $Id$
/*
 * SysEx page
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

#include "file.h"
#include "seq_file.h"
#include "seq_midi_port.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           8
#define ITEM_DEV1              0
#define ITEM_DEV2              1
#define ITEM_DEV3              2
#define ITEM_DEV4              3
#define ITEM_DEV5              4
#define ITEM_DEV6              5
#define ITEM_DEV7              6
#define ITEM_DEV8              7

#define NUM_LIST_DISPLAYED_ITEMS NUM_OF_ITEMS
#define LIST_ENTRY_WIDTH 9

// for debugging list display
#define TEST_LIST 0



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 dir_num_items; // contains FILE error status if < 0
static u8 dir_view_offset = 0; // only changed once after startup
static mios32_midi_port_t sysex_port = DEFAULT; // only changed once after startup
static u16 sysex_delay_between_dumps = 100; // only changed once after startup
static char dir_name[12]; // directory name of device (first char is 0 if no device selected)


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_UI_SYSEX_UpdateDirList(void);
static s32 SEQ_UI_SYSEX_StoreConfig(void);
static s32 SEQ_UI_SYSEX_RestoreConfig(void);


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
  // any encoder changes the dir view
  if( !dir_name[0] ) {
    if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &dir_view_offset) ) {
      SEQ_UI_SYSEX_UpdateDirList();
    }
  } else {
    switch( encoder ) {
      case SEQ_UI_ENCODER_GP13: {
	// select MIDI port
	u8 port_ix = SEQ_MIDI_PORT_OutIxGet(sysex_port);
	if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	  sysex_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15: {
	// select delay
	if( SEQ_UI_Var16_Inc(&sysex_delay_between_dumps, 0, 999, incrementer) >= 0 ) {
	  ui_store_file_required = 1;
	  return 1; // value changed
	}
	return 0; // no change
      } break;

      case SEQ_UI_ENCODER_GP16:
	// EXIT only via button
	if( incrementer == 0 ) {
	  if( ui_store_file_required )
	    SEQ_UI_SYSEX_StoreConfig();

	  // Exit
	  dir_name[0] = 0;
	  ui_selected_item = 0;
	  dir_view_offset = 0;
	  SEQ_UI_SYSEX_UpdateDirList();
	}
	return 1;

      default:
	if( SEQ_UI_SelectListItem(incrementer, dir_num_items, NUM_LIST_DISPLAYED_ITEMS-2, &ui_selected_item, &dir_view_offset) )
	  SEQ_UI_SYSEX_UpdateDirList();
    }
  }

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
  if( depressed ) return -1; // ignore when button depressed, -1 ensures that message still print

  if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {

    if( button != SEQ_UI_BUTTON_Select )
      ui_selected_item = button / 2;

#if TEST_LIST
    char buffer[30];
    int i;
    for(i=0; i<LIST_ENTRY_WIDTH; ++i)
      buffer[i] = ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item + i];
    buffer[i] = 0;

    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Selected:", buffer);
#else
    if( dir_name[0] == 0 && dir_num_items >= 0 ) {
      // Select MIDI Device
      int i;
      char *p = (char *)&dir_name[0];
      for(i=0; i<8; ++i) {
	char c = ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item + i];
	if( c != ' ' )
	  *p++ = c;
      }
      *p++ = 0;
      ui_selected_item = 0;
      dir_view_offset = 0;
      SEQ_UI_SYSEX_UpdateDirList();
      SEQ_UI_SYSEX_RestoreConfig();
    } else {
      switch( button ) {
        case SEQ_UI_BUTTON_GP13:
	  // increment MIDI port
	  return Encoder_Handler(button, 1);

        case SEQ_UI_BUTTON_GP14:
	  // decrement delay
	  return Encoder_Handler(button, -1);

        case SEQ_UI_BUTTON_GP15:
	  // increment delay
	  return Encoder_Handler(button, 1);

        case SEQ_UI_BUTTON_GP16:
	  // EXIT
	  return Encoder_Handler(button, 0);

        default:
          if( dir_num_items >= 1 && (ui_selected_item+dir_view_offset) < dir_num_items ) {
	    // Send SysEx Dump
	    char syx_file[30];
	    int i;
	    char *p = (char *)&syx_file[0];
	    for(i=0; i<8; ++i) {
	      char c = ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item + i];
	      if( c != ' ' )
		*p++ = c;
	    }
	    *p++ = 0;

	    char path[40];
	    sprintf(path, "/SYSEX/%s/%s.SYX", dir_name, syx_file);
	    SEQ_UI_Msg((ui_selected_item < 4) ? SEQ_UI_MSG_USER : SEQ_UI_MSG_USER_R, 10000, "Sending:", syx_file);
	    MUTEX_SDCARD_TAKE;
	    MUTEX_MIDIOUT_TAKE;
	    s32 status = FILE_SendSyxDump(path, sysex_port, sysex_delay_between_dumps);
	    MUTEX_MIDIOUT_GIVE;
	    MUTEX_SDCARD_GIVE;
	    if( status < 0 )
	      SEQ_UI_SDCardErrMsg(2000, status);
	    else {
	      char buffer[20];
	      sprintf(buffer, "Sent %d bytes", status);
	      SEQ_UI_Msg((ui_selected_item < 4) ? SEQ_UI_MSG_USER : SEQ_UI_MSG_USER_R, 1000, buffer, syx_file);
	    }
	  }
      }
    }
#endif

    return 1;
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Select MIDI Device with GP Button:      2 Devices found under /SYSEX            
  //   MBSID     MBFM  ....


  // Select MIDI Device with GP Button:      10 Devices found under /SYSEX           
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx>



  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintSpaces(80);

  SEQ_LCD_CursorSet(0, 0);
  if( dir_num_items < 0 ) {
    if( dir_name[0] != 0 && dir_num_items == FILE_ERR_NO_DIR )
      SEQ_LCD_PrintFormattedString("/SYSEX/%s directory not found on SD Card!", dir_name);
    else if( dir_num_items == FILE_ERR_NO_DIR )
      SEQ_LCD_PrintString("/SYSEX directory not found on SD Card!");
    else
      SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
  } else if( dir_num_items == 0 ) {
    if( dir_name[0] != 0 )
      SEQ_LCD_PrintFormattedString("No .SYX files found under /SYSEX/%s!", dir_name);
    else
      SEQ_LCD_PrintString("No subdirectories found in /SYSEX directory on SD Card!");
  } else {
    if( dir_name[0] != 0 ) {
      SEQ_LCD_PrintString("Select .SYX file with GP Button:");
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintFormattedString("/SYSEX/%-8s       Port  Delay       ", dir_name);
    } else {
      SEQ_LCD_PrintString("Select MIDI Device with GP Button:");
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintFormattedString("%d Devices found under /SYSEX", dir_num_items);
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( !dir_name[0] ) {
    SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, dir_view_offset);
  } else {
    SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dir_num_items, NUM_LIST_DISPLAYED_ITEMS-2, ui_selected_item, dir_view_offset);
    SEQ_LCD_PrintChar(' ');
    SEQ_LCD_PrintChar(' ');
    char str_buffer[5];
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutPortToName(sysex_port, str_buffer));
    SEQ_LCD_PrintFormattedString("  %3d mS", sysex_delay_between_dumps);
    SEQ_LCD_PrintString("  EXIT");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( ui_store_file_required ) {
    SEQ_UI_SYSEX_StoreConfig();
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SYSEX_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  dir_name[0] = 0;

#if TEST_LIST
  dir_num_items = 32;
#else
  SEQ_UI_SYSEX_UpdateDirList();
#endif

  return 0; // no error
}


static s32 SEQ_UI_SYSEX_UpdateDirList(void)
{
  int item;

#if TEST_LIST
  for(item=0; item<NUM_LIST_DISPLAYED_ITEMS && item<dir_num_items; ++item) {
    char *list_item = (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*item];
    sprintf(list_item, "test%d", item + dir_view_offset);
  }
#else
  MUTEX_SDCARD_TAKE;
  if( !dir_name[0] ) {
    dir_num_items = FILE_GetDirs("/SYSEX", (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
  } else {
    char path[25];
    sprintf(path, "/SYSEX/%s", dir_name);
    dir_num_items = FILE_GetFiles(path, "SYX", (char *)&ui_global_dir_list[0], NUM_LIST_DISPLAYED_ITEMS-2, dir_view_offset);
  }
  MUTEX_SDCARD_GIVE;

  if( dir_num_items < 0 )
    item = 0;
  else if( dir_num_items < NUM_LIST_DISPLAYED_ITEMS )
    item = dir_num_items;
  else
    item = NUM_LIST_DISPLAYED_ITEMS;
#endif

  while( item < NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Store configuration in /SYSEX/<device>/DEVCFG.V4 file
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_SYSEX_StoreConfig(void)
{
  s32 status = 0;

  if( !dir_name[0] ) {
    // not in device dir
    return -1;
  }

  char filepath[MAX_PATH];
  sprintf(filepath, "/SYSEX/%s/DEVCFG.V4", dir_name);

  MUTEX_SDCARD_TAKE;

  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
    FILE_WriteClose(); // important to free memory given by malloc
    DEBUG_MSG("[SEQ_UI_SYSEX] Failed to open %s file, status: %d\n", filepath, status);
    SEQ_UI_SDCardErrMsg(2000, status);
  } else {
    char line_buffer[128];

#define FLUSH_BUFFER { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

    char str_buffer[5];
    sprintf(line_buffer, "DevicePort %s\n", SEQ_MIDI_PORT_OutPortToName(sysex_port, str_buffer));
    FLUSH_BUFFER;

    sprintf(line_buffer, "DelayBetweenDumps %d\n", sysex_delay_between_dumps);
    FLUSH_BUFFER;

    // close file
    status |= FILE_WriteClose();

    if( status < 0 ) {
      DEBUG_MSG("[SEQ_UI_SYSEX] Errors while writing into %s file\n", filepath);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Errors while writing into", filepath);
    }
  }

  MUTEX_SDCARD_GIVE;

  ui_store_file_required = 0;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
// Restore configuration from /SYSEX/<device>/DEVCFG.V4 file
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_SYSEX_RestoreConfig(void)
{
  s32 status = 0;

  if( !dir_name[0] ) {
    // not in device dir
    return -1;
  }

  file_t file;
  char filepath[MAX_PATH];
  sprintf(filepath, "/SYSEX/%s/DEVCFG.V4", dir_name);

  MUTEX_SDCARD_TAKE;

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
    // no error if file doesn't exist...
    if( FILE_FileExists(filepath) >= 1 ) {
      DEBUG_MSG("[SEQ_UI_SYSEX] Failed to open %s file, status: %d\n", filepath, status);
      SEQ_UI_SDCardErrMsg(2000, status);
    }
    // take default values
    sysex_port = DEFAULT;
    sysex_delay_between_dumps = 100;
  } else {
    char line_buffer[128];
    do {
      status=FILE_ReadLine((u8 *)line_buffer, 128);

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
	} else {
	  char *word = strtok_r(NULL, separators, &brkt);

	  if( strcasecmp(parameter, "DevicePort") == 0 ) {
	    s32 value = SEQ_MIDI_PORT_OutPortFromNameGet(word);
	    if( value < 0 ) {
	      DEBUG_MSG("[SEQ_UI_SYSEX] ERROR: invalid parameter value: %s %s\n", parameter, word);
	    } else {
	      sysex_port = value;
	    }
	  } else if( strcasecmp(parameter, "DelayBetweenDumps") == 0 ) {
	    s32 value = get_dec(word);
	    if( value < 0 || value > 65535 ) {
	      DEBUG_MSG("[SEQ_UI_SYSEX] ERROR: invalid parameter value: %s %s\n", parameter, word);
	    } else {
	      sysex_delay_between_dumps = value;
	    }
	  } else {
	    DEBUG_MSG("[SEQ_UI_SYSEX] ERROR: unknown parameter: %s\n", parameter);
	  }
	}
      }

    } while( status >= 1 );

    // close file
    status |= FILE_ReadClose(&file);

    if( status < 0 ) {
      DEBUG_MSG("[SEQ_UI_SYSEX] Errors while reading from %s file\n", filepath);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Errors while reading from", filepath);
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}
