// $Id$
/*
 * User Interface Routines
 *
 * TODO: allow to loop the selected view while editing a track
 * TODO: add loop fx page for setting loop point interactively
 * TODO: scroll function should use this loop point
 * TODO: utility page in drum mode: allow to handle only selected instrument
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

// with this switch, seq_ui.h/seq_ui_pages.inc will create local variables
#define SEQ_UI_PAGES_INC_LOCAL_VARS 1

#include <mios32.h>
#include <string.h>
#include <blm_x.h>
#include <seq_midi_out.h>
#include <seq_midi_sysex.h>
#include <seq_bpm.h>

#include "tasks.h"
#include "seq_ui.h"
#include "seq_hwcfg.h"
#include "seq_lcd.h"
#include "seq_led.h"
#include "seq_midply.h"
#include "seq_core.h"
#include "seq_song.h"
#include "seq_par.h"
#include "seq_layer.h"
#include "seq_cc.h"
#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 seq_ui_display_update_req;
u8 seq_ui_display_init_req;

seq_ui_button_state_t seq_ui_button_state;

u8 ui_selected_group;
u16 ui_selected_tracks;
u8 ui_selected_par_layer;
u8 ui_selected_trg_layer;
u8 ui_selected_instrument;
u8 ui_selected_step_view;
u8 ui_selected_step;
u8 ui_selected_item;

u8 ui_selected_item;

u16 ui_hold_msg_ctr;

seq_ui_page_t ui_page;
seq_ui_page_t ui_selected_page;
seq_ui_page_t ui_stepview_prev_page;
seq_ui_page_t ui_trglayer_prev_page;
seq_ui_page_t ui_parlayer_prev_page;

volatile u8 ui_cursor_flash;
u16 ui_cursor_flash_ctr;

u8 ui_edit_name_cursor;
u8 ui_edit_preset_num_category;
u8 ui_edit_preset_num_label;

u8 ui_seq_pause;

seq_ui_remote_mode_t seq_ui_remote_mode;
seq_ui_remote_mode_t seq_ui_remote_active_mode;
mios32_midi_port_t seq_ui_remote_port;
mios32_midi_port_t seq_ui_remote_active_port;
u8 seq_ui_remote_id;
u16 seq_ui_remote_client_timeout_ctr;
u8 seq_ui_remote_force_lcd_update;
u8 seq_ui_remote_force_led_update;

u8 seq_ui_backup_req;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 (*ui_button_callback)(seq_ui_button_t button, s32 depressed);
static s32 (*ui_encoder_callback)(seq_ui_encoder_t encoder, s32 incrementer);
static s32 (*ui_led_callback)(u16 *gp_leds);
static s32 (*ui_lcd_callback)(u8 high_prio);
static s32 (*ui_exit_callback)(void);

static u16 ui_gp_leds;

#define NUM_BLM_LED_SRS (2*4)
static u8 ui_blm_leds[NUM_BLM_LED_SRS];

#define UI_MSG_MAX_CHAR 21
static char ui_msg[2][UI_MSG_MAX_CHAR];
static u16 ui_msg_ctr;
static seq_ui_msg_type_t ui_msg_type;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Init(u32 mode)
{
  int i;
  // clear all LEDs
  for(i=0; i<SEQ_LED_NUM_SR; ++i)
    SEQ_LED_SRSet(i, 0x00);

  // init selection variables
  ui_selected_group = 0;
  ui_selected_tracks = (1 << 0);
  ui_selected_par_layer = 0;
  ui_selected_trg_layer = 0;
  ui_selected_instrument = 0;
  ui_selected_step_view = 0;
  ui_selected_step = 0;
  ui_selected_item = 0;

  ui_hold_msg_ctr = 0;
  ui_msg_ctr = 0;

  ui_cursor_flash_ctr = 0;
  ui_cursor_flash = 0;

  seq_ui_button_state.ALL = 0;

  ui_seq_pause = 0;

  // visible GP pattern
  ui_gp_leds = 0x0000;

  // default remote mode
  seq_ui_remote_mode = SEQ_UI_REMOTE_MODE_AUTO;
  seq_ui_remote_active_mode = SEQ_UI_REMOTE_MODE_AUTO;
  seq_ui_remote_port = DEFAULT;
  seq_ui_remote_active_port = DEFAULT;
  seq_ui_remote_id = 0x00;
  seq_ui_remote_client_timeout_ctr = 0;
  seq_ui_remote_force_lcd_update = 0;
  seq_ui_remote_force_led_update = 0;

  // misc
  seq_ui_backup_req = 0;

  // change to edit page
  ui_page = SEQ_UI_PAGE_NONE;
  SEQ_UI_PageSet(SEQ_UI_PAGE_EDIT);


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Inits the speed mode of all encoders
// Auto mode should be used whenever:
//    - the edit screen is entered
//    - the group is changed
//    - a track is changed
//    - a layer is changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_InitEncSpeed(u32 auto_config)
{
  mios32_enc_config_t enc_config;

  if( auto_config ) {

    if( !seq_hwcfg_enc.auto_fast )
      return 0; // auto mode not enabled - ignore auto reconfiguration request

    switch( SEQ_PAR_AssignmentGet(SEQ_UI_VisibleTrackGet(), ui_selected_par_layer) ) {
      case SEQ_PAR_Type_Velocity:
      case SEQ_PAR_Type_Length:
      case SEQ_PAR_Type_CC:
      case SEQ_PAR_Type_PitchBend:
      case SEQ_PAR_Type_Probability:
      case SEQ_PAR_Type_Delay:
	seq_ui_button_state.FAST_ENCODERS = 1;
	break;

      default:
	seq_ui_button_state.FAST_ENCODERS = 0;
    }
  }

  // change for datawheel and GP encoders
  int enc;
  for(enc=0; enc<17; ++enc) {
    enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.speed = seq_ui_button_state.FAST_ENCODERS ? FAST : NORMAL;
    enc_config.cfg.speed_par = (enc == 0) ? seq_hwcfg_enc.datawheel_fast_speed : seq_hwcfg_enc.gp_fast_speed;
    MIOS32_ENC_ConfigSet(enc, enc_config);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Various installation routines for menu page LCD handlers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_InstallButtonCallback(void *callback)
{
  ui_button_callback = callback;
  return 0; // no error
}

s32 SEQ_UI_InstallEncoderCallback(void *callback)
{
  ui_encoder_callback = callback;
  return 0; // no error
}

s32 SEQ_UI_InstallLEDCallback(void *callback)
{
  ui_led_callback = callback;
  return 0; // no error
}

s32 SEQ_UI_InstallLCDCallback(void *callback)
{
  ui_lcd_callback = callback;
  return 0; // no error
}

s32 SEQ_UI_InstallExitCallback(void *callback)
{
  ui_exit_callback = callback;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Change the menu page
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PageSet(seq_ui_page_t page)
{
  if( page != ui_page ) {

    // call page exit callback
    if( ui_exit_callback != NULL )
      ui_exit_callback();

    // disable hooks of previous page and request re-initialisation
    portENTER_CRITICAL();
    ui_page = page;
    ui_button_callback = NULL;
    ui_encoder_callback = NULL;
    ui_led_callback = NULL;
    ui_lcd_callback = NULL;
    ui_exit_callback = NULL;
    portEXIT_CRITICAL();

    if( seq_hwcfg_button_beh.menu )
      seq_ui_button_state.MENU_PRESSED = 0; // MENU page selection finished

    // request display initialisation
    seq_ui_display_init_req = 1;
  }

  // for MENU button:
  // first page has been selected - display new screen
  seq_ui_button_state.MENU_FIRST_PAGE_SELECTED = 1;

  // for SEQ_UI_MENU which is accessible with EXIT button
  // remember the current selectable page
  if( ui_page >= SEQ_UI_FIRST_MENU_SELECTION_PAGE )
    ui_selected_page = ui_page;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of menu page (18 characters)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_UI_PageNameGet(seq_ui_page_t page)
{
  return ui_menu_pages[page].name;
}


/////////////////////////////////////////////////////////////////////////////
// Dedicated button functions
// Mapped to physical buttons in SEQ_UI_Button_Handler()
// Will also be mapped to MIDI keys later (for MIDI remote function)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_Button_GP(s32 depressed, u32 gp)
{
  // in MENU page: overrule GP buttons so long MENU button is pressed/active
  if( seq_ui_button_state.MENU_PRESSED ) {
    if( depressed ) return -1;
    SEQ_UI_PageSet(ui_shortcut_menu_pages[gp]);
  } else {
    // forward to menu page
    if( ui_button_callback != NULL )
      ui_button_callback(gp, depressed);
    ui_cursor_flash_ctr = 0;
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Left(s32 depressed)
{
  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Left, depressed);
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Right(s32 depressed)
{
  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Right, depressed);
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Down(s32 depressed)
{
  seq_ui_button_state.DOWN = depressed ? 0 : 1;

  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Down, depressed);
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Up(s32 depressed)
{
  seq_ui_button_state.UP = depressed ? 0 : 1;

  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Up, depressed);
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Stop(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if sequencer running: stop it
  // if sequencer already stopped: reset song position
#if MID_PLAYER_TEST
  if( SEQ_BPM_IsRunning() )
    SEQ_BPM_Stop();
  else
    SEQ_MIDPLY_Reset();
#else
  if( SEQ_BPM_IsRunning() )
    SEQ_BPM_Stop();
  else
    SEQ_CORE_Reset();
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Pause(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if in auto mode and BPM generator is clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

  // toggle pause mode
  ui_seq_pause ^= 1;

  // execute stop/continue depending on new mode
  if( ui_seq_pause )
    SEQ_BPM_Stop();
  else
    SEQ_BPM_Cont();

  return 0; // no error
}

static s32 SEQ_UI_Button_Play(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if in auto mode and BPM generator is clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

#if 0
  // if sequencer running: restart it
  // if sequencer stopped: continue at last song position
  if( SEQ_BPM_IsRunning() )
    SEQ_BPM_Start();
  else
    SEQ_BPM_Cont();
#else
  // always restart sequencer
  SEQ_BPM_Start();
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Rew(s32 depressed)
{
  seq_ui_button_state.REW = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  if( SEQ_SONG_ActiveGet() ) {
    portENTER_CRITICAL();
    SEQ_SONG_Rew();
    portEXIT_CRITICAL();
  } else
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "We are not", "in Song Mode!");

  return 0; // no error
}

static s32 SEQ_UI_Button_Fwd(s32 depressed)
{
  seq_ui_button_state.FWD = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  if( SEQ_SONG_ActiveGet() ) {
    portENTER_CRITICAL();
    SEQ_SONG_Fwd();
    portEXIT_CRITICAL();
  } else
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "We are not", "in Song Mode!");

  return 0; // no error
}

static s32 SEQ_UI_Button_Loop(s32 depressed)
{
  if( seq_hwcfg_button_beh.loop ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    // should be atomic
    portENTER_CRITICAL();
    seq_core_state.LOOP ^= 1;
  } else {
    // should be atomic
    portENTER_CRITICAL();
    // set mode
    seq_core_state.LOOP = depressed ? 0 : 1;
  }
  portEXIT_CRITICAL();

  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Loop Mode", seq_core_state.LOOP ? "    on" : "   off");

  return 0; // no error
}

static s32 SEQ_UI_Button_Scrub(s32 depressed)
{
  // double function: -> Loop if menu button pressed
  if( seq_ui_button_state.MENU_PRESSED )
    return SEQ_UI_Button_Loop(depressed);

  if( seq_hwcfg_button_beh.scrub ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.SCRUB ^= 1;
  } else {
    // set mode
    seq_ui_button_state.SCRUB = depressed ? 0 : 1;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Scrub Mode", seq_ui_button_state.SCRUB ? "    on" : "   off");

  return 0; // no error
}

static s32 SEQ_UI_Button_TempoPreset(s32 depressed)
{
  static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;

  if( seq_hwcfg_button_beh.tempo_preset ) {
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.TEMPO_PRESET ^= 1; // toggle TEMPO_PRESET pressed (will also be released once GP button has been pressed)
  } else {
    // set mode
    seq_ui_button_state.TEMPO_PRESET = depressed ? 0 : 1;
  }

  if( seq_ui_button_state.TEMPO_PRESET ) {
    prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_BPM_PRESETS);
  } else {
    if( ui_page == SEQ_UI_PAGE_BPM_PRESETS )
      SEQ_UI_PageSet(prev_page);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_TapTempo(s32 depressed)
{
  seq_ui_button_state.TAP_TEMPO = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_ExtRestart(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // should be atomic
  portENTER_CRITICAL();
  seq_core_state.EXT_RESTART_REQ = 1;
  portEXIT_CRITICAL();

  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "External Restart", "requested");

  return 0; // no error
}

static s32 SEQ_UI_Button_Metronome(s32 depressed)
{
  // double function: -> ExtRestart if menu button pressed
  if( seq_ui_button_state.MENU_PRESSED )
    return SEQ_UI_Button_ExtRestart(depressed);

  if( seq_hwcfg_button_beh.metronome ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    // should be atomic
    portENTER_CRITICAL();
    seq_core_state.METRONOME ^= 1;
  } else {
    // should be atomic
    portENTER_CRITICAL();
    // set mode
    seq_core_state.METRONOME = depressed ? 0 : 1;
  }
  portEXIT_CRITICAL();

  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Metronome", seq_core_state.METRONOME ? "    on" : "   off");

#if 0
  // metronome button can be used to trigger pattern file fixes
  if( !depressed )
    SEQ_PATTERN_FixAll();
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Record(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // change to utility page
  SEQ_UI_PageSet(SEQ_UI_PAGE_TRKREC);

  return 0; // no error
}

static s32 SEQ_UI_Button_Utility(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // change to utility page
  SEQ_UI_PageSet(SEQ_UI_PAGE_UTIL);

  return 0; // no error
}

static s32 SEQ_UI_Button_Copy(s32 depressed)
{
  static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;

  seq_ui_button_state.COPY = depressed ? 0 : 1;

  if( ui_page == SEQ_UI_PAGE_MIXER ) {
    if( depressed ) return -1;
    SEQ_UI_MIXER_Copy();
    return 1;
  } else {
    if( !depressed ) {
      prev_page = ui_page;
      SEQ_UI_PageSet(SEQ_UI_PAGE_UTIL);
    }

    s32 status = SEQ_UI_UTIL_CopyButton(depressed);

    if( depressed ) {
      if( prev_page != SEQ_UI_PAGE_UTIL )
	SEQ_UI_PageSet(prev_page);

      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Track", "copied");
    }

    return status;
  }
}

static s32 SEQ_UI_Button_Paste(s32 depressed)
{
  static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;

  seq_ui_button_state.PASTE = depressed ? 0 : 1;

  if( ui_page == SEQ_UI_PAGE_MIXER ) {
    if( depressed ) return -1;
    SEQ_UI_MIXER_Paste();
    return 1;
  } else {
    if( !depressed ) {
      prev_page = ui_page;
      SEQ_UI_PageSet(SEQ_UI_PAGE_UTIL);
    }

    s32 status = SEQ_UI_UTIL_PasteButton(depressed);

    if( depressed ) {
      if( prev_page != SEQ_UI_PAGE_UTIL )
	SEQ_UI_PageSet(prev_page);

      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Track", "pasted");
    }

    return status;
  }
}

static s32 SEQ_UI_Button_Clear(s32 depressed)
{
  seq_ui_button_state.CLEAR = depressed ? 0 : 1;

  if( ui_page == SEQ_UI_PAGE_MIXER ) {
    if( depressed ) return -1;
    SEQ_UI_MIXER_Clear();

    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Track", "cleared");

    return 1;
  } else {
    return SEQ_UI_UTIL_ClearButton(depressed);
  }
}

static s32 SEQ_UI_Button_Menu(s32 depressed)
{
  if( seq_hwcfg_button_beh.menu ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.MENU_FIRST_PAGE_SELECTED = 0;
    seq_ui_button_state.MENU_PRESSED ^= 1; // toggle MENU pressed (will also be released once GP button has been pressed)
  } else {
    // set mode
    seq_ui_button_state.MENU_FIRST_PAGE_SELECTED = 0;
    seq_ui_button_state.MENU_PRESSED = depressed ? 0 : 1;
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Select(s32 depressed)
{
  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED ) {
    seq_ui_button_state.SELECT_PRESSED = depressed ? 0 : 1;
    if( ui_button_callback != NULL )
      ui_button_callback(SEQ_UI_BUTTON_Select, depressed);
  }
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Exit(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  u8 prev_ui_page = ui_page;

  seq_ui_button_state.EXIT_PRESSED = depressed ? 0 : 1;

  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Exit, depressed);
  ui_cursor_flash_ctr = 0;

  // release all button states
  seq_ui_button_state.ALL = 0;

  // enter menu page if we were not there before
  if( prev_ui_page != SEQ_UI_PAGE_MENU )
    SEQ_UI_PageSet(SEQ_UI_PAGE_MENU);

  return 0; // no error
}

static s32 SEQ_UI_Button_Edit(s32 depressed)
{
  seq_ui_button_state.EXIT_PRESSED = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  // change to edit page
  SEQ_UI_PageSet(SEQ_UI_PAGE_EDIT);

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_Mute(s32 depressed)
{
  seq_ui_button_state.MUTE_PRESSED = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_MUTE);

  return 0; // no error
}

static s32 SEQ_UI_Button_Pattern(s32 depressed)
{
  seq_ui_button_state.PATTERN_PRESSED = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_PATTERN);

  return 0; // no error
}

static s32 SEQ_UI_Button_Song(s32 depressed)
{
  seq_ui_button_state.SONG_PRESSED = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  // toggle active mode if already in song page
  if( ui_page == SEQ_UI_PAGE_SONG ) {
    SEQ_SONG_ActiveSet(SEQ_SONG_ActiveGet() ? 0 : 1);
  }

  SEQ_UI_PageSet(SEQ_UI_PAGE_SONG);

  return 0; // no error
}

static s32 SEQ_UI_Button_Solo(s32 depressed)
{
  if( seq_hwcfg_button_beh.solo ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.SOLO ^= 1;
  } else {
    // set mode
    seq_ui_button_state.SOLO = depressed ? 0 : 1;
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Fast(s32 depressed)
{
  if( seq_hwcfg_button_beh.fast ) {
    // toggle mode
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.FAST_ENCODERS ^= 1;
  } else {
    // set mode
    seq_ui_button_state.FAST_ENCODERS = depressed ? 0 : 1;
  }

  SEQ_UI_InitEncSpeed(0); // no auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_All(s32 depressed)
{
  seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE = depressed ? 0 : 1;

  if( seq_hwcfg_button_beh.all ) {
    // toggle mode
    if( depressed ) return -1;
    seq_ui_button_state.CHANGE_ALL_STEPS ^= 1;
  } else {
    // set mode
    seq_ui_button_state.CHANGE_ALL_STEPS = depressed ? 0 : 1;
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_StepView(s32 depressed)
{
  //  static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;
  // also used by seq_ui_stepsel
  
  if( seq_hwcfg_button_beh.step_view ) {
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.STEP_VIEW ^= 1; // toggle STEP_VIEW pressed (will also be released once GP button has been pressed)
  } else {
    // set mode
    seq_ui_button_state.STEP_VIEW = depressed ? 0 : 1;
  }

  if( seq_ui_button_state.STEP_VIEW ) {
    ui_stepview_prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_STEPSEL);
  } else {
    if( ui_page == SEQ_UI_PAGE_STEPSEL )
      SEQ_UI_PageSet(ui_stepview_prev_page);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_TrackSel(s32 depressed)
{
  static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;

  if( seq_hwcfg_button_beh.track_sel ) {
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.TRACK_SEL ^= 1; // toggle TRACKSEL status (will also be released once GP button has been pressed)
  } else {
    seq_ui_button_state.TRACK_SEL = depressed ? 0 : 1;
  }

  if( seq_ui_button_state.TRACK_SEL ) {
    prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_TRACKSEL);
  } else {
    if( ui_page == SEQ_UI_PAGE_TRACKSEL )
      SEQ_UI_PageSet(prev_page);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Track(s32 depressed, u32 track_button)
{
  static u8 button_state = 0x0f; // all 4 buttons depressed

  if( track_button >= 4 ) return -2; // max. 4 track buttons

  if( depressed ) {
    button_state |= (1 << track_button);
    return 0; // no error
  }

  button_state &= ~(1 << track_button);

  if( button_state == (~(1 << track_button) & 0xf) ) {
    // if only one select button pressed: radio-button function (1 of 4)
    ui_selected_tracks = 1 << (track_button + 4*ui_selected_group);
  } else {
    // if more than one select button pressed: toggle function (4 of 4)
    ui_selected_tracks ^= 1 << (track_button + 4*ui_selected_group);
  }

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_Group(s32 depressed, u32 group)
{
  if( depressed ) return -1; // ignore when button depressed

  if( group >= 4 ) return -2; // max. 4 group buttons

  // if group has changed:
  if( group != ui_selected_group ) {
    // get current track selection
    u16 old_tracks = ui_selected_tracks >> (4*ui_selected_group);

    // select new group
    ui_selected_group = group;

    // take over old track selection
    ui_selected_tracks = old_tracks << (4*ui_selected_group);
  }

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_ParLayerSel(s32 depressed)
{
  // static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;
  // also used by seq_ui_parsel.c

  if( seq_hwcfg_button_beh.par_layer ) {
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.PAR_LAYER_SEL ^= 1; // toggle PARSEL status (will also be released once GP button has been pressed)
  } else {
    seq_ui_button_state.PAR_LAYER_SEL = depressed ? 0 : 1;
  }

  if( seq_ui_button_state.PAR_LAYER_SEL ) {
    ui_parlayer_prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_PARSEL);
  } else {
    if( ui_page == SEQ_UI_PAGE_PARSEL )
      SEQ_UI_PageSet(ui_parlayer_prev_page);
  }

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_ParLayer(s32 depressed, u32 par_layer)
{
  if( par_layer >= 3 ) return -2; // max. 3 parlayer buttons

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);

  if( num_p_layers <= 3 ) {
    // 3 layers: direct selection with LayerA/B/C button
    if( depressed ) return -1; // ignore when button depressed
    if( par_layer >= num_p_layers ) {
      char str1[21];
      sprintf(str1, "Parameter Layer %c", 'A'+par_layer);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, str1, "not available!");
    } else {
      seq_ui_button_state.PAR_LAYER_SEL = 0;
      ui_selected_par_layer = par_layer;
    }
  } else if( num_p_layers <= 4 ) {
    // 4 layers: LayerC Button toggles between C and D
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.PAR_LAYER_SEL = 0;
    if( par_layer == 2 )
      ui_selected_par_layer = (ui_selected_par_layer == 2) ? 3 : 2;
    else
      ui_selected_par_layer = par_layer;
  } else {
    // >4 layers: LayerA/B button selects directly, Layer C button enters layer selection page
    if( par_layer <= 1 ) {
      if( depressed ) return -1; // ignore when button depressed
      seq_ui_button_state.PAR_LAYER_SEL = 0;
      ui_selected_par_layer = par_layer;
    } else {
      return SEQ_UI_Button_ParLayerSel(depressed);
    }
  }

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_TrgLayerSel(s32 depressed)
{
  // static seq_ui_page_t prev_page = SEQ_UI_PAGE_NONE;
  // also used by seq_ui_trgsel.c

  if( seq_hwcfg_button_beh.trg_layer ) {
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.TRG_LAYER_SEL ^= 1; // toggle TRGSEL status (will also be released once GP button has been pressed)
  } else {
    seq_ui_button_state.TRG_LAYER_SEL = depressed ? 0 : 1;
  }

  if( seq_ui_button_state.TRG_LAYER_SEL ) {
    ui_trglayer_prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_TRGSEL);
  } else {
    if( ui_page == SEQ_UI_PAGE_TRGSEL )
      SEQ_UI_PageSet(ui_trglayer_prev_page);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_TrgLayer(s32 depressed, u32 trg_layer)
{
  if( trg_layer >= 3 ) return -2; // max. 3 trglayer buttons

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);

  if( event_mode != SEQ_EVENT_MODE_Drum && num_t_layers <= 3 ) {
    // 3 layers: direct selection with LayerA/B/C button
    if( depressed ) return -1; // ignore when button depressed
    if( trg_layer >= num_t_layers ) {
      char str1[21];
      sprintf(str1, "Trigger Layer %c", 'A'+trg_layer);
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, str1, "not available!");
    } else {
      seq_ui_button_state.TRG_LAYER_SEL = 0;
      ui_selected_trg_layer = trg_layer;
    }
  } else if( event_mode != SEQ_EVENT_MODE_Drum && num_t_layers <= 4 ) {
    // 4 layers: LayerC Button toggles between C and D
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.TRG_LAYER_SEL = 0;
    if( trg_layer == 2 )
      ui_selected_trg_layer = (ui_selected_trg_layer == 2) ? 3 : 2;
    else
      ui_selected_trg_layer = trg_layer;
  } else {
    // >4 layers or drum mode: LayerA/B button selects directly, Layer C button enters trigger selection page
    // also used for drum tracks
    if( trg_layer <= 1 ) {
      if( depressed ) return -1; // ignore when button depressed
      seq_ui_button_state.TRG_LAYER_SEL = 0;
      ui_selected_trg_layer = trg_layer;
    } else {
      return SEQ_UI_Button_TrgLayerSel(depressed);
    }
  }

  return 0; // no error
}


static s32 SEQ_UI_Button_Morph(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_TRKMORPH);

  return 0; // no error
}

static s32 SEQ_UI_Button_Mixer(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_MIXER);

  return 0; // no error
}

static s32 SEQ_UI_Button_Transpose(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_TRKTRAN);

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// Button handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value)
{
  int i;

  // send MIDI event in remote mode and exit
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return SEQ_MIDI_SYSEX_REMOTE_Client_SendButton(pin, pin_value);

  // ignore so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  // ignore during a backup is created
  if( seq_ui_backup_req )
    return -1;

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // request display update
  seq_ui_display_update_req = 1;

  // stop current message if (new) button has been pressed
  if( pin_value == 0 )
    SEQ_UI_MsgStop();


  // MEMO: we could also use a jump table with references to the functions
  // here, but this "spagetthi code" simplifies the configuration and
  // the resulting ASM doesn't look that bad!

  for(i=0; i<SEQ_HWCFG_NUM_GP; ++i)
    if( pin == seq_hwcfg_button.gp[i] )
      return SEQ_UI_Button_GP(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_TRACK; ++i)
    if( pin == seq_hwcfg_button.track[i] )
      return SEQ_UI_Button_Track(pin_value, i);
  if( pin == seq_hwcfg_button.track_sel )
    return SEQ_UI_Button_TrackSel(pin_value);

  for(i=0; i<SEQ_HWCFG_NUM_GROUP; ++i)
    if( pin == seq_hwcfg_button.group[i] )
      return SEQ_UI_Button_Group(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_PAR_LAYER; ++i)
    if( pin == seq_hwcfg_button.par_layer[i] )
      return SEQ_UI_Button_ParLayer(pin_value, i);
  if( pin == seq_hwcfg_button.par_layer_sel )
    return SEQ_UI_Button_ParLayerSel(pin_value);

  for(i=0; i<SEQ_HWCFG_NUM_TRG_LAYER; ++i)
    if( pin == seq_hwcfg_button.trg_layer[i] )
      return SEQ_UI_Button_TrgLayer(pin_value, i);
  if( pin == seq_hwcfg_button.trg_layer_sel )
    return SEQ_UI_Button_TrgLayerSel(pin_value);

  if( pin == seq_hwcfg_button.left )
    return SEQ_UI_Button_Left(pin_value);
  if( pin == seq_hwcfg_button.right )
    return SEQ_UI_Button_Right(pin_value);
  if( pin == seq_hwcfg_button.down )
    return SEQ_UI_Button_Down(pin_value);
  if( pin == seq_hwcfg_button.up )
    return SEQ_UI_Button_Up(pin_value);

  if( pin == seq_hwcfg_button.scrub )
    return SEQ_UI_Button_Scrub(pin_value);
  if( pin == seq_hwcfg_button.metronome )
    return SEQ_UI_Button_Metronome(pin_value);

  if( pin == seq_hwcfg_button.record )
    return SEQ_UI_Button_Record(pin_value);

  if( pin == seq_hwcfg_button.stop )
    return SEQ_UI_Button_Stop(pin_value);
  if( pin == seq_hwcfg_button.pause )
    return SEQ_UI_Button_Pause(pin_value);
  if( pin == seq_hwcfg_button.play )
    return SEQ_UI_Button_Play(pin_value);
  if( pin == seq_hwcfg_button.rew )
    return SEQ_UI_Button_Rew(pin_value);
  if( pin == seq_hwcfg_button.fwd )
    return SEQ_UI_Button_Fwd(pin_value);
  if( pin == seq_hwcfg_button.loop )
    return SEQ_UI_Button_Loop(pin_value);

  if( pin == seq_hwcfg_button.utility )
    return SEQ_UI_Button_Utility(pin_value);
  if( pin == seq_hwcfg_button.copy )
    return SEQ_UI_Button_Copy(pin_value);
  if( pin == seq_hwcfg_button.paste )
    return SEQ_UI_Button_Paste(pin_value);
  if( pin == seq_hwcfg_button.clear )
    return SEQ_UI_Button_Clear(pin_value);

  if( pin == seq_hwcfg_button.menu )
    return SEQ_UI_Button_Menu(pin_value);
  if( pin == seq_hwcfg_button.select )
    return SEQ_UI_Button_Select(pin_value);
  if( pin == seq_hwcfg_button.exit )
    return SEQ_UI_Button_Exit(pin_value);

  if( pin == seq_hwcfg_button.tap_tempo )
    return SEQ_UI_Button_TapTempo(pin_value);
  if( pin == seq_hwcfg_button.tempo_preset )
    return SEQ_UI_Button_TempoPreset(pin_value);
  if( pin == seq_hwcfg_button.ext_restart )
    return SEQ_UI_Button_ExtRestart(pin_value);

  if( pin == seq_hwcfg_button.edit )
    return SEQ_UI_Button_Edit(pin_value);
  if( pin == seq_hwcfg_button.mute )
    return SEQ_UI_Button_Mute(pin_value);
  if( pin == seq_hwcfg_button.pattern )
    return SEQ_UI_Button_Pattern(pin_value);
  if( pin == seq_hwcfg_button.song )
    return SEQ_UI_Button_Song(pin_value);

  if( pin == seq_hwcfg_button.solo )
    return SEQ_UI_Button_Solo(pin_value);
  if( pin == seq_hwcfg_button.fast )
    return SEQ_UI_Button_Fast(pin_value);
  if( pin == seq_hwcfg_button.all )
    return SEQ_UI_Button_All(pin_value);

  if( pin == seq_hwcfg_button.step_view )
    return SEQ_UI_Button_StepView(pin_value);
  if( pin == seq_hwcfg_button.tap_tempo )
    return SEQ_UI_Button_TapTempo(pin_value);

  if( pin == seq_hwcfg_button.morph )
    return SEQ_UI_Button_Morph(pin_value);
  if( pin == seq_hwcfg_button.mixer )
    return SEQ_UI_Button_Mixer(pin_value);
  if( pin == seq_hwcfg_button.transpose )
    return SEQ_UI_Button_Transpose(pin_value);

  // always print debugging message
#if 1
  MIOS32_MIDI_SendDebugMessage("[SEQ_UI_Button_Handler] Button SR:%d, Pin:%d not mapped, it has been %s.\n", 
			       (pin >> 3) + 1,
			       pin & 7,
			       pin_value ? "depressed" : "pressed");
#endif

  return -1; // button not mapped
}


/////////////////////////////////////////////////////////////////////////////
// BLM Button handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BLM_Button_Handler(u32 row, u32 pin, u32 pin_value)
{
  // send MIDI event in remote mode and exit
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return SEQ_MIDI_SYSEX_REMOTE_Client_Send_BLM_Button(row, pin, pin_value);

  // ignore so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  // ignore during a backup is created
  if( seq_ui_backup_req )
    return -1;

  if( row >= SEQ_CORE_NUM_TRACKS_PER_GROUP )
    return -1; // more than 4 tracks not supported (yet) - could be done in this function w/o much effort

  if( pin >= 16 )
    return -1; // more than 16 step buttons not supported (yet) - could be done by selecting the step view

  // select track depending on row
  ui_selected_tracks = 1 << (row + 4*ui_selected_group);

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // request display update
  seq_ui_display_update_req = 1;

  // emulate general purpose button
  return SEQ_UI_Button_GP(pin_value, pin);
}


/////////////////////////////////////////////////////////////////////////////
// Encoder handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer)
{
  // send MIDI event in remote mode and exit
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return SEQ_MIDI_SYSEX_REMOTE_Client_SendEncoder(encoder, incrementer);

  // ignore so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  // ignore during a backup is created
  if( seq_ui_backup_req )
    return -1;

  if( encoder > 16 )
    return -1; // encoder doesn't exist

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // stop current message
  SEQ_UI_MsgStop();

  // limit incrementer
  if( incrementer > 3 )
    incrementer = 3;
  else if( incrementer < -3 )
    incrementer = -3;

  if( seq_ui_button_state.SCRUB && encoder == 0 ) {
    // if sequencer isn't already running, continue it (don't restart)
    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Cont();
    ui_seq_pause = 0; // clear pause mode
    // Scrub sequence back or forth
    portENTER_CRITICAL(); // should be atomic
    SEQ_CORE_Scrub(incrementer);
    portEXIT_CRITICAL();
  } else if( !seq_ui_button_state.MENU_PRESSED && ui_encoder_callback != NULL ) {
    ui_encoder_callback((encoder == 0) ? SEQ_UI_ENCODER_Datawheel : (encoder-1), incrementer);
    ui_cursor_flash_ctr = 0; // ensure that value is visible when it has been changed
  }

  // request display update
  seq_ui_display_update_req = 1;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_REMOTE_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
#if 1
  // check for active remote mode
  if( seq_ui_remote_active_mode != SEQ_UI_REMOTE_MODE_SERVER )
    return 0; // no error
#endif

  if( (seq_ui_remote_port == DEFAULT && seq_ui_remote_active_port != port) ||
      (seq_ui_remote_port != DEFAULT && port != seq_ui_remote_port) )
    return 0; // wrong port

  // for easier parsing: convert Note Off -> Note On with velocity 0
  if( midi_package.event == NoteOff ) {
    midi_package.event = NoteOn;
    midi_package.velocity = 0;
  }

  switch( midi_package.event ) {
    case NoteOn: {
      switch( midi_package.chn ) {
        case Chn1:
	  SEQ_UI_Button_Handler(midi_package.note + 0x00, midi_package.velocity ? 0 : 1);
	  break;
        case Chn2:
	  SEQ_UI_Button_Handler(midi_package.note + 0x80, midi_package.velocity ? 0 : 1);
	  break;
        case Chn3:
	  SEQ_UI_BLM_Button_Handler(midi_package.note >> 5, midi_package.note & 0x1f, midi_package.velocity ? 0 : 1);
	  break;
      }	
    } break;

    case CC: {
      if( midi_package.cc_number >= 15 && midi_package.cc_number <= 31 )
	SEQ_UI_Encoder_Handler(midi_package.cc_number-15, (int)midi_package.value - 0x40);
    } break;
  }

  return 1; // MIDI event has been taken for remote function -> don't forward to router/MIDI event parser
}


/////////////////////////////////////////////////////////////////////////////
// Update LCD messages
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Handler(void)
{
  // special handling in remote client mode
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return SEQ_UI_LCD_Update();

  if( seq_ui_display_init_req ) {
    seq_ui_display_init_req = 0; // clear request

    // clear force update of LCD
    SEQ_LCD_Clear();
    SEQ_LCD_Update(1);

    // select first menu item
    ui_selected_item = 0;

    // call init function of current page
    if( ui_menu_pages[ui_page].init_callback != NULL )
      ui_menu_pages[ui_page].init_callback(0); // mode

    // request display update
    seq_ui_display_update_req = 1;
  }

  // print boot screen so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() ) {
    SEQ_LCD_Clear();

    SEQ_LCD_CursorSet(0, 0);
    //                   <-------------------------------------->
    //                   0123456789012345678901234567890123456789
    SEQ_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE2);
      
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintString("Searching for SD Card...");
    // TODO: print nice animation
  } else if( seq_ui_backup_req ) {
    SEQ_LCD_Clear();
    SEQ_LCD_CursorSet(0, 0);
    //                   <-------------------------------------->
    //                   0123456789012345678901234567890123456789
    SEQ_LCD_PrintString("Creating File Backup - be patient!!!");

    if( seq_file_backup_notification != NULL ) {
      int i;

      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintFormattedString("Creating '%s'", seq_file_backup_notification);

      SEQ_LCD_CursorSet(40+3, 0);
      SEQ_LCD_PrintString("Total: [");
      for(i=0; i<20; ++i)
	SEQ_LCD_PrintChar((i>(seq_file_backup_percentage/5)) ? ' ' : '#');
      SEQ_LCD_PrintFormattedString("] %3d%%", seq_file_backup_percentage);

      SEQ_LCD_CursorSet(40+3, 1);
      SEQ_LCD_PrintString("File:  [");
      for(i=0; i<20; ++i)
	SEQ_LCD_PrintChar((i>(seq_file_copy_percentage/5)) ? ' ' : '#');
      SEQ_LCD_PrintFormattedString("] %3d%%", seq_file_copy_percentage);
    }

  } else if( seq_ui_button_state.MENU_PRESSED && !seq_ui_button_state.MENU_FIRST_PAGE_SELECTED ) {
    SEQ_LCD_CursorSet(0, 0);
    //                   <-------------------------------------->
    //                   0123456789012345678901234567890123456789
    SEQ_LCD_PrintString("Menu Shortcuts:");
    SEQ_LCD_PrintSpaces(25 + 40);
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString(UI_SHORTCUT_STR); // defined in seq_ui_pages.inc
  } else {
    // perform high priority LCD update request
    if( ui_lcd_callback != NULL )
      ui_lcd_callback(1); // high_prio

    // perform low priority LCD update request if requested
    if( seq_ui_display_update_req ) {
      seq_ui_display_update_req = 0; // clear request

      // ensure that selections are matching with track constraints
      SEQ_UI_CheckSelections();

      if( ui_lcd_callback != NULL )
	ui_lcd_callback(0); // no high_prio
    }
  }

  // transfer all changed characters to LCD
  SEQ_UI_LCD_Update();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_UI_LCD_Handler(), but optionally also from other tasks
// to update the LCD screen immediately
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Update(void)
{
  // special handling in remote client mode
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT ) {
    MIOS32_IRQ_Disable();
    u8 force = seq_ui_remote_force_lcd_update;
    seq_ui_remote_force_lcd_update = 0;
    MIOS32_IRQ_Enable();
    return SEQ_LCD_Update(force);
  }

  // if UI message active: copy over the text
  if( ui_msg_ctr ) {
    const char *animation_l_ptr;
    const char *animation_r_ptr;
    u8 msg_x = 0;
    u8 right_aligned = 0;

    switch( ui_msg_type ) {
      case SEQ_UI_MSG_SDCARD: {
	//                             00112233
	const char animation_l[2*4] = "   >>>> ";
	//                             00112233
	const char animation_r[2*4] = "  < << <";
	animation_l_ptr = animation_l;
	animation_r_ptr = animation_r;
	msg_x = 0; // MEMO: print such important information at first LCD for the case the user hasn't connected the second LCD yet
	right_aligned = 0;
      } break;

      default: {
	//                             00112233
	const char animation_l[2*4] = "   **** ";
	//                             00112233
	const char animation_r[2*4] = "  * ** *";
	animation_l_ptr = animation_l;
	animation_r_ptr = animation_r;
	msg_x = 39;
	right_aligned = 1;
      } break;

    }
    int anum = (ui_msg_ctr % 1000) / 250;

    int len[2];
    len[0] = strlen((char *)ui_msg[0]);
    len[1] = strlen((char *)ui_msg[1]);
    int len_max = len[0];
    if( len[1] > len_max )
      len_max = len[1];

    if( right_aligned )
      msg_x -= (9 + len_max);

    int line;
    for(line=0; line<2; ++line) {
      SEQ_LCD_CursorSet(msg_x, line);

      // ensure that both lines are padded with same number of spaces
      int end_pos = len[line];
      while( end_pos < len_max )
	ui_msg[line][end_pos++] = ' ';
      ui_msg[line][end_pos] = 0;

      SEQ_LCD_PrintFormattedString(" %c%c| %s |%c%c ",
				   *(animation_l_ptr + 2*anum + 0), *(animation_l_ptr + 2*anum + 1),
				   (char *)ui_msg[line], 
				   *(animation_r_ptr + 2*anum + 0), *(animation_r_ptr + 2*anum + 1));
    }
  }

  // MSD USB notification at right corner if not in Disk page
  // to warn user that USB MIDI is disabled and seq performance is bad now!
  if( (TASK_MSD_EnableGet() > 0) && ui_page != SEQ_UI_PAGE_DISK ) {
    SEQ_LCD_CursorSet(80-11, 0);
    if( ui_cursor_flash ) SEQ_LCD_PrintSpaces(13); else SEQ_LCD_PrintString(" [MSD USB] ");
    SEQ_LCD_CursorSet(80-11, 1);
    char str[5];
    TASK_MSD_FlagStrGet(str);
    if( ui_cursor_flash ) SEQ_LCD_PrintSpaces(13); else SEQ_LCD_PrintFormattedString(" [ %s  ] ", str);
  }

  // transfer all changed characters to LCD
  // SEQ_LCD_Update provides a MUTEX handling to allow updates from different tasks
  return SEQ_LCD_Update(0);
}


/////////////////////////////////////////////////////////////////////////////
// Update all LEDs
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler(void)
{
  static u8 remote_led_sr[SEQ_LED_NUM_SR];

  int i;

  // ignore in remote client mode
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return 0; // no error

  // ignore so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // track LEDs
  u8 selected_tracks = ui_selected_tracks >> (4*ui_selected_group);
  SEQ_LED_PinSet(seq_hwcfg_led.track[0], (selected_tracks & (1 << 0)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[1], (selected_tracks & (1 << 1)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[2], (selected_tracks & (1 << 2)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[3], (selected_tracks & (1 << 3)));
  SEQ_LED_PinSet(seq_hwcfg_led.track_sel, seq_ui_button_state.TRACK_SEL);
  
  // parameter layer LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[0], (ui_selected_par_layer == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[1], (ui_selected_par_layer == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[2], (ui_selected_par_layer >= 2) || seq_ui_button_state.PAR_LAYER_SEL);
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer_sel, seq_ui_button_state.PAR_LAYER_SEL);
  
  // group LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.group[0], (ui_selected_group == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.group[1], (ui_selected_group == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.group[2], (ui_selected_group == 2));
  SEQ_LED_PinSet(seq_hwcfg_led.group[3], (ui_selected_group == 3));
  
  // trigger layer LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[0], (ui_selected_trg_layer == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[1], (ui_selected_trg_layer == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[2], (ui_selected_trg_layer >= 2) || seq_ui_button_state.TRG_LAYER_SEL);
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer_sel, seq_ui_button_state.TRG_LAYER_SEL);
  
  // remaining LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.edit, ui_page == SEQ_UI_PAGE_EDIT);
  SEQ_LED_PinSet(seq_hwcfg_led.mute, ui_page == SEQ_UI_PAGE_MUTE);
  SEQ_LED_PinSet(seq_hwcfg_led.pattern, ui_page == SEQ_UI_PAGE_PATTERN);
  if( SEQ_SONG_ActiveGet() )
    SEQ_LED_PinSet(seq_hwcfg_led.song, 1);
  else
    SEQ_LED_PinSet(seq_hwcfg_led.song, ui_cursor_flash ? 0 : (ui_page == SEQ_UI_PAGE_SONG));
  
  SEQ_LED_PinSet(seq_hwcfg_led.solo, seq_ui_button_state.SOLO);
  SEQ_LED_PinSet(seq_hwcfg_led.fast, seq_ui_button_state.FAST_ENCODERS);
  SEQ_LED_PinSet(seq_hwcfg_led.all, seq_ui_button_state.CHANGE_ALL_STEPS);
  
  SEQ_LED_PinSet(seq_hwcfg_led.play, SEQ_BPM_IsRunning());
  SEQ_LED_PinSet(seq_hwcfg_led.stop, !SEQ_BPM_IsRunning() && !ui_seq_pause);
  SEQ_LED_PinSet(seq_hwcfg_led.pause, ui_seq_pause);

  SEQ_LED_PinSet(seq_hwcfg_led.rew, seq_ui_button_state.REW);
  SEQ_LED_PinSet(seq_hwcfg_led.fwd, seq_ui_button_state.FWD);

  SEQ_LED_PinSet(seq_hwcfg_led.loop, seq_core_state.LOOP);
  
  SEQ_LED_PinSet(seq_hwcfg_led.step_view, seq_ui_button_state.STEP_VIEW);

  SEQ_LED_PinSet(seq_hwcfg_led.exit, seq_ui_button_state.EXIT_PRESSED);
  SEQ_LED_PinSet(seq_hwcfg_led.select, seq_ui_button_state.SELECT_PRESSED);
  SEQ_LED_PinSet(seq_hwcfg_led.menu, seq_ui_button_state.MENU_PRESSED);

  // handle double functions
  if( seq_ui_button_state.MENU_PRESSED ) {
    SEQ_LED_PinSet(seq_hwcfg_led.scrub, seq_core_state.LOOP);
    SEQ_LED_PinSet(seq_hwcfg_led.metronome, seq_core_state.EXT_RESTART_REQ);
  } else {
    SEQ_LED_PinSet(seq_hwcfg_led.scrub, seq_ui_button_state.SCRUB);
    SEQ_LED_PinSet(seq_hwcfg_led.metronome, seq_core_state.METRONOME);
  }

  SEQ_LED_PinSet(seq_hwcfg_led.record, ui_page == SEQ_UI_PAGE_TRKREC);

  SEQ_LED_PinSet(seq_hwcfg_led.utility, ui_page == SEQ_UI_PAGE_UTIL);
  SEQ_LED_PinSet(seq_hwcfg_led.copy, seq_ui_button_state.COPY);
  SEQ_LED_PinSet(seq_hwcfg_led.paste, seq_ui_button_state.PASTE);
  SEQ_LED_PinSet(seq_hwcfg_led.clear, seq_ui_button_state.CLEAR);

  SEQ_LED_PinSet(seq_hwcfg_led.tap_tempo, seq_ui_button_state.TAP_TEMPO);
  SEQ_LED_PinSet(seq_hwcfg_led.tempo_preset, seq_ui_button_state.TEMPO_PRESET);
  SEQ_LED_PinSet(seq_hwcfg_led.ext_restart, seq_core_state.EXT_RESTART_REQ);

  SEQ_LED_PinSet(seq_hwcfg_led.down, seq_ui_button_state.DOWN);
  SEQ_LED_PinSet(seq_hwcfg_led.up, seq_ui_button_state.UP);


  // in MENU page: overrule GP LEDs so long MENU button is pressed/active
  if( seq_ui_button_state.MENU_PRESSED ) {
    if( ui_cursor_flash ) // if flashing flag active: no LED flag set
      ui_gp_leds = 0x0000;
    else {
      int i;
      u16 new_ui_gp_leds = 0x0000;
      for(i=0; i<16; ++i)
	if( ui_page == ui_shortcut_menu_pages[i] )
	  new_ui_gp_leds |= (1 << i);
      ui_gp_leds = new_ui_gp_leds;
    }
  } else {
    // note: the background function is permanently interrupted - therefore we write the GP pattern
    // into a temporary variable, and take it over once completed
    u16 new_ui_gp_leds = 0x0000;
    // request GP LED values from current menu page
    // will be transfered to DOUT registers in SEQ_UI_LED_Handler_Periodic
    new_ui_gp_leds = 0x0000;

    if( ui_led_callback != NULL )
      ui_led_callback(&new_ui_gp_leds);

    ui_gp_leds = new_ui_gp_leds;
  }


  // BLM LEDs
  u8 visible_track0 = 4*ui_selected_group;
  u8 visible_sr0  = 2*ui_selected_step_view;

  for(i=0; i<NUM_BLM_LED_SRS; ++i)
    ui_blm_leds[i] = SEQ_TRG_Get8(visible_track0+(i>>1), visible_sr0+(i&1), ui_selected_trg_layer, ui_selected_instrument);


  // send LED changes in remote server mode
  if( seq_ui_remote_mode == SEQ_UI_REMOTE_MODE_SERVER || seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_SERVER ) {
    int first_sr = -1;
    int last_sr = -1;
    int sr;
    for(sr=0; sr<SEQ_LED_NUM_SR; ++sr) {
      u8 value = SEQ_LED_SRGet(sr);
      if( value != remote_led_sr[sr] ) {
	if( first_sr == -1 )
	  first_sr = sr;
	last_sr = sr;
	remote_led_sr[sr] = value;
      }
    }

    MIOS32_IRQ_Disable();
    if( seq_ui_remote_force_led_update ) {
      first_sr = 0;
      last_sr = SEQ_LED_NUM_SR-1;
    }
    seq_ui_remote_force_led_update = 0;
    MIOS32_IRQ_Enable();

    if( first_sr >= 0 )
      SEQ_MIDI_SYSEX_REMOTE_Server_SendLED(first_sr, (u8 *)&remote_led_sr[first_sr], last_sr-first_sr+1);
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// updates high-prio LED functions (GP LEDs and Beat LED)
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler_Periodic()
{
  // ignore in remote client mode
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return 0; // no error

  // ignore so long hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  // GP LEDs are only updated when ui_gp_leds or pos_marker_mask has changed
  static u16 prev_ui_gp_leds = 0x0000;
  static u16 prev_pos_marker_mask = 0x0000;

  // beat LED: tmp. for demo w/o real sequencer
  u8 sequencer_running = SEQ_BPM_IsRunning();
  SEQ_LED_PinSet(seq_hwcfg_led.beat, sequencer_running && ((seq_core_state.ref_step & 3) == 0));

  // for song position marker (supports 16 LEDs, check for selected step view)
  u16 pos_marker_mask = 0x0000;
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 played_step = seq_core_trk[visible_track].step;
  if( seq_ui_button_state.STEP_VIEW ) {
    // if STEP_VIEW button pressed: pos marker correlated to zoom ratio
    if( sequencer_running )
      pos_marker_mask = 1 << (played_step / (SEQ_TRG_NumStepsGet(visible_track)/16));
  } else {
    if( sequencer_running && (played_step >> 4) == ui_selected_step_view )
      pos_marker_mask = 1 << (played_step & 0xf);
  }

  // exit of pattern hasn't changed
  if( prev_ui_gp_leds == ui_gp_leds && prev_pos_marker_mask == pos_marker_mask )
    return 0;
  prev_ui_gp_leds = ui_gp_leds;
  prev_pos_marker_mask = pos_marker_mask;

  // transfer to GP LEDs
  if( seq_hwcfg_led.gp_dout_l_sr ) {
    if( seq_hwcfg_led.gp_dout_l2_sr )
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_l_sr-1, (ui_gp_leds >> 0) & 0xff);
    else
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_l_sr-1, ((ui_gp_leds ^ pos_marker_mask) >> 0) & 0xff);
  }

  if( seq_hwcfg_led.gp_dout_r_sr ) {
    if( seq_hwcfg_led.gp_dout_r2_sr )
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_r_sr-1, (ui_gp_leds >> 8) & 0xff);
    else
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_r_sr-1, ((ui_gp_leds ^ pos_marker_mask) >> 8) & 0xff);
  }

  if( seq_hwcfg_led.gp_dout_l2_sr )
    SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_l2_sr-1, (pos_marker_mask >> 0) & 0xff);
  if( seq_hwcfg_led.gp_dout_r2_sr )
    SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_r2_sr-1, (pos_marker_mask >> 8) & 0xff);


  if( seq_hwcfg_blm.enabled ) {
    // Red LEDs (position marker)
    int track_ix;
    for(track_ix=0; track_ix<(NUM_BLM_LED_SRS/2); ++track_ix) {
      u16 pos_marker_mask = 0x0000;
      if( sequencer_running ) {
	u8 track = ui_selected_group + track_ix;
	u8 played_step = seq_core_trk[track].step;
	if( (played_step >> 4) == ui_selected_step_view )
	  pos_marker_mask = 1 << (played_step & 0xf);
      }

      // Prepare Green LEDs (triggers)
      u8 green_l = ui_blm_leds[2*track_ix+0];
      u8 green_r = ui_blm_leds[2*track_ix+1];

      // Red LEDs (position marker)
      if( seq_hwcfg_blm.dout_duocolour ) {
	BLM_DOUT_SRSet(1, 2*track_ix+0, pos_marker_mask & 0xff);
	BLM_DOUT_SRSet(1, 2*track_ix+1, (pos_marker_mask >> 8) & 0xff);

	if( seq_hwcfg_blm.dout_duocolour == 2 ) {
	  // Colour Mode 2: clear green LED, so that only one LED is lit
	  if( pos_marker_mask & 0x00ff )
	    green_l &= ~pos_marker_mask;
	  else if( pos_marker_mask & 0xff00 )
	    green_r &= ~(pos_marker_mask >> 8);
	}
      } else {
	// If Duo-LEDs not enabled: invert Green LEDs
	if( pos_marker_mask & 0x00ff )
	  green_l ^= pos_marker_mask;
	else if( pos_marker_mask & 0xff00 )
	  green_r ^= (pos_marker_mask >> 8);
      }

      // Set Green LEDs
      BLM_DOUT_SRSet(0, 2*track_ix+0, green_l);
      BLM_DOUT_SRSet(0, 2*track_ix+1, green_r);
    }
  }


  if( seq_hwcfg_blm8x8.enabled && seq_hwcfg_blm8x8.dout_gp_mapping ) {
    // for wilba's frontpanel

    // BLM_X DOUT -> GP LED mapping
    // 0 = 15,16	1 = 13,14	2 = 11,12	3 = 9,10
    // 4 = 1,2	5 = 3,4		6 = 5,6		7 = 7,8

    // bit 7: first green (i.e. GP1-G)
    // bit 6: first red (i.e. GP1-R)
    // bit 5: second green (i.e. GP2-G)
    // bit 4: second red (i.e. GP2-R)

    // this mapping routine takes ca. 5 uS
    // since it's only executed when ui_gp_leds or gp_mask has changed, it doesn't really hurt

    u16 modified_gp_leds = ui_gp_leds;
#if 1
    // extra: red LED is lit exclusively for higher contrast
    modified_gp_leds &= ~pos_marker_mask;
#endif

    int sr;
    const u8 blm_x_sr_map[8] = {4, 5, 6, 7, 3, 2, 1, 0};
    u16 gp_mask = 1 << 0;
    for(sr=0; sr<8; ++sr) {
      u8 pattern = 0;

      if( modified_gp_leds & gp_mask )
	pattern |= 0x80;
      if( pos_marker_mask & gp_mask )
	pattern |= 0x40;
      gp_mask <<= 1;
      if( modified_gp_leds & gp_mask )
	pattern |= 0x20;
      if( pos_marker_mask & gp_mask )
	pattern |= 0x10;
      gp_mask <<= 1;

      u8 mapped_sr = blm_x_sr_map[sr];
      BLM_X_LED_rows[mapped_sr][0] = (BLM_X_LED_rows[mapped_sr][0] & 0x0f) | pattern;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// for menu handling (e.g. flashing cursor, doubleclick counter, etc...)
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_Handler_Periodic()
{
  // ignore in remote client mode
  if( seq_ui_remote_active_mode == SEQ_UI_REMOTE_MODE_CLIENT )
    return 0; // no error

  if( ++ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_MAX ) {
    ui_cursor_flash_ctr = 0;
    seq_ui_display_update_req = 1;
  } else if( ui_cursor_flash_ctr == SEQ_UI_CURSOR_FLASH_CTR_LED_OFF ) {
    seq_ui_display_update_req = 1;
  }
  // important: flash flag has to be recalculated on each invocation of this
  // handler, since counter could also be reseted outside this function
  ui_cursor_flash = ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF;


  // used in some pages for temporary messages
  if( ui_hold_msg_ctr ) {
    --ui_hold_msg_ctr;

    if( !ui_hold_msg_ctr )
      seq_ui_display_update_req = 1;
  }

  // used for temporary messages
  if( ui_msg_ctr )
    --ui_msg_ctr;

  // VU meters (used in MUTE menu, could also be available as LED matrix...)
  static u8 vu_meter_prediv = 0; // predivider for VU meters

  if( ++vu_meter_prediv >= 4 ) {
    vu_meter_prediv = 0;


    portENTER_CRITICAL();

    u8 track;
    seq_core_trk_t *t = &seq_core_trk[0];
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++t, ++track)
      if( t->vu_meter )
	--t->vu_meter;

    int i;
    u8 *vu_meter = (u8 *)&seq_layer_vu_meter[0];
    for(i=0; i<sizeof(seq_layer_vu_meter); ++i, ++vu_meter) {
      if( *vu_meter && !(*vu_meter & 0x80) ) // if bit 7 set: static value
	*vu_meter -= 1;
    }

    portEXIT_CRITICAL();
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Should be regulary called to check if the layer/instrument/step selection
// is valid for the current track
// At least executed before button/encoder and LCD function calls
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CheckSelections(void)
{
  if( (ui_selected_tracks >> (4*ui_selected_group)) == 0 )
    ui_selected_tracks = 1 << (4*ui_selected_group);

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( ui_selected_instrument >= SEQ_PAR_NumInstrumentsGet(visible_track) )
    ui_selected_instrument = 0;

  if( ui_selected_par_layer >= SEQ_PAR_NumLayersGet(visible_track) )
    ui_selected_par_layer = 0;

  if( ui_selected_trg_layer >= SEQ_TRG_NumLayersGet(visible_track) )
    ui_selected_trg_layer = 0;

  if( ui_selected_step >= SEQ_TRG_NumStepsGet(visible_track) )
    ui_selected_step = 0;

  if( ui_selected_step_view >= (SEQ_TRG_NumStepsGet(visible_track)/16) ) {
    ui_selected_step_view = 0;
    ui_selected_step %= 16;
  }
  
  if( ui_selected_step < (16*ui_selected_step_view) || 
      ui_selected_step >= (16*(ui_selected_step_view+1)) )
    ui_selected_step_view = ui_selected_step / 16;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the currently visible track
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_UI_VisibleTrackGet(void)
{
  u8 offset = 0;

  u8 selected_tracks = ui_selected_tracks >> (4*ui_selected_group);
  if( selected_tracks & (1 << 3) )
    offset = 3;
  if( selected_tracks & (1 << 2) )
    offset = 2;
  if( selected_tracks & (1 << 1) )
    offset = 1;
  if( selected_tracks & (1 << 0) )
    offset = 0;

  return 4*ui_selected_group + offset;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if 'track' is selected
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_IsSelectedTrack(u8 track)
{
  return (ui_selected_tracks & (1 << track)) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a new selected step and updates the step view
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SelectedStepSet(u8 step)
{
  ui_selected_step = step;
  SEQ_UI_CheckSelections();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Increments the selected tracks/groups
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_GxTyInc(s32 incrementer)
{
  int gxty = SEQ_UI_VisibleTrackGet();
  int prev_gxty = gxty;

  if( incrementer >= 0 ) {
    if( (gxty += incrementer) >= SEQ_CORE_NUM_TRACKS )
      gxty = SEQ_CORE_NUM_TRACKS-1;
  } else {
    if( (gxty += incrementer) < 0 )
      gxty = 0;
  }

  if( gxty == prev_gxty )
    return 0; // no change

  ui_selected_tracks = 1 << gxty;
  ui_selected_group = gxty / 4;

  return 1; // value changed
}


/////////////////////////////////////////////////////////////////////////////
// Increments a 16bit variable within given min/max range
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer)
{
  int new_value = *value;
  int prev_value = new_value;

  if( incrementer >= 0 ) {
    if( (new_value += incrementer) >= max )
      new_value = max;
  } else {
    if( (new_value += incrementer) < min )
      new_value = min;
  }

  if( new_value == prev_value )
    return 0; // no change

  *value = new_value;

  return 1; // value changed
}

/////////////////////////////////////////////////////////////////////////////
// Increments an 8bit variable within given min/max range
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Var8_Inc(u8 *value, u16 min, u16 max, s32 incrementer)
{
  u16 tmp = *value;
  if( SEQ_UI_Var16_Inc(&tmp, min, max, incrementer) ) {
    *value = tmp;
    return 1; // value changed
  }

  return 0; // value hasn't been changed
}


/////////////////////////////////////////////////////////////////////////////
// Increments a CC within given min/max range
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CC_Inc(u8 cc, u8 min, u8 max, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  int new_value = SEQ_CC_Get(visible_track, cc);
  int prev_value = new_value;

  if( incrementer >= 0 ) {
    if( (new_value += incrementer) >= max )
      new_value = max;
  } else {
    if( (new_value += incrementer) < min )
      new_value = min;
  }

  if( new_value == prev_value )
    return 0; // no change

  SEQ_CC_Set(visible_track, cc, new_value);

  // set same value for all selected tracks
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    if( track != visible_track && SEQ_UI_IsSelectedTrack(track) )
      SEQ_CC_Set(track, cc, new_value);

  return 1; // value changed
}


/////////////////////////////////////////////////////////////////////////////
// Sets a CC value on all selected tracks
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CC_Set(u8 cc, u8 value)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  int prev_value = SEQ_CC_Get(visible_track, cc);

  if( value == prev_value )
    return 0; // no change

  SEQ_CC_Set(visible_track, cc, value);

  // set same value for all selected tracks
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    if( track != visible_track && SEQ_UI_IsSelectedTrack(track) )
      SEQ_CC_Set(track, cc, value);

  return 1; // value changed
}

/////////////////////////////////////////////////////////////////////////////
// Modifies a bitfield in a CC value to a given value
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CC_SetFlags(u8 cc, u8 flag_mask, u8 value)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  int new_value = SEQ_CC_Get(visible_track, cc);
  int prev_value = new_value;

  new_value = (new_value & ~flag_mask) | value;

  if( new_value == prev_value )
    return 0; // no change

  SEQ_CC_Set(visible_track, cc, new_value);

  // do same modification for all selected tracks
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    if( track != visible_track && SEQ_UI_IsSelectedTrack(track) ) {
      int new_value = SEQ_CC_Get(track, cc);
      new_value = (new_value & ~flag_mask) | value;
      SEQ_CC_Set(track, cc, new_value);
    }

  return 1; // value changed
}


/////////////////////////////////////////////////////////////////////////////
// Print temporary user messages (e.g. warnings, errors)
// expects mS delay and two lines, each up to 20 characters
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Msg(seq_ui_msg_type_t msg_type, u16 delay, char *line1, char *line2)
{
  ui_msg_type = msg_type;
  ui_msg_ctr = delay;
  strncpy((char *)ui_msg[0], line1, UI_MSG_MAX_CHAR);
  strncpy((char *)ui_msg[1], line2, UI_MSG_MAX_CHAR);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Stops temporary message if no SD card warning
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MsgStop(void)
{
  if( ui_msg_type != SEQ_UI_MSG_SDCARD )
    ui_msg_ctr = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Prints a temporary error messages after file operation
// Expects error status number (as defined in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SDCardErrMsg(u16 delay, s32 status)
{
  // TODO: add more verbose error messages, they are clearly defined in seq_file.h)
  char str[21];
  sprintf(str, "E%3d (DOSFS: D%3d)", -status, seq_file_dfs_errno < 1000 ? seq_file_dfs_errno : 999);
  return SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, delay, "!! SD Card Error !!!", str);
}
