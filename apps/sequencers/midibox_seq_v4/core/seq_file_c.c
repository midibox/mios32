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

#include <dosfs.h>
#include <string.h>

#include "seq_file.h"
#include "seq_file_c.h"


#include "seq_bpm.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBSEQ files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH ""
//#define SEQ_FILES_PATH "MySongs/"


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
s32 SEQ_FILE_C_Load(void)
{
  s32 error;
  error = SEQ_FILE_C_Read();
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
s32 SEQ_FILE_C_Read(void)
{
  s32 status = 0;
  seq_file_c_info_t *info = &seq_file_c_info;
  FILEINFO fi;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_C.V4", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] Open config file '%s'\n", filepath);
#endif

  if( (status=SEQ_FILE_ReadOpen(&fi, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_C] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // change to file header
  if( (status=SEQ_FILE_Seek(&fi, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_C] failed to change offset in file, status: %d\n", status);
#endif
    return SEQ_FILE_C_ERR_READ;
  }

  // read config values
  char line_buffer[80];
  do {
    status=SEQ_FILE_ReadLine(&fi, (u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_C] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( parameter = strtok_r(line_buffer, separators, &brkt) ) {

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
	  } else if( strcmp(parameter, "BPM_IntDiv") == 0 ) {
	    seq_core_bpm_div_int = value;
	  } else if( strcmp(parameter, "BPM_ExtDiv") == 0 ) {
	    seq_core_bpm_div_ext = value;
	  } else if( strcmp(parameter, "SynchedPatternChange") == 0 ) {
	    seq_core_options.SYNCHED_PATTERN_CHANGE = value;
	  } else if( strcmp(parameter, "StepsPerMeasure") == 0 ) {
	    seq_core_steps_per_measure = value;
	  } else if( strcmp(parameter, "GlobalScale") == 0 ) {
	    seq_core_global_scale = value;
	  } else if( strcmp(parameter, "GlobalScaleCtrl") == 0 ) {
	    seq_core_global_scale_ctrl = value;
	  } else if( strcmp(parameter, "GlobalScaleRoot") == 0 ) {
	    seq_core_global_scale_root_selection = value;
	  } else if( strcmp(parameter, "MIDI_DefaultPort") == 0 ) {
	    MIOS32_MIDI_DefaultPortSet(value);
	  } else if( strcmp(parameter, "MIDI_IN_Channel") == 0 ) {
	    seq_midi_in_channel = value;
	  } else if( strcmp(parameter, "MIDI_IN_Port") == 0 ) {
	    seq_midi_in_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "MIDI_IN_MClock_Port") == 0 ) {
	    seq_midi_in_mclk_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "MIDI_IN_TA_Split") == 0 ) {
	    if( value )
	      seq_midi_in_ta_split_note |= 0x80;
	    else
	      seq_midi_in_ta_split_note &= ~0x80;
	  } else if( strcmp(parameter, "MIDI_IN_TA_SplitNote") == 0 ) {
	    seq_midi_in_ta_split_note = (seq_midi_in_ta_split_note & 0x80) | (value & 0x7f);
	  } else if( strcmp(parameter, "MIDI_OUT_MClock") == 0 ) {
	    seq_midi_router_mclk_out = (mios32_midi_port_t)value;
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
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown parameter: %s", line_buffer);
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
// writes the config file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Write(void)
{
  seq_file_c_info_t *info = &seq_file_c_info;

  FILEINFO fi;

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_C.V4", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] Open config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(&fi, filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_C] Failed to open/create config file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(&fi); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  char line_buffer[128];

  // write config values
  u8 bpm_preset;
  for(bpm_preset=0; bpm_preset<SEQ_CORE_NUM_BPM_PRESETS; ++bpm_preset) {
    sprintf(line_buffer, "BPMx10_P %d %d %d\n", 
	    bpm_preset,
	    (int)(seq_core_bpm_preset_tempo[bpm_preset]*10),
	    (int)(seq_core_bpm_preset_ramp[bpm_preset]*10));
    status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));
  }

  sprintf(line_buffer, "BPM_Preset %d\n", seq_core_bpm_preset_num);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "BPM_Mode %d\n", SEQ_BPM_ModeGet());
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "BPM_IntDiv %d\n", seq_core_bpm_div_int);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "BPM_ExtDiv %d\n", seq_core_bpm_div_ext);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "SynchedPatternChange %d\n", seq_core_options.SYNCHED_PATTERN_CHANGE);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "StepsPerMeasure %d\n", seq_core_steps_per_measure);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "GlobalScale %d\n", seq_core_global_scale);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "GlobalScaleCtrl %d\n", seq_core_global_scale_ctrl);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "GlobalScaleRoot %d\n", seq_core_global_scale_root_selection);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_DefaultPort %d\n", MIOS32_MIDI_DefaultPortGet());
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_IN_Channel %d\n", seq_midi_in_channel);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_IN_Port %d\n", (u8)seq_midi_in_port);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_IN_MClock_Port %d\n", (u8)seq_midi_in_mclk_port);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_IN_TA_Split %d\n", (seq_midi_in_ta_split_note & 0x80) ? 1 : 0);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_IN_TA_SplitNote %d\n", seq_midi_in_ta_split_note & 0x7f);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  sprintf(line_buffer, "MIDI_OUT_MClock %d\n", seq_midi_router_mclk_out);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));

  u8 node;
  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    sprintf(line_buffer, "MIDI_RouterNode %d %d %d %d %d\n", node, n->src_port, n->src_chn, n->dst_port, n->dst_chn);
    status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)line_buffer, strlen(line_buffer));
  }

  // close file
  status |= SEQ_FILE_WriteClose(&fi);


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_C] config file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_C_ERR_WRITE : 0;
}
