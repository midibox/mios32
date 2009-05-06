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


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 (*ui_button_callback)(seq_ui_button_t button, s32 depressed);
static s32 (*ui_encoder_callback)(seq_ui_encoder_t encoder, s32 incrementer);
static s32 (*ui_led_callback)(u16 *gp_leds);
static s32 (*ui_lcd_callback)(u8 high_prio);
static s32 (*ui_exit_callback)(void);

static u16 ui_gp_leds;

#define SDCARD_MSG_MAX_CHAR 21
static char sdcard_msg[2][SDCARD_MSG_MAX_CHAR];
static u16 sdcard_msg_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Init(u32 mode)
{
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
  sdcard_msg_ctr = 0;

  ui_cursor_flash_ctr = 0;
  ui_cursor_flash = 0;

  seq_ui_button_state.ALL = 0;

  ui_seq_pause = 0;

  // visible GP pattern
  ui_gp_leds = 0x0000;

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
#if DEFAULT_AUTO_FAST_BUTTON
    switch( SEQ_PAR_AssignmentGet(SEQ_UI_VisibleTrackGet(), ui_selected_par_layer) ) {
      case SEQ_PAR_Type_Velocity:
      case SEQ_PAR_Type_Length:
      case SEQ_PAR_Type_CC:
      case SEQ_PAR_Type_PitchBend:
      case SEQ_PAR_Type_Probability:
      case SEQ_PAR_Type_Delay:
      case SEQ_PAR_Type_Loopback:
	seq_ui_button_state.FAST_ENCODERS = 1;
	break;

      default:
	seq_ui_button_state.FAST_ENCODERS = 0;
    }
#else
    // auto mode not enabled - ignore auto reconfiguration request
    return 0;
#endif
  }

  // change for datawheel and GP encoders
  int enc;
  for(enc=0; enc<17; ++enc) {
    enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.speed = seq_ui_button_state.FAST_ENCODERS ? FAST : NORMAL;
    enc_config.cfg.speed_par = (enc == 0) ? DEFAULT_DATAWHEEL_SPEED_VALUE : DEFAULT_ENC_SPEED_VALUE;
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

#if DEFAULT_BEHAVIOUR_BUTTON_MENU
    seq_ui_button_state.MENU_PRESSED = 0; // MENU page selection finished
#endif

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

static s32 SEQ_UI_Button_Scrub(s32 depressed)
{
#if DEFAULT_BEHAVIOUR_BUTTON_SCRUB
  // toggle mode
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.SCRUB ^= 1;
#else
  // set mode
  seq_ui_button_state.SCRUB = depressed ? 0 : 1;
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Metronome(s32 depressed)
{
#if DEFAULT_BEHAVIOUR_BUTTON_METRON
  // toggle mode
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.METRONOME ^= 1;
#else
  // set mode
  seq_ui_button_state.METRONOME = depressed ? 0 : 1;
#endif

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

  return 0; // no error
}

static s32 SEQ_UI_Button_Fwd(s32 depressed)
{
  seq_ui_button_state.FWD = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F1(s32 depressed)
{
  seq_ui_button_state.F1 = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  // change to utility page
  SEQ_UI_PageSet(SEQ_UI_PAGE_UTIL);

  return 0; // no error
}

static s32 SEQ_UI_Button_F2(s32 depressed)
{
  seq_ui_button_state.F2 = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F3(s32 depressed)
{
  seq_ui_button_state.F3 = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  return 0; // no error
}

static s32 SEQ_UI_Button_F4(s32 depressed)
{
  seq_ui_button_state.F4 = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

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

    if( depressed )
      SEQ_UI_PageSet(prev_page);

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

    if( depressed )
      SEQ_UI_PageSet(prev_page);

    return status;
  }
}

static s32 SEQ_UI_Button_Clear(s32 depressed)
{
  seq_ui_button_state.CLEAR = depressed ? 0 : 1;

  if( ui_page == SEQ_UI_PAGE_MIXER ) {
    if( depressed ) return -1;
    SEQ_UI_MIXER_Clear();
    return 1;
  } else {
    return SEQ_UI_UTIL_ClearButton(depressed);
  }
}

static s32 SEQ_UI_Button_Menu(s32 depressed)
{
  // TODO: define generic #define for this button behaviour
#if DEFAULT_BEHAVIOUR_BUTTON_MENU
  // toggle mode
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.MENU_FIRST_PAGE_SELECTED = 0;
  seq_ui_button_state.MENU_PRESSED ^= 1; // toggle MENU pressed (will also be released once GP button has been pressed)
#else
  // set mode
  seq_ui_button_state.MENU_FIRST_PAGE_SELECTED = 0;
  seq_ui_button_state.MENU_PRESSED = depressed ? 0 : 1;
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Select(s32 depressed)
{
  // forward to menu page
  if( !seq_ui_button_state.MENU_PRESSED && ui_button_callback != NULL )
    ui_button_callback(SEQ_UI_BUTTON_Select, depressed);
  ui_cursor_flash_ctr = 0;

  return 0; // no error
}

static s32 SEQ_UI_Button_Exit(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  u8 prev_ui_page = ui_page;

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
  if( depressed ) return -1; // ignore when button depressed

  // change to edit page
  SEQ_UI_PageSet(SEQ_UI_PAGE_EDIT);

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_Mute(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_MUTE);

  return 0; // no error
}

static s32 SEQ_UI_Button_Pattern(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  SEQ_UI_PageSet(SEQ_UI_PAGE_PATTERN);

  return 0; // no error
}

static s32 SEQ_UI_Button_Song(s32 depressed)
{
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
#if DEFAULT_BEHAVIOUR_BUTTON_ALL
  // toggle mode
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.SOLO ^= 1;
#else
  // set mode
  seq_ui_button_state.SOLO = depressed ? 0 : 1;
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_Fast(s32 depressed)
{
#if DEFAULT_BEHAVIOUR_BUTTON_FAST
  // toggle mode
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.FAST_ENCODERS ^= 1;
#else
  // set mode
  seq_ui_button_state.FAST_ENCODERS = depressed ? 0 : 1;
#endif

  SEQ_UI_InitEncSpeed(0); // no auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_All(s32 depressed)
{
  seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE = depressed ? 0 : 1;

#if DEFAULT_BEHAVIOUR_BUTTON_ALL
  // toggle mode
  if( depressed ) return -1;
  seq_ui_button_state.CHANGE_ALL_STEPS ^= 1;
#else
  // set mode
  seq_ui_button_state.CHANGE_ALL_STEPS = depressed ? 0 : 1;
#endif

  return 0; // no error
}

static s32 SEQ_UI_Button_StepView(s32 depressed)
{

#if DEFAULT_BEHAVIOUR_BUTTON_STEPVIEW
  if( depressed ) return -1; // ignore when button depressed
  seq_ui_button_state.STEPVIEW ^= 1; // toggle STEPVIEW pressed (will also be released once GP button has been pressed)
#else
  // set mode
  seq_ui_button_state.STEPVIEW = depressed ? 0 : 1;
#endif

  if( seq_ui_button_state.STEPVIEW ) {
    ui_stepview_prev_page = ui_page;
    SEQ_UI_PageSet(SEQ_UI_PAGE_STEPSEL);
  } else {
    SEQ_UI_PageSet(ui_stepview_prev_page);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_TapTempo(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

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

static s32 SEQ_UI_Button_ParLayer(s32 depressed, u32 par_layer)
{
  if( par_layer >= 3 ) return -2; // max. 3 parlayer buttons

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);

  if( num_p_layers <= 3 ) {
    // 3 layers: direct selection with LayerA/B/C button
    if( depressed ) return -1; // ignore when button depressed
    seq_ui_button_state.PAR_LAYER_SEL = 0;
    ui_selected_par_layer = par_layer;
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
#if DEFAULT_BEHAVIOUR_BUTTON_PAR_LAYER
      if( depressed ) return -1; // ignore when button depressed
      seq_ui_button_state.PAR_LAYER_SEL ^= 1; // toggle PARSEL status (will also be released once GP button has been pressed)
#else
      // set mode
      seq_ui_button_state.PAR_LAYER_SEL = depressed ? 0 : 1;
#endif

      if( seq_ui_button_state.PAR_LAYER_SEL ) {
	ui_parlayer_prev_page = ui_page;
	SEQ_UI_PageSet(SEQ_UI_PAGE_PARSEL);
      } else {
	SEQ_UI_PageSet(ui_parlayer_prev_page);
      }
    }
  }

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

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
    seq_ui_button_state.TRG_LAYER_SEL = 0;
    ui_selected_trg_layer = trg_layer;
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
#if DEFAULT_BEHAVIOUR_BUTTON_TRG_LAYER
      if( depressed ) return -1; // ignore when button depressed
      seq_ui_button_state.TRG_LAYER_SEL ^= 1; // toggle TRGSEL status (will also be released once GP button has been pressed)
#else
      // set mode
      seq_ui_button_state.TRG_LAYER_SEL = depressed ? 0 : 1;
#endif

      if( seq_ui_button_state.TRG_LAYER_SEL ) {
	ui_trglayer_prev_page = ui_page;
	SEQ_UI_PageSet(SEQ_UI_PAGE_TRGSEL);
      } else {
	SEQ_UI_PageSet(ui_trglayer_prev_page);
      }
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Button handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value)
{
  int i;

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // request display update
  seq_ui_display_update_req = 1;


  // MEMO: we could also use a jump table with references to the functions
  // here, but this "spagetthi code" simplifies the configuration and
  // the resulting ASM doesn't look that bad!

  for(i=0; i<SEQ_HWCFG_NUM_GP; ++i)
    if( pin == seq_hwcfg_button.gp[i] )
      return SEQ_UI_Button_GP(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_TRACK; ++i)
    if( pin == seq_hwcfg_button.track[i] )
      return SEQ_UI_Button_Track(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_GROUP; ++i)
    if( pin == seq_hwcfg_button.group[i] )
      return SEQ_UI_Button_Group(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_PAR_LAYER; ++i)
    if( pin == seq_hwcfg_button.par_layer[i] )
      return SEQ_UI_Button_ParLayer(pin_value, i);

  for(i=0; i<SEQ_HWCFG_NUM_TRG_LAYER; ++i)
    if( pin == seq_hwcfg_button.trg_layer[i] )
      return SEQ_UI_Button_TrgLayer(pin_value, i);

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

  if( pin == seq_hwcfg_button.f1 )
    return SEQ_UI_Button_F1(pin_value);
  if( pin == seq_hwcfg_button.f2 )
    return SEQ_UI_Button_F2(pin_value);
  if( pin == seq_hwcfg_button.f3 )
    return SEQ_UI_Button_F3(pin_value);
  if( pin == seq_hwcfg_button.f4 )
    return SEQ_UI_Button_F4(pin_value);

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

  return -1; // button not mapped
}


/////////////////////////////////////////////////////////////////////////////
// Encoder handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer)
{
  if( encoder > 16 )
    return -1; // encoder doesn't exist

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // limit incrementer
  if( incrementer > 3 )
    incrementer = 3;
  else if( incrementer < -3 )
    incrementer = -3;

  if( !seq_ui_button_state.MENU_PRESSED && ui_encoder_callback != NULL ) {
    ui_encoder_callback((encoder == 0) ? SEQ_UI_ENCODER_Datawheel : (encoder-1), incrementer);
    ui_cursor_flash_ctr = 0; // ensure that value is visible when it has been changed
  }

  // request display update
  seq_ui_display_update_req = 1;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Update LCD messages
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Handler(void)
{
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

  // in MENU page: overrule LCD output so long MENU button is pressed/active
  if( seq_ui_button_state.MENU_PRESSED && !seq_ui_button_state.MENU_FIRST_PAGE_SELECTED ) {
    SEQ_LCD_CursorSet(0, 0);
    //                      <-------------------------------------->
    //                      0123456789012345678901234567890123456789
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
  // if SD card message active: copy over the text
  if( sdcard_msg_ctr ) {
    const char animation_l[4][3] = {
      "  ", " >", ">>", "> " };
    const char animation_r[4][3] = {
      "  ", "< ", "<<", " <" };
    int anum = (sdcard_msg_ctr % 1000) / 250;

    int line;
    for(line=0; line<2; ++line) {
      SEQ_LCD_CursorSet(0, line); // MEMO: print such important information at first LCD for the case the user hasn't connected the second LCD yet
      SEQ_LCD_PrintFormattedString(" %s| %-20s |%s ",
				   (char *)animation_l[anum], 
				   (char *)sdcard_msg[line], 
				   (char *)animation_r[anum]);
    }
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
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // track LEDs
  u8 selected_tracks = ui_selected_tracks >> (4*ui_selected_group);
  SEQ_LED_PinSet(seq_hwcfg_led.track[0], (selected_tracks & (1 << 0)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[1], (selected_tracks & (1 << 1)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[2], (selected_tracks & (1 << 2)));
  SEQ_LED_PinSet(seq_hwcfg_led.track[3], (selected_tracks & (1 << 3)));
  
  // parameter layer LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[0], (ui_selected_par_layer == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[1], (ui_selected_par_layer == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.par_layer[2], (ui_selected_par_layer >= 2) || seq_ui_button_state.PAR_LAYER_SEL);
  
  // group LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.group[0], (ui_selected_group == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.group[1], (ui_selected_group == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.group[2], (ui_selected_group == 2));
  SEQ_LED_PinSet(seq_hwcfg_led.group[3], (ui_selected_group == 3));
  
  // trigger layer LEDs
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[0], (ui_selected_trg_layer == 0));
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[1], (ui_selected_trg_layer == 1));
  SEQ_LED_PinSet(seq_hwcfg_led.trg_layer[2], (ui_selected_trg_layer >= 2) || seq_ui_button_state.TRG_LAYER_SEL);
  
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
  
  SEQ_LED_PinSet(seq_hwcfg_led.step_view, seq_ui_button_state.STEPVIEW);

  SEQ_LED_PinSet(seq_hwcfg_led.menu, seq_ui_button_state.MENU_PRESSED);
  SEQ_LED_PinSet(seq_hwcfg_led.scrub, seq_ui_button_state.SCRUB);
  SEQ_LED_PinSet(seq_hwcfg_led.metronome, seq_ui_button_state.METRONOME);

  SEQ_LED_PinSet(seq_hwcfg_led.utility, ui_page == SEQ_UI_PAGE_UTIL);
  SEQ_LED_PinSet(seq_hwcfg_led.copy, seq_ui_button_state.COPY);
  SEQ_LED_PinSet(seq_hwcfg_led.paste, seq_ui_button_state.PASTE);
  SEQ_LED_PinSet(seq_hwcfg_led.clear, seq_ui_button_state.CLEAR);

  SEQ_LED_PinSet(seq_hwcfg_led.f1, seq_ui_button_state.F1);
  SEQ_LED_PinSet(seq_hwcfg_led.f2, seq_ui_button_state.F2);
  SEQ_LED_PinSet(seq_hwcfg_led.f3, seq_ui_button_state.F3);
  SEQ_LED_PinSet(seq_hwcfg_led.f4, seq_ui_button_state.F4);

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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// updates high-prio LED functions (GP LEDs and Beat LED)
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler_Periodic()
{
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
  if( seq_ui_button_state.STEPVIEW ) {
    // if STEPVIEW button pressed: pos marker correlated to zoom ratio
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

#ifdef DEFAULT_GP_DOUT_SR_L
# ifdef DEFAULT_GP_DOUT_SR_L2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L-1, (ui_gp_leds >> 0) & 0xff);
# else
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L-1, ((ui_gp_leds ^ pos_marker_mask) >> 0) & 0xff);
# endif
#endif
#ifdef DEFAULT_GP_DOUT_SR_R
# ifdef DEFAULT_GP_DOUT_SR_R2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R-1, (ui_gp_leds >> 8) & 0xff);
#else
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R-1, ((ui_gp_leds ^ pos_marker_mask) >> 8) & 0xff);
#endif
#endif

#ifdef DEFAULT_GP_DOUT_SR_L2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_L2-1, (pos_marker_mask >> 0) & 0xff);
#endif
#ifdef DEFAULT_GP_DOUT_SR_R2
  SEQ_LED_SRSet(DEFAULT_GP_DOUT_SR_R2-1, (pos_marker_mask >> 8) & 0xff);
#endif

#if DEFAULT_SRM_ENABLED && DEFAULT_SRM_DOUT_M_MAPPING == 1
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
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// for menu handling (e.g. flashing cursor, doubleclick counter, etc...)
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_Handler_Periodic()
{
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

  // used for temporary SD Card messages
  if( sdcard_msg_ctr )
    --sdcard_msg_ctr;

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
// Print temporary messages after file operations
// expects mS delay and two lines, each up to 20 characters
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SDCardMsg(u16 delay, char *line1, char *line2)
{
  sdcard_msg_ctr = delay;
  strncpy((char *)sdcard_msg[0], line1, SDCARD_MSG_MAX_CHAR);
  strncpy((char *)sdcard_msg[1], line2, SDCARD_MSG_MAX_CHAR);

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
  return SEQ_UI_SDCardMsg(delay, "!! SD Card Error !!!", str);
}
