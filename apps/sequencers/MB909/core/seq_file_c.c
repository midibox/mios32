// $Id: seq_file_c.c 2228 2015-10-27 22:00:30Z tk $
/*
 * Config File access functions
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
#include "tasks.h"

#include <string.h>

#include "file.h"
#include "seq_file.h"
#include "seq_file_c.h"
#include "seq_file_b.h"


#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_song.h"
#include "seq_mixer.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_midi_port.h"
#include "seq_blm.h"
#include "seq_pattern.h"
#include "seq_core.h"
#include "seq_live.h"
#include "seq_groove.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} seq_file_c_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_c_info_t seq_file_c_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Init(u32 mode)
{
  // invalidate file info
  SEQ_FILE_C_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Load(char *session)
{
  s32 error;
  error = SEQ_FILE_C_Read(session);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] Tried to open config file, status: %d\n", error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Unload(void)
{
  seq_file_c_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Valid(void)
{
  return seq_file_c_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -256 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -256; // modification for SEQ_FILE_C: return -256 if value invalid, since we read signed bytes

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// Another help function which also returns an error message if range is violated
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec_range(char *word, char *parameter, int min, int max)
{
  int value = get_dec(word);
  if( value < -255 || value < min || value > max ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_C] ERROR invalid value '%s' for parameter '%s'\n", word, parameter);
#endif
    return -255; // invalid value or range
  }
  return value;
}


/////////////////////////////////////////////////////////////////////////////
// reads the config file content (again)
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Read(char *session)
{
  s32 status = 0;
  seq_file_c_info_t *info = &seq_file_c_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_C.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] Open config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_C] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_C] read: %s", line_buffer);
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

	  if( strcmp(parameter, "BPMx10_P") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, SEQ_CORE_NUM_BPM_PRESETS-1);
	    if( value < 0 )
	      continue;

	    word = strtok_r(NULL, separators, &brkt);
	    s32 tempo = get_dec(word);

	    if( tempo < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR invalid or missing tempo for parameter '%s'\n", parameter);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    s32 ramp = get_dec(word);

	    if( ramp < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR invalid or missing ramp for parameter '%s'\n", parameter);
#endif
	      continue;
	    }

	    seq_core_bpm_preset_tempo[value] = (float)(tempo/10.0);
	    seq_core_bpm_preset_ramp[value] = (float)(ramp/10.0);
	  } else if( strcmp(parameter, "BPM_Preset") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, SEQ_CORE_NUM_BPM_PRESETS-1);
	    if( value >= 0 )
	      seq_core_bpm_preset_num = value;
	  } else if( strcmp(parameter, "BPM_Mode") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      SEQ_BPM_ModeSet(value);
	  } else if( strcmp(parameter, "LastSong") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, SEQ_CORE_NUM_BPM_PRESETS-1);
	    if( value )
	      SEQ_SONG_NumSet(value-1);
	  } else if( strcmp(parameter, "LastMixerMap") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, SEQ_MIXER_NUM-1);
	    if( value )
	      SEQ_MIXER_NumSet(value-1);
	  } else if( strcmp(parameter, "MixerCC1234BeforePC") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_mixer_cc1234_before_pc = value;
	  } else if( strcmp(parameter, "LastPattern") == 0 ) {
	    int group = get_dec_range(word, parameter, 1, SEQ_CORE_NUM_GROUPS);
	    if( group >= 1 ) {
	      group -= 1; // 0..3

	      // expected format: "LastPattern %d %d:%c%d"
	      word = strtok_r(NULL, separators, &brkt);
	      if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR Pattern: missing second parameter!\n");
#endif
	      } else if( strlen(word) != 4 || word[1] != ':' ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR Pattern: invalid format for second parameter!\n");
#endif
	      } else {
		int bank = word[0] - '1';
		int pgroup = word[2] - 'A';
		int num = word[3] - '1';

		if( bank < 0 || bank >= SEQ_FILE_B_NUM_BANKS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[SEQ_FILE_C] ERROR Pattern: invalid bank!\n");
#endif
		} else if( pgroup < 0 || pgroup >= 8 || num < 0 || num >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[SEQ_FILE_C] ERROR Pattern: invalid pattern!\n");
#endif
		} else {
		  seq_pattern[group].num = num;
		  seq_pattern[group].group = pgroup;
		  seq_pattern[group].bank = bank;
		}
	      }
	    }
	  } else if( strcmp(parameter, "SynchedPatternChange") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_core_options.SYNCHED_PATTERN_CHANGE = value;
	  } else if( strcmp(parameter, "SynchedMute") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_core_options.SYNCHED_MUTE = value;
	  } else if( strcmp(parameter, "SynchedUnmute") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_core_options.SYNCHED_UNMUTE = value;
	  } else if( strcmp(parameter, "RATOPC") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_core_options.RATOPC = value;
	  } else if( strcmp(parameter, "StepsPerMeasure") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 256);
	    if( value >= 0 )
	      seq_core_steps_per_measure = value;
	  } else if( strcmp(parameter, "StepsPerPattern") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 256);
	    if( value >= 0 )
	      seq_core_steps_per_pattern = value;
	  } else if( strcmp(parameter, "GlobalScale") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_core_global_scale = value;
	  } else if( strcmp(parameter, "GlobalScaleCtrl") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_core_global_scale_ctrl = value;
	  } else if( strcmp(parameter, "GlobalScaleRoot") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_core_global_scale_root_selection = value;
	  } else if( strcmp(parameter, "LocalGrooveSelection") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_groove_ui_local_selection = value;
	  } else if( strcmp(parameter, "LoopMode") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_core_glb_loop_mode = value;
	  } else if( strcmp(parameter, "LoopOffset") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 1, 255);
	    if( value >= 0 )
	      seq_core_glb_loop_offset = value-1;
	  } else if( strcmp(parameter, "LoopSteps") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 1, 255);
	    if( value >= 0 )
	      seq_core_glb_loop_steps = value-1;
	  } else if( strcmp(parameter, "LiveOctTranspose") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value >= 0 )
	      seq_live_options.OCT_TRANSPOSE = value;
	  } else if( strcmp(parameter, "LiveVelocity") == 0 ) {
	    //seq_live_options.VELOCITY = value;
	    // obsolete - silently ignore
	  } else if( strcmp(parameter, "LiveForceToScale") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_live_options.FORCE_SCALE = value;
	  } else if( strcmp(parameter, "LiveKeepChannel") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_live_options.KEEP_CHANNEL = value;
	  } else if( strcmp(parameter, "LiveFx") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 1);
	    if( value >= 0 )
	      seq_live_options.FX = value;
	  } else if( strcmp(parameter, "LivePattern") == 0 ) {
	      int num = get_dec(word);
	      if( num < 1 || num > SEQ_LIVE_NUM_ARP_PATTERNS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR LivePattern: invalid pattern number (expect 1..%d)!\n",
			  1, SEQ_LIVE_NUM_ARP_PATTERNS);
#endif
		break;
	      } else {
		word = strtok_r(NULL, separators, &brkt);
		if( word == NULL ) {
		  DEBUG_MSG("[SEQ_FILE_C] ERROR LivePattern %d: missing pattern!\n", num);
		} else {
		  int i;

		  u16 mask = 1;
		  seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[num-1];
		  pattern->gate = 0x0000;
		  pattern->accent = 0x0000;
		  for(i=0; i<16; ++i, mask <<= 1) {
		    if( word[i] == 0 ) {
		      DEBUG_MSG("[SEQ_FILE_C] ERROR LivePattern %d: missing characters in pattern (expecting 16 chars)!\n", num);
		      break;
		    } else if( word[i] == '.' ) {
		      // no change
		    } else if( word[i] == 'o' ) {
		      pattern->gate |= mask;
		    } else if( word[i] == '*' ) {
		      pattern->gate |= mask;
		      pattern->accent |= mask;
		    } else {
		      DEBUG_MSG("[SEQ_FILE_C] ERROR LivePattern %d: invalid character '%c' in pattern, expecting '.' or 'o' or '*'!\n", num, word[i]);
		    }
		  }
		}
	      }
	  } else if( strcmp(parameter, "MIDI_OUT_ExtCtrlPort") == 0 ) {
	    s32 port = SEQ_MIDI_PORT_OutPortFromNameGet(word);
	    if( port < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid OUT port '%s'!", parameter, word);
#endif
	    } else {
	      seq_midi_in_ext_ctrl_out_port = (mios32_midi_port_t)port;
	    }
	  } else if( strcmp(parameter, "QuickSelLength") == 0 ) {
	    int i;
	    for(i=0; i<UI_QUICKSEL_NUM_PRESETS; ++i) {
	      if( i >= 1 ) word = strtok_r(NULL, separators, &brkt);
	      int v = get_dec(word);
	      if( v < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR QuickSelLength: less items than expected (expect %d, got %d)!\n",
			  UI_QUICKSEL_NUM_PRESETS, i);
#endif
		break;
	      } else {
		if( --v < 0 ) v = 0;
		if( v > 255 ) v = 255;
		ui_quicksel_length[i] = v;
	      }
	    }
	  } else if( strcmp(parameter, "QuickSelLoopLength") == 0 ) {
	    int i;
	    for(i=0; i<UI_QUICKSEL_NUM_PRESETS; ++i) {
	      if( i >= 1 ) word = strtok_r(NULL, separators, &brkt);
	      int v = get_dec(word);
	      if( v < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR QuickSelLoopLength: less items than expected (expect %d, got %d)!\n",
			  UI_QUICKSEL_NUM_PRESETS, i);
#endif
		break;
	      } else {
		if( --v < 0 ) v = 0;
		if( v > 255 ) v = 255;
		ui_quicksel_loop_length[i] = v;
	      }
	    }
	  } else if( strcmp(parameter, "QuickSelLoopLoop") == 0 ) {
	    int i;
	    for(i=0; i<UI_QUICKSEL_NUM_PRESETS; ++i) {
	      if( i >= 1 ) word = strtok_r(NULL, separators, &brkt);
	      int v = get_dec(word);
	      if( v < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR QuickSelLoopLoop: less items than expected (expect %d, got %d)!\n",
			  UI_QUICKSEL_NUM_PRESETS, i);
#endif
		break;
	      } else {
		if( --v < 0 ) v = 0;
		if( v > 255 ) v = 255;
		ui_quicksel_loop_loop[i] = v;
	      }
	    }
	  } else if( strcmp(parameter, "MIDI_DefaultPort") == 0 ) {
	    s32 port = SEQ_MIDI_PORT_OutPortFromNameGet(word);
	    if( port < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid default port '%s'!", parameter, word);
#endif
	    } else {
	      MIOS32_MIDI_DefaultPortSet(port);
	    }
	  } else if( strncmp(parameter, "MIDI_BUS_", 9) == 0 ) {
	    s32 bus = get_dec_range(word, parameter, 0, SEQ_MIDI_IN_NUM_BUSSES-1);
	    if( bus >= 0 && bus < SEQ_MIDI_IN_NUM_BUSSES ) {
	      word = strtok_r(NULL, separators, &brkt);
	      int v = get_dec(word);
	      if( v >= 0 ) {
		if( strcmp(parameter+9, "Channel") == 0 )
		  seq_midi_in_channel[bus] = v;
		else if( strcmp(parameter+9, "Port") == 0 )
		  seq_midi_in_port[bus] = v;
		else if( strcmp(parameter+9, "Lower") == 0 )
		  seq_midi_in_lower[bus] = v;
		else if( strcmp(parameter+9, "Upper") == 0 )
		  seq_midi_in_upper[bus] = v;
		else if( strcmp(parameter+9, "Options") == 0 )
		  seq_midi_in_options[bus].ALL = v;
	      }
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR: unknown parameter: %s", line_buffer);
#endif
	    }
	  } else if( strncmp(parameter, "MIDI_IN_", 8) == 0 ) {
	    if( strcmp(parameter+8, "MClock_Ports") == 0 ) {
	      s32 value = get_dec_range(word, parameter, 0, 0x7fffffff);
	      if( value >= 0 )
		seq_midi_router_mclk_in = value;
	    } else if( strcmp(parameter+8, "RecChannel") == 0 ) {
	      // seq_midi_in_rec_channel = value;
	      // parameter is obsolete - silently ignore
	    } else if( strcmp(parameter+8, "RecPort") == 0 ) {
	      // seq_midi_in_rec_port = (mios32_midi_port_t)value;
	      // parameter is obsolete - silently ignore
	    } else if( strcmp(parameter+8, "SectChannel") == 0 ) {
	      s32 value = get_dec_range(word, parameter, 0, 255);
	      if( value >= 0 )
		seq_midi_in_sect_channel = value;
	    } else if( strcmp(parameter+8, "SectPort") == 0 ) {
	      s32 port = SEQ_MIDI_PORT_InPortFromNameGet(word);
	      if( port < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid IN port '%s'!", parameter, word);
#endif
	      } else {
		seq_midi_in_sect_port = (mios32_midi_port_t)port;
	      }
	    } else if( strcmp(parameter+8, "SectFwdPort") == 0 ) {
	      s32 port = SEQ_MIDI_PORT_InPortFromNameGet(word);
	      if( port < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid IN port '%s'!", parameter, word);
#endif
	      } else {
		seq_midi_in_sect_fwd_port = (mios32_midi_port_t)port;
	      }
	    } else if( strcmp(parameter+8, "SectNotes") == 0 ) {
	      int values[4];

	      s32 value = get_dec_range(word, parameter, 0, 255);
	      if( value < 0 )
		continue;

	      values[0] = value;
	      int i;
	      for(i=1; i<4; ++i) {
		word = strtok_r(NULL, separators, &brkt);
		values[i] = get_dec(word);
		if( values[i] < 0 ) {
		  break;
		}
	      }

	      if( i != 4 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR MIDI_IN_SectNotes: missing parameter %d\n", i);
#endif
	      } else {
		for(i=0; i<4; ++i)
		  seq_midi_in_sect_note[i] = values[i];
	      }
	    } else if( strncmp(parameter+8, "ExtCtrl", 7) == 0 ) {
	      if( strcmp(parameter+8+7, "Channel") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_channel = value;
	      } else if( strcmp(parameter+8+7, "Port") == 0 ) {
		s32 port = SEQ_MIDI_PORT_InPortFromNameGet(word);
		if( port < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid IN port '%s'!", parameter, word);
#endif
		} else {
		  seq_midi_in_ext_ctrl_port = (mios32_midi_port_t)port;
		}
	      } else if( strcmp(parameter+8+7, "CcMorph") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH] = value;
	      } else if( strcmp(parameter+8+7, "CcScale") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SCALE] = value;
	      } else if( strcmp(parameter+8+7, "CcSong") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SONG] = value;
	      } else if( strcmp(parameter+8+7, "CcPhrase") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PHRASE] = value;
	      } else if( strcmp(parameter+8+7, "CcMixerMap") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG1") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG2") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G2] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG3") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G3] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG4") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG1") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G1] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG2") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G2] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG3") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G3] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG4") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G4] = value;
	      } else if( strcmp(parameter+8+7, "CcAllNotesOff") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF] = value;
	      } else if( strcmp(parameter+8+7, "NrpnEnabled") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED] = value;
	      } else if( strcmp(parameter+8+7, "PcMode") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE] = value;
	      } else if( strcmp(parameter+8+7, "CcMutes") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MUTES] = value;
	      } else if( strcmp(parameter+8+7, "CcSteps") == 0 ) {
		s32 value = get_dec_range(word, parameter, 0, 255);
		if( value >= 0 )
		  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_STEPS] = value;
	      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR: unknown parameter: %s", line_buffer);
#endif
	      }
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR: unknown parameter: %s", line_buffer);
#endif
	    }
	  } else if( strcmp(parameter, "MIDI_OUT_MClock_Ports") == 0 ) {
	    s32 value = get_dec_range(word, parameter, 0, 0x7fffffff);
	    if( value >= 0 )
	      seq_midi_router_mclk_out = value;
	  } else if( strcmp(parameter, "MIDI_OUT_MClock_Delay") == 0 ) {
	    s32 port = SEQ_MIDI_PORT_ClkPortFromNameGet(word);

	    if( port < 0 || port >= 0x100 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid port number '%s'!", parameter, word);
#endif
	    } else {
	      word = strtok_r(NULL, separators, &brkt);
	      s32 value = get_dec(word);
	      if( value < -128 || value > 127 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR in %s definition: invalid delay value '%s'!", parameter, word);
#endif
	      } else {
		SEQ_MIDI_PORT_ClkDelaySet(port, value);
		SEQ_MIDI_PORT_ClkDelayUpdate(port);
	      }
	    }
	  } else if( strcmp(parameter, "BLM_Fader") == 0 ) {
	    int values[3];

	    s32 fader_ix = get_dec_range(word, parameter, 1, SEQ_BLM_NUM_FADERS);
	    if( fader_ix < 1 )
	      continue;
	    --fader_ix; // counting from 0
	      
	    int i;
	    for(i=0; i<3; ++i) {
	      word = strtok_r(NULL, separators, &brkt);
	      if( i == 0 ) {
		if( strcmp(word, "Trk") == 0 ) {
		  values[i] = 0;
		} else {
		  values[i] = SEQ_MIDI_PORT_OutPortFromNameGet(word);
		}
	      } else if( i == 1 ) {
		if( strcmp(word, "Trk") == 0 ) {
		  values[i] = 0;
		} else {
		  values[i] = get_dec_range(word, parameter, 0, 16);
		}
	      } else {
		values[i] = get_dec_range(word, parameter, 0, 65535);
	      }
	      if( values[i] < 0 ) {
		break;
	      }
	    }

	    if( i != 3 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR BLM_Fader: missing or wrong parameter %d\n", i+1);
#endif
	    } else {
	      seq_blm_fader_t *fader = &seq_blm_fader[fader_ix];
	      fader->port = values[0];
	      fader->chn = values[1];
	      fader->send_function = values[2];
	    }
	  } else if( strcmp(parameter, "MIDI_RouterNode") == 0 ) {
	    int values[5];

	    s32 value = get_dec_range(word, parameter, 0, 255);
	    if( value < 0 )
	      continue;
	      
	    values[0] = value;
	    int i;
	    for(i=1; i<5; ++i) {
	      word = strtok_r(NULL, separators, &brkt);
	      if( i == 1 ) {
		values[i] = SEQ_MIDI_PORT_InPortFromNameGet(word);
	      } else if( i == 3 ) {
		values[i] = SEQ_MIDI_PORT_OutPortFromNameGet(word);
	      } else {
		values[i] = get_dec(word);
	      }
	      if( values[i] < 0 ) {
		break;
	      }
	    }

	    if( i != 5 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR MIDI_RouterNode: missing or wrong parameter %d\n", i);
#endif
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	      DEBUG_MSG("[SEQ_FILE_C] MIDI_RouterNode %d %d %d %d %d\n", values[0], values[1], values[2], values[3], values[4]);
#endif
	      if( values[0] >= SEQ_MIDI_ROUTER_NUM_NODES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_C] ERROR MIDI_RouterNode: invalid node number %d\n", values[0]);
#endif
	      } else {
		seq_midi_router_node_t *n = &seq_midi_router_node[values[0]];
		n->src_port = values[1];
		n->src_chn = values[2];
		n->dst_port = values[3];
		n->dst_chn = values[4];
	      }
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    // changed error level from 1 to 2 here, since people are sometimes confused about these messages
	    // on file format changes
	    DEBUG_MSG("[SEQ_FILE_C] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_C] ERROR no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_C] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_C_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  // change tempo to given preset
  SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_C_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  // write config values
  u8 bpm_preset;
  for(bpm_preset=0; bpm_preset<SEQ_CORE_NUM_BPM_PRESETS; ++bpm_preset) {
    sprintf(line_buffer, "BPMx10_P %d %d %d\n", 
	    bpm_preset,
	    (int)(seq_core_bpm_preset_tempo[bpm_preset]*10),
	    (int)(seq_core_bpm_preset_ramp[bpm_preset]*10));
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "BPM_Preset %d\n", seq_core_bpm_preset_num);
  FLUSH_BUFFER;

  sprintf(line_buffer, "BPM_Mode %d\n", SEQ_BPM_ModeGet());
  FLUSH_BUFFER;

  // quick&dirty...
  sprintf(line_buffer, "QuickSelLength %d %d %d %d %d %d %d %d\n",
	  (int)ui_quicksel_length[0] + 1,
	  (int)ui_quicksel_length[1] + 1,
	  (int)ui_quicksel_length[2] + 1,
	  (int)ui_quicksel_length[3] + 1,
	  (int)ui_quicksel_length[4] + 1,
	  (int)ui_quicksel_length[5] + 1,
	  (int)ui_quicksel_length[6] + 1,
	  (int)ui_quicksel_length[7] + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "QuickSelLoopLength %d %d %d %d %d %d %d %d\n",
	  (int)ui_quicksel_loop_length[0] + 1,
	  (int)ui_quicksel_loop_length[1] + 1,
	  (int)ui_quicksel_loop_length[2] + 1,
	  (int)ui_quicksel_loop_length[3] + 1,
	  (int)ui_quicksel_loop_length[4] + 1,
	  (int)ui_quicksel_loop_length[5] + 1,
	  (int)ui_quicksel_loop_length[6] + 1,
	  (int)ui_quicksel_loop_length[7] + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "QuickSelLoopLoop %d %d %d %d %d %d %d %d\n",
	  (int)ui_quicksel_loop_loop[0] + 1,
	  (int)ui_quicksel_loop_loop[1] + 1,
	  (int)ui_quicksel_loop_loop[2] + 1,
	  (int)ui_quicksel_loop_loop[3] + 1,
	  (int)ui_quicksel_loop_loop[4] + 1,
	  (int)ui_quicksel_loop_loop[5] + 1,
	  (int)ui_quicksel_loop_loop[6] + 1,
	  (int)ui_quicksel_loop_loop[7] + 1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LastSong %d\n", SEQ_SONG_NumGet()+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LastMixerMap %d\n", SEQ_MIXER_NumGet()+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MixerCC1234BeforePC %d\n", seq_mixer_cc1234_before_pc);
  FLUSH_BUFFER;

  int group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    sprintf(line_buffer, "LastPattern %d %d:%c%d\n", group+1, seq_pattern[group].bank+1, 'A'+seq_pattern[group].group, seq_pattern[group].num+1);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "SynchedPatternChange %d\n", seq_core_options.SYNCHED_PATTERN_CHANGE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "SynchedMute %d\n", seq_core_options.SYNCHED_MUTE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "SynchedUnmute %d\n", seq_core_options.SYNCHED_UNMUTE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RATOPC %d\n", seq_core_options.RATOPC);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsPerMeasure %d\n", seq_core_steps_per_measure);
  FLUSH_BUFFER;

  sprintf(line_buffer, "StepsPerPattern %d\n", seq_core_steps_per_pattern);
  FLUSH_BUFFER;

  sprintf(line_buffer, "GlobalScale %d\n", seq_core_global_scale);
  FLUSH_BUFFER;

  sprintf(line_buffer, "GlobalScaleCtrl %d\n", seq_core_global_scale_ctrl);
  FLUSH_BUFFER;

  sprintf(line_buffer, "GlobalScaleRoot %d\n", seq_core_global_scale_root_selection);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LocalGrooveSelection 0x%04x\n", seq_groove_ui_local_selection);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LoopMode %d\n", seq_core_glb_loop_mode);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LoopOffset %d\n", (int)seq_core_glb_loop_offset+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LoopSteps %d\n", (int)seq_core_glb_loop_steps+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LiveOctTranspose %d\n", seq_live_options.OCT_TRANSPOSE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LiveForceToScale %d\n", seq_live_options.FORCE_SCALE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LiveFx %d\n", seq_live_options.FX);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LiveKeepChannel %d\n", seq_live_options.KEEP_CHANNEL);
  FLUSH_BUFFER;

  {
    int i;
    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[0];
    for(i=0; i<SEQ_LIVE_NUM_ARP_PATTERNS; ++i, ++pattern) {
      int j;
      u16 mask = 1;
      char buffer[17];
      for(j=0; j<16; ++j, mask <<= 1) {
	if( pattern->accent & mask )
	  buffer[j] = '*';
	else if( pattern->gate & mask )
	  buffer[j] = 'o';
	else
	  buffer[j] = '.';
      }
      buffer[j] = 0;

      sprintf(line_buffer, "LivePattern %2d  %s\n", i+1, buffer);
      FLUSH_BUFFER;
    }
  }

  {
    char buffer[5];
    SEQ_MIDI_PORT_OutPortToName(MIOS32_MIDI_DefaultPortGet(), buffer);
    sprintf(line_buffer, "MIDI_DefaultPort %s\n", buffer);
    FLUSH_BUFFER;
  }

  u8 bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    sprintf(line_buffer, "MIDI_BUS_Channel %d %d\n", bus, seq_midi_in_channel[bus]);
    FLUSH_BUFFER;

    {
      char buffer[5];
      SEQ_MIDI_PORT_InPortToName(seq_midi_in_port[bus], buffer);
      sprintf(line_buffer, "MIDI_BUS_Port %d %s\n", bus, buffer);
      FLUSH_BUFFER;
    }

    sprintf(line_buffer, "MIDI_BUS_Lower %d %d\n", bus, seq_midi_in_lower[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Upper %d %d\n", bus, seq_midi_in_upper[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Options %d 0x%02x\n", bus, (u8)seq_midi_in_options[bus].ALL);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "MIDI_IN_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_in);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_SectChannel %d\n", seq_midi_in_sect_channel);
  FLUSH_BUFFER;

  {
    char buffer[5];
    SEQ_MIDI_PORT_InPortToName(seq_midi_in_sect_port, buffer);
    sprintf(line_buffer, "MIDI_IN_SectPort %s\n", buffer);
    FLUSH_BUFFER;
  }

  {
    char buffer[5];
    SEQ_MIDI_PORT_InPortToName(seq_midi_in_sect_fwd_port, buffer);
    sprintf(line_buffer, "MIDI_IN_SectFwdPort %s\n", buffer);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "MIDI_IN_SectNotes %d %d %d %d\n", (u8)seq_midi_in_sect_note[0], (u8)seq_midi_in_sect_note[1], (u8)seq_midi_in_sect_note[2], (u8)seq_midi_in_sect_note[3]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_ExtCtrlChannel %d\n", seq_midi_in_ext_ctrl_channel);
  FLUSH_BUFFER;

  {
    char buffer[5];
    SEQ_MIDI_PORT_InPortToName(seq_midi_in_ext_ctrl_port, buffer);
    sprintf(line_buffer, "MIDI_IN_ExtCtrlPort %s\n", buffer);
    FLUSH_BUFFER;
  }

  {
    char buffer[5];
    SEQ_MIDI_PORT_OutPortToName(seq_midi_in_ext_ctrl_out_port, buffer);
    sprintf(line_buffer, "MIDI_OUT_ExtCtrlPort %s\n", buffer);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcMorph %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcScale %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SCALE]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcSong %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SONG]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPhrase %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PHRASE]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcMixerMap %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPatternG1 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPatternG2 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G2]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPatternG3 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G3]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPatternG4 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcBankG1 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G1]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcBankG2 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G2]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcBankG3 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G3]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcBankG4 %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G4]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcAllNotesOff %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlNrpnEnabled %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlPcMode %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcMutes %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MUTES]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcSteps %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_STEPS]); FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_OUT_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_out); FLUSH_BUFFER;

  {
    u8 port_ix;
    u8 num_clk_ports = SEQ_MIDI_PORT_ClkNumGet();

    for(port_ix=0; port_ix<num_clk_ports; ++port_ix) {
      s8 delay = SEQ_MIDI_PORT_ClkIxDelayGet(port_ix);
      sprintf(line_buffer, "MIDI_OUT_MClock_Delay %s %d\n", SEQ_MIDI_PORT_ClkNameGet(port_ix), delay);
      FLUSH_BUFFER;
    }
  }

  {
    u8 fader_ix;
    seq_blm_fader_t *fader = &seq_blm_fader[0];
    for(fader_ix=0; fader_ix<SEQ_BLM_NUM_FADERS; ++fader_ix, ++fader) {
      char port_buffer[5];
      char chn_buffer[5];

      if( fader->port == 0 ) {
	strcpy(port_buffer, "Trk");
      } else {
	SEQ_MIDI_PORT_OutPortToName(fader->port, port_buffer);
      }

      if( fader->chn == 0 ) {
	strcpy(chn_buffer, "Trk");
      } else {
	sprintf(chn_buffer, "%d", fader->chn);
      }

      sprintf(line_buffer, "BLM_Fader %d %s %s %d\n", fader_ix+1, port_buffer, chn_buffer, fader->send_function);
      FLUSH_BUFFER;
    }
  }

  {
    u8 node;
    seq_midi_router_node_t *n = &seq_midi_router_node[0];
    for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      char src_buffer[5];
      char dst_buffer[5];

      SEQ_MIDI_PORT_InPortToName(n->src_port, src_buffer);
      SEQ_MIDI_PORT_OutPortToName(n->dst_port, dst_buffer);

      sprintf(line_buffer, "MIDI_RouterNode %d %s %d %s %d\n", node, src_buffer, n->src_chn, dst_buffer, n->dst_chn);
      FLUSH_BUFFER;
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into config file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Write(char *session)
{
  seq_file_c_info_t *info = &seq_file_c_info;

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_C.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] Open config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_C] Failed to open/create config file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= SEQ_FILE_C_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] config file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_C_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends config data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Debug(void)
{
  return SEQ_FILE_C_Write_Hlp(0); // send to debug terminal
}

