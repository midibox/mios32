// $Id$
/*
 * Routines for MBHP_BLM_SCALAR access
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

#include "seq_blm.h"
#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_song.h"
#include "seq_midply.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"
#include "seq_scale.h"
#include "seq_midi_in.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Variants
/////////////////////////////////////////////////////////////////////////////

// inverted velocity (piano like)
#define SEQ_BLM_KEYBOARD_INVERTED 1



/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define SYSEX_DISACK_INVALID_COMMAND      0x0c


#define SYSEX_BLM_CMD_REQUEST      0x00
#define SYSEX_BLM_CMD_LAYOUT       0x01


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  BLM_MODE_GRID,
  BLM_MODE_TRACKS,
  BLM_MODE_PATTERNS,
  BLM_MODE_KEYBOARD,
} blm_mode_t;


// command states
typedef enum {
  SYSEX_CMD_STATE_BEGIN,
  SYSEX_CMD_STATE_CONT,
  SYSEX_CMD_STATE_END
} sysex_cmd_state_t;

typedef union {
  struct {
    unsigned ALL:32;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned PING_BYTE_RECEIVED:1;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned COLUMNS_RECEIVED:1;
    unsigned ROWS_RECEIVED:1;
    unsigned COLOURS_RECEIVED:1;
  };

} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

mios32_midi_port_t seq_blm_port;

// will be decremented each second by SEQ_TASK_Period1S()
u8 blm_timeout_ctr;
// timeout after 15 seconds
#define BLM_TIMEOUT_RELOAD_VALUE 15


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_BLM_SYSEX_CmdFinished(void);
static s32 SEQ_BLM_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_BLM_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_BLM_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_BLM_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);

static u8 SEQ_BLM_BUTTON_Hlp_TransposeNote(u8 track, u8 note);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static blm_mode_t blm_mode;

static const u8 seq_blm_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4e }; // Header of MBHP_BLM_SCALAR

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;

static u16 blm_leds_green[SEQ_BLM_NUM_ROWS];
static u16 blm_leds_green_sent[SEQ_BLM_NUM_ROWS];
static u16 blm_leds_red[SEQ_BLM_NUM_ROWS];
static u16 blm_leds_red_sent[SEQ_BLM_NUM_ROWS];

static mios32_midi_port_t blm_keyboard_port[SEQ_BLM_NUM_ROWS];
static u8 blm_keyboard_chn[SEQ_BLM_NUM_ROWS];
static u8 blm_keyboard_note[SEQ_BLM_NUM_ROWS];
static u8 blm_keyboard_velocity[SEQ_BLM_NUM_ROWS];

static u16 blm_leds_extracolumn_green;
static u16 blm_leds_extracolumn_green_sent;
static u16 blm_leds_extracolumn_red;
static u16 blm_leds_extracolumn_red_sent;
static u16 blm_leds_extrarow_green;
static u16 blm_leds_extrarow_green_sent;
static u16 blm_leds_extrarow_red;
static u16 blm_leds_extrarow_red_sent;
static u8  blm_leds_extra_green;
static u8  blm_leds_extra_green_sent;
static u8  blm_leds_extra_red;
static u8  blm_leds_extra_red_sent;

static u8 blm_leds_rotate_view;
static u8 blm_led_row_offset;

static u8 blm_num_columns;
static u8 blm_num_rows;
static u8 blm_num_colours;
static u8 blm_force_update;
static u8 blm_shift_active;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_Init(u32 mode)
{
  seq_blm_port = 0; // disabled
  blm_mode = BLM_MODE_TRACKS;
  blm_num_columns = 16;
  blm_num_rows = 16;
  blm_num_colours = 2;
  blm_force_update = 0;
  blm_shift_active = 0;

  sysex_device_id = 0; // only device 0 supported yet

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns current green/red pattern of given row
/////////////////////////////////////////////////////////////////////////////
u16 SEQ_BLM_PatternGreenGet(u8 row)
{
  return (row < SEQ_BLM_NUM_ROWS) ? blm_leds_green[row] : 0;
}

u16 SEQ_BLM_PatternRedGet(u8 row)
{
  return (row < SEQ_BLM_NUM_ROWS) ? blm_leds_red[row] : 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
// Called from 
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  if( !seq_blm_port )
    return -1; // MIDI In not configured

  // check for MIDI port
  if( port != seq_blm_port )
    return -2;

  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(seq_blm_sysex_header) && midi_in != seq_blm_sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(seq_blm_sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      SEQ_BLM_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(seq_blm_sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	SEQ_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_END, midi_in);
      }
      SEQ_BLM_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	SEQ_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	SEQ_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case SYSEX_BLM_CMD_REQUEST: // ignore to avoid loopbacks
      break;

    case SYSEX_BLM_CMD_LAYOUT:
      SEQ_BLM_SYSEX_Cmd_Layout(port, cmd_state, midi_in);
      break;

    case 0x0e: // ignore to avoid loopbacks
      break;

    case 0x0f:
      SEQ_BLM_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      SEQ_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND);
      SEQ_BLM_SYSEX_CmdFinished();      
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Remote Command Handler
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.COLUMNS_RECEIVED ) {
	sysex_state.COLUMNS_RECEIVED = 1;
	blm_num_columns = midi_in;
	if( blm_num_columns >= SEQ_BLM_NUM_COLUMNS ) // limit to provided number of steps
	  blm_num_columns = SEQ_BLM_NUM_COLUMNS;
      } else if( !sysex_state.ROWS_RECEIVED ) {
	sysex_state.ROWS_RECEIVED = 1;
	blm_num_rows = midi_in;
	if( blm_num_rows >= SEQ_BLM_NUM_ROWS ) // limit to provided number of tracks
	  blm_num_rows = SEQ_BLM_NUM_ROWS;
      } else if( !sysex_state.COLOURS_RECEIVED ) {
	sysex_state.COLOURS_RECEIVED = 1;
	blm_num_colours = midi_in;
      }
      // ignore all other bytes
      // don't sent error message to allow future extensions
      break;

    default: // SYSEX_CMD_STATE_END
      // update BLM
      blm_force_update = 1;
      // send acknowledge
      SEQ_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);
      // and reload timeout counter
      blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      sysex_state.PING_BYTE_RECEIVED = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      sysex_state.PING_BYTE_RECEIVED = 1;
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // send acknowledge if no additional byte has been received
      // to avoid feedback loop if two cores are directly connected
      if( !sysex_state.PING_BYTE_RECEIVED )
	SEQ_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      // and reload timeout counter
      blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(seq_blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_blm_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  MUTEX_MIDIOUT_GIVE;
  return status;
}

/////////////////////////////////////////////////////////////////////////////
// This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
// MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_SYSEX_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX )
    SEQ_BLM_SYSEX_CmdFinished();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called to request the layout
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_SYSEX_SendRequest(u8 req)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  if( !seq_blm_port )
    return -1; // MIDI Out not configured

  for(i=0; i<sizeof(seq_blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_blm_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send request
  *sysex_buffer_ptr++ = SYSEX_BLM_CMD_REQUEST;
  *sysex_buffer_ptr++ = req;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(seq_blm_port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  MUTEX_MIDIOUT_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Grid Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateGridMode(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);


  ///////////////////////////////////////////////////////////////////////////
  // prepare red LEDs: clear all
  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<SEQ_BLM_NUM_ROWS; ++i)
    blm_leds_red[i] = 0x0000;


  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display pattern    
  // red LEDs: used in parameter view
  ///////////////////////////////////////////////////////////////////////////
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
    if( num_instruments > SEQ_BLM_NUM_ROWS )
      num_instruments = SEQ_BLM_NUM_ROWS;
    u16 step16 = ui_selected_step_view;
    int i;
    for(i=0; i<num_instruments; ++i) {
      // TODO: how about using red LEDs for accent?
      blm_leds_green[i] = SEQ_TRG_Get16(visible_track, step16, ui_selected_trg_layer, i);
    }
    while( i < SEQ_BLM_NUM_ROWS )
      blm_leds_green[i++] = 0x0000;

    // adjust display offset on limited displays
    blm_led_row_offset = 0;
    if( blm_num_rows <= 4 )
      blm_led_row_offset = ui_selected_instrument & 0xfc;
    else if( blm_num_rows <= 8 )
      blm_led_row_offset = ui_selected_instrument & 0xf8;
  } else {
#if 0
    u8 num_layers = SEQ_TRG_NumLayersGet(visible_track);
    u16 step16 = 16*ui_selected_step_view;
    int i;
    for(i=0; i<num_layers; ++i)
      blm_leds_green[i] = SEQ_TRG_Get16(visible_track, step16, i, 0);
    while( i < SEQ_BLM_NUM_ROWS )
      blm_leds_green[i++] = 0x0000;

    // adjust display offset on limited displays
    blm_led_row_offset = 0;
    if( blm_num_rows <= 4 )
      blm_led_row_offset = ui_selected_trg_layer & 0xfc;
    else if( blm_num_rows <= 8 )
      blm_led_row_offset = ui_selected_trg_layer & 0xf8;
#else
    blm_leds_rotate_view = 1;

    // branch depending on parameter layer type
    if( (seq_par_layer_type_t)seq_cc_trk[visible_track].lay_const[ui_selected_par_layer] != SEQ_PAR_Type_Note ) {
      u8 instrument = 0;
      int step = 16*ui_selected_step_view;
      int i;
      for(i=0; i<SEQ_BLM_NUM_COLUMNS; ++i, ++step) {
	if( SEQ_TRG_GateGet(visible_track, step, 0) ) {
	  u8 par = (SEQ_PAR_Get(visible_track, step, ui_selected_par_layer, instrument) >> 3) & 0x0f;
	  blm_leds_green[i] = (0xffff8000 >> par) & 0xfff8;
	  blm_leds_red[i] = (0xffff8000 >> par) & 0x00ff;
	} else {
	  blm_leds_green[i] = 0x0000;
	}
      }
    } else {
      u8 use_scale = 1; // should we use this only for force-to-scale mode? I don't think so - for best "first impression" :)
      u8 scale, root_selection, root;
      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
      root = 0; // force root to C

      u8 instrument = 0;
      int step = 16*ui_selected_step_view;
      int i;
      for(i=0; i<SEQ_BLM_NUM_COLUMNS; ++i, ++step) {
	u16 pattern = 0;
	if( SEQ_TRG_GateGet(visible_track, step, 0) ) {
	  int note;
	  if( use_scale )
	    note = 0x30; // C-2
	  else
	    note = 0x3c-8; // E-2

	  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
	  u8 *layer_type = (u8 *)&seq_cc_trk[visible_track].lay_const[0];
	  int note_ix;
	  for(note_ix=0; note_ix<16; ++note_ix) {
	    u8 next_note;
	    if( use_scale )
	      next_note = SEQ_SCALE_NextNoteInScale(note, scale, root);
	    else
	      next_note = note + 1;

	    int par_layer;
	    for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	      seq_par_layer_type_t par_type = layer_type[par_layer];
	      if( (par_type == SEQ_PAR_Type_Note) ) {
		u8 stored_note = SEQ_PAR_Get(visible_track, step, par_layer, instrument);
		if( stored_note >= note && stored_note < next_note )
		  pattern |= (1 << SEQ_BLM_NUM_COLUMNS-1-note_ix);
	      }
	    }

	    if( use_scale )
	      note = next_note;
	    else
	      ++note;
	  }
	}

	blm_leds_green[i] = pattern;
      }
    }
#endif
  }

  ///////////////////////////////////////////////////////////////////////////
  // red LEDs: display position marker
  ///////////////////////////////////////////////////////////////////////////
  u8 sequencer_running = SEQ_BPM_IsRunning();
  if( sequencer_running ) {
    int played_step = seq_core_trk[visible_track].step;
    int step_in_view = played_step % 16;
    if( (played_step / 16) == ui_selected_step_view ) {
      if( blm_leds_rotate_view ) {
	blm_leds_red[step_in_view] = 0xffff;
	blm_leds_green[step_in_view] = 0x0000; // disable green LEDs
      } else {
	u16 mask = 1 << step_in_view;
	for(i=0; i<SEQ_BLM_NUM_ROWS; ++i) {
	  blm_leds_red[i] |= mask;
	  blm_leds_green[i] &= ~mask; // disable green LEDs
	}
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_shift_active ) {
    blm_leds_extracolumn_green = ui_selected_tracks;
    blm_leds_extracolumn_red = seq_core_trk_muted;

    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_leds_extrarow_red = 15 << (4*played_step_view);
  } else {
    blm_leds_extracolumn_green = 0x0000;
    blm_leds_extracolumn_red = (1 << (15-blm_mode)) | (SEQ_BPM_IsRunning() ? 0x0001 : 0x0002);
    blm_leds_extrarow_green = 0x0000;
    blm_leds_extrarow_red = 0x0000;
  }

  return 0; // no error
}

static s32 SEQ_BLM_BUTTON_GP_GridMode(u8 button_row, u8 button_column, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    // change trigger layer/instrument if required
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);

    if( button_row >= num_instruments )
      return 0;

    if( blm_num_rows <= 4 ) {
      if( button_row >= 4 )
	return 0; // invalid channel
      ui_selected_instrument = (ui_selected_instrument & 0xfc) + button_row;
    } else if( blm_num_rows <= 8 ) {
      if( button_row >= 8 )
	return 0; // invalid channel
      ui_selected_instrument = (ui_selected_instrument & 0xf8) + button_row;
    } else {
      ui_selected_instrument = button_row;
    }

    // -> common edit handling
    SEQ_UI_EDIT_Button_Handler(button_column % 16, depressed);
  } else {
    if( depressed ) // ignore depressed key
      return 0;

    u8 instrument = 0;

    // TODO: consider ui_selected_trg_layer?

    // branch depending on parameter layer type
    if( (seq_par_layer_type_t)seq_cc_trk[visible_track].lay_const[ui_selected_par_layer] != SEQ_PAR_Type_Note ) {
      if( button_row < 16 ) {
	ui_selected_step = 16*ui_selected_step_view + button_column;
	u8 par = (((15-button_row) << 3) + 4) & 0x7f;
	SEQ_PAR_Set(visible_track, ui_selected_step, ui_selected_par_layer, instrument, par);

	if( seq_cc_trk[visible_track].event_mode == SEQ_EVENT_MODE_CC ) {
	  // enable gate in any case (or should we disable it if CC value is 0 or button pressed twice?)
	  SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 1);
	}
      }
    } else {
      u8 use_scale = 1; // should we use this only for force-to-scale mode? I don't think so - for best "first impression" :)
      u8 scale, root_selection, root;
      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
      root = 0; // force root to C

      u8 note_start;
      u8 note_next;
      if( use_scale ) {
	// determine matching note range in scale
	note_start = 0x30; // C-2
	note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	int i;
	for(i=0; i<(15-button_row); ++i) {
	  note_start = note_next;
	  note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	}
      } else {
	note_start = 0x3c-8 + 15-button_row; // E-2 ..
	note_next = note_start;
      }


      u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
      u8 *layer_type = (u8 *)&seq_cc_trk[visible_track].lay_const[0];
      ui_selected_step = 16*ui_selected_step_view + button_column;
      if( !SEQ_TRG_GateGet(visible_track, ui_selected_step, instrument) ) {
	// set note on first layer
	int par_layer = 0;
	SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, note_start);
	ui_selected_par_layer = par_layer;

	// clear remaining layers assigned to note
	for(par_layer=1; par_layer<num_p_layers; ++par_layer) {
	  seq_par_layer_type_t par_type = layer_type[par_layer];
	  if( par_type == SEQ_PAR_Type_Note )
	    SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, 0x00);
	}

	// enable gate
	SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 1);
      } else {
	// search all layers for matching note. if found, clear this note
	// otherwise use layer where no note has been assigned yet
	// if all layers allocated, use first layer
	// TODO: should we cycle the layer that is used if no free layer has been found?
	int par_layer;
	int unused_layer = -1;
	int num_used_layers = 0;
	int found_note_in_layer = -1;
	for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	  seq_par_layer_type_t par_type = layer_type[par_layer];
	  if( par_type == SEQ_PAR_Type_Note ) {
	    u8 par_value = SEQ_PAR_Get(visible_track, ui_selected_step, par_layer, instrument);
	    if( !par_value ) {
	      if( unused_layer < 0 )
		unused_layer = par_layer;
	    } else {
	      ++num_used_layers;
	      if( par_value >= note_start && par_value < note_next ) {
		if( found_note_in_layer >= 0 )
		  SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, 0x00); // clear redundant note
		else
		  found_note_in_layer = par_layer; // unassigned note in scale
	      }
	    }
	  }
	}

	if( found_note_in_layer >= 0 ) {
	  if( num_used_layers < 2 ) {
	    // only one note: clear gate
	    SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 0);
	  } else {
	    // clear note
	    SEQ_PAR_Set(visible_track, ui_selected_step, found_note_in_layer, instrument, 0x00);
	  }
	} else {
	  if( unused_layer < 0 )
	    unused_layer = 0;
	  SEQ_PAR_Set(visible_track, ui_selected_step, unused_layer, instrument, note_start);
	  ui_selected_par_layer = unused_layer;
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Track Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateTrackMode(void)
{
  int i;
  u8 sequencer_running = SEQ_BPM_IsRunning();

  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display selected trigger layer
  // red LEDs: display position marker
  ///////////////////////////////////////////////////////////////////////////
  for(i=0; i<SEQ_BLM_NUM_ROWS; ++i) {
    blm_leds_green[i] = SEQ_TRG_Get16(i, ui_selected_step_view, ui_selected_trg_layer, ui_selected_instrument);
    blm_leds_red[i] = 0x0000;

    if( sequencer_running ) {
      int played_step = seq_core_trk[i].step;
      int step_in_view = played_step % 16;
      if( (played_step / 16) == ui_selected_step_view ) {
	u16 mask = 1 << step_in_view;
	blm_leds_red[i] = mask;
	blm_leds_green[i] &= ~mask; // disable green LEDs
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // adjust display offset on limited displays
  ///////////////////////////////////////////////////////////////////////////
  blm_led_row_offset = 0;
  if( blm_num_rows <= 4 )
    blm_led_row_offset = 4*ui_selected_group;
  else if( blm_num_rows <= 8 )
    blm_led_row_offset = 8*(ui_selected_group >> 1);


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_shift_active ) {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    blm_leds_extracolumn_green = ui_selected_tracks;
    blm_leds_extracolumn_red = seq_core_trk_muted;

    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_leds_extrarow_red = 15 << (4*played_step_view);
  } else {
    blm_leds_extracolumn_green = 0x0000;
    blm_leds_extracolumn_red = (1 << (15-blm_mode)) | (SEQ_BPM_IsRunning() ? 0x0001 : 0x0002);
    blm_leds_extrarow_green = 0x0000;
    blm_leds_extrarow_red = 0x0000;
  }

  return 0; // no error
}


static s32 SEQ_BLM_BUTTON_GP_TrackMode(u8 button_row, u8 button_column, u8 depressed)
{
  // -> common edit handling
  return SEQ_UI_EDIT_Button_Handler(button_column % 16, depressed);
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Pattern Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdatePatternMode(void)
{
  int i;

  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display selected pattern
  // red LEDs: active if pattern is played
  ///////////////////////////////////////////////////////////////////////////
  for(i=0; i<SEQ_BLM_NUM_ROWS; ++i) {
    u8 group = i / SEQ_CORE_NUM_TRACKS_PER_GROUP;
    u8 pnum = seq_pattern_req[group].pattern;
    int x = pnum & 0xf;
    int y = (pnum >> 4) & 0x3;

    blm_leds_green[i] = ((i&3) == y) ? (1 << x) : 0x0000;

    pnum = seq_pattern[group].pattern;
    x = pnum & 0xf;
    y = (pnum >> 4) & 0x3;
    blm_leds_red[i] = ((i&3) == y) ? (1 << x) : 0x0000;
  }

  ///////////////////////////////////////////////////////////////////////////
  // adjust display offset on limited displays
  ///////////////////////////////////////////////////////////////////////////
  blm_led_row_offset = 0;
  if( blm_num_rows <= 4 )
    blm_led_row_offset = 4*ui_selected_group;
  else if( blm_num_rows <= 8 )
    blm_led_row_offset = 8*(ui_selected_group >> 1);


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_shift_active ) {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    blm_leds_extracolumn_green = ui_selected_tracks;
    blm_leds_extracolumn_red = seq_core_trk_muted;

    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_leds_extrarow_red = 15 << (4*played_step_view);
  } else {
    blm_leds_extracolumn_green = 0x0000;
    blm_leds_extracolumn_red = (1 << (15-blm_mode)) | (SEQ_BPM_IsRunning() ? 0x0001 : 0x0002);
    blm_leds_extrarow_green = 0x0000;
    blm_leds_extrarow_red = 0x0000;
  }

  return 0; // no error
}

static s32 SEQ_BLM_BUTTON_GP_PatternMode(u8 button_row, u8 button_column, u8 depressed)
{
  if( button_column >= 16 )
    return 0; // only key 0..15 supported

  seq_pattern_t new_pattern = seq_pattern[ui_selected_group];
  new_pattern.pattern = ((button_row&0x3) << 4) | button_column;
  SEQ_PATTERN_Change(ui_selected_group, new_pattern);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Keyboard Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateKeyboardMode(void)
{
  int i;
  u8 sequencer_running = SEQ_BPM_IsRunning();

  ///////////////////////////////////////////////////////////////////////////
  // green/red LEDs: display played note and velocity
  ///////////////////////////////////////////////////////////////////////////

  blm_leds_rotate_view = 1;

  for(i=0; i<SEQ_BLM_NUM_COLUMNS; ++i) {
    if( blm_keyboard_velocity[i] ) {
      u8 vel4 = (blm_keyboard_velocity[i] >> 3) & 0x0f;
#if SEQ_BLM_KEYBOARD_INVERTED
      // inverted velocity (piano like)
      // 0: 0x0001
      // 1: 0x0003
      // 2: 0x0007
      // 3: 0x000f
      // ..
      // 15: 0xffff
      u32 pattern = (0x1ffff << vel4) >> 16;

      blm_leds_green[i] = pattern & 0x1fff;
      blm_leds_red[i] = pattern & 0xff00;
#else
      // 0: 0x8000
      // 1: 0xc000
      // 2: 0xe000
      // 3: 0xf000
      // ..
      // 15: 0xffff
      u32 pattern = 0xffff8000 >> vel4;

      blm_leds_green[i] = pattern & 0xfff8;
      blm_leds_red[i] = pattern & 0x00ff;
#endif
    } else {
      blm_leds_green[i] = 0x0000;
      blm_leds_red[i] = 0x0000;
    }
  }

  if( sequencer_running ) {
    int played_step = seq_core_state.ref_step % 16;
    blm_leds_red[played_step] = 0xffff; // set all red LEDs
    blm_leds_green[played_step] = 0x0000; // disable green LEDs
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_shift_active ) {
    blm_leds_extracolumn_green = ui_selected_tracks;
    blm_leds_extracolumn_red = seq_core_trk_muted;
    u8 transposer_note = SEQ_MIDI_IN_TransposerNoteGet(0, 1); // hold mode
    if( transposer_note >= 0x3c && transposer_note <= 0x4b )
      blm_leds_extrarow_green = 1 << (transposer_note - 0x3c);
    else
      blm_leds_extrarow_green = 0x0000;
    blm_leds_extrarow_red = 0x0000;
  } else {
    blm_leds_extracolumn_green = 0x0000;
    blm_leds_extracolumn_red = (1 << (15-blm_mode)) | (SEQ_BPM_IsRunning() ? 0x0001 : 0x0002);
    blm_leds_extrarow_green = 0x0000;
    blm_leds_extrarow_red = 0x0000;
  }

  return 0; // no error
}


static s32 SEQ_BLM_BUTTON_GP_KeyboardMode(u8 button_row, u8 button_column, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 play_note = 0;
#if SEQ_BLM_KEYBOARD_INVERTED
  // inverted velocity (piano like)
  u8 velocity = 8*button_row + 4;
#else
  u8 velocity = 8*(15-button_row) + 4;
#endif

  if( depressed ) {
    // play off event - but only if depressed button matches with last one that played the note
    if( blm_keyboard_velocity[button_column] == velocity ) {
      blm_keyboard_velocity[button_column] = 0;
      play_note = 1;
    }
  } else {
    u8 use_scale = 1; // should we use this only for force-to-scale mode? I don't think so - for best "first impression" :)
    u8 scale, root_selection, root;
    SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);

    u8 note_start;
    u8 note_next;
    if( use_scale ) {
      // determine matching note range in scale
      note_start = SEQ_MIDI_IN_TransposerNoteGet(0, 1); // hold mode      
      note_start = SEQ_BLM_BUTTON_Hlp_TransposeNote(visible_track, note_start); // transpose this note based on track settings
      note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
      int i;
      for(i=0; i<button_column; ++i) {
	note_start = note_next;
	note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
      }
    } else {
      note_start = 0x3c-8 + 15-button_column; // E-2 ..
      note_start = SEQ_BLM_BUTTON_Hlp_TransposeNote(visible_track, note_start); // transpose this note based on track settings
      note_next = note_start;
    }

    // play off event if note still active (e.g. different velocity of same note played)
    if( blm_keyboard_velocity[button_column] ) {
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendNoteOn(blm_keyboard_port[button_column],
			     blm_keyboard_chn[button_column],
			     blm_keyboard_note[button_column],
			     0x00);
      MUTEX_MIDIOUT_GIVE;
    }
    
    // set new port/channel/note/velocity
    blm_keyboard_port[button_column] = seq_cc_trk[visible_track].midi_port;
    blm_keyboard_chn[button_column] = seq_cc_trk[visible_track].midi_chn;
    blm_keyboard_note[button_column] = note_start;
    blm_keyboard_velocity[button_column] = velocity;
    play_note = 1;
  }

  if( play_note ) {
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendNoteOn(blm_keyboard_port[button_column],
			   blm_keyboard_chn[button_column],
			   blm_keyboard_note[button_column],
			   blm_keyboard_velocity[button_column]);
    MUTEX_MIDIOUT_GIVE;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// All notes off for all keyboard notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_KeyboardAllNotesOff(void)
{
  int i;

  for(i=0; i<SEQ_BLM_NUM_COLUMNS; ++i) {
    if( blm_keyboard_velocity[i] ) {
      blm_keyboard_velocity[i] = 0;

      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendNoteOn(blm_keyboard_port[i],
			     blm_keyboard_chn[i],
			     blm_keyboard_note[i],
			     blm_keyboard_velocity[i]);
      MUTEX_MIDIOUT_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED update routine of BLM
// periodically called from low-prio task
//
// MEMO: issue with using blm_num_columns: if somebody would ever use a directly
// connected BLM together with a MIDI accessed BLM_SCALAR, and if he would use a BLM_SCALAR
// with smaller size (very unlikely), this update wouldn't be complete!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_LED_Update(void)
{
  ///////////////////////////////////////////////////////////////////////////
  // call LED update handler depending on current mode
  ///////////////////////////////////////////////////////////////////////////

  // will be set to 1 if view should be rotated
  blm_leds_rotate_view = 0;
  // row offset is used if BLM supports less than 16 rows
  blm_led_row_offset = 0;

  switch( blm_mode ) {
    case BLM_MODE_GRID:
      SEQ_BLM_LED_UpdateGridMode();
      break;

    case BLM_MODE_PATTERNS:
      SEQ_BLM_LED_UpdatePatternMode();
      break;

    case BLM_MODE_KEYBOARD:
      SEQ_BLM_LED_UpdateKeyboardMode();
      break;

    default: // BLM_MODE_TRACKS
      SEQ_BLM_LED_UpdateTrackMode();
  }


  ///////////////////////////////////////////////////////////////////////////
  // Don't send MIDI events on timeout
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_timeout_ctr )
    return -1;


  ///////////////////////////////////////////////////////////////////////////
  // Take over update force flag
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_IRQ_Disable();
  u8 force_update = blm_force_update;
  blm_force_update = 0;
  MIOS32_IRQ_Enable();


  ///////////////////////////////////////////////////////////////////////////
  // send LED changes to BLM16x16
  ///////////////////////////////////////////////////////////////////////////
  if( seq_blm_port ) {
    int i;
    for(i=0; i<blm_num_rows; ++i) {
      u8 led_row = i + blm_led_row_offset;

      u16 pattern_green = blm_leds_green[led_row];
      u16 prev_pattern_green = blm_leds_green_sent[led_row];
      u16 pattern_red = blm_leds_red[led_row];
      u16 prev_pattern_red = blm_leds_red_sent[led_row];

      if( force_update || pattern_green != prev_pattern_green || pattern_red != prev_pattern_red ) {
	MUTEX_MIDIOUT_TAKE;

	// Note: the MIOS32 MIDI driver will take care about running status to optimize the stream
	if( force_update || (pattern_green ^ prev_pattern_green) & 0x00ff ) {
	  u8 pattern8 = pattern_green;
	  MIOS32_MIDI_SendCC(seq_blm_port, // port
			     led_row, // Channel (== LED Row)
			     8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 17 : 16), // CC number + MSB LED
			     pattern8 & 0x7f); // remaining 7 LEDs
	}

	if( force_update || (pattern_green ^ prev_pattern_green) & 0xff00 ) {
	  u8 pattern8 = pattern_green >> 8;
	  MIOS32_MIDI_SendCC(seq_blm_port, // port
			     led_row, // Channel (== LED Row)
			     8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 19 : 18), // CC number + MSB LED
			     pattern8 & 0x7f); // remaining 7 LEDs
	}	

	if( force_update || (pattern_red ^ prev_pattern_red) & 0x00ff ) {
	  u8 pattern8 = pattern_red;
	  MIOS32_MIDI_SendCC(seq_blm_port, // port
			     led_row, // Channel (== LED Row)
			     8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 33 : 32), // CC number + MSB LED
			     pattern8 & 0x7f); // remaining 7 LEDs
	}

	if( force_update || (pattern_red ^ prev_pattern_red) & 0xff00 ) {
	  u8 pattern8 = pattern_red >> 8;
	  MIOS32_MIDI_SendCC(seq_blm_port, // port
			     led_row, // Channel (== LED Row)
			     8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 35 : 34), // CC number + MSB LED
			     pattern8 & 0x7f); // remaining 7 LEDs
	}	

	MUTEX_MIDIOUT_GIVE;
	blm_leds_green_sent[led_row] = pattern_green;
	blm_leds_red_sent[led_row] = pattern_red;
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // send LED changes to extra buttons
  ///////////////////////////////////////////////////////////////////////////
  if( seq_blm_port ) {
    u8 sequencer_running = SEQ_BPM_IsRunning();

    blm_leds_extra_green = (sequencer_running && ((seq_core_state.ref_step & 3) == 0)) ? 0x01 : 0x00;
    blm_leds_extra_red = blm_shift_active ? 0x01 : 0x00;

    MUTEX_MIDIOUT_TAKE;
    if( force_update || blm_leds_extra_green != blm_leds_extra_green_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 15, 0x60, blm_leds_extra_green);
      blm_leds_extra_green_sent = blm_leds_extra_green;
    }

    if( force_update || blm_leds_extra_red != blm_leds_extra_red_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 15, 0x68, blm_leds_extra_red);
      blm_leds_extra_red_sent = blm_leds_extra_red;
    }

    if( force_update || blm_leds_extracolumn_green != blm_leds_extracolumn_green_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extracolumn_green & 0x0080) ? 0x41 : 0x40, (blm_leds_extracolumn_green >> 0) & 0x7f);
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extracolumn_green & 0x8000) ? 0x43 : 0x42, (blm_leds_extracolumn_green >> 8) & 0x7f);
      blm_leds_extracolumn_green_sent = blm_leds_extracolumn_green;
    }

    if( force_update || blm_leds_extracolumn_red != blm_leds_extracolumn_red_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extracolumn_red & 0x0080) ? 0x49 : 0x48, (blm_leds_extracolumn_red >> 0) & 0x7f);
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extracolumn_red & 0x8000) ? 0x4b : 0x4a, (blm_leds_extracolumn_red >> 8) & 0x7f);
      blm_leds_extracolumn_red_sent = blm_leds_extracolumn_red;
    }

    if( force_update || blm_leds_extrarow_green != blm_leds_extrarow_green_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extrarow_green & 0x0080) ? 0x61 : 0x60, (blm_leds_extrarow_green >> 0) & 0x7f);
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extrarow_green & 0x8000) ? 0x63 : 0x62, (blm_leds_extrarow_green >> 8) & 0x7f);
      blm_leds_extrarow_green_sent = blm_leds_extrarow_green;
    }

    if( force_update || blm_leds_extrarow_red != blm_leds_extrarow_red_sent ) {
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extrarow_red & 0x0080) ? 0x69 : 0x68, (blm_leds_extrarow_red >> 0) & 0x7f);
      MIOS32_MIDI_SendCC(seq_blm_port, 0, (blm_leds_extrarow_red & 0x8000) ? 0x6b : 0x6a, (blm_leds_extrarow_red >> 8) & 0x7f);
      blm_leds_extrarow_red_sent = blm_leds_extrarow_red;
    }
    MUTEX_MIDIOUT_GIVE;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c) if port
// matches with seq_blm_port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // ignore any event on timeout
  if( !blm_timeout_ctr )
    return -1;

  // for easier parsing: convert Note Off -> Note On with velocity 0
  if( midi_package.event == NoteOff ) {
    midi_package.event = NoteOn;
    midi_package.velocity = 0;
  }

  if( midi_package.event == NoteOn ) {

    if( midi_package.note < SEQ_BLM_NUM_COLUMNS ) {
      // change display view if required
      if( blm_mode == BLM_MODE_TRACKS || blm_mode == BLM_MODE_GRID ) {
	if( blm_num_columns <= 16 ) {
	  // don't change view
	} else if( blm_num_columns <= 32 ) {
	  ui_selected_step_view = (ui_selected_step_view & 0xfe) + ((midi_package.note/16) & 0x01);
	} else if( blm_num_columns <= 64 ) {
	  ui_selected_step_view = (ui_selected_step_view & 0xfc) + ((midi_package.note/16) & 0x03);
	} else if( blm_num_columns <= 128 ) {
	  ui_selected_step_view = (ui_selected_step_view & 0xf8) + ((midi_package.note/16) & 0x07);
	} else {
	  ui_selected_step_view = (midi_package.note>>8) & 0x0f;
	}
      }

      // change track if required
      if( blm_mode == BLM_MODE_TRACKS || blm_mode == BLM_MODE_PATTERNS ) {
	if( blm_num_rows <= 4 ) {
	  if( midi_package.chn >= 4 )
	    return 0; // invalid channel
	} else if( blm_num_rows <= 8 ) {
	  if( midi_package.chn >= 8 )
	    return 0; // invalid channel
	  ui_selected_group = (ui_selected_group & 2) + (midi_package.chn / 4);
	} else {
	  ui_selected_group = (midi_package.chn / 4);
	}
	if( blm_mode != BLM_MODE_PATTERNS )
	  ui_selected_tracks = 1 << (4*ui_selected_group + (midi_package.chn % 4));
      }

      // request display update
      seq_ui_display_update_req = 1;
      ui_cursor_flash_ctr = ui_cursor_flash_overrun_ctr = 0;

      // call button handler
      switch( blm_mode ) {
      case BLM_MODE_PATTERNS:
	SEQ_BLM_BUTTON_GP_PatternMode(midi_package.chn, midi_package.note, midi_package.velocity ? 0 : 1);
	break;

      case BLM_MODE_GRID:
	SEQ_BLM_BUTTON_GP_GridMode(midi_package.chn, midi_package.note, midi_package.velocity ? 0 : 1);
	break;

      case BLM_MODE_KEYBOARD:
	SEQ_BLM_BUTTON_GP_KeyboardMode(midi_package.chn, midi_package.note, midi_package.velocity ? 0 : 1);
	break;

      default: // BLM_MODE_TRACKS
	SEQ_BLM_BUTTON_GP_TrackMode(midi_package.chn, midi_package.note, midi_package.velocity ? 0 : 1);
      }

      return 1; // MIDI event has been taken

    } else if( midi_package.note == 0x40 ) {

      if( midi_package.velocity > 0 ) {
	// disable all keyboard notes (important when switching between modes or tracks!)
	SEQ_BLM_KeyboardAllNotesOff();
      }

      // Extra Column
      if( !blm_shift_active ) {
	if( midi_package.velocity > 0 ) {
	  u16 track_mask = 1 << midi_package.chn;

	  if( ui_selected_tracks == track_mask ) // track already selected: mute it
	    seq_core_trk_muted ^= track_mask;
	  else { // track not selected yet: select it now
	    ui_selected_tracks = track_mask;
	    ui_selected_group = (midi_package.chn / 4);
	    blm_force_update = 1;	

	    // set/clear encoder fast function if required
	    SEQ_UI_InitEncSpeed(1); // auto config
	  }
	}
	return 1; // MIDI event has been taken
      } else {

	if( midi_package.velocity > 0 ) {
	  switch( midi_package.chn ) {
	  case 0x0:
	    // if in auto mode and BPM generator is clocked in slave mode:
	    // change to master mode
	    SEQ_BPM_CheckAutoMaster();
	    // always restart sequencer
	    SEQ_BPM_Start();
	    return 1; // MIDI event has been taken

	  case 0x1:
	    // if sequencer running: stop it
	    // if sequencer already stopped: reset song position
	    if( SEQ_BPM_IsRunning() )
	      SEQ_BPM_Stop();
	    else {
	      SEQ_SONG_Reset();
	      SEQ_CORE_Reset();
	      SEQ_MIDPLY_Reset();
	    }
	    return 1; // MIDI event has been taken

#if 0
	  case 0x02: {
	    // tmp. for the show ;-)
	    u8 visible_track = SEQ_UI_VisibleTrackGet();
	    if( SEQ_CC_Get(visible_track, SEQ_CC_STEPS_FORWARD) ) {
	      SEQ_CC_Set(visible_track, SEQ_CC_STEPS_FORWARD, 0);
	      SEQ_CC_Set(visible_track, SEQ_CC_STEPS_JMPBCK, 0);
	      MIOS32_IRQ_Disable();
	      seq_core_trk[visible_track].state.SYNC_MEASURE = 1;
	      MIOS32_IRQ_Enable();
	    } else {
	      SEQ_CC_Set(visible_track, SEQ_CC_STEPS_FORWARD, 5-1);
	      SEQ_CC_Set(visible_track, SEQ_CC_STEPS_JMPBCK, 3);
	    }
	    
	    return 1; // MIDI event has been taken
	  } break;

	  case 0x03: {
	    // tmp. for the show ;-)
	    u8 visible_track = SEQ_UI_VisibleTrackGet();
	    if( SEQ_CC_Get(visible_track, SEQ_CC_ECHO_REPEATS) ) {
	      SEQ_CC_Set(visible_track, SEQ_CC_ECHO_REPEATS, 0);
	      SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_NOTE, 24);
	    } else {
	      SEQ_CC_Set(visible_track, SEQ_CC_ECHO_REPEATS, 3);
	      SEQ_CC_Set(visible_track, SEQ_CC_ECHO_FB_NOTE, 24+3);
	    }
	    
	    return 1; // MIDI event has been taken
	  } break;
#endif

	  case 0xf:
	    blm_mode = BLM_MODE_GRID;
	    blm_force_update = 1;	
	    return 1; // MIDI event has been taken

	  case 0xe:
	    blm_mode = BLM_MODE_TRACKS;
	    blm_force_update = 1;	
	    return 1; // MIDI event has been taken

	  case 0xd:
	    blm_mode = BLM_MODE_PATTERNS;
	    blm_force_update = 1;	
	    return 1; // MIDI event has been taken

	  case 0xc:
	    blm_mode = BLM_MODE_KEYBOARD;
	    blm_force_update = 1;	
	    return 1; // MIDI event has been taken
	  }
	}
      }
    } else if( midi_package.chn == Chn1 && midi_package.note >= 0x60 && midi_package.note <= 0x6f ) {
      // extra row
      u8 button = midi_package.note - 0x60;

      if( !blm_shift_active ) {
	if( blm_mode == BLM_MODE_KEYBOARD ) {
	  // forward to loopback port
	  mios32_midi_package_t p;
	  p.type = NoteOn;
	  p.event = NoteOn;
	  p.chn = Chn1;
	  p.note = button + 0x3c;
	  p.velocity = midi_package.velocity;
	  SEQ_MIDI_IN_BusReceive(0xf0, p, 1);
	} else {
	  if( midi_package.velocity > 0 ) {
	    u8 visible_track = SEQ_UI_VisibleTrackGet();
	    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
	   
	    // select new step view
	    ui_selected_step_view = (button * (num_steps/16)) / 16;
	    // select step within view
	    ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);
	  }
	}
	return 1; // MIDI event has been taken
      } else {
	// no function yet
	return 1; // MIDI event has been taken
      }
    } else if( midi_package.chn == Chn16 && midi_package.note == 0x60 ) {
      // shift button
      blm_shift_active = (midi_package.velocity > 0) ? 1 : 0;
      blm_force_update = 1;	
      return 1; // MIDI event has been taken
    }

  } else if( midi_package.event == CC && midi_package.chn < 8 ) {
    // Fader Event
    // TODO: map it to customized port/channel/CC
  }

  return 0; // MIDI event has not been taken
}


/////////////////////////////////////////////////////////////////////////////
// Help function to transpose a note based settings of given track
/////////////////////////////////////////////////////////////////////////////
static u8 SEQ_BLM_BUTTON_Hlp_TransposeNote(u8 track, u8 note)
{
  int tr_note = note;
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  int inc_oct = tcc->transpose_oct;
  if( inc_oct >= 8 )
    inc_oct -= 16;

  int inc_semi = tcc->transpose_semi;
  if( inc_semi >= 8 )
    inc_semi -= 16;

  // apply transpose octave/semitones parameter
  if( inc_oct ) {
    tr_note += 12 * inc_oct;
    if( inc_oct < 0 ) {
      while( tr_note < 0 )
	tr_note += 12;
    } else {
      while( tr_note >= 128 )
	tr_note -= 12;
    }
  }

  if( inc_semi ) {
    tr_note += inc_semi;
    if( inc_semi < 0 ) {
      while( tr_note < 0 )
	tr_note += 12;
    } else {
      while( tr_note >= 128 )
	tr_note -= 12;
    }
  }

  return tr_note;
}
