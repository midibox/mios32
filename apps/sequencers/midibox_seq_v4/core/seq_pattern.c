// $Id$
/*
 * Pattern Routines
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

#include <seq_bpm.h>

#include "tasks.h"

#include "seq_pattern.h"
#include "seq_cc.h"
#include "seq_core.h"
#include "seq_ui.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_statistics.h"
#include "seq_song.h"
#include "seq_mixer.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// set this to 1 if performance of pattern handler should be measured with a scope
// (LED toggling in APP_Background() has to be disabled!)
#define LED_PERFORMANCE_MEASURING 0


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_pattern_t seq_pattern[SEQ_CORE_NUM_GROUPS];
seq_pattern_t seq_pattern_req[SEQ_CORE_NUM_GROUPS];
char seq_pattern_name[SEQ_CORE_NUM_GROUPS][21];

mios32_sys_time_t seq_pattern_start_time;
u8 seq_pattern_log_load_time; // can be changed in terminal with "set seq_pattern_log_load_time on"
u8 seq_pattern_mixer_num;
u16 seq_pattern_remix_map;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Init(u32 mode)
{
  seq_pattern_start_time.seconds = 0;
  seq_pattern_mixer_num = 0;
  seq_pattern_remix_map = 0;
  seq_pattern_log_load_time = 0;
	
  // pre-init pattern numbers
  u8 group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    seq_pattern[group].ALL = 0;
#if 0
    seq_pattern[group].group = 2*group; // A/C/E/G
#else
    seq_pattern[group].bank = group; // each group has it's own bank
#endif
    seq_pattern_req[group].ALL = 0;

#if 0
    sprintf((char *)seq_pattern_name[group], "Pattern %c%d          ", ('A'+((pattern>>3)&7)), (pattern&7)+1);
#else
    // if pattern name only contains spaces, the UI will print 
    // the pattern number instead of an empty message
    // this ensures highest flexibility (e.g. any pattern can be copied to another slot w/o name inconsistencies)
    // -> see SEQ_LCD_PrintPatternName()
    int i;
    for(i=0; i<20; ++i)
      seq_pattern_name[group][i] = ' ';
    seq_pattern_name[group][20] = 0;
#endif

  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of a pattern (20 characters)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_PATTERN_NameGet(u8 group)
{
  if( group >= SEQ_CORE_NUM_GROUPS )
    return "<invalid group>     ";
  return seq_pattern_name[group];
}


/////////////////////////////////////////////////////////////////////////////
// Requests a pattern change
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Change(u8 group, seq_pattern_t pattern, u8 force_immediate_change)
{
  if( group >= SEQ_CORE_NUM_GROUPS )
    return -1; // invalid group

  // change immediately if sequencer not running
  if( force_immediate_change || !SEQ_BPM_IsRunning() || SEQ_BPM_TickGet() == 0 || ui_seq_pause ) {
    // store requested pattern
    portENTER_CRITICAL();
    pattern.REQ = 0; // request not required - we load the pattern immediately
    seq_pattern_req[group] = pattern;
    portEXIT_CRITICAL();

#if LED_PERFORMANCE_MEASURING
    MIOS32_BOARD_LED_Set(0x00000001, 1);
#endif
    SEQ_PATTERN_Load(group, pattern);
#if LED_PERFORMANCE_MEASURING
    MIOS32_BOARD_LED_Set(0x00000001, 0);
#endif
  } else {

    // TODO: stall here if previous pattern change hasn't been finished yet!

    // in song mode it has to be considered, that this function is called multiple times
    // to request pattern changes for all groups

    // else request change
    portENTER_CRITICAL();
    pattern.REQ = 1;
    seq_pattern_req[group] = pattern;
    portEXIT_CRITICAL();

    if( seq_core_options.SYNCHED_PATTERN_CHANGE && !SEQ_SONG_ActiveGet() ) {
      // done in SEQ_CORE_Tick() when last step reached
    } else {
      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] Req G%d %c%d", SEQ_BPM_TickGet(), group+1, 'A'+pattern.group, pattern.num+1);
      }
      // pregenerate bpm ticks
      // (won't be generated again if there is already an ongoing request)
      MUTEX_MIDIOUT_TAKE;
      s32 delay_ticks = SEQ_CORE_AddForwardDelay(seq_core_pattern_switch_margin_ms);
      if( delay_ticks >= 0 ) {
      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] Forward Delay %d ticks based on %d mS margin", SEQ_BPM_TickGet(), delay_ticks, seq_core_pattern_switch_margin_ms);
      }
      }
      MUTEX_MIDIOUT_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from a separate task to handle pattern
// change requests
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Handler(void)
{
  u8 group;
  u8 any_pattern_loaded = 0;

#if LED_PERFORMANCE_MEASURING
  MIOS32_BOARD_LED_Set(0x00000001, 1);
#endif

  MUTEX_SDCARD_TAKE; // take SD Card Mutex before entering critical section, because within the section we won't get it anymore -> hangup
  portENTER_CRITICAL();

  if( seq_pattern_log_load_time ) {
    MIOS32_STOPWATCH_Reset(); // note: conflicts with SEQ_STATISTICS_Stopwatch, but can be accepted if executed in critical section
  }

  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    if( seq_pattern_req[group].REQ ) {
      seq_pattern_req[group].REQ = 0;

      if( seq_core_options.PATTERN_MIXER_MAP_COUPLING ) {
	u8 mixer_num = 0;
	u8 track;
				
	if (seq_pattern_req[0].lower) {
	  mixer_num = ((seq_pattern_req[0].group) * 8) + seq_pattern_req[0].num;
	} else {
	  mixer_num = (((seq_pattern_req[0].group) + 8) * 8) + seq_pattern_req[0].num;
	}
				
	// setup our requested pattern mixer map
	SEQ_MIXER_NumSet(mixer_num);
	SEQ_MIXER_Load(mixer_num);
			
	// dump mixer for tracks
	for(track = group * 4; track<((group+1)*4); ++track) {
					
	  // if we got the track bit setup inside our remix_map, them do not send mixer data for that track channel, let it be mixed down
	  if ( ((1 << track) | seq_pattern_remix_map) == seq_pattern_remix_map ) {
	    // do nothing for now...
	  } else {
	    SEQ_MIXER_SendAllByChannel(track);
	  }
	}
      }

      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] Load begin G%d %c%d", SEQ_BPM_TickGet(), group+1, 'A'+seq_pattern_req[group].group, seq_pattern_req[group].num+1);
      }
      SEQ_PATTERN_Load(group, seq_pattern_req[group]);
      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] Load end G%d %c%d", SEQ_BPM_TickGet(), group+1, 'A'+seq_pattern_req[group].group, seq_pattern_req[group].num+1);
      }
      any_pattern_loaded = 1;

      // restart *all* patterns?
      if( seq_core_options.RATOPC ) {
	MIOS32_IRQ_Disable(); // must be atomic
	seq_core_state.reset_trkpos_req |= (0xf << (4*group));
	MIOS32_IRQ_Enable();
      }
    }
  }
  u32 stopwatch_delta = MIOS32_STOPWATCH_ValueGet();
  portEXIT_CRITICAL();
  MUTEX_SDCARD_GIVE;
  
#if LED_PERFORMANCE_MEASURING
  MIOS32_BOARD_LED_Set(0x00000001, 0);
#endif

  if( any_pattern_loaded ) {
    if( stopwatch_delta == 0xffffffff ) {
      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] All patterns loaded in more than 65 mS!", SEQ_BPM_TickGet());
      }
    } else {
      if( seq_pattern_log_load_time ) {
	DEBUG_MSG("[SEQ_PATTERN:%d] All patterns loaded within %d.%03d mS", SEQ_BPM_TickGet(), stopwatch_delta / 1000, stopwatch_delta % 1000);
      }
    }
  }
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Load a pattern from SD Card
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Load(u8 group, seq_pattern_t pattern)
{
  s32 status;

  seq_pattern[group] = pattern;

  MUTEX_SDCARD_TAKE;

  if( (status=SEQ_FILE_B_PatternRead(pattern.bank, pattern.pattern, group, seq_pattern_remix_map)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
	
  seq_pattern_start_time = MIOS32_SYS_TimeGet();

  MUTEX_SDCARD_GIVE;

  // cancel sustain if there are no notes played by the track anymore
  {
    int i;
    u8 track = group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
    for(i=0; i<SEQ_CORE_NUM_TRACKS_PER_GROUP; ++i, ++track)
      SEQ_CORE_CancelSustainedNotes(track);
  }

  // optionally unmute loaded tracks
  if( seq_core_options.UNMUTE_ON_PATTERN_CHANGE ) {
    u16 pattern = 0xf << (4*group);
    portENTER_CRITICAL();
    seq_core_trk_muted &= ~pattern;
    seq_core_trk_synched_mute &= ~pattern;
    seq_core_trk_synched_unmute &= ~pattern;
    portEXIT_CRITICAL();
  }

  // reset latched PB/CC values (because assignments could change)
  if( !seq_core_options.PATTERN_CHANGE_DONT_RESET_LATCHED_PC ) {
    SEQ_LAYER_ResetLatchedValues();
  }

  // send program change & bank selects
  {
    MUTEX_MIDIOUT_TAKE;
    int i;
    u8 track = group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
    for(i=0; i<SEQ_CORE_NUM_TRACKS_PER_GROUP; ++i, ++track)
      SEQ_LAYER_SendPCBankValues(track, 0, 1);
    MUTEX_MIDIOUT_GIVE;
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// Stores a pattern into SD Card
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Save(u8 group, seq_pattern_t pattern)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_B_PatternWrite(seq_file_session_name, pattern.bank, pattern.pattern, group, 1);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Returns pattern name of a bank w/o overwriting RAM
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_PeekName(seq_pattern_t pattern, char *pattern_name)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_B_PatternPeekName(pattern.bank, pattern.pattern, 0, pattern_name); // always cached!
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Fixes a pattern (load/modify/store)
// Can be used on format changes
// Uses group as temporal "storage"
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Fix(u8 group, seq_pattern_t pattern)
{
  s32 status;

  MUTEX_SDCARD_TAKE;

  MIOS32_MIDI_SendDebugMessage("Loading bank #%d pattern %d\n", pattern.bank+1, pattern.pattern+1);
  if( (status=SEQ_FILE_B_PatternRead(pattern.bank, pattern.pattern, group, 0)) < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    MIOS32_MIDI_SendDebugMessage("Read failed with status: %d\n", status);
  } else {
    // insert modification here
    int track_i;
    int track = group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
    for(track_i=0; track_i<SEQ_CORE_NUM_TRACKS_PER_GROUP; ++track_i, ++track) {
      // Usage example (disabled as it isn't required anymore)
      // seq_cc_trk[track].clkdiv.value = 15; // due to changed resultion

#if 0
      seq_cc_trk[track].lfo_waveform = 0;
      seq_cc_trk[track].lfo_amplitude = 128 + 64;
      seq_cc_trk[track].lfo_phase = 0;
      seq_cc_trk[track].lfo_steps = 15;
      seq_cc_trk[track].lfo_steps_rst = 15;
      seq_cc_trk[track].lfo_enable_flags.ALL = 0;
      seq_cc_trk[track].lfo_cc = 0;
      seq_cc_trk[track].lfo_cc_offset = 64;
      seq_cc_trk[track].lfo_cc_ppqn = 6; // 96 ppqn
#endif
    }

    MIOS32_MIDI_SendDebugMessage("Saving bank #%d pattern %d\n", pattern.bank+1, pattern.pattern+1);
    if( (status=SEQ_FILE_B_PatternWrite(seq_file_session_name, pattern.bank, pattern.pattern, group, 1)) < 0 ) {
      SEQ_UI_SDCardErrMsg(2000, status);
      MIOS32_MIDI_SendDebugMessage("Write failed with status: %d\n", status);
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Fixes all patterns of all banks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_FixAll(void)
{
  s32 status = 0;

  int bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    int pattern_i;
    for(pattern_i=0; pattern_i<SEQ_FILE_B_NumPatterns(bank); ++pattern_i) {
      seq_pattern_t pattern;
      pattern.bank = bank;
      pattern.pattern = pattern_i;
      if( (status=SEQ_PATTERN_Fix(0, pattern)) < 0 )
	return status; // break process
    }
  }

  return status;
}
