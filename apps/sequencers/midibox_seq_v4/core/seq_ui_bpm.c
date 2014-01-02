// $Id$
/*
 * BPM configuration page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file.h"
#include "seq_file_c.h"

#include "seq_bpm.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_port.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       7
#define ITEM_MODE          0
#define ITEM_PRESET        1
#define ITEM_BPM           2
#define ITEM_RAMP          3
#define ITEM_MCLK_PORT     4
#define ITEM_MCLK_IN       5
#define ITEM_MCLK_OUT      6


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 store_file_required;
static u8 selected_mclk_port = USB0;


static u32 tap_tempo_last_timestamp;
#define TAP_DELAY_STORAGE_SIZE 4
static u16 tap_tempo_delay[TAP_DELAY_STORAGE_SIZE];
static u8 tap_tempo_beat_ix;
static u8 tap_tempo_beat_ctr;


// resets the tap tempo variables
static s32 resetTapTempo(void)
{
  tap_tempo_last_timestamp = 0;
  tap_tempo_beat_ix = 0;
  tap_tempo_beat_ctr = 0;

  int tap;
  for(tap=0; tap<TAP_DELAY_STORAGE_SIZE; ++tap)
    tap_tempo_delay[tap] = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  // enable GP14/15 LED if external sync not finished
  if( seq_core_state.EXT_RESTART_REQ )
    *gp_leds |= 0x6000;

  // enable GP16 LED if tap tempo not finished
  if( tap_tempo_beat_ctr )
    *gp_leds |= 0x8000;

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_MODE: *gp_leds |= 0x0001; break;
    case ITEM_PRESET: *gp_leds |= 0x0002; break;
    case ITEM_BPM: *gp_leds |= 0x000c; break;
    case ITEM_RAMP: *gp_leds |= 0x0010; break;
    case ITEM_MCLK_PORT: *gp_leds |= 0x0100; break;
    case ITEM_MCLK_IN: *gp_leds |= 0x0200; break;
    case ITEM_MCLK_OUT: *gp_leds |= 0x0400; break;
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
  // reset tap tempo if any encoder != GP16 has been pressed
  if( encoder != SEQ_UI_ENCODER_GP16 )
    resetTapTempo();


  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_MODE;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_PRESET;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_BPM;
      // special feature: these two encoders increment *10
      incrementer *= 10;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_BPM;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_RAMP;
      break;

    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      return -1; // not mapped to encoder

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_MCLK_PORT;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_MCLK_IN;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_MCLK_OUT;
      break;

    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
      return -1; // not used (yet)

    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
      // external restart request should be atomic
      portENTER_CRITICAL();
      seq_core_state.EXT_RESTART_REQ = 1;
      portEXIT_CRITICAL();
      return 1;

    case SEQ_UI_ENCODER_GP16:
      SEQ_UI_BPM_TapTempo();
      return 1;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_MODE: {
      u8 value = SEQ_BPM_ModeGet();
      if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) ) {
	SEQ_BPM_ModeSet(value);
	store_file_required = 1;
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_PRESET: {
      return SEQ_UI_Var8_Inc(&seq_core_bpm_preset_num, 0, SEQ_CORE_NUM_BPM_PRESETS-1, incrementer);
    } break;

    case ITEM_BPM: {
      u16 value = (u16)(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num]*10);
      if( SEQ_UI_Var16_Inc(&value, 25, 3000, incrementer) ) { // at 384ppqn, the minimum BPM rate is ca. 2.5
	// set new BPM
      	seq_core_bpm_preset_tempo[seq_core_bpm_preset_num] = (float)value/10.0;
	SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);
	store_file_required = 1;
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_RAMP: {
      u16 value = (u16)seq_core_bpm_preset_ramp[seq_core_bpm_preset_num];
      if( SEQ_UI_Var16_Inc(&value, 0, 99, incrementer) ) {
	seq_core_bpm_preset_ramp[seq_core_bpm_preset_num] = (float)value;
	store_file_required = 1;
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_MCLK_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_ClkIxGet(selected_mclk_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_ClkNumGet()-1, incrementer) >= 0 ) {
	selected_mclk_port = SEQ_MIDI_PORT_ClkPortGet(port_ix);
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_MCLK_IN: {
      s32 status = SEQ_MIDI_ROUTER_MIDIClockInGet(selected_mclk_port);
      if( status < 0 )
	return 0; // no change
      u8 enable = status;
      if( SEQ_UI_Var8_Inc(&enable, 0, 1, incrementer) >= 0 ) {
	SEQ_MIDI_ROUTER_MIDIClockInSet(selected_mclk_port, enable);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_MCLK_OUT: {
      s32 status = SEQ_MIDI_ROUTER_MIDIClockOutGet(selected_mclk_port);
      if( status < 0 )
	return 0; // no change
      u8 enable = status;
      if( SEQ_UI_Var8_Inc(&enable, 0, 1, incrementer) >= 0 ) {
	SEQ_MIDI_ROUTER_MIDIClockOutSet(selected_mclk_port, enable);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
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

  // reset tap tempo if any button != GP16 has been pressed
  if( button != SEQ_UI_BUTTON_GP16 )
    resetTapTempo();

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    switch( button ) {
      case SEQ_UI_BUTTON_GP6:
      case SEQ_UI_BUTTON_GP7:
	// fire preset
	SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);
	return 1;

      case SEQ_UI_BUTTON_GP8:
	// enter preset selection page
	SEQ_UI_PageSet(SEQ_UI_PAGE_BPM_PRESETS);
	return 1;
    }

    // re-use encoder handler - only select UI item, don't increment
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
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
  //  Mode Preset Tempo  Ramp    Fire  Preset  MClk In/Out              Ext.    Tap 
  // Master   1   140.0   1s    Preset  Page USB1 I:on O:off           Restart Tempo

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString(" Mode Preset Tempo  Ramp    Fire  Preset  MClk In/Out               ");
  SEQ_LCD_PrintString(seq_core_state.EXT_RESTART_REQ ? "Ongoing" : " Ext.  ");
  SEQ_LCD_PrintString(" Tap ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    const char mode_str[3][7] = { " Auto ", "Master", "Slave "};
    SEQ_LCD_PrintString((char *)mode_str[SEQ_BPM_ModeGet()]);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PRESET && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintFormattedString("%2d", seq_core_bpm_preset_num+1);
  }
  SEQ_LCD_PrintSpaces(3);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_BPM && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    float bpm = seq_core_bpm_preset_tempo[seq_core_bpm_preset_num];
    SEQ_LCD_PrintFormattedString("%3d.%d", (int)bpm, (int)(10*bpm)%10);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RAMP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    float ramp = seq_core_bpm_preset_ramp[seq_core_bpm_preset_num];
    SEQ_LCD_PrintFormattedString("%2ds", (int)ramp);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("    Preset  Page  ");

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MCLK_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_ClkNameGet(SEQ_MIDI_PORT_ClkIxGet(selected_mclk_port)));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("I:");
  if( ui_selected_item == ITEM_MCLK_IN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    s32 status = SEQ_MIDI_ROUTER_MIDIClockInGet(selected_mclk_port);

    if( !SEQ_MIDI_PORT_ClkCheckAvailable(selected_mclk_port) )
      status = -1; // MIDI In port not available

    switch( status ) {
      case 0:  SEQ_LCD_PrintString("off"); break;
      case 1:  SEQ_LCD_PrintString("on "); break;
      default: SEQ_LCD_PrintString("---");
    }
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("O:");
  if( ui_selected_item == ITEM_MCLK_OUT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    s32 status = SEQ_MIDI_ROUTER_MIDIClockOutGet(selected_mclk_port);

    if( !SEQ_MIDI_PORT_ClkCheckAvailable(selected_mclk_port) )
      status = -1; // MIDI Out port not available

    switch( status ) {
      case 0:  SEQ_LCD_PrintString("off"); break;
      case 1:  SEQ_LCD_PrintString("on "); break;
      default: SEQ_LCD_PrintString("---");
    }
  }
  SEQ_LCD_PrintSpaces(3);

  // DIN Sync moved to CV configuration
  SEQ_LCD_PrintSpaces(3+4);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("Restart Tempo");


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    store_file_required = 0;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BPM_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  store_file_required = 0;

  // reset tap tempo function
  resetTapTempo();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// (Public) Tap Tempo Function
// Can also be accessed by pressing MENU+PLAY
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BPM_TapTempo(void)
{
  // determine current timestamp
  u32 timestamp = MIOS32_TIMESTAMP_Get();
  u32 delay = MIOS32_TIMESTAMP_GetDelay(tap_tempo_last_timestamp);

  // if delay < 100 (mS) we assume a bouncing button - ignore this tap!
  if( delay < 100 )
    return -1;

  // reset counter if last timestamp zero (first tap) or difference between timestamps > 2 seconds (< 30 BPM...)
  if( !tap_tempo_last_timestamp || delay > 2000 ) {
    resetTapTempo();
  } else {    
    // store measured delay
    tap_tempo_delay[tap_tempo_beat_ix] = delay;

    // increment index, wrap at number of stored tap delays
    tap_tempo_beat_ix = (tap_tempo_beat_ix + 1) % TAP_DELAY_STORAGE_SIZE;
  }

  // store timestamp
  tap_tempo_last_timestamp = timestamp;

  // take over BPM and start sequencer once we reached the 5th tap
  if( tap_tempo_beat_ctr >= TAP_DELAY_STORAGE_SIZE ) {
    // hold counter at delay storage size + 1 (for display)
    tap_tempo_beat_ctr = TAP_DELAY_STORAGE_SIZE + 1;

    // determine BPM
    int tap;
    int accumulated_delay = 0;
    for(tap=0; tap<TAP_DELAY_STORAGE_SIZE; ++tap)
      accumulated_delay += tap_tempo_delay[tap];
    u32 bpm = 60000000 / (accumulated_delay / TAP_DELAY_STORAGE_SIZE);

    // set BPM
    float bpmF = bpm / 1000.0;
    seq_core_bpm_preset_tempo[seq_core_bpm_preset_num] = bpmF;
    SEQ_BPM_Set(bpmF);

    // if in auto mode and BPM generator is clocked in slave mode:
    // change to master mode
    SEQ_BPM_CheckAutoMaster();

    // if sequencer not running: start it!
    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Start();

  } else {
    // increment counter
    ++tap_tempo_beat_ctr;
  }

  // print message
  char buffer1[30];
  if( tap_tempo_beat_ctr > TAP_DELAY_STORAGE_SIZE ) {
    float bpm = SEQ_BPM_Get();
    sprintf(buffer1, "Tap Tempo: %3d.%d", (int)bpm, (int)(10*bpm)%10);
  } else
    sprintf(buffer1, "Tap Tempo: ???.?");

  char buffer2[30];
  int i;
  for(i=0; i<((tap_tempo_beat_ix+1)*4); ++i)
    buffer2[i] = '>';
  for(; i<16; ++i)
    buffer2[i] = ' ';
  buffer2[i] = 0;
  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, buffer1, buffer2);

  return 0; // no error
}

