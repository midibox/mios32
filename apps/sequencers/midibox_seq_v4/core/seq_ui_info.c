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


#define NUM_LIST_ITEMS 10
#define LIST_ITEM_SYSTEM       0
#define LIST_ITEM_GLOBALS      1
#define LIST_ITEM_TRACKS       2
#define LIST_ITEM_TRACK_INFO   3
#define LIST_ITEM_PAR_LAYERS   4
#define LIST_ITEM_TRG_LAYERS   5
#define LIST_ITEM_MIXER_MAP    6
#define LIST_ITEM_SONG         7
#define LIST_ITEM_GROOVES      8
#define LIST_ITEM_SD_CARD      9


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
  "ParLayers"
  "TrgLayers"
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
  if( SEQ_UI_SelectListItem(incrementer, NUM_LIST_ITEMS, NUM_OF_ITEMS, &ui_selected_item, &list_view_offset) )
    SEQ_UI_INFO_UpdateList();

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
  char str_buffer[128];

  if( depressed ) return 0; // ignore when button depressed

  if( button <= SEQ_UI_BUTTON_GP8 || button == SEQ_UI_BUTTON_Select ) {
    if( button != SEQ_UI_BUTTON_Select )
      ui_selected_item = button / 2;

    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 10000, "Sending Informations", "to MIOS Terminal!");

    switch( ui_selected_item + list_view_offset ) {

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_SYSTEM: {
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
	seq_cc_trk_t *tcc = &seq_cc_trk[track];

	DEBUG_MSG("\n");
	DEBUG_MSG("Track Parameters of G%dT%d", (track/4)+1, (track%4)+1);
	DEBUG_MSG("========================\n");

	DEBUG_MSG("Name: '%s'\n", seq_core_trk[track].name);

	switch( tcc->mode.playmode ) {
	  case SEQ_CORE_TRKMODE_Off: sprintf(str_buffer, "off"); break;
	  case SEQ_CORE_TRKMODE_Normal: sprintf(str_buffer, "Normal"); break;
	  case SEQ_CORE_TRKMODE_Transpose: sprintf(str_buffer, "Transpose"); break;
	  case SEQ_CORE_TRKMODE_Arpeggiator: sprintf(str_buffer, "Arpeggiator"); break;
	  default: sprintf(str_buffer, "unknown"); break;
	}
	DEBUG_MSG("Track Mode: %d (%s)\n", tcc->mode.playmode, str_buffer);
	DEBUG_MSG("Track Mode Flags: %d (Unsorted: %s, Hold: %s, Restart: %s, Force Scale: %s, Sustain: %s)\n", 
		  tcc->mode.flags,
		  tcc->mode.UNSORTED ? "on" : "off",
		  tcc->mode.HOLD ? "on" : "off",
		  tcc->mode.RESTART ? "on" : "off",
		  tcc->mode.FORCE_SCALE ? "on" : "off",
		  tcc->mode.SUSTAIN ? "on" : "off");

	DEBUG_MSG("Event Mode: %d (%s)\n", tcc->event_mode, SEQ_LAYER_GetEvntModeName(tcc->event_mode));
	DEBUG_MSG("MIDI Port: 0x%02x (%s%c)\n",
		  tcc->midi_port,
		  SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(tcc->midi_port)),
		  SEQ_MIDI_PORT_OutCheckAvailable(tcc->midi_port) ? ' ' : '*');
	DEBUG_MSG("MIDI Channel: %d (#%d)\n", tcc->midi_chn, (int)tcc->midi_chn + 1);

	switch( tcc->dir_mode ) {
  	  case SEQ_CORE_TRKDIR_Forward: sprintf(str_buffer, "Forward"); break;
  	  case SEQ_CORE_TRKDIR_Backward: sprintf(str_buffer, "Backward"); break;
	  case SEQ_CORE_TRKDIR_PingPong: sprintf(str_buffer, "Ping Pong"); break;
	  case SEQ_CORE_TRKDIR_Pendulum: sprintf(str_buffer, "Pendulum"); break;
	  case SEQ_CORE_TRKDIR_Random_Dir: sprintf(str_buffer, "Random Dir"); break;
	  case SEQ_CORE_TRKDIR_Random_Step: sprintf(str_buffer, "Random Step"); break;
	  case SEQ_CORE_TRKDIR_Random_D_S: sprintf(str_buffer, "Random Dir/Step"); break;
	  default: sprintf(str_buffer, "unknown"); break;
	}
	DEBUG_MSG("Direction Mode: %d (%s)\n", tcc->dir_mode, str_buffer);

	DEBUG_MSG("Steps Forward: %d (%d Steps)\n", tcc->steps_forward, (int)tcc->steps_forward + 1);
	DEBUG_MSG("Steps Jump Back: %d (%d Steps)\n", tcc->steps_jump_back, tcc->steps_jump_back);
	DEBUG_MSG("Steps Replay: %d\n", tcc->steps_replay);
	DEBUG_MSG("Steps Repeat: %d (%d times)\n", tcc->steps_repeat, tcc->steps_repeat);
	DEBUG_MSG("Steps Skip: %d (%d Steps)\n", tcc->steps_skip, tcc->steps_skip);
	DEBUG_MSG("Steps Repeat/Skip Interval: %d (%d Steps)\n", tcc->steps_rs_interval, (int)tcc->steps_rs_interval + 1);
	DEBUG_MSG("Clockdivider: %d (%d/384 ppqn)\n", tcc->clkdiv.value, (int)tcc->clkdiv.value + 1);
	DEBUG_MSG("Triplets: %s\n", tcc->clkdiv.TRIPLETS ? "yes" : "no");
	DEBUG_MSG("Synch-To-Measure: %s\n", tcc->clkdiv.SYNCH_TO_MEASURE? "yes" : "no");
	DEBUG_MSG("Length: %d (%d Steps)\n", tcc->length, (int)tcc->length + 1);
	DEBUG_MSG("Loop: %d (Step %d)\n", tcc->loop, (int)tcc->loop + 1);
	DEBUG_MSG("Transpose Semitones: %d (%c%d)\n", tcc->transpose_semi, (tcc->transpose_semi < 8) ? '+' : '-', (tcc->transpose_semi < 8) ? tcc->transpose_semi : (16-tcc->transpose_semi));
	DEBUG_MSG("Transpose Octaves: %d (%c%d)\n", tcc->transpose_oct, (tcc->transpose_oct < 8) ? '+' : '-', (tcc->transpose_oct < 8) ? tcc->transpose_oct : (16-tcc->transpose_oct));

	DEBUG_MSG("Morph Mode: %d (%s)\n", tcc->morph_mode, tcc->morph_mode ? "on" : "off");
	int dst_begin = tcc->morph_dst + 1;
	int dst_end = dst_begin + tcc->length;
	if( dst_end > 256 )
	  dst_end = 256;
	DEBUG_MSG("Morph Destination Range: %d (%d..%d)\n", tcc->morph_dst, dst_begin, dst_end);

	DEBUG_MSG("Humanize Mode: %d (Note: %s, Velocity: %s, Length: %s)\n", 
		  tcc->humanize_mode,
		  (tcc->humanize_mode & (1 << 0)) ? "on" : "off",
		  (tcc->humanize_mode & (1 << 1)) ? "on" : "off",
		  (tcc->humanize_mode & (1 << 2)) ? "on" : "off");
	DEBUG_MSG("Humanize Intensity: %d\n", tcc->humanize_value);

	DEBUG_MSG("Groove Style: %d\n", tcc->groove_style);
	DEBUG_MSG("Groove Intensity: %d\n", tcc->groove_value);

	const char trg_asg_str[16] = "-ABCDEFGH???????";
	DEBUG_MSG("Trigger Assignment Gate: %d (%c)\n", tcc->trg_assignments.gate, trg_asg_str[tcc->trg_assignments.gate]);
	DEBUG_MSG("Trigger Assignment Accent: %d (%c)\n", tcc->trg_assignments.accent, trg_asg_str[tcc->trg_assignments.accent]);
	DEBUG_MSG("Trigger Assignment Roll: %d (%c)\n", tcc->trg_assignments.roll, trg_asg_str[tcc->trg_assignments.roll]);
	DEBUG_MSG("Trigger Assignment Glide: %d (%c)\n", tcc->trg_assignments.glide, trg_asg_str[tcc->trg_assignments.glide]);
	DEBUG_MSG("Trigger Assignment Skip: %d (%c)\n", tcc->trg_assignments.skip, trg_asg_str[tcc->trg_assignments.skip]);
	DEBUG_MSG("Trigger Assignment Random Gate: %d (%c)\n", tcc->trg_assignments.random_gate, trg_asg_str[tcc->trg_assignments.random_gate]);
	DEBUG_MSG("Trigger Assignment Random Value: %d (%c)\n", tcc->trg_assignments.random_value, trg_asg_str[tcc->trg_assignments.random_value]);
	DEBUG_MSG("Trigger Assignment No Fx: %d (%c)\n", tcc->trg_assignments.no_fx, trg_asg_str[tcc->trg_assignments.no_fx]);

	DEBUG_MSG("Drum Parameter Assignment A: %d (%s)\n", tcc->par_assignment_drum[0], SEQ_PAR_TypeStr(tcc->par_assignment_drum[0]));
	DEBUG_MSG("Drum Parameter Assignment B: %d (%s)\n", tcc->par_assignment_drum[1], SEQ_PAR_TypeStr(tcc->par_assignment_drum[1]));

	DEBUG_MSG("Echo Repeats: %d\n", tcc->echo_repeats);
	DEBUG_MSG("Echo Delay: %d (%s)\n", tcc->echo_delay, SEQ_CORE_Echo_GetDelayModeName(tcc->echo_delay));
	DEBUG_MSG("Echo Velocity: %d\n", tcc->echo_velocity);
	DEBUG_MSG("Echo Feedback Velocity: %d (%d%%)\n", tcc->echo_fb_velocity, (int)tcc->echo_fb_velocity * 5);
	DEBUG_MSG("Echo Feedback Note: %d (%c%d)\n", tcc->echo_fb_note, 
		  (tcc->echo_fb_note < 24) ? '-' : '+', 
		  (tcc->echo_fb_note < 24) ? (24-tcc->echo_fb_note) : (tcc->echo_fb_note - 24));
	DEBUG_MSG("Echo Feedback Gatelength: %d (%d%%)\n", tcc->echo_fb_gatelength, (int)tcc->echo_fb_gatelength * 5);
	DEBUG_MSG("Echo Feedback Ticks: %d (%d%%)\n", tcc->echo_fb_ticks, (int)tcc->echo_fb_ticks * 5);

	if( tcc->lfo_waveform <= 3 ) {
	  const char waveform_str[4][6] = {
	    " off ",
	    "Sine ",
	    "Tri. ",
	    "Saw. "
	  };
	  
	  DEBUG_MSG("LFO Waveform: %d (%s)\n", tcc->lfo_waveform, (char *)waveform_str[tcc->lfo_waveform]);
	} else {
	  DEBUG_MSG("LFO Waveform: %d (R%02d)\n", tcc->lfo_waveform, (tcc->lfo_waveform-4+1)*5);
	}

	DEBUG_MSG("LFO Amplitude: %d (%d)\n", tcc->lfo_amplitude, (int)tcc->lfo_amplitude - 128);
	DEBUG_MSG("LFO Phase: %d (%d%%)\n", tcc->lfo_phase, tcc->lfo_phase);
	DEBUG_MSG("LFO Interval: %d (%d Steps)\n", tcc->lfo_steps, (int)tcc->lfo_steps + 1);
	DEBUG_MSG("LFO Reset Interval: %d (%d Steps)\n", tcc->lfo_steps_rst, tcc->lfo_steps_rst + 1);
	DEBUG_MSG("LFO Flags: %d (Oneshot: %s, Note: %s, Velocity: %s, Length: %s, CC: %s)\n", 
		  tcc->lfo_enable_flags.ALL,
		  tcc->lfo_enable_flags.ONE_SHOT ? "on" : "off",
		  tcc->lfo_enable_flags.NOTE ? "on" : "off",
		  tcc->lfo_enable_flags.VELOCITY ? "on" : "off",
		  tcc->lfo_enable_flags.LENGTH ? "on" : "off",
		  tcc->lfo_enable_flags.CC ? "on" : "off");
	DEBUG_MSG("LFO Extra CC: %d\n", tcc->lfo_cc);
	DEBUG_MSG("LFO Extra CC Offset: %d\n", tcc->lfo_cc_offset);
	DEBUG_MSG("LFO Extra CC PPQN: %d\n", tcc->lfo_cc_ppqn ? (3 << (tcc->lfo_cc_ppqn-1)) : 1);

	DEBUG_MSG("Note Limit Lower: %d\n", tcc->limit_lower);
	DEBUG_MSG("Note Limit Upper: %d\n", tcc->limit_upper);

	DEBUG_MSG("Constant Array A (MIDI Notes for Drum Tracks):\n");
	MIOS32_MIDI_SendDebugHexDump((u8 *)&tcc->lay_const[0*16], 16);
	DEBUG_MSG("Constant Array B (MIDI Velocity for Drum Tracks):\n");
	MIOS32_MIDI_SendDebugHexDump((u8 *)&tcc->lay_const[1*16], 16);
	DEBUG_MSG("Constant Array C (MIDI Accent Velocity for Drum Tracks):\n");
	MIOS32_MIDI_SendDebugHexDump((u8 *)&tcc->lay_const[2*16], 16);

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;


      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_PAR_LAYERS: {
	MUTEX_MIDIOUT_TAKE;
	u8 track = SEQ_UI_VisibleTrackGet();
	u16 num_instruments = SEQ_PAR_NumInstrumentsGet(track);
	u16 num_par_layers = SEQ_PAR_NumLayersGet(track);
	u16 num_par_steps = SEQ_PAR_NumStepsGet(track);

	DEBUG_MSG("\n");
	DEBUG_MSG("Parameter Layers of G%dT%d", (track/4)+1, (track%4)+1);
	DEBUG_MSG("========================\n");

	int instrument;
	int par_layer;
	for(instrument=0; instrument<num_instruments; ++instrument) {
	  for(par_layer=0; par_layer<num_par_layers; ++par_layer) {
	    DEBUG_MSG("Instrument %d Layer %c\n", instrument+1, 'A'+par_layer);
	    u16 step_ix = (instrument * num_par_layers * num_par_steps) + (par_layer * num_par_steps);
	    MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_par_layer_value[track][step_ix], num_par_steps);
	  }
	}

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_TRG_LAYERS: {
	MUTEX_MIDIOUT_TAKE;
	u8 track = SEQ_UI_VisibleTrackGet();
	u16 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
	u16 num_trg_layers = SEQ_TRG_NumLayersGet(track);
	u16 num_trg_steps = SEQ_TRG_NumStepsGet(track);

	DEBUG_MSG("\n");
	DEBUG_MSG("Trigger Layers of G%dT%d", (track/4)+1, (track%4)+1);
	DEBUG_MSG("======================\n");

	int instrument;
	int trg_layer;
	for(instrument=0; instrument<num_instruments; ++instrument) {
	  for(trg_layer=0; trg_layer<num_trg_layers; ++trg_layer) {
	    DEBUG_MSG("Instrument %d Layer %c\n", instrument+1, 'A'+trg_layer);
	    u16 step_ix = (instrument * num_trg_layers * (num_trg_steps/8)) + (trg_layer * (num_trg_steps/8));
	    MIOS32_MIDI_SendDebugHexDump((u8 *)&seq_trg_layer_value[track][step_ix], num_trg_steps / 8);
	  }
	}

	DEBUG_MSG("done.\n");
	MUTEX_MIDIOUT_GIVE;
      } break;

      //////////////////////////////////////////////////////////////////////////////////////////////
      case LIST_ITEM_MIXER_MAP: {
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
	  for(par=1; par<16; ++par)
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
  // About this MIDIbox:             00:00:00MIDIbox SEQ V4.0Beta5      CPU Load: 40%
  //   System   Globals    Tracks  TrackInfo>MIDI Scheduler: Alloc   0/  0 Drops:   0


  // Select MIDI Device with GP Button:      10 Devices found under /sysex           
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx>



  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("About this MIDIbox:             ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, NUM_OF_ITEMS);

  if( list_view_offset == 0 && ui_selected_item == 0 )
    SEQ_LCD_PrintChar(0x01); // right arrow
  else if( (list_view_offset+ui_selected_item+1) >= NUM_LIST_ITEMS )
    SEQ_LCD_PrintChar(0x00); // left arrow
  else
    SEQ_LCD_PrintChar(0x02); // left/right arrow

  ///////////////////////////////////////////////////////////////////////////
  switch( ui_selected_item + list_view_offset ) {
    case LIST_ITEM_SYSTEM:
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
      SEQ_LCD_PrintSpaces(10);
      SEQ_LCD_CursorSet(40 + 27, 0);
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

  for(item=0; item<NUM_OF_ITEMS && item<NUM_LIST_ITEMS; ++item) {
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

  while( item < NUM_OF_ITEMS ) {
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


