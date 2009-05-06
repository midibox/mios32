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
  char _line_buffer[80];
  char *line_buffer;
  do {
    line_buffer = _line_buffer;
    status=SEQ_FILE_ReadLine(&fi, (u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_C] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      while( *line_buffer == ' ' || *line_buffer == '\t' )
	++line_buffer;
      
      if( *line_buffer == 0 || *line_buffer == '#' )
	continue;

      char *space = strchr(line_buffer, ' ');
      if( space != NULL ) {
	// separate line buffer into keyword and value string
	size_t space_pos = space-line_buffer;
	char *value_str = (char *)(line_buffer + space_pos + 1);
	line_buffer[space_pos] = 0;

	// search for keywords
    	       if( strcmp(line_buffer, "BPMx10") == 0 ) {
	  SEQ_BPM_Set((float)atoi(value_str)/10);
	} else if( strcmp(line_buffer, "BPM_Mode") == 0 ) {
	  SEQ_BPM_ModeSet(atoi(value_str));
	} else if( strcmp(line_buffer, "BPM_IntDiv") == 0 ) {
	  seq_core_bpm_div_int = atoi(value_str);
	} else if( strcmp(line_buffer, "BPM_ExtDiv") == 0 ) {
	  seq_core_bpm_div_ext = atoi(value_str);
	} else if( strcmp(line_buffer, "SynchedPatternChange") == 0 ) {
	  seq_core_options.SYNCHED_PATTERN_CHANGE = atoi(value_str);
	} else if( strcmp(line_buffer, "StepsPerMeasure") == 0 ) {
	  seq_core_steps_per_measure = atoi(value_str);
	} else if( strcmp(line_buffer, "GlobalScale") == 0 ) {
	  seq_core_global_scale = atoi(value_str);
	} else if( strcmp(line_buffer, "GlobalScaleCtrl") == 0 ) {
	  seq_core_global_scale_ctrl = atoi(value_str);
	} else if( strcmp(line_buffer, "GlobalScaleRoot") == 0 ) {
	  seq_core_global_scale_root_selection = atoi(value_str);
	} else if( strcmp(line_buffer, "MIDI_DefaultPort") == 0 ) {
	  MIOS32_MIDI_DefaultPortSet(atoi(value_str));
	} else if( strcmp(line_buffer, "MIDI_IN_Channel") == 0 ) {
	  seq_midi_in_channel = atoi(value_str);
	} else if( strcmp(line_buffer, "MIDI_IN_Port") == 0 ) {
	  seq_midi_in_port = (mios32_midi_port_t)atoi(value_str);
	} else if( strcmp(line_buffer, "MIDI_IN_MClock_Port") == 0 ) {
	  seq_midi_in_mclk_port = (mios32_midi_port_t)atoi(value_str);
	} else if( strcmp(line_buffer, "MIDI_IN_TA_Split") == 0 ) {
	  int value = atoi(value_str);
	  if( value )
	    seq_midi_in_ta_split_note |= 0x80;
	  else
	    seq_midi_in_ta_split_note &= ~0x80;
	} else if( strcmp(line_buffer, "MIDI_IN_TA_SplitNote") == 0 ) {
	  int value = atoi(value_str);
	  seq_midi_in_ta_split_note = (seq_midi_in_ta_split_note & 0x80) | (value & 0x7f);
	} else if( strcmp(line_buffer, "MIDI_OUT_MClock") == 0 ) {
	  seq_midi_router_mclk_out = (mios32_midi_port_t)atoi(value_str);
	} else if( strcmp(line_buffer, "MIDI_RouterNode") == 0 ) {
	  // very primitive and error-prone scan routine for 5 decimal values
	  int values[5];
	  int i;
	  for(i=0; i<5; ++i) {
	    if( i < 4 ) {
	      if( (space=strchr(value_str, ' ')) == NULL )
		break;

	      *space = 0;
	    }

	    values[i] = (mios32_midi_port_t)atoi(value_str);

	    if( i < 4 ) {
	      size_t space_pos = space-line_buffer;
	      value_str = (char *)(line_buffer + space_pos + 1);
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
	  DEBUG_MSG("[SEQ_FILE_C] ERROR unknown setting: %s %s", line_buffer, value_str);
#endif
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
  sprintf(line_buffer, "BPMx10 %d\n", (int)(SEQ_BPM_Get()*10));
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
