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


#include "seq_ui.h"
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
  char line_buffer[128];
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
	  } else if( strcmp(parameter, "BPM_DINSyncDiv") == 0 ) {
	    seq_core_bpm_din_sync_div = value;
	  } else if( strcmp(parameter, "SynchedPatternChange") == 0 ) {
	    seq_core_options.SYNCHED_PATTERN_CHANGE = value;
	  } else if( strcmp(parameter, "FollowSong") == 0 ) {
	    seq_core_options.FOLLOW_SONG = value;
	  } else if( strcmp(parameter, "PasteClrAll") == 0 ) {
	    seq_core_options.PASTE_CLR_ALL = value;
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
	  } else if( strcmp(parameter, "MIDI_DefaultPort") == 0 ) {
	    MIOS32_MIDI_DefaultPortSet(value);
	  } else if( strcmp(parameter, "MIDI_IN_Channel") == 0 ) {
	    seq_midi_in_channel = value;
	  } else if( strcmp(parameter, "MIDI_IN_Port") == 0 ) {
	    seq_midi_in_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "MIDI_IN_MClock_Ports") == 0 ) {
	    seq_midi_router_mclk_in = value;
	  } else if( strcmp(parameter, "MIDI_IN_TA_Split") == 0 ) {
	    if( value )
	      seq_midi_in_ta_split_note |= 0x80;
	    else
	      seq_midi_in_ta_split_note &= ~0x80;
	  } else if( strcmp(parameter, "MIDI_IN_TA_SplitNote") == 0 ) {
	    seq_midi_in_ta_split_note = (seq_midi_in_ta_split_note & 0x80) | (value & 0x7f);
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
	  } else if( strcmp(parameter, "MetronomePort") == 0 ) {
	    seq_core_metronome_port = (mios32_midi_port_t)value;
	  } else if( strcmp(parameter, "MetronomeChannel") == 0 ) {
	    seq_core_metronome_chn = value;
	  } else if( strcmp(parameter, "MetronomeNoteM") == 0 ) {
	    seq_core_metronome_note_m = value;
	  } else if( strcmp(parameter, "MetronomeNoteB") == 0 ) {
	    seq_core_metronome_note_b = value;
	  } else if( strcmp(parameter, "RemoteMode") == 0 ) {
	    seq_ui_remote_mode = (value > 2) ? 0 : value;
	  } else if( strcmp(parameter, "RemotePort") == 0 ) {
	    seq_ui_remote_port = value;
	  } else if( strcmp(parameter, "RemoteID") == 0 ) {
	    seq_ui_remote_id = (value > 128) ? 0 : value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
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
  status |= SEQ_FILE_ReadClose(&fi);

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
static s32 SEQ_FILE_C_Write_Hlp(PFILEINFO fileinfo)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( fileinfo == NULL ) { DEBUG_MSG(line_buffer); } else { status |= SEQ_FILE_WriteBuffer(fileinfo, (u8 *)line_buffer, strlen(line_buffer)); }

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

  sprintf(line_buffer, "BPM_DINSyncDiv %d\n", seq_core_bpm_din_sync_div);
  FLUSH_BUFFER;

  sprintf(line_buffer, "SynchedPatternChange %d\n", seq_core_options.SYNCHED_PATTERN_CHANGE);
  FLUSH_BUFFER;

  sprintf(line_buffer, "FollowSong %d\n", seq_core_options.FOLLOW_SONG);
  FLUSH_BUFFER;

  sprintf(line_buffer, "PasteClrAll %d\n", seq_core_options.PASTE_CLR_ALL);
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

  sprintf(line_buffer, "MIDI_IN_Channel %d\n", seq_midi_in_channel);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_Port %d\n", (u8)seq_midi_in_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_in);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_TA_Split %d\n", (seq_midi_in_ta_split_note & 0x80) ? 1 : 0);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_IN_TA_SplitNote %d\n", seq_midi_in_ta_split_note & 0x7f);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MIDI_OUT_MClock_Ports 0x%08x\n", (u32)seq_midi_router_mclk_out);
  FLUSH_BUFFER;

  u8 node;
  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    sprintf(line_buffer, "MIDI_RouterNode %d %d %d %d %d\n", node, n->src_port, n->src_chn, n->dst_port, n->dst_chn);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "MetronomePort %d\n", (u8)seq_core_metronome_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeChannel %d\n", (u8)seq_core_metronome_chn);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeNoteM %d\n", (u8)seq_core_metronome_note_m);
  FLUSH_BUFFER;

  sprintf(line_buffer, "MetronomeNoteB %d\n", (u8)seq_core_metronome_note_b);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RemoteMode %d\n", (u8)seq_ui_remote_mode);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RemotePort %d\n", (u8)seq_ui_remote_port);
  FLUSH_BUFFER;

  sprintf(line_buffer, "RemoteID %d\n", (u8)seq_ui_remote_id);
  FLUSH_BUFFER;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into config file
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

  // write file
  status |= SEQ_FILE_C_Write_Hlp(&fi);

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

/////////////////////////////////////////////////////////////////////////////
// sends config data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_C_Debug(void)
{
  return SEQ_FILE_C_Write_Hlp(NULL); // send to debug terminal
}

