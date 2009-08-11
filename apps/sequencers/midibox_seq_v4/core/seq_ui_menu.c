// $Id$
/*
 * Menu page (visible when EXIT button is pressed)
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
#include <seq_midi_out.h>
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 stopwatch_value;
static u32 stopwatch_value_max;
static s32 cpu_load_in_percent;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
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
  switch( encoder ) {
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
    case SEQ_UI_ENCODER_Datawheel: {
      u8 tmp = ui_selected_page; // required conversion from ui_page_t to u8
      if( SEQ_UI_Var8_Inc(&tmp, SEQ_UI_FIRST_MENU_SELECTION_PAGE, SEQ_UI_PAGES-1, incrementer) ) {
	ui_selected_page = tmp;
	return 1; // value changed
      }
      return 0; // value not changed
    } break;
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1: // clears stopwatch value
    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
      if( stopwatch_value_max ) {
	stopwatch_value_max = 0;
	return 1;
      }
      return -1;
      
    case SEQ_UI_BUTTON_GP12: // clears max counter
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      seq_midi_out_max_allocated = 0;
      return 1;

    case SEQ_UI_BUTTON_GP15: // clears drop counter
    case SEQ_UI_BUTTON_GP16:
      seq_midi_out_dropouts = 0;
      return 1;

    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

    case SEQ_UI_BUTTON_GP5:
    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
    case SEQ_UI_BUTTON_Exit:
      SEQ_UI_PageSet(ui_selected_page);
      return 1;
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
  // Stopwatch:          Select Menu Page:   MIDI Scheduler: Alloc xxx/xxx Drops: xxx
  // xxxxx/xxxxx uS      xxxxxxxxxxxxxxxxxx<>SD Card: not available     CPU Load: 52%


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  if( stopwatch_value_max ) {
    SEQ_LCD_PrintString("Stopwatch:");
    SEQ_LCD_PrintSpaces(10);
  } else
    SEQ_LCD_PrintSpaces(20);

  SEQ_LCD_PrintString("Select Menu Page:   ");

  SEQ_LCD_CursorSet(0, 1);
  if( stopwatch_value_max == 0xffffffff ) {
    SEQ_LCD_PrintFormattedString("Overrun!", stopwatch_value, stopwatch_value_max);
    SEQ_LCD_PrintSpaces(12);
  } else if( stopwatch_value_max ) {
    SEQ_LCD_PrintFormattedString("%5d/%5d uS", stopwatch_value, stopwatch_value_max);
    SEQ_LCD_PrintSpaces(6);
  } else
    SEQ_LCD_PrintSpaces(20);

  SEQ_LCD_PrintString(SEQ_UI_PageNameGet(ui_selected_page));
  SEQ_LCD_PrintChar((ui_selected_page == SEQ_UI_FIRST_MENU_SELECTION_PAGE) ? ' ' : '<');
  SEQ_LCD_PrintChar((ui_selected_page < (SEQ_UI_PAGES-1)) ? '>' : ' ');


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);
  if( seq_midi_out_allocated > 1000 || seq_midi_out_max_allocated > 1000 || seq_midi_out_dropouts > 1000 ) {
    SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %4d/%4d Dr: %4d",
      seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
  } else {
    SEQ_LCD_PrintFormattedString("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
      seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);
  }

  SEQ_LCD_CursorSet(40, 1);
  SEQ_LCD_PrintFormattedString("SD Card: ");
  char status_str[128];
  if( !SEQ_FILE_SDCardAvailable() ) {
    sprintf(status_str, "not connected");
  } else if( !SEQ_FILE_VolumeAvailable() ) {
    sprintf(status_str, "Invalid FAT  ");
  } else {
#if 0
    // disabled: determing the number of free bytes takes too long!
    sprintf(status_str, "%d/%d MB", 
	    SEQ_FILE_VolumeBytesFree()/1000000,
	    SEQ_FILE_VolumeBytesTotal()/1000000);
#else
    // print volume label instead
    sprintf(status_str, "\"%11s\"", SEQ_FILE_VolumeLabel());
#endif
  }
  SEQ_LCD_PrintStringPadded(status_str, 18);

  SEQ_LCD_PrintFormattedString("CPU Load: %02d%%", cpu_load_in_percent);


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Optional stopwatch for measuring performance
// Will be displayed on menu page once stopwatch_max > 0
// Value can be reseted by pressing GP button below the max number
// Usage example: see seq_core.c
// Only one task should control the stopwatch!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  MIOS32_STOPWATCH_Init(1); // 1 uS resolution

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets stopwatch
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_StopwatchReset(void)
{
  return MIOS32_STOPWATCH_Reset();
}


/////////////////////////////////////////////////////////////////////////////
// Captures value of stopwatch and displays on screen
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_StopwatchCapture(void)
{
  u32 value = MIOS32_STOPWATCH_ValueGet();
  portENTER_CRITICAL();
  stopwatch_value = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  portEXIT_CRITICAL();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Handles Idle Counter (frequently called from Background task)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_Idle(void)
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


