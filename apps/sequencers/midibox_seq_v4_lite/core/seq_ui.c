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
#include <seq_midi_out.h>
#include <seq_bpm.h>
#include <blm_cheapo.h>
#include <blm_x.h>

#include "tasks.h"
#include "seq_led.h"
#include "seq_ui.h"
#include "seq_ui_pages.h"
#include "seq_hwcfg.h"
#include "seq_core.h"
#include "seq_record.h"
#include "seq_par.h"
#include "seq_layer.h"
#include "seq_cc.h"
#include "seq_midi_sysex.h"
#include "seq_blm.h"

#include "file.h"
#include "seq_file.h"
#include "seq_file_hw.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_ui_button_state_t seq_ui_button_state;

u8 ui_selected_group;
u16 ui_selected_tracks;
u8 ui_selected_par_layer;
u8 ui_selected_trg_layer;
u8 ui_selected_instrument;
u8 ui_selected_step;
u8 ui_selected_step_view;
u16 ui_selected_gp_buttons;

volatile u8 ui_cursor_flash;
volatile u8 ui_cursor_flash_overrun_ctr;
u16 ui_cursor_flash_ctr;

u8 seq_ui_display_update_req;

u8 ui_seq_pause;

u8 ui_song_edit_pos;

u8 seq_ui_backup_req;
u8 seq_ui_format_req;
u8 seq_ui_saveall_req;

u8 ui_quicksel_length[UI_QUICKSEL_NUM_PRESETS];
u8 ui_quicksel_loop_length[UI_QUICKSEL_NUM_PRESETS];
u8 ui_quicksel_loop_loop[UI_QUICKSEL_NUM_PRESETS];

seq_ui_bookmark_t seq_ui_bookmarks[SEQ_UI_BOOKMARKS_NUM];

u8 ui_controller_mode;
mios32_midi_port_t ui_controller_port;
u8 ui_controller_chn;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 tap_tempo_last_timestamp;
#define TAP_DELAY_STORAGE_SIZE 4
static u16 tap_tempo_delay[TAP_DELAY_STORAGE_SIZE];
static u8 tap_tempo_beat_ix;
static u8 tap_tempo_beat_ctr;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 resetTapTempo(void);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Init(u32 mode)
{
  seq_ui_button_state.ALL = 0;

  ui_selected_group = 0;
  ui_selected_tracks = 0x00ff;
  ui_selected_par_layer = 0;
  ui_selected_trg_layer = 0;
  ui_selected_instrument = 0;
  ui_selected_step = 0;
  ui_selected_step_view = 0;
  ui_seq_pause = 0;
  ui_song_edit_pos = 0;
  ui_selected_gp_buttons = 0;

  ui_cursor_flash_ctr = ui_cursor_flash_overrun_ctr = 0;
  ui_cursor_flash = 0;

  seq_ui_display_update_req = 1;

  // misc
  seq_ui_backup_req = 0;
  seq_ui_format_req = 0;
  seq_ui_saveall_req = 0;

  ui_controller_mode = UI_CONTROLLER_MODE_OFF;
  //ui_controller_port = 0xc0; // combined
  ui_controller_port = UART1; // OUT2
  ui_controller_chn = Chn16;

  resetTapTempo();

  // set initial page
  SEQ_UI_PAGES_Set(SEQ_UI_PAGE_TRIGGER);

  // finally init bookmarks
  //ui_page_before_bookmark = SEQ_UI_PAGE_EDIT;
  int i;
  for(i=0; i<SEQ_UI_BOOKMARKS_NUM; ++i) {
    char buffer[10];
    seq_ui_bookmark_t *bm = (seq_ui_bookmark_t *)&seq_ui_bookmarks[i];

    sprintf(buffer, "BM%2d ", i+1);
    memcpy((char *)bm->name, buffer, 6);
    bm->enable.ALL = ~0;
    bm->flags.LOCKED = 0;
    SEQ_UI_Bookmark_Store(i);
  }

  return 0; // no error
}


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
// Dedicated button functions
// Mapped to physical buttons in SEQ_UI_Button_Handler()
// Will also be mapped to MIDI keys later (for MIDI remote function)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_Button_GP(s32 depressed, u32 gp)
{
  s32 status;

  if( depressed )
    ui_selected_gp_buttons &= ~(1 << gp);
  else
    ui_selected_gp_buttons |= (1 << gp);

  if( (status=SEQ_UI_PAGES_GP_Button_Handler(gp, depressed)) >= 0 ) {
    ui_cursor_flash_ctr = ui_cursor_flash_overrun_ctr = 0;
  }
  return status;
}


static s32 SEQ_UI_Button_Bar(s32 depressed, u32 bar)
{
  if( depressed ) return -1; // ignore when button depressed
  ui_selected_step_view = bar;
  return 0; // no error
}

static s32 SEQ_UI_Button_Seq(s32 depressed, u32 seq)
{
  static u16 seq_pressed = 0x0000;

  u16 selected_tracks = seq ? 0xff00 : 0x00ff;

  // allow multi-selections
  if( depressed ) {
    seq_pressed &= ~selected_tracks;
  } else {
    seq_pressed |= selected_tracks;
    ui_selected_tracks = seq_pressed;
  }

  // armed tracks handling: alternate and select all tracks depending on seq selection
  // can be changed in RecArm page
  if( seq ) {
    if( (seq_record_state.ARMED_TRACKS & 0x00ff) )
      seq_record_state.ARMED_TRACKS = 0xff00;
  } else {
    if( (seq_record_state.ARMED_TRACKS & 0xff00) )
      seq_record_state.ARMED_TRACKS = 0x00ff;
  }

  return 0; // no error
}


s32 SEQ_UI_Button_Stop(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if sequencer running: stop it
  // if sequencer already stopped: reset song position
  if( SEQ_BPM_IsRunning() ) {
#if 0
    // TK: maybe to complicated to understand: STOP sequencer in slave mode if stop button pressed twice
    u8 enable_slaveclk_mute = !SEQ_BPM_IsMaster() && seq_core_slaveclk_mute != SEQ_CORE_SLAVECLK_MUTE_Enabled;
#else
    // always mute tracks, never stop sequencer (can only be done from external)
    u8 enable_slaveclk_mute = !SEQ_BPM_IsMaster();
#endif
    if( enable_slaveclk_mute ) {
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Enabled;
    } else {
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
      SEQ_BPM_Stop();
    }
  } else {
    seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
    SEQ_CORE_Reset(0);
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Pause(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if in auto mode and BPM generator is not clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

  // toggle pause
  ui_seq_pause ^= 1;

  // execute stop/continue depending on new mode
  MIOS32_IRQ_Disable();
  if( ui_seq_pause ) {
    if( !SEQ_BPM_IsMaster() ) {
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Enabled;
    } else {
      SEQ_BPM_Stop();
    }
  } else {
    if( !SEQ_BPM_IsMaster() ) {
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
    }

    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Cont();
  }
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 SEQ_UI_Button_Play(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // if in auto mode and BPM generator is not clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

  // slave mode and tracks muted: enable on next measure
  if( !SEQ_BPM_IsMaster() && SEQ_BPM_IsRunning() ) {
    if( seq_core_slaveclk_mute != SEQ_CORE_SLAVECLK_MUTE_Off )
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_OffOnNextMeasure;
    // TK: note - in difference to master mode pressing PLAY twice won't reset the sequencer!
  } else {
#if 0
    // if sequencer running: restart it
    // if sequencer stopped: continue at last song position
    if( SEQ_BPM_IsRunning() )
      SEQ_BPM_Start();
    else
      SEQ_BPM_Cont();
#else
    // always restart sequencer
    seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
    SEQ_BPM_Start();
#endif
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_TapTempo(s32 depressed)
{
  seq_ui_button_state.TAP_TEMPO = depressed ? 0 : 1;

  if( depressed ) return -1; // ignore when button depressed

  // enter tempo page
  SEQ_UI_PAGES_Set(SEQ_UI_PAGE_TEMPO);

  // determine current timestamp
  u32 timestamp = seq_core_timestamp_ms;
  u32 delay = timestamp - tap_tempo_last_timestamp;

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
    if( !SEQ_BPM_IsRunning() ) {
      SEQ_BPM_ModeSet(SEQ_BPM_MODE_Master); // force master mode
      SEQ_BPM_Start();
    }

  } else {
    // increment counter
    ++tap_tempo_beat_ctr;
  }

  return 0; // no error
}

static s32 SEQ_UI_Button_Master(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // note: after power-on SEQ_BPM_MODE_Auto is selected
  if( SEQ_BPM_ModeGet() == SEQ_BPM_MODE_Slave )
    SEQ_BPM_ModeSet(SEQ_BPM_MODE_Master);
  else
    SEQ_BPM_ModeSet(SEQ_BPM_MODE_Slave);

  return 0; // no error
}

static s32 SEQ_UI_Button_ExtRestart(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // should be atomic
  portENTER_CRITICAL();
  seq_core_state.EXT_RESTART_REQ = 1;
  portEXIT_CRITICAL();

  return 0; // no error
}

static s32 SEQ_UI_Button_Metronome(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  // should be atomic
  portENTER_CRITICAL();
  seq_core_state.METRONOME ^= 1;
  portEXIT_CRITICAL();

  return 0; // no error
}


static s32 SEQ_UI_Button_Load(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_LOAD);
}

static s32 SEQ_UI_Button_Save(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_SAVE);
}


static s32 SEQ_UI_Button_Copy(s32 depressed)
{
  seq_ui_button_state.COPY = depressed ? 0 : 1;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_UTIL_Copy();
}


static s32 SEQ_UI_Button_Paste(s32 depressed)
{
  seq_ui_button_state.PASTE = depressed ? 0 : 1;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_UTIL_Paste();
}

static s32 SEQ_UI_Button_Clear(s32 depressed)
{
  seq_ui_button_state.CLEAR = depressed ? 0 : 1;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_UTIL_Clear();
}


static s32 SEQ_UI_Button_Undo(s32 depressed)
{
  seq_ui_button_state.UNDO = depressed ? 0 : 1;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_UTIL_Undo();
}


static s32 SEQ_UI_Button_TrackTrigger(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_TRIGGER);
}

static s32 SEQ_UI_Button_TrackMute(s32 depressed)
{
  seq_ui_button_state.MUTE_PRESSED = depressed == 0;

  // special key combination to toggle controller mode
  if( seq_ui_button_state.MUTE_PRESSED && seq_ui_button_state.MIDICHN_PRESSED ) {
    if( ++ui_controller_mode >= UI_CONTROLLER_MODE_NUM )
      ui_controller_mode = 0;
    return 1;
  }

  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_MUTE);
}


static s32 SEQ_UI_Button_TrackMidiChn(s32 depressed)
{
  seq_ui_button_state.MIDICHN_PRESSED = depressed == 0;

  // special key combination to toggle controller mode
  if( seq_ui_button_state.MUTE_PRESSED && seq_ui_button_state.MIDICHN_PRESSED ) {
    if( ++ui_controller_mode >= UI_CONTROLLER_MODE_NUM )
      ui_controller_mode = 0;
    return 1;
  }

  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_MIDICHN);
}


static s32 SEQ_UI_Button_TrackGroove(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_GROOVE);
}

static s32 SEQ_UI_Button_TrackLength(s32 depressed)
{
  // show loop point when length button pressed
  seq_ui_button_state.LENGTH_PRESSED = depressed == 0;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_LENGTH);
}

static s32 SEQ_UI_Button_TrackProgression(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_PROGRESSION);
}

static s32 SEQ_UI_Button_TrackEcho(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_ECHO);
}


static s32 SEQ_UI_Button_TrackHumanizer(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_HUMANIZER);
}

static s32 SEQ_UI_Button_TrackLFO(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_LFO);
}

static s32 SEQ_UI_Button_TrackScale(s32 depressed)
{
  // show scale selection when scale button pressed
  seq_ui_button_state.SCALE_PRESSED = depressed == 0;
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_SCALE);
}

static s32 SEQ_UI_Button_RecArm(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_REC_ARM);
}

static s32 SEQ_UI_Button_RecStep(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_REC_STEP);
}

static s32 SEQ_UI_Button_RecLive(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed
  return SEQ_UI_PAGES_Set(SEQ_UI_PAGE_REC_LIVE);
}

static s32 SEQ_UI_Button_RecPoly(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  seq_record_options.POLY_RECORD ^= 1;

  return 0; // no error
}

static s32 SEQ_UI_Button_InOutFwd(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  seq_record_options.FWD_MIDI ^= 1;

  return 0; // no error
}

static s32 SEQ_UI_Button_TrackTranspose(s32 depressed)
{
  if( depressed ) return -1; // ignore when button depressed

  seq_core_global_transpose_enabled ^= 1;

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// Button handler for controller mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler_Controller(u32 pin, u32 pin_value)
{
  u8 depressed = pin_value == 1;
  mios32_midi_port_t port = ui_controller_port;
  u8 chn = ui_controller_chn;

  if( pin == seq_hwcfg_button.mute )
    seq_ui_button_state.MUTE_PRESSED = depressed == 0;
  if( pin == seq_hwcfg_button.midichn )
    seq_ui_button_state.MIDICHN_PRESSED = depressed == 0;

  if( pin == seq_hwcfg_button.mute || pin == seq_hwcfg_button.midichn ) {
    // special key combination to toggle controller mode
    if( seq_ui_button_state.MUTE_PRESSED && seq_ui_button_state.MIDICHN_PRESSED ) {
      if( ++ui_controller_mode >= UI_CONTROLLER_MODE_NUM )
	ui_controller_mode = 0;
      return 0;
    }
  }

  switch( ui_controller_mode ) {
  case UI_CONTROLLER_MODE_MAQ16_3:
    // only react on pressed buttons
    if( depressed )
      return 0;

    if( pin == seq_hwcfg_button.bar1 ) { MIOS32_MIDI_SendProgramChange(port, chn, 0x00); return 1; } // Select Row1
    if( pin == seq_hwcfg_button.bar2 ) { MIOS32_MIDI_SendProgramChange(port, chn, 0x01); return 1; } // Select Row2
    if( pin == seq_hwcfg_button.bar3 ) { MIOS32_MIDI_SendProgramChange(port, chn, 0x02); return 1; } // Select Row2
    if( pin == seq_hwcfg_button.bar4 ) { return 1; } // not assigned

    if( pin == seq_hwcfg_button.seq1  ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.seq2  ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.load ) { MIOS32_MIDI_SendProgramChange(port, chn, 0x05); return 1; } // Steps on
    if( pin == seq_hwcfg_button.save ) { MIOS32_MIDI_SendProgramChange(port, chn, 0x06); return 1; } // Steps off?

    if( pin == seq_hwcfg_button.copy  ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.paste ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.clear ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.undo  ) { return 1; } // not assigned

    // note: master/tempo/stop/play still working as usual

    // note: UI pages handled in seq_ui_pages.c

    if( pin == seq_hwcfg_button.rec_poly )  { return 1; } // not assigned
    if( pin == seq_hwcfg_button.inout_fwd ) { return 1; } // not assigned
    if( pin == seq_hwcfg_button.transpose ) { return 1; } // not assigned

    break;
  }

  return -1; // button not mapped
}

/////////////////////////////////////////////////////////////////////////////
// Button handler for normal mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value)
{
  int i;

  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() )
    return -1;

  // ignore during a backup or format is created
  if( seq_ui_backup_req || seq_ui_format_req )
    return -1;

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // update display
  seq_ui_display_update_req = 1;

  // MEMO: we could also use a jump table with references to the functions
  // here, but this "spagetthi code" simplifies the configuration and
  // the resulting ASM doesn't look that bad!

  if( pin != seq_hwcfg_button.tap_tempo )
    resetTapTempo();

  // controller mode?
  if( ui_controller_mode && SEQ_UI_Button_Handler_Controller(pin, pin_value) >= 0 )
    return 1;

  for(i=0; i<8; ++i)
    if( pin == (8*(seq_hwcfg_button.gp_din_l_sr-1) + i) )
      return SEQ_UI_Button_GP(pin_value, i + 0);

  for(i=0; i<8; ++i)
    if( pin == (8*(seq_hwcfg_button.gp_din_r_sr-1) + i) )
      return SEQ_UI_Button_GP(pin_value, i + 8);

  if( pin == seq_hwcfg_button.bar1 )
    return SEQ_UI_Button_Bar(pin_value, 0);
  if( pin == seq_hwcfg_button.bar2 )
    return SEQ_UI_Button_Bar(pin_value, 1);
  if( pin == seq_hwcfg_button.bar3 )
    return SEQ_UI_Button_Bar(pin_value, 2);
  if( pin == seq_hwcfg_button.bar4 )
    return SEQ_UI_Button_Bar(pin_value, 3);

  if( pin == seq_hwcfg_button.seq1 )
    return SEQ_UI_Button_Seq(pin_value, 0);
  if( pin == seq_hwcfg_button.seq2 )
    return SEQ_UI_Button_Seq(pin_value, 1);

  if( pin == seq_hwcfg_button.load )
    return SEQ_UI_Button_Load(pin_value);
  if( pin == seq_hwcfg_button.save )
    return SEQ_UI_Button_Save(pin_value);

  if( pin == seq_hwcfg_button.copy )
    return SEQ_UI_Button_Copy(pin_value);
  if( pin == seq_hwcfg_button.paste )
    return SEQ_UI_Button_Paste(pin_value);
  if( pin == seq_hwcfg_button.clear )
    return SEQ_UI_Button_Clear(pin_value);
  if( pin == seq_hwcfg_button.undo )
    return SEQ_UI_Button_Undo(pin_value);

  if( pin == seq_hwcfg_button.master )
    return SEQ_UI_Button_Master(pin_value);
  if( pin == seq_hwcfg_button.tap_tempo )
    return SEQ_UI_Button_TapTempo(pin_value);
  if( pin == seq_hwcfg_button.stop )
    return SEQ_UI_Button_Stop(pin_value);
  if( pin == seq_hwcfg_button.play )
    return SEQ_UI_Button_Play(pin_value);

  if( pin == seq_hwcfg_button.pause )
    return SEQ_UI_Button_Pause(pin_value);
  if( pin == seq_hwcfg_button.metronome )
    return SEQ_UI_Button_Metronome(pin_value);
  if( pin == seq_hwcfg_button.ext_restart )
    return SEQ_UI_Button_ExtRestart(pin_value);

  if( pin == seq_hwcfg_button.trigger )
    return SEQ_UI_Button_TrackTrigger(pin_value);
  if( pin == seq_hwcfg_button.length )
    return SEQ_UI_Button_TrackLength(pin_value);
  if( pin == seq_hwcfg_button.progression )
    return SEQ_UI_Button_TrackProgression(pin_value);
  if( pin == seq_hwcfg_button.groove )
    return SEQ_UI_Button_TrackGroove(pin_value);
  if( pin == seq_hwcfg_button.echo )
    return SEQ_UI_Button_TrackEcho(pin_value);
  if( pin == seq_hwcfg_button.humanizer )
    return SEQ_UI_Button_TrackHumanizer(pin_value);
  if( pin == seq_hwcfg_button.lfo )
    return SEQ_UI_Button_TrackLFO(pin_value);
  if( pin == seq_hwcfg_button.scale )
    return SEQ_UI_Button_TrackScale(pin_value);

  if( pin == seq_hwcfg_button.mute )
    return SEQ_UI_Button_TrackMute(pin_value);
  if( pin == seq_hwcfg_button.midichn )
    return SEQ_UI_Button_TrackMidiChn(pin_value);

  if( pin == seq_hwcfg_button.rec_arm )
    return SEQ_UI_Button_RecArm(pin_value);
  if( pin == seq_hwcfg_button.rec_step )
    return SEQ_UI_Button_RecStep(pin_value);
  if( pin == seq_hwcfg_button.rec_live )
    return SEQ_UI_Button_RecLive(pin_value);
  if( pin == seq_hwcfg_button.rec_poly )
    return SEQ_UI_Button_RecPoly(pin_value);
  if( pin == seq_hwcfg_button.inout_fwd )
    return SEQ_UI_Button_InOutFwd(pin_value);
  if( pin == seq_hwcfg_button.transpose )
    return SEQ_UI_Button_TrackTranspose(pin_value);

  // always print debugging message
#if 1
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("[SEQ_UI_Button_Handler] Button SR:%d, Pin:%d not mapped, it has been %s.\n", 
	    (pin >> 3) + 1,
	    pin & 7,
	    pin_value ? "depressed" : "pressed");
  MUTEX_MIDIOUT_GIVE;
#endif

  return -1; // button not mapped
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Remote Keyboard Function (called from SEQ_MIDI_IN)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_REMOTE_MIDI_Keyboard(u8 key, u8 depressed)
{
#if 0
  MIOS32_MIDI_SendDebugMessage("SEQ_UI_REMOTE_MIDI_Keyboard(%d, %d)\n", key, depressed);
#endif

#if 1
  switch( key ) {
    case 0x24: // C-2
      return SEQ_UI_Button_GP(depressed, 0);
    case 0x25: // C#2
      return SEQ_UI_Button_Bar(depressed, 0);
    case 0x26: // D-2
      return SEQ_UI_Button_GP(depressed, 1);
    case 0x27: // D#2
      return SEQ_UI_Button_Bar(depressed, 1);
    case 0x28: // E-2
      return SEQ_UI_Button_GP(depressed, 2);
    case 0x29: // F-2
      return SEQ_UI_Button_GP(depressed, 3);
    case 0x2a: // F#2
      return SEQ_UI_Button_Bar(depressed, 2);
    case 0x2b: // G-2
      return SEQ_UI_Button_GP(depressed, 4);
    case 0x2c: // G#2
      return SEQ_UI_Button_Bar(depressed, 3);
    case 0x2d: // A-2
      return SEQ_UI_Button_GP(depressed, 5);
    case 0x2e: // A#2
      return SEQ_UI_Button_Seq(depressed, 0);
    case 0x2f: // B-2
      return SEQ_UI_Button_GP(depressed, 6);

    case 0x30: // C-3
      return SEQ_UI_Button_GP(depressed, 7);
    case 0x31: // C#3
      return SEQ_UI_Button_Seq(depressed, 1);
    case 0x32: // D-3
      return SEQ_UI_Button_GP(depressed, 8);
    case 0x33: // D#3
      return 0 ; //SEQ_UI_Button_ParLayer(depressed, 2);
    case 0x34: // E-3
      return SEQ_UI_Button_GP(depressed, 9);
    case 0x35: // F-3
      return SEQ_UI_Button_GP(depressed, 10);
    case 0x36: // F#3
      return SEQ_UI_Button_RecArm(depressed);
    case 0x37: // G-3
      return SEQ_UI_Button_GP(depressed, 11);
    case 0x38: // G#3
      return SEQ_UI_Button_RecStep(depressed);
    case 0x39: // A-3
      return SEQ_UI_Button_GP(depressed, 12);
    case 0x3a: // A#3
      return SEQ_UI_Button_RecLive(depressed);
    case 0x3b: // B-3
      return SEQ_UI_Button_GP(depressed, 13);
  
    case 0x3c: // C-4
      return SEQ_UI_Button_GP(depressed, 14);
    case 0x3d: // C#4
      return SEQ_UI_Button_Stop(depressed);
    case 0x3e: // D-4
      return SEQ_UI_Button_GP(depressed, 15);
    case 0x3f: // D#4
      return SEQ_UI_Button_Play(depressed);
    case 0x40: // E-4
      return 0; // ignore
    case 0x41: // F-4
      return SEQ_UI_Button_TrackTrigger(depressed);
    case 0x42: // F#4
      return SEQ_UI_Button_Load(depressed);
    case 0x43: // G-4
      return SEQ_UI_Button_TrackLength(depressed); //0; // ignore
    case 0x44: // G#4
      return SEQ_UI_Button_Save(depressed);
    case 0x45: // A-4
      return SEQ_UI_Button_TrackProgression(depressed);
    case 0x46: // A#4
      return SEQ_UI_Button_Copy(depressed);
    case 0x47: // B-4
      return SEQ_UI_Button_TrackGroove(depressed);
  
    case 0x48: // C-5
      return SEQ_UI_Button_TrackEcho(depressed);
    case 0x49: // C#5
      return SEQ_UI_Button_Paste(depressed);
    case 0x4a: // D-5
      return SEQ_UI_Button_TrackHumanizer(depressed);
    case 0x4b: // D#5
      return SEQ_UI_Button_Clear(depressed);
    case 0x4c: // E-5
      return SEQ_UI_Button_TrackLFO(depressed);
    case 0x4d: // F-5
      return SEQ_UI_Button_TrackScale(depressed);
    case 0x4e: // F#5
      return SEQ_UI_Button_Undo(depressed);
    case 0x4f: // G-5
      return SEQ_UI_Button_TrackMute(depressed);
    case 0x50: // G#5
      return SEQ_UI_Button_Master(depressed);
    case 0x51: // A-5
      return SEQ_UI_Button_TrackMidiChn(depressed);
    case 0x52: // A#5
      return SEQ_UI_Button_TapTempo(depressed);
    case 0x53: // B-5
      return SEQ_UI_Button_RecArm(depressed);
  
    case 0x54: // C-6
      return SEQ_UI_Button_RecStep(depressed);
    case 0x55: // C#6
      return SEQ_UI_Button_Stop(depressed);
    case 0x56: // D-6
      return SEQ_UI_Button_RecLive(depressed);
    case 0x57: // D#6
      return SEQ_UI_Button_Play(depressed);
    case 0x58: // E-6
      return SEQ_UI_Button_RecPoly(depressed);
    case 0x59: // F-6
      return SEQ_UI_Button_InOutFwd(depressed);
    case 0x5a: // F#6
      return 0; // ignore
    case 0x5b: // G-6
      return SEQ_UI_Button_TrackTranspose(depressed);
    case 0x5c: // G#6
      return 0; //SEQ_UI_Button_Select(depressed);
    case 0x5d: // A-6
      return 0 ; //SEQ_UI_Button_Exit(depressed);
    case 0x5e: // A#6
      return 0; //SEQ_UI_Button_Down(depressed);
    case 0x5f: // B-6
      return 0 ;// SEQ_UI_Button_Up(depressed);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Encoder handler
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer)
{
  // send MIDI event in remote mode and exit
  if( seq_midi_sysex_remote_active_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_CLIENT )
    return SEQ_MIDI_SYSEX_REMOTE_Client_SendEncoder(encoder, incrementer);

  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() || seq_file_backup_notification )
    return -1;

  // ignore during a backup or format is created
  //if( seq_ui_backup_req || seq_ui_format_req )
  //return -1;

  if( encoder >= SEQ_HWCFG_NUM_ENCODERS )
    return -1; // encoder doesn't exist

  // ensure that selections are matching with track constraints
  SEQ_UI_CheckSelections();

  // stop current message
  //SEQ_UI_MsgStop();

  // limit incrementer
  if( incrementer > 3 )
    incrementer = 3;
  else if( incrementer < -3 )
    incrementer = -3;

  // encoder 0 increments BPM
  if( encoder == 0 ) {
    u16 value = (u16)(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num]*10);
    if( SEQ_UI_Var16_Inc(&value, 25, 3000, incrementer) ) { // at 384ppqn, the minimum BPM rate is ca. 2.5
      // set new BPM
      seq_core_bpm_preset_tempo[seq_core_bpm_preset_num] = (float)value/10.0;
      SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);
      //store_file_required = 1;
      seq_ui_display_update_req = 1;      
    }
    return 0;
  }

  return -1; // not mapped
}



/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_REMOTE_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  return 0; // not relevant
}


/////////////////////////////////////////////////////////////////////////////
// Called by BLM
// Replacement for MBSEQV4L
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Button_Handler(u8 button, u8 depressed)
{
  u8 prev_page = ui_page;
  ui_page = SEQ_UI_PAGE_TRIGGER;
  SEQ_UI_PAGES_GP_Button_Handler(button, depressed);
  ui_page = prev_page;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Update all LEDs
// Usually called from background task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler(void)
{
  //u8 visible_track = SEQ_UI_VisibleTrackGet();

  // check if update requested
  if( !seq_ui_display_update_req ) {
    // update BLM LEDs
    SEQ_BLM_LED_Update();

    return 0;
  }
  seq_ui_display_update_req = 0;

  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() || seq_file_backup_notification )
    return -1;

  u8 seq_running = SEQ_BPM_IsRunning() && (!seq_core_slaveclk_mute || ((seq_core_state.ref_step & 3) == 0));

  switch( ui_controller_mode ) {
  case UI_CONTROLLER_MODE_OFF:
    SEQ_LED_PinSet(seq_hwcfg_led.seq1, ui_selected_tracks & (1 << 0));
    SEQ_LED_PinSet(seq_hwcfg_led.seq2, ui_selected_tracks & (1 << 8));

    SEQ_LED_PinSet(seq_hwcfg_led.load, ui_page == SEQ_UI_PAGE_LOAD);
    SEQ_LED_PinSet(seq_hwcfg_led.save, ui_page == SEQ_UI_PAGE_SAVE);

    SEQ_LED_PinSet(seq_hwcfg_led.copy, seq_ui_button_state.COPY);
    SEQ_LED_PinSet(seq_hwcfg_led.paste, seq_ui_button_state.PASTE);
    SEQ_LED_PinSet(seq_hwcfg_led.clear, seq_ui_button_state.CLEAR);
    SEQ_LED_PinSet(seq_hwcfg_led.undo, seq_ui_button_state.UNDO);

    SEQ_LED_PinSet(seq_hwcfg_led.rec_poly, seq_record_options.POLY_RECORD);
    SEQ_LED_PinSet(seq_hwcfg_led.inout_fwd, seq_record_options.FWD_MIDI);
    SEQ_LED_PinSet(seq_hwcfg_led.transpose, seq_core_global_transpose_enabled);
    break;

  case UI_CONTROLLER_MODE_MAQ16_3:
    // no special LED functions...
    SEQ_LED_PinSet(seq_hwcfg_led.seq1, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.seq2, 0);

    SEQ_LED_PinSet(seq_hwcfg_led.load, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.save, 0);

    SEQ_LED_PinSet(seq_hwcfg_led.copy, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.paste, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.clear, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.undo, 0);

    SEQ_LED_PinSet(seq_hwcfg_led.rec_poly, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.inout_fwd, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.transpose, 0);
    break;
  }

  SEQ_LED_PinSet(seq_hwcfg_led.master, SEQ_BPM_IsMaster());
  // SEQ_LED_PinSet(seq_hwcfg_led.tap_tempo, 0); // handled in SEQ_UI_LED_Handler_Periodic()
  SEQ_LED_PinSet(seq_hwcfg_led.stop, 0);
  // note: no bug: we added check for ref_step&3 for flashing the LEDs to give a sign of activity in slave mode with slaveclk_muted
  SEQ_LED_PinSet(seq_hwcfg_led.play, seq_running);
  SEQ_LED_PinSet(seq_hwcfg_led.stop, !seq_running && !ui_seq_pause);
  SEQ_LED_PinSet(seq_hwcfg_led.pause, ui_seq_pause && !seq_core_slaveclk_mute);

  SEQ_LED_PinSet(seq_hwcfg_led.metronome, seq_core_state.METRONOME);
  SEQ_LED_PinSet(seq_hwcfg_led.ext_restart, seq_core_state.EXT_RESTART_REQ);

  {
    u8 flash = ui_controller_mode && ui_cursor_flash;
    SEQ_LED_PinSet(seq_hwcfg_led.trigger, (!flash && ui_page == SEQ_UI_PAGE_TRIGGER) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.length, (!flash && ui_page == SEQ_UI_PAGE_LENGTH) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.progression, (!flash && ui_page == SEQ_UI_PAGE_PROGRESSION) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.groove, (!flash && ui_page == SEQ_UI_PAGE_GROOVE) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.echo, (!flash && ui_page == SEQ_UI_PAGE_ECHO) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.humanizer, (!flash && ui_page == SEQ_UI_PAGE_HUMANIZER) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.lfo, (!flash && ui_page == SEQ_UI_PAGE_LFO) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.scale, (!flash && ui_page == SEQ_UI_PAGE_SCALE) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.mute, (!flash && ui_page == SEQ_UI_PAGE_MUTE) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.midichn, (!flash && ui_page == SEQ_UI_PAGE_MIDICHN) ? 1 : 0);

    SEQ_LED_PinSet(seq_hwcfg_led.rec_arm, (!flash && ui_page == SEQ_UI_PAGE_REC_ARM) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.rec_step, (!flash && ui_page == SEQ_UI_PAGE_REC_STEP) ? 1 : 0);
    SEQ_LED_PinSet(seq_hwcfg_led.rec_live, (!flash && ui_page == SEQ_UI_PAGE_REC_LIVE) ? 1 : 0);
  }

  // update BLM LEDs
  SEQ_BLM_LED_Update();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// updates high-prio LED functions (GP LEDs and Beat LED)
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LED_Handler_Periodic()
{
  static u16 fx_ctr = 0;

  // ignore as long as hardware config hasn't been read
  if( !SEQ_FILE_HW_ConfigLocked() || seq_file_backup_notification ) {
    // stupid LED FX during power on as long as HWCFG hasn't been read
    // just to give the user the message that something is going on (no LCD support...)
    int delay = 33;
    if( ++fx_ctr >= 32*delay )
      fx_ctr = 0;

    int fx_state = fx_ctr / delay;
    if( seq_file_backup_notification != NULL )
      fx_state = (15 * seq_file_backup_percentage) / 100;

    if( fx_state < 16 ) {
      u8 step = fx_state;
      u8 mask_dot = (1 << (step & 7));
      u8 mask_full = (1 << ((step & 7)+1))-1;
      u8 left_half = step < 8;
      u8 right_half = step >= 8;

      SEQ_LED_SRSet(0, left_half  ? mask_dot : 0x00);
      SEQ_LED_SRSet(4, right_half ? mask_dot : 0x00);

      SEQ_LED_SRSet(1, left_half  ? mask_full : 0xff);
      SEQ_LED_SRSet(2, left_half  ? mask_full : 0xff);
      SEQ_LED_SRSet(5, right_half ? mask_full : 0x00);
      SEQ_LED_SRSet(6, right_half ? mask_full : 0x00);

      SEQ_LED_SRSet(3, left_half  ? mask_dot : 0x00);
      SEQ_LED_SRSet(7, right_half ? mask_dot : 0x00);
    } else {
      u8 step = fx_state & 0xf;
      u8 mask_dot = (1 << (7-(step & 7)));
      u8 mask_full = ~((1 << (((step & 7))+1))-1);
      u8 left_half = step >= 8;
      u8 right_half = step < 8;

      SEQ_LED_SRSet(0, left_half  ? mask_dot : 0x00);
      SEQ_LED_SRSet(4, right_half ? mask_dot : 0x00);

      SEQ_LED_SRSet(1, !left_half  ? mask_full : 0x00);
      SEQ_LED_SRSet(2, !left_half  ? mask_full : 0x00);
      SEQ_LED_SRSet(5, !right_half ? mask_full : 0xff);
      SEQ_LED_SRSet(6, !right_half ? mask_full : 0xff);

      SEQ_LED_SRSet(3, left_half  ? mask_dot : 0x00);
      SEQ_LED_SRSet(7, right_half ? mask_dot : 0x00);
    }

    return -1;
  }
  
  // GP LEDs are updated when ui_page_gp_leds has changed
  static u16 prev_ui_page_gp_leds = 0x0000;
  u16 ui_page_gp_leds = SEQ_UI_PAGES_GP_LED_Handler();

  // beat LED
  u8 sequencer_running = SEQ_BPM_IsRunning();
  u8 beat_led_on = sequencer_running && ((seq_core_state.ref_step & 3) == 0);
  SEQ_LED_PinSet(seq_hwcfg_led.tap_tempo, (ui_page == SEQ_UI_PAGE_TEMPO) ? 1 : beat_led_on);

  // mirror to status LED (inverted, so that LED is normaly on)
  MIOS32_BOARD_LED_Set(0xffffffff, beat_led_on ? 0 : 1);

  // don't continue if no new step has been generated and GP LEDs haven't changed
  if( !seq_core_step_update_req && prev_ui_page_gp_leds == ui_page_gp_leds && sequencer_running && !tap_tempo_beat_ctr ) // sequencer running check: workaround - as long as sequencer not running, we won't get an step update request!
    return 0;
  seq_core_step_update_req = 0; // requested from SEQ_CORE if any step has been changed
  prev_ui_page_gp_leds = ui_page_gp_leds; // take over new GP pattern

  // for song position marker (supports 16 LEDs, check for selected step view)
  u16 pos_marker_mask = 0x0000;
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 played_step = seq_core_trk[visible_track].step;

  if( seq_core_slaveclk_mute != SEQ_CORE_SLAVECLK_MUTE_Enabled ) { // Off and OffOnNextMeasure
    if( sequencer_running && (played_step >> 4) == ui_selected_step_view )
      pos_marker_mask = 1 << (played_step & 0xf);
  }


  // follow step position if enabled
  if( seq_core_state.FOLLOW ) {
    u8 trk_step = seq_core_trk[visible_track].step;
    if( (trk_step & 0xf0) != (16*ui_selected_step_view) ) {
      ui_selected_step_view = trk_step / 16;
      ui_selected_step = (ui_selected_step % 16) + 16*ui_selected_step_view;
    }
  }

  // transfer to GP LEDs
  if( seq_hwcfg_led.pos_dout_l_sr ) {
    if( seq_hwcfg_led.pos_dout_l_sr )
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_l_sr-1, (ui_page_gp_leds >> 0) & 0xff);
    else
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_l_sr-1, ((ui_page_gp_leds ^ pos_marker_mask) >> 0) & 0xff);
  }

  if( seq_hwcfg_led.pos_dout_r_sr ) {
    if( seq_hwcfg_led.pos_dout_r_sr )
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_r_sr-1, (ui_page_gp_leds >> 8) & 0xff);
    else
      SEQ_LED_SRSet(seq_hwcfg_led.gp_dout_r_sr-1, ((ui_page_gp_leds ^ pos_marker_mask) >> 8) & 0xff);
  }


  // POS LEDs tap tempo overlay
  u16 pos_overlay = 0x0000;
  if( tap_tempo_beat_ctr ) {
    if( tap_tempo_beat_ctr == 1 )
      pos_overlay = 0x000f;
    else if( tap_tempo_beat_ctr == 2 )
      pos_overlay = 0x00ff;
    else if( tap_tempo_beat_ctr == 3 )
      pos_overlay = 0x0fff;
    else
      pos_overlay = 0xffff;
  }

  // pos LEDs
  SEQ_LED_SRSet(seq_hwcfg_led.pos_dout_l_sr-1, ((pos_marker_mask ^ pos_overlay) >> 0) & 0xff);
  SEQ_LED_SRSet(seq_hwcfg_led.pos_dout_r_sr-1, ((pos_marker_mask ^ pos_overlay) >> 8) & 0xff);

  u8 bar1, bar2, bar3, bar4;

  if( ui_page == SEQ_UI_PAGE_LENGTH ) {
    u8 length = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
    bar1 = 1;
    bar2 = length >= 16;
    bar3 = length >= 32;
    bar4 = length >= 48;

    if( ui_cursor_flash ) {
      if( ui_selected_step_view == 0 )
	bar1 ^= 1;
      else if( ui_selected_step_view == 1 )
	bar2 ^= 1;
      else if( ui_selected_step_view == 2 )
	bar3 ^= 1;
      else if( ui_selected_step_view == 3 )
	bar4 ^= 1;
    }
  } else {
    bar1 = ui_selected_step_view == 0;
    bar2 = ui_selected_step_view == 1;
    bar3 = ui_selected_step_view == 2;
    bar4 = ui_selected_step_view == 3;

    if( beat_led_on ) {
      if( played_step < 16 )
	bar1 ^= 1;
      else if( played_step < 32 )
	bar2 ^= 1;
      else if( played_step < 48 )
	bar3 ^= 1;
      else if( played_step < 64 )
	bar4 ^= 1;
    }
  }

  switch( ui_controller_mode ) {
  case UI_CONTROLLER_MODE_OFF:
    SEQ_LED_PinSet(seq_hwcfg_led.bar1, bar1);
    SEQ_LED_PinSet(seq_hwcfg_led.bar2, bar2);
    SEQ_LED_PinSet(seq_hwcfg_led.bar3, bar3);
    SEQ_LED_PinSet(seq_hwcfg_led.bar4, bar4);
    break;

  case UI_CONTROLLER_MODE_MAQ16_3:
    // no LED function defined...
    SEQ_LED_PinSet(seq_hwcfg_led.bar1, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.bar2, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.bar3, 0);
    SEQ_LED_PinSet(seq_hwcfg_led.bar4, 0);
    break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// dummy-stub for MBSEQV4L
// called each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Handler(void)
{
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
    ++ui_cursor_flash_overrun_ctr;
    seq_ui_display_update_req = 1;
  }

  // important: flash flag has to be recalculated on each invocation of this
  // handler, since counter could also be reseted outside this function
  u8 old_ui_cursor_flash = ui_cursor_flash;
  ui_cursor_flash = ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF;
  if( old_ui_cursor_flash != ui_cursor_flash )
    seq_ui_display_update_req = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Should be regulary called to check if the layer/instrument/step selection
// is valid for the current track
// At least executed before button/encoder and LCD function calls
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CheckSelections(void)
{
  if( !ui_selected_tracks )
    ui_selected_tracks = 0x00ff;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the currently visible track
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_UI_VisibleTrackGet(void)
{
  u8 track;
  u16 mask = 1;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, mask<<=1)
    if( ui_selected_tracks & mask )
      return track;

  return 0; // first track
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
// Prints a temporary error messages after file operation
// Expects error status number (as defined in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SDCardErrMsg(u16 delay, s32 status)
{
  // send error message to MIOS terminal
  MUTEX_MIDIOUT_TAKE;
  FILE_SendErrorMessage(status);
  MUTEX_MIDIOUT_GIVE;

#if 0
  // print on LCD
  char str[21];
  sprintf(str, "E%3d (FatFs: D%3d)", -status, file_dfs_errno < 1000 ? file_dfs_errno : 999);
  return SEQ_UI_Msg(SEQ_UI_MSG_SDCARD, delay, "!! SD Card Error !!!", str);
#else
  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Increments a 16bit variable within given min/max range
// OUT: 1 if value has been changed, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer)
{
  int new_value = *value;
  int prev_value = new_value;

  // extra: in fast mode increment 16bit values faster!
  if( max > 0x100 && (seq_ui_button_state.FAST_ENCODERS || seq_ui_button_state.FAST2_ENCODERS) )
    incrementer *= 10;

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
// stores a bookmark
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Bookmark_Store(u8 bookmark)
{
  if( bookmark >= SEQ_UI_BOOKMARKS_NUM )
    return -1;

  //seq_ui_bookmark_t *bm = (seq_ui_bookmark_t *)&seq_ui_bookmarks[bookmark];

  return -1; // not supported by MBSEQV4L
}

/////////////////////////////////////////////////////////////////////////////
// restores a bookmark
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_Bookmark_Restore(u8 bookmark)
{
  if( bookmark >= SEQ_UI_BOOKMARKS_NUM )
    return -1;

  //seq_ui_bookmark_t *bm = (seq_ui_bookmark_t *)&seq_ui_bookmarks[bookmark];

  return -1; // not supported by MBSEQV4L
}
