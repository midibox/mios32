// $Id$
/*
 * User Interface Routines
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
#include <blm8x8.h>
#include <seq_midi_out.h>
#include <seq_bpm.h>

#include "tasks.h"
#include "seq_ui.h"
#include "seq_lcd.h"
#include "seq_led.h"
#include "seq_midply.h"
#include "seq_core.h"
#include "seq_layer.h"
#include "seq_cc.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_file_m.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 seq_ui_display_update_req;
u8 seq_ui_display_init_req;

seq_ui_button_state_t seq_ui_button_state;

u8 ui_selected_group;
u8 ui_selected_tracks;
u8 ui_selected_par_layer;
u8 ui_selected_trg_layer;
u8 ui_selected_step_view;
u8 ui_selected_step;
u8 ui_selected_item;

u8 ui_selected_item;

u16 ui_hold_msg_ctr;

seq_ui_page_t ui_page;
seq_ui_page_t ui_selected_page;
seq_ui_page_t ui_stepview_prev_page;

volatile u8 ui_cursor_flash;
u16 ui_cursor_flash_ctr;

char ui_edit_name[21];
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
    switch( SEQ_LAYER_GetVControlType(SEQ_UI_VisibleTrackGet(), ui_selected_par_layer) ) {
      case SEQ_LAYER_ControlType_Velocity:
      case SEQ_LAYER_ControlType_Chord1_Velocity:
      case SEQ_LAYER_ControlType_Chord2_Velocity:
      case SEQ_LAYER_ControlType_CC:
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


/////////////////////////////////////////////////////////////////////////////
// Change the menu page
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PageSet(seq_ui_page_t page)
{
  if( page != ui_page ) {
    // disable hooks of previous page and request re-initialisation
    MIOS32_IRQ_Disable();
    ui_page = page;
    ui_button_callback = NULL;
    ui_encoder_callback = NULL;
    ui_led_callback = NULL;
    ui_lcd_callback = NULL;
    MIOS32_IRQ_Enable();

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

#if 1
  MUTEX_SDCARD_TAKE;

#if 0
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    SEQ_FILE_B_Create(bank);
    SEQ_FILE_B_Open(bank);
  }
#else
    SEQ_FILE_M_Create();
    SEQ_FILE_M_Open();
#endif

  MUTEX_SDCARD_GIVE;
#endif

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

static s32 SEQ_UI_Button_Track(s32 depressed, u32 track)
{
  if( depressed ) return -1; // ignore when button depressed

  if( track >= 4 ) return -2; // max. 4 track buttons

  ui_selected_tracks = (1 << track); // TODO: multi-selections!

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_Group(s32 depressed, u32 group)
{
  if( depressed ) return -1; // ignore when button depressed

  if( group >= 4 ) return -2; // max. 4 group buttons

  ui_selected_group = group;

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_ParLayer(s32 depressed, u32 par_layer)
{
  if( depressed ) return -1; // ignore when button depressed

  if( par_layer >= 3 ) return -2; // max. 3 parlayer buttons

  ui_selected_par_layer = par_layer;

  // set/clear encoder fast function if required
  SEQ_UI_InitEncSpeed(1); // auto config

  return 0; // no error
}

static s32 SEQ_UI_Button_TrgLayer(s32 depressed, u32 trg_layer)
{
  if( depressed ) return -1; // ignore when button depressed

  if( trg_layer >= 3 ) return -2; // max. 3 trglayer buttons

  ui_selected_trg_layer = trg_layer;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Button handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value)
{
  switch( pin ) {
#if BUTTON_GP1 != BUTTON_DISABLED
    case BUTTON_GP1:   SEQ_UI_Button_GP(pin_value, 0); break;
#endif
#if BUTTON_GP2 != BUTTON_DISABLED
    case BUTTON_GP2:   SEQ_UI_Button_GP(pin_value, 1); break;
#endif
#if BUTTON_GP3 != BUTTON_DISABLED
    case BUTTON_GP3:   SEQ_UI_Button_GP(pin_value, 2); break;
#endif
#if BUTTON_GP4 != BUTTON_DISABLED
    case BUTTON_GP4:   SEQ_UI_Button_GP(pin_value, 3); break;
#endif
#if BUTTON_GP5 != BUTTON_DISABLED
    case BUTTON_GP5:   SEQ_UI_Button_GP(pin_value, 4); break;
#endif
#if BUTTON_GP6 != BUTTON_DISABLED
    case BUTTON_GP6:   SEQ_UI_Button_GP(pin_value, 5); break;
#endif
#if BUTTON_GP7 != BUTTON_DISABLED
    case BUTTON_GP7:   SEQ_UI_Button_GP(pin_value, 6); break;
#endif
#if BUTTON_GP8 != BUTTON_DISABLED
    case BUTTON_GP8:   SEQ_UI_Button_GP(pin_value, 7); break;
#endif
#if BUTTON_GP9 != BUTTON_DISABLED
    case BUTTON_GP9:   SEQ_UI_Button_GP(pin_value, 8); break;
#endif
#if BUTTON_GP10 != BUTTON_DISABLED
    case BUTTON_GP10:  SEQ_UI_Button_GP(pin_value, 9); break;
#endif
#if BUTTON_GP11 != BUTTON_DISABLED
    case BUTTON_GP11:  SEQ_UI_Button_GP(pin_value, 10); break;
#endif
#if BUTTON_GP12 != BUTTON_DISABLED
    case BUTTON_GP12:  SEQ_UI_Button_GP(pin_value, 11); break;
#endif
#if BUTTON_GP13 != BUTTON_DISABLED
    case BUTTON_GP13:  SEQ_UI_Button_GP(pin_value, 12); break;
#endif
#if BUTTON_GP14 != BUTTON_DISABLED
    case BUTTON_GP14:  SEQ_UI_Button_GP(pin_value, 13); break;
#endif
#if BUTTON_GP15 != BUTTON_DISABLED
    case BUTTON_GP15:  SEQ_UI_Button_GP(pin_value, 14); break;
#endif
#if BUTTON_GP16 != BUTTON_DISABLED
    case BUTTON_GP16:  SEQ_UI_Button_GP(pin_value, 15); break;
#endif

#if BUTTON_LEFT != BUTTON_DISABLED
    case BUTTON_LEFT:  SEQ_UI_Button_Left(pin_value); break;
#endif
#if BUTTON_RIGHT != BUTTON_DISABLED
    case BUTTON_RIGHT: SEQ_UI_Button_Right(pin_value); break;
#endif

#if BUTTON_DOWN != BUTTON_DISABLED
    case BUTTON_DOWN:  SEQ_UI_Button_Down(pin_value); break;
#endif
#if BUTTON_UP != BUTTON_DISABLED
    case BUTTON_UP:    SEQ_UI_Button_Up(pin_value); break;
#endif

#if BUTTON_SCRUB != BUTTON_DISABLED
    case BUTTON_SCRUB: SEQ_UI_Button_Scrub(pin_value); break;
#endif
#if BUTTON_METRONOME != BUTTON_DISABLED
    case BUTTON_METRONOME: SEQ_UI_Button_Metronome(pin_value); break;
#endif

#if BUTTON_STOP != BUTTON_DISABLED
    case BUTTON_STOP:  SEQ_UI_Button_Stop(pin_value); break;
#endif
#if BUTTON_PAUSE != BUTTON_DISABLED
    case BUTTON_PAUSE: SEQ_UI_Button_Pause(pin_value); break;
#endif
#if BUTTON_PLAY != BUTTON_DISABLED
    case BUTTON_PLAY:  SEQ_UI_Button_Play(pin_value); break;
#endif
#if BUTTON_REW != BUTTON_DISABLED
    case BUTTON_REW:   SEQ_UI_Button_Rew(pin_value); break;
#endif
#if BUTTON_FWD != BUTTON_DISABLED
    case BUTTON_FWD:   SEQ_UI_Button_Fwd(pin_value); break;
#endif

#if BUTTON_F1 != BUTTON_DISABLED
    case BUTTON_F1:    SEQ_UI_Button_F1(pin_value); break;
#endif
#if BUTTON_F2 != BUTTON_DISABLED
    case BUTTON_F2:    SEQ_UI_Button_F2(pin_value); break;
#endif
#if BUTTON_F3 != BUTTON_DISABLED
    case BUTTON_F3:    SEQ_UI_Button_F3(pin_value); break;
#endif
#if BUTTON_F4 != BUTTON_DISABLED
    case BUTTON_F4:    SEQ_UI_Button_F4(pin_value); break;
#endif

#if BUTTON_UTILITY != BUTTON_DISABLED
    case BUTTON_UTILITY: SEQ_UI_Button_Utility(pin_value); break;
#endif
#if BUTTON_COPY != BUTTON_DISABLED
    case BUTTON_COPY:  SEQ_UI_Button_Copy(pin_value); break;
#endif
#if BUTTON_PASTE != BUTTON_DISABLED
    case BUTTON_PASTE: SEQ_UI_Button_Paste(pin_value); break;
#endif
#if BUTTON_CLEAR != BUTTON_DISABLED
    case BUTTON_CLEAR: SEQ_UI_Button_Clear(pin_value); break;
#endif

#if BUTTON_MENU != BUTTON_DISABLED
    case BUTTON_MENU:  SEQ_UI_Button_Menu(pin_value); break;
#endif
#if BUTTON_SELECT != BUTTON_DISABLED
    case BUTTON_SELECT:SEQ_UI_Button_Select(pin_value); break;
#endif
#if BUTTON_EXIT != BUTTON_DISABLED
    case BUTTON_EXIT:  SEQ_UI_Button_Exit(pin_value); break;
#endif

#if BUTTON_TRACK1 != BUTTON_DISABLED
    case BUTTON_TRACK1: SEQ_UI_Button_Track(pin_value, 0); break;
#endif
#if BUTTON_TRACK2 != BUTTON_DISABLED
    case BUTTON_TRACK2: SEQ_UI_Button_Track(pin_value, 1); break;
#endif
#if BUTTON_TRACK3 != BUTTON_DISABLED
    case BUTTON_TRACK3: SEQ_UI_Button_Track(pin_value, 2); break;
#endif
#if BUTTON_TRACK4 != BUTTON_DISABLED
    case BUTTON_TRACK4: SEQ_UI_Button_Track(pin_value, 3); break;
#endif

#if BUTTON_PAR_LAYER_A != BUTTON_DISABLED
    case BUTTON_PAR_LAYER_A: SEQ_UI_Button_ParLayer(pin_value, 0); break;
#endif
#if BUTTON_PAR_LAYER_B != BUTTON_DISABLED
    case BUTTON_PAR_LAYER_B: SEQ_UI_Button_ParLayer(pin_value, 1); break;
#endif
#if BUTTON_PAR_LAYER_C != BUTTON_DISABLED
    case BUTTON_PAR_LAYER_C: SEQ_UI_Button_ParLayer(pin_value, 2); break;
#endif

#if BUTTON_EDIT != BUTTON_DISABLED
    case BUTTON_EDIT:   SEQ_UI_Button_Edit(pin_value); break;
#endif
#if BUTTON_MUTE != BUTTON_DISABLED
    case BUTTON_MUTE:   SEQ_UI_Button_Mute(pin_value); break;
#endif
#if BUTTON_PATTERN != BUTTON_DISABLED
    case BUTTON_PATTERN:SEQ_UI_Button_Pattern(pin_value); break;
#endif
#if BUTTON_SONG != BUTTON_DISABLED
    case BUTTON_SONG:   SEQ_UI_Button_Song(pin_value); break;
#endif

#if BUTTON_SOLO != BUTTON_DISABLED
    case BUTTON_SOLO:   SEQ_UI_Button_Solo(pin_value); break;
#endif
#if BUTTON_FAST != BUTTON_DISABLED
    case BUTTON_FAST:   SEQ_UI_Button_Fast(pin_value); break;
#endif
#if BUTTON_ALL != BUTTON_DISABLED
    case BUTTON_ALL:    SEQ_UI_Button_All(pin_value); break;
#endif

#if BUTTON_GROUP1 != BUTTON_DISABLED
    case BUTTON_GROUP1: SEQ_UI_Button_Group(pin_value, 0); break;
#endif
#if BUTTON_GROUP2 != BUTTON_DISABLED
    case BUTTON_GROUP2: SEQ_UI_Button_Group(pin_value, 1); break;
#endif
#if BUTTON_GROUP3 != BUTTON_DISABLED
    case BUTTON_GROUP3: SEQ_UI_Button_Group(pin_value, 2); break;
#endif
#if BUTTON_GROUP4 != BUTTON_DISABLED
    case BUTTON_GROUP4: SEQ_UI_Button_Group(pin_value, 3); break;
#endif

#if BUTTON_TRG_LAYER_A != BUTTON_DISABLED
    case BUTTON_TRG_LAYER_A: SEQ_UI_Button_TrgLayer(pin_value, 0); break;
#endif
#if BUTTON_TRG_LAYER_B != BUTTON_DISABLED
    case BUTTON_TRG_LAYER_B: SEQ_UI_Button_TrgLayer(pin_value, 1); break;
#endif
#if BUTTON_TRG_LAYER_C != BUTTON_DISABLED
    case BUTTON_TRG_LAYER_C: SEQ_UI_Button_TrgLayer(pin_value, 2); break;
#endif

#if BUTTON_STEP_VIEW != BUTTON_DISABLED
    case BUTTON_STEP_VIEW: SEQ_UI_Button_StepView(pin_value); break;
#endif

#if BUTTON_TAP_TEMPO != BUTTON_DISABLED
    case BUTTON_TAP_TEMPO:   SEQ_UI_Button_TapTempo(pin_value); break;
#endif

    default:
      return -1; // button function not mapped to physical button
  }

  // request display update
  seq_ui_display_update_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Encoder handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer)
{
  if( encoder > 16 )
    return -1; // encoder doesn't exist

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
      
      if( ui_lcd_callback != NULL )
	ui_lcd_callback(0); // no high_prio
    }
  }

  // if SD card message active: copy over the text
  if( sdcard_msg_ctr ) {
    const char animation_l[4][3] = {
      "  ", " >", ">>", "> " };
    const char animation_r[4][3] = {
      "  ", "< ", "<<", " <" };
    int anum = (sdcard_msg_ctr % 1000) / 250;

    int line;
    for(line=0; line<2; ++line) {
      SEQ_LCD_CursorSet(40, line);
      SEQ_LCD_PrintFormattedString(" %s| %-20s |%s ",
				   (char *)animation_l[anum], 
				   (char *)sdcard_msg[line], 
				   (char *)animation_r[anum]);
    }
  }

  // transfer all changed characters to LCD
  SEQ_LCD_Update(0);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Update all LEDs
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler(void)
{
  // track LEDs
  SEQ_LED_PinSet(LED_TRACK1, (ui_selected_tracks & (1 << 0)));
  SEQ_LED_PinSet(LED_TRACK2, (ui_selected_tracks & (1 << 1)));
  SEQ_LED_PinSet(LED_TRACK3, (ui_selected_tracks & (1 << 2)));
  SEQ_LED_PinSet(LED_TRACK4, (ui_selected_tracks & (1 << 3)));
  
  // parameter layer LEDs
  SEQ_LED_PinSet(LED_PAR_LAYER_A, (ui_selected_par_layer == 0));
  SEQ_LED_PinSet(LED_PAR_LAYER_B, (ui_selected_par_layer == 1));
  SEQ_LED_PinSet(LED_PAR_LAYER_C, (ui_selected_par_layer == 2));
  
  // group LEDs
  SEQ_LED_PinSet(LED_GROUP1, (ui_selected_group == 0));
  SEQ_LED_PinSet(LED_GROUP2, (ui_selected_group == 1));
  SEQ_LED_PinSet(LED_GROUP3, (ui_selected_group == 2));
  SEQ_LED_PinSet(LED_GROUP4, (ui_selected_group == 3));
  
  // trigger layer LEDs
  SEQ_LED_PinSet(LED_TRG_LAYER_A, (ui_selected_trg_layer == 0));
  SEQ_LED_PinSet(LED_TRG_LAYER_B, (ui_selected_trg_layer == 1));
  SEQ_LED_PinSet(LED_TRG_LAYER_C, (ui_selected_trg_layer == 2));
  
  // remaining LEDs
  SEQ_LED_PinSet(LED_EDIT, ui_page == SEQ_UI_PAGE_EDIT);
  SEQ_LED_PinSet(LED_MUTE, ui_page == SEQ_UI_PAGE_MUTE);
  SEQ_LED_PinSet(LED_PATTERN, ui_page == SEQ_UI_PAGE_PATTERN);
  SEQ_LED_PinSet(LED_SONG, 0);
  
  SEQ_LED_PinSet(LED_SOLO, seq_ui_button_state.SOLO);
  SEQ_LED_PinSet(LED_FAST, seq_ui_button_state.FAST_ENCODERS);
  SEQ_LED_PinSet(LED_ALL, seq_ui_button_state.CHANGE_ALL_STEPS);
  
  SEQ_LED_PinSet(LED_PLAY, SEQ_BPM_IsRunning());
  SEQ_LED_PinSet(LED_STOP, !SEQ_BPM_IsRunning() && !ui_seq_pause);
  SEQ_LED_PinSet(LED_PAUSE, ui_seq_pause);

  SEQ_LED_PinSet(LED_REW, seq_ui_button_state.REW);
  SEQ_LED_PinSet(LED_FWD, seq_ui_button_state.FWD);
  
  SEQ_LED_PinSet(LED_STEPVIEW, seq_ui_button_state.STEPVIEW);

  SEQ_LED_PinSet(LED_MENU, seq_ui_button_state.MENU_PRESSED);
  SEQ_LED_PinSet(LED_SCRUB, seq_ui_button_state.SCRUB);
  SEQ_LED_PinSet(LED_METRONOME, seq_ui_button_state.METRONOME);

  SEQ_LED_PinSet(LED_UTILITY, ui_page == SEQ_UI_PAGE_UTIL);
  SEQ_LED_PinSet(LED_COPY, seq_ui_button_state.COPY);
  SEQ_LED_PinSet(LED_PASTE, seq_ui_button_state.PASTE);
  SEQ_LED_PinSet(LED_CLEAR, seq_ui_button_state.CLEAR);

  SEQ_LED_PinSet(LED_F1, seq_ui_button_state.F1);
  SEQ_LED_PinSet(LED_F2, seq_ui_button_state.F2);
  SEQ_LED_PinSet(LED_F3, seq_ui_button_state.F3);
  SEQ_LED_PinSet(LED_F4, seq_ui_button_state.F4);

  SEQ_LED_PinSet(LED_DOWN, seq_ui_button_state.DOWN);
  SEQ_LED_PinSet(LED_UP, seq_ui_button_state.UP);


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
  SEQ_LED_PinSet(LED_BEAT, (sequencer_running && (seq_core_state.ref_step & 3) == 0));

  // for song position marker (supports 16 LEDs, check for selected step view)
  u16 pos_marker_mask = 0x0000;
  u8 played_step = seq_core_trk[SEQ_UI_VisibleTrackGet()].step;
  if( sequencer_running && (played_step >> 4) == ui_selected_step_view )
    pos_marker_mask = 1 << (played_step & 0xf);

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

  // BLM8X8 DOUT -> GP LED mapping
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
  const u8 blm8x8_sr_map[8] = {4, 5, 6, 7, 3, 2, 1, 0};
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

    u8 mapped_sr = blm8x8_sr_map[sr];
    blm8x8_led_row[mapped_sr] = (blm8x8_led_row[mapped_sr] & 0x0f) | pattern;
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
  if( ui_hold_msg_ctr )
    --ui_hold_msg_ctr;

  // used for temporary SD Card messages
  if( sdcard_msg_ctr )
    --sdcard_msg_ctr;

  // VU meters (used in MUTE menu, could also be available as LED matrix...)
  static u8 vu_meter_prediv = 0; // predivider for VU meters

  if( ++vu_meter_prediv >= 4 ) {
    vu_meter_prediv = 0;

    u8 track;
    seq_core_trk_t *t = &seq_core_trk[0];
    MIOS32_IRQ_Disable();
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++t, ++track)
      if( t->vu_meter )
	--t->vu_meter;
    MIOS32_IRQ_Enable();
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the currently visible track
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_UI_VisibleTrackGet(void)
{
  u8 offset = 0;

  if( ui_selected_tracks & (1 << 3) )
    offset = 3;
  if( ui_selected_tracks & (1 << 2) )
    offset = 2;
  if( ui_selected_tracks & (1 << 1) )
    offset = 1;
  if( ui_selected_tracks & (1 << 0) )
    offset = 0;

  return 4*ui_selected_group + offset;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if 'track' is selected
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_IsSelectedTrack(u8 track)
{
  if( (track>>2) != ui_selected_group )
    return 0;
  return (ui_selected_tracks & (1 << (track&3))) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a new selected step and updates the step view
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SelectedStepSet(u8 step)
{
  ui_selected_step = step;
  ui_selected_step_view = (ui_selected_step >= 16) ? 1 : 0;
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

  ui_selected_tracks = 1 << (gxty % 4);
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
