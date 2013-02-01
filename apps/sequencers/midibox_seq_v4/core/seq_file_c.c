// $Id$
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
#include "seq_blm.h"
#include "seq_pattern.h"
#include "seq_core.h"


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
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_C] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "BPMx10_P") == 0 ) {
	    if( value >= SEQ_CORE_NUM_BPM_PRESETS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR invalid preset slot %d for parameter '%s'\n", value, parameter);
#endif
	      continue;
	    }

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
	    if( value >= SEQ_CORE_NUM_BPM_PRESETS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR invalid preset number %d for parameter '%s'\n", value, parameter);
#endif
	      continue;
	    }
	    seq_core_bpm_preset_num = value;
	  } else if( strcmp(parameter, "BPM_Mode") == 0 ) {
	    SEQ_BPM_ModeSet(value);
	  } else if( strcmp(parameter, "LastSong") == 0 ) {
	    if( value )
	      SEQ_SONG_NumSet(value-1);
	  } else if( strcmp(parameter, "LastMixerMap") == 0 ) {
	    if( value )
	      SEQ_MIXER_NumSet(value-1);
	  } else if( strcmp(parameter, "LastPattern") == 0 ) {
	    int group = value;

	    if( group < 1 || group > SEQ_CORE_NUM_GROUPS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR Pattern: wrong group %d\n", group);
#endif
	    } else {
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
	    seq_core_options.SYNCHED_PATTERN_CHANGE = value;
	  } else if( strcmp(parameter, "SynchedMute") == 0 ) {
	    seq_core_options.SYNCHED_MUTE = value;
	  } else if( strcmp(parameter, "SynchedUnmute") == 0 ) {
	    seq_core_options.SYNCHED_UNMUTE = value;
	  } else if( strcmp(parameter, "RATOPC") == 0 ) {
	    seq_core_options.RATOPC = value;
	  } else if( strcmp(parameter, "StepsPerMeasure") == 0 ) {
	    seq_core_steps_per_measure = value;
	  } else if( strcmp(parameter, "StepsPerPattern") == 0 ) {
	    seq_core_steps_per_pattern = value;
	  } else if( strcmp(parameter, "GlobalScale") == 0 ) {
	    seq_core_global_scale = value;
	  } else if( strcmp(parameter, "GlobalScaleCtrl") == 0 ) {
	    seq_core_global_scale_ctrl = value;
	  } else if( strcmp(parameter, "GlobalScaleRoot") == 0 ) {
	    seq_core_global_scale_root_selection = value;
	  } else if( strcmp(parameter, "LoopMode") == 0 ) {
	    seq_core_glb_loop_mode = value;
	  } else if( strcmp(parameter, "LoopOffset") == 0 ) {
	    seq_core_glb_loop_offset = value-1;
	  } else if( strcmp(parameter, "LoopSteps") == 0 ) {
	    seq_core_glb_loop_steps = value-1;
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
	    MIOS32_MIDI_DefaultPortSet(value);
	  } else if( strncmp(parameter, "MIDI_BUS_", 9) == 0 ) {
	    int bus = value;
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
	      seq_midi_router_mclk_in = value;
	    } else if( strcmp(parameter+8, "RecChannel") == 0 ) {
	      seq_midi_in_rec_channel = value;
	    } else if( strcmp(parameter+8, "RecPort") == 0 ) {
	      seq_midi_in_rec_port = (mios32_midi_port_t)value;
	    } else if( strcmp(parameter+8, "SectChannel") == 0 ) {
	      seq_midi_in_sect_channel = value;
	    } else if( strcmp(parameter+8, "SectPort") == 0 ) {
	      seq_midi_in_sect_port = (mios32_midi_port_t)value;
	    } else if( strcmp(parameter+8, "SectFwdPort") == 0 ) {
	      seq_midi_in_sect_fwd_port = (mios32_midi_port_t)value;
	    } else if( strcmp(parameter+8, "SectNotes") == 0 ) {
	      int values[4];

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
		seq_midi_in_ext_ctrl_channel = value;
	      } else if( strcmp(parameter+8+7, "Port") == 0 ) {
		seq_midi_in_ext_ctrl_port = (mios32_midi_port_t)value;
	      } else if( strcmp(parameter+8+7, "CcMorph") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH] = value;
	      } else if( strcmp(parameter+8+7, "CcScale") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SCALE] = value;
	      } else if( strcmp(parameter+8+7, "CcSong") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SONG] = value;
	      } else if( strcmp(parameter+8+7, "CcPhrase") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PHRASE] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG1") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG2") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G2] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG3") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G3] = value;
	      } else if( strcmp(parameter+8+7, "CcPatternG4") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG1") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G1] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG2") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G2] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG3") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G3] = value;
	      } else if( strcmp(parameter+8+7, "CcBankG4") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G4] = value;
	      } else if( strcmp(parameter+8+7, "CcAllNotesOff") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF] = value;
	      } else if( strcmp(parameter+8+7, "NrpnEnabled") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED] = value;
	      } else if( strcmp(parameter+8+7, "PcMode") == 0 ) {
		seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE] = value;
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
	    seq_midi_router_mclk_out = value;
	  } else if( strcmp(parameter, "MIDI_RouterNode") == 0 ) {
	    int values[5];

	    values[0] = value;
	    int i;
	    for(i=1; i<5; ++i) {
	      word = strtok_r(NULL, separators, &brkt);
	      values[i] = get_dec(word);
	      if( values[i] < 0 ) {
		break;
	      }
	    }

	    if( i != 5 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_C] ERROR MIDI_RouterNode: missing parameter %d\n", i);
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

  sprintf(line_buffer, "LoopMode %d\n", seq_core_glb_loop_mode);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LoopOffset %d\n", (int)seq_core_glb_loop_offset+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "LoopSteps %d\n", (int)seq_core_glb_loop_steps+1);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_DefaultPort %d\n", MIOS32_MIDI_DefaultPortGet());
  FLUSH_BUFFER;

  u8 bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    sprintf(line_buffer, "MIDI_BUS_Channel %d %d\n", bus, seq_midi_in_channel[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Port %d %d\n", bus, (u8)seq_midi_in_port[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Lower %d %d\n", bus, seq_midi_in_lower[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Upper %d %d\n", bus, seq_midi_in_upper[bus]);
    FLUSH_BUFFER;

    sprintf(line_buffer, "MIDI_BUS_Options %d 0x%02x\n", bus, (u8)seq_midi_in_options[bus].ALL);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "MIDI_IN_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_in);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_RecChannel %d\n", seq_midi_in_rec_channel);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_RecPort %d\n", (u8)seq_midi_in_rec_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_SectChannel %d\n", seq_midi_in_sect_channel);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_SectPort %d\n", (u8)seq_midi_in_sect_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_SectFwdPort %d\n", (u8)seq_midi_in_sect_fwd_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_SectNotes %d %d %d %d\n", (u8)seq_midi_in_sect_note[0], (u8)seq_midi_in_sect_note[1], (u8)seq_midi_in_sect_note[2], (u8)seq_midi_in_sect_note[3]);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_ExtCtrlChannel %d\n", seq_midi_in_ext_ctrl_channel);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_ExtCtrlPort %d\n", (u8)seq_midi_in_ext_ctrl_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcMorph %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcScale %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SCALE]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcSong %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SONG]); FLUSH_BUFFER;
  sprintf(line_buffer, "MIDI_IN_ExtCtrlCcPhrase %d\n", (u8)seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PHRASE]); FLUSH_BUFFER;
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

  sprintf(line_buffer, "MIDI_OUT_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_out);
  FLUSH_BUFFER;

  u8 node;
  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    sprintf(line_buffer, "MIDI_RouterNode %d %d %d %d %d\n", node, n->src_port, n->src_chn, n->dst_port, n->dst_chn);
    FLUSH_BUFFER;
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

