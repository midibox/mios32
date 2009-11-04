// $Id$
/*
 * Track Preset File access functions
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
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

#include <dosfs.h>
#include <string.h>

#include "seq_file.h"
#include "seq_file_t.h"


#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_T_Init(u32 mode)
{
  return 0; // no error
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
// reads the track preset file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_T_Read(char *filepath, u8 track, seq_file_t_import_flags_t flags)
{
  s32 status = 0;
  FILEINFO fi;

  if( track > SEQ_CORE_NUM_TRACKS )
    return SEQ_FILE_T_ERR_TRACK;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_T] Open track preset file '%s'\n", filepath);
#endif

  if( (status=SEQ_FILE_ReadOpen(&fi, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_T] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // change to file header
  if( (status=SEQ_FILE_Seek(&fi, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_T] failed to change offset in file, status: %d\n", status);
#endif
    return SEQ_FILE_T_ERR_READ;
  }

  // read track definitions
  char line_buffer[128];
  do {
    status=SEQ_FILE_ReadLine(&fi, (u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_T] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
	} else {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_T] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "FollowSong") == 0 ) {
	    seq_core_options.FOLLOW_SONG = value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_T] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_T] ERROR no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= SEQ_FILE_ReadClose(&fi);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_T] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_T_ERR_READ;
  }

  // change tempo to given preset
  SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_T_Write_Hlp(PFILEINFO fileinfo, u8 track)
{
  s32 status = 0;
  char line_buffer[128];
  char str_buffer[128];
  int i, j;

  if( track > SEQ_CORE_NUM_TRACKS )
    return SEQ_FILE_T_ERR_TRACK;

  seq_cc_trk_t *tcc = &seq_cc_trk[track];

#define FLUSH_BUFFER if( fileinfo == NULL ) { DEBUG_MSG(line_buffer); } else { status |= SEQ_FILE_WriteBuffer(fileinfo, (u8 *)line_buffer, strlen(line_buffer)); }

  // write comments if target is a file
  if( fileinfo != NULL ) {
    sprintf(line_buffer, "# Only keyword and the first value is parsed for each line\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Value ranges comply to CCs documented under doc/mbseqv4_cc_implementation.txt\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# The remaining text is used to improve readability.\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "\n# DON'T CHANGE THE ORDER OF PARAMETERS!\n\n");
    FLUSH_BUFFER;
  }


  sprintf(line_buffer, "ParInstruments %d\n", SEQ_PAR_NumInstrumentsGet(track));
  FLUSH_BUFFER;  
  sprintf(line_buffer, "ParLayers %d\n", SEQ_PAR_NumLayersGet(track));
  FLUSH_BUFFER;  
  sprintf(line_buffer, "ParSteps %d\n", SEQ_PAR_NumStepsGet(track));
  FLUSH_BUFFER;  

  sprintf(line_buffer, "TrgInstruments %d\n", SEQ_TRG_NumInstrumentsGet(track));
  FLUSH_BUFFER;  
  sprintf(line_buffer, "TrgLayers %d\n", SEQ_TRG_NumLayersGet(track));
  FLUSH_BUFFER;  
  sprintf(line_buffer, "TrgSteps %d\n", SEQ_TRG_NumStepsGet(track));
  FLUSH_BUFFER;  

  switch( tcc->mode.playmode ) {
    case SEQ_CORE_TRKMODE_Off: sprintf(str_buffer, "off"); break;
    case SEQ_CORE_TRKMODE_Normal: sprintf(str_buffer, "Normal"); break;
    case SEQ_CORE_TRKMODE_Transpose: sprintf(str_buffer, "Transpose"); break;
    case SEQ_CORE_TRKMODE_Arpeggiator: sprintf(str_buffer, "Arpeggiator"); break;
    default: sprintf(str_buffer, "unknown"); break;
  }
  sprintf(line_buffer, "TrackMode %d (%s)\n", tcc->mode.playmode, str_buffer);
  FLUSH_BUFFER;


  // write comments if target is a file
  if( fileinfo != NULL ) {
    sprintf(line_buffer, "\n# Track will be partitioned and initialized here.\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "\n# The parameters below will be added to the default setup.\n\n");
    FLUSH_BUFFER;
  }


  // write track definitions
  sprintf(line_buffer, "Name '%s'\n", seq_core_trk[track].name);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TrackModeFlags %d (Unsorted: %s, Hold: %s, Restart: %s, Force Scale: %s, Sustain: %s)\n", 
	  tcc->mode.flags,
	  tcc->mode.UNSORTED ? "on" : "off",
	  tcc->mode.HOLD ? "on" : "off",
	  tcc->mode.RESTART ? "on" : "off",
	  tcc->mode.FORCE_SCALE ? "on" : "off",
	  tcc->mode.SUSTAIN ? "on" : "off");
  FLUSH_BUFFER;

  sprintf(line_buffer, "EventMode %d (%s)\n", tcc->event_mode, SEQ_LAYER_GetEvntModeName(tcc->event_mode));
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_Port 0x%02x (%s%c)\n",
	  tcc->midi_port,
	  SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(tcc->midi_port)),
	  SEQ_MIDI_PORT_OutCheckAvailable(tcc->midi_port) ? ' ' : '*');
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_Channel %d (#%d)\n", tcc->midi_chn, (int)tcc->midi_chn + 1);
  FLUSH_BUFFER;

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
  sprintf(line_buffer, "DirectionMode %d (%s)\n", tcc->dir_mode, str_buffer);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsForward %d (%d Steps)\n", tcc->steps_forward, (int)tcc->steps_forward + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsJumpBack %d (%d Steps)\n", tcc->steps_jump_back, tcc->steps_jump_back);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsReplay %d\n", tcc->steps_replay);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsRepeat %d (%d times)\n", tcc->steps_repeat, tcc->steps_repeat);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsSkip %d (%d Steps)\n", tcc->steps_skip, tcc->steps_skip);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsRepeatSkipInterval %d (%d Steps)\n", tcc->steps_rs_interval, (int)tcc->steps_rs_interval + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "Clockdivider %d (%d/384 ppqn)\n", tcc->clkdiv.value, (int)tcc->clkdiv.value + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "Triplets %s\n", tcc->clkdiv.TRIPLETS ? "yes" : "no");
  FLUSH_BUFFER;

  sprintf(line_buffer, "SynchToMeasure %s\n", tcc->clkdiv.SYNCH_TO_MEASURE? "yes" : "no");
  FLUSH_BUFFER;

  sprintf(line_buffer, "Length %d (%d Steps)\n", tcc->length, (int)tcc->length + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "Loop %d (Step %d)\n", tcc->loop, (int)tcc->loop + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TransposeSemitones %d (%c%d)\n", tcc->transpose_semi, (tcc->transpose_semi < 8) ? '+' : '-', (tcc->transpose_semi < 8) ? tcc->transpose_semi : (16-tcc->transpose_semi));
  FLUSH_BUFFER;

  sprintf(line_buffer, "TransposeOctaves %d (%c%d)\n", tcc->transpose_oct, (tcc->transpose_oct < 8) ? '+' : '-', (tcc->transpose_oct < 8) ? tcc->transpose_oct : (16-tcc->transpose_oct));
  FLUSH_BUFFER;


  sprintf(line_buffer, "MorphMode %d (%s)\n", tcc->morph_mode, tcc->morph_mode ? "on" : "off");
  FLUSH_BUFFER;

  int dst_begin = tcc->morph_dst + 1;
  int dst_end = dst_begin + tcc->length;
  if( dst_end > 256 )
    dst_end = 256;
  sprintf(line_buffer, "MorphDestinationRange %d (%d..%d)\n", tcc->morph_dst, dst_begin, dst_end);
  FLUSH_BUFFER;


  sprintf(line_buffer, "HumanizeMode %d (Note: %s, Velocity: %s, Length: %s)\n", 
	  tcc->humanize_mode,
	  (tcc->humanize_mode & (1 << 0)) ? "on" : "off",
	  (tcc->humanize_mode & (1 << 1)) ? "on" : "off",
	  (tcc->humanize_mode & (1 << 2)) ? "on" : "off");
  sprintf(line_buffer, "HumanizeIntensity %d\n", tcc->humanize_value);
  FLUSH_BUFFER;


  sprintf(line_buffer, "GrooveStyle %d\n", tcc->groove_style);
  FLUSH_BUFFER;

  sprintf(line_buffer, "GrooveIntensity %d\n", tcc->groove_value);
  FLUSH_BUFFER;


  const char trg_asg_str[16] = "-ABCDEFGH???????";
  sprintf(line_buffer, "TriggerAsngGate %d (%c)\n", tcc->trg_assignments.gate, trg_asg_str[tcc->trg_assignments.gate]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsngAccent %d (%c)\n", tcc->trg_assignments.accent, trg_asg_str[tcc->trg_assignments.accent]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsngRoll %d (%c)\n", tcc->trg_assignments.roll, trg_asg_str[tcc->trg_assignments.roll]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsngGlide %d (%c)\n", tcc->trg_assignments.glide, trg_asg_str[tcc->trg_assignments.glide]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsgnSkip %d (%c)\n", tcc->trg_assignments.skip, trg_asg_str[tcc->trg_assignments.skip]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsgnRandomGate %d (%c)\n", tcc->trg_assignments.random_gate, trg_asg_str[tcc->trg_assignments.random_gate]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsgnRandomValue %d (%c)\n", tcc->trg_assignments.random_value, trg_asg_str[tcc->trg_assignments.random_value]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "TriggerAsgnNoFx %d (%c)\n", tcc->trg_assignments.no_fx, trg_asg_str[tcc->trg_assignments.no_fx]);
  FLUSH_BUFFER;

  
  sprintf(line_buffer, "DrumParAsgnA %d (%s)\n", tcc->par_assignment_drum[0], SEQ_PAR_TypeStr(tcc->par_assignment_drum[0]));
  FLUSH_BUFFER;

  sprintf(line_buffer, "DrumParAsgnB %d (%s)\n", tcc->par_assignment_drum[1], SEQ_PAR_TypeStr(tcc->par_assignment_drum[1]));
  FLUSH_BUFFER;


  sprintf(line_buffer, "EchoRepeats %d\n", tcc->echo_repeats);
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoDelay %d (%s)\n", tcc->echo_delay, SEQ_CORE_Echo_GetDelayModeName(tcc->echo_delay));
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoVelocity %d\n", tcc->echo_velocity);
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoFeedbackVelocity %d (%d%%)\n", tcc->echo_fb_velocity, (int)tcc->echo_fb_velocity * 5);
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoFeedbackNote %d (%c%d)\n", tcc->echo_fb_note, 
	  (tcc->echo_fb_note < 24) ? '-' : '+', 
	  (tcc->echo_fb_note < 24) ? (24-tcc->echo_fb_note) : (tcc->echo_fb_note - 24));
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoFeedbackGatelength %d (%d%%)\n", tcc->echo_fb_gatelength, (int)tcc->echo_fb_gatelength * 5);
  FLUSH_BUFFER;

  sprintf(line_buffer, "EchoFeedbackTicks %d (%d%%)\n", tcc->echo_fb_ticks, (int)tcc->echo_fb_ticks * 5);
  FLUSH_BUFFER;


  if( tcc->lfo_waveform <= 3 ) {
    const char waveform_str[4][6] = {
      " off ",
      "Sine ",
      "Tri. ",
      "Saw. "
    };
    sprintf(line_buffer, "LFO_Waveform %d (%s)\n", tcc->lfo_waveform, (char *)waveform_str[tcc->lfo_waveform]);
  } else {
    sprintf(line_buffer, "LFO_Waveform %d (R%02d)\n", tcc->lfo_waveform, (tcc->lfo_waveform-4+1)*5);
  }
  FLUSH_BUFFER;

  sprintf(line_buffer, "LFO_Amplitude %d (%d)\n", tcc->lfo_amplitude, (int)tcc->lfo_amplitude - 128);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_Phase %d (%d%%)\n", tcc->lfo_phase, tcc->lfo_phase);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_Interval %d (%d Steps)\n", tcc->lfo_steps, (int)tcc->lfo_steps + 1);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_Reset_Interval %d (%d Steps)\n", tcc->lfo_steps_rst, tcc->lfo_steps_rst + 1);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_Flags %d (Oneshot: %s, Note: %s, Velocity: %s, Length: %s, CC: %s)\n", 
	  tcc->lfo_enable_flags.ALL,
	  tcc->lfo_enable_flags.ONE_SHOT ? "on" : "off",
	  tcc->lfo_enable_flags.NOTE ? "on" : "off",
	  tcc->lfo_enable_flags.VELOCITY ? "on" : "off",
	  tcc->lfo_enable_flags.LENGTH ? "on" : "off",
	  tcc->lfo_enable_flags.CC ? "on" : "off");
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_ExtraCC %d\n", tcc->lfo_cc);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_ExtraCC_Offset %d\n", tcc->lfo_cc_offset);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "LFO_ExtraCC_PPQN %d\n", tcc->lfo_cc_ppqn ? (3 << (tcc->lfo_cc_ppqn-1)) : 1);
  FLUSH_BUFFER;
	  

  sprintf(line_buffer, "NoteLimitLower %d\n", tcc->limit_lower);
  FLUSH_BUFFER;
	  
  sprintf(line_buffer, "NoteLimitUpper %d\n", tcc->limit_upper);
  FLUSH_BUFFER;
	  

  for(i=0; i<3; ++i) {
    switch( i ) {
      case 0:
	sprintf(line_buffer, (tcc->event_mode == SEQ_EVENT_MODE_Drum) 
		? "\n# MIDI Notes for Drum Instruments:\n"
		: "\n# Parameter Layer Assignments:\n");
	FLUSH_BUFFER;
	sprintf(line_buffer, "ConstArrayA");
	break;

      case 1:
	sprintf(line_buffer, (tcc->event_mode == SEQ_EVENT_MODE_Drum) 
		? "\n# MIDI Velocity:\n"
		: "\n# CC Assignments:\n");
	FLUSH_BUFFER;
	sprintf(line_buffer, "ConstArrayB");
	break;

      case 2:
	sprintf(line_buffer, (tcc->event_mode == SEQ_EVENT_MODE_Drum) 
		? "\n# MIDI Accent Velocity:\n"
		: "\n# Constant Array C:\n");
	FLUSH_BUFFER;
	sprintf(line_buffer, "ConstArrayC");
	break;
	
      default:
	sprintf(line_buffer, "\n# UNSUPPORTED");
    }

    for(j=0; j<16; ++j) {
      sprintf(str_buffer, " 0x%02x", tcc->lay_const[i*16 + j]);
      strcat(line_buffer, str_buffer);
    }
    strcat(line_buffer, "\n");
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "\n# Parameter Layers:\n");
  FLUSH_BUFFER;  

  for(i=0; i<SEQ_PAR_MAX_BYTES; i+=16) {
    sprintf(line_buffer, "Par 0x%03x  ", i);
    for(j=0; j<16; ++j) {
      sprintf(str_buffer, " 0x%02x", seq_par_layer_value[track][i+j]);
      strcat(line_buffer, str_buffer);
    }
    strcat(line_buffer, "\n");
    FLUSH_BUFFER;
  }


  sprintf(line_buffer, "\n# Trigger Layers:\n");
  FLUSH_BUFFER;  

  for(i=0; i<SEQ_TRG_MAX_BYTES; i+=16) {
    sprintf(line_buffer, "Trg 0x%03x  ", i);
    for(j=0; j<16; ++j) {
      sprintf(str_buffer, " 0x%02x", seq_trg_layer_value[track][i+j]);
      strcat(line_buffer, str_buffer);
    }
    strcat(line_buffer, "\n");
    FLUSH_BUFFER;
  }


  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into track preset file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_T_Write(char *filepath, u8 track)
{
  FILEINFO fi;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_T] Open track preset file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(&fi, filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_T] Failed to open/create track preset file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(&fi); // important to free memory given by malloc
    return status;
  }

  // write file
  status |= SEQ_FILE_T_Write_Hlp(&fi, track);

  // close file
  status |= SEQ_FILE_WriteClose(&fi);


#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_T] track preset file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_T_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends track preset file content to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_T_Debug(u8 track)
{
  return SEQ_FILE_T_Write_Hlp(NULL, track); // send to debug terminal
}
