// $Id$
/*
 * Patch File access functions
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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

#include <midi_router.h>
#include <seq_bpm.h>

#include "file.h"
#include "midio_file.h"
#include "midio_file_p.h"
#include "midio_patch.h"
#include "seq.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"
#include "osc_client.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MIDIO_FILES_PATH "/"
//#define MIDIO_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} midio_file_p_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static midio_file_p_info_t midio_file_p_info;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
char midio_file_p_patch_name[MIDIO_FILE_P_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Init(u32 mode)
{
  // invalidate file info
  MIDIO_FILE_P_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads patch file
// Called from MIDIO_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Load(char *filename)
{
  s32 error;
  error = MIDIO_FILE_P_Read(filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Tried to open patch %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads patch file
// Called from MIDIO_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Unload(void)
{
  midio_file_p_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if current patch file valid
// Returns 0 if current patch file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Valid(void)
{
  return midio_file_p_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// help function which removes the quotes of an argument (e.g. .csv file format)
// can be cascaded with strtok_r
/////////////////////////////////////////////////////////////////////////////
static char *remove_quotes(char *word)
{
  if( word == NULL )
    return NULL;

  if( *word == '"' )
    ++word;

  int len = strlen(word);
  if( len && word[len-1] == '"' )
    word[len-1] = 0;

  return word;
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
// help function which parses an IP value
// returns > 0 if value is valid
// returns 0 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static u32 get_ip(char *brkt)
{
  u8 ip[4];
  char *word;

  int i;
  for(i=0; i<4; ++i) {
    if( (word=remove_quotes(strtok_r(NULL, ".", &brkt))) ) {
      s32 value = get_dec(word);
      if( value >= 0 && value <= 255 )
	ip[i] = value;
      else
	return 0;
    }
  }

  if( i == 4 )
    return (ip[0]<<24)|(ip[1]<<16)|(ip[2]<<8)|(ip[3]<<0);
  else
    return 0; // invalid IP
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses a SR definition (<dec>.D<dec>)
// returns >= 0 if value is valid
// returns -1 if value is invalid
// returns -2 if SR number 0 has been passed (could disable a SR definition)
/////////////////////////////////////////////////////////////////////////////
static s32 get_sr(char *word)
{
  if( word == NULL )
    return -1;

  // check for '.D' separator
  char *word2 = word;
  while( *word2 != '.' )
    if( *word2++ == 0 )
      return -1;
  word2++;
  if( *word2++ != 'D' )
    return -1;

  s32 srNum = get_dec(word);
  if( srNum < 0 )
    return -1;

  if( srNum == 0 )
    return -2; // SR has been disabled...

  s32 pinNum = get_dec(word2);
  if( pinNum < 0 )
    return -1;

  if( pinNum >= 8 )
    return -1;

  return 8*(srNum-1) + pinNum;
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses a binary value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
// not required anymore
#if 0
static s32 get_bin(char *word, int numBits)
{
  if( word == NULL )
    return -1;

  s32 value = 0;
  int bit = 0;
  while( 1 ) {
    if( *word == '1' ) {
      value |= 1 << bit;
    } else if( *word != '0' ) {
      break;
    }
    ++word;
    ++bit;
  }

  if( bit != numBits )
    return -1; // invalid number of bits

  return value;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// reads the patch file content (again)
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Read(char *filename)
{
  s32 status = 0;
  midio_file_p_info_t *info = &midio_file_p_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(midio_file_p_patch_name, filename, MIDIO_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.MIO", MIDIO_FILES_PATH, midio_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Open patch '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_P] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read patch values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MIDIO_FILE_P] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
      char *brkt;
      char *parameter;

      if( (parameter = remove_quotes(strtok_r(line_buffer, separators, &brkt))) ) {

	if( *parameter == 0 || *parameter == '#' ) {
	  // ignore comments and empty lines
	} else if( strcmp(parameter, "DIN") == 0 ) {
	  s32 sr;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (sr=get_sr(word)) < 0 || sr >= MIDIO_PATCH_NUM_DIN  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid SR.Pin format for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports = 0;
	    int bit;
	    for(bit=0; bit<16; ++bit) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int enable = get_dec(word);
	      if( enable < 0 )
		break;
	      if( enable >= 1 )
		enabled_ports |= (1 << bit);
	    }

	    if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, sr%8);
#endif
	    } else {
	      s32 mode;
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (mode=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Mode format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, sr%8);
#endif
	      } else {
		u8 events[6];
		int i;
		for(i=0; i<6; ++i) {
		  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		  if( (events[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Event format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, sr%8);
#endif
		    break;
		  } else {
		    events[i] &= 0x7f;
		  }
		}

		if( i == 6 ) {
		  // finally a valid line!
		  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[sr];
		  din_cfg->enabled_ports = enabled_ports;
		  din_cfg->mode = mode;
		  din_cfg->evnt0_on = events[0];
		  din_cfg->evnt1_on = events[1];
		  din_cfg->evnt2_on = events[2];
		  din_cfg->evnt0_off = events[3];
		  din_cfg->evnt1_off = events[4];
		  din_cfg->evnt2_off = events[5];
		}
	      }
	    }
	    
	  }	  
	} else if( strcmp(parameter, "DOUT") == 0 ) {
	  s32 sr;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (sr=get_sr(word)) < 0 || sr >= MIDIO_PATCH_NUM_DOUT  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid SR.Pin format for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports = 0;
	    int bit;
	    for(bit=0; bit<16; ++bit) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int enable = get_dec(word);
	      if( enable < 0 )
		break;
	      if( enable >= 1 )
		enabled_ports |= (1 << bit);
	    }

	    if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, 7-(sr%8));
#endif
	    } else {
	      u8 events[2];
	      int i;
	      for(i=0; i<2; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (events[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Event format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, (7-sr%8));
#endif
		  break;
		} else {
		  events[i] &= 0x7f;
		}
	      }

	      if( i == 2 ) {
		// finally a valid line!
		sr ^= 7; // DOUT SR pins are mirrored
		midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[sr];
		dout_cfg->enabled_ports = enabled_ports;
		dout_cfg->evnt0 = events[0];
		dout_cfg->evnt1 = events[1];
	      }
	    }
	    
	  }	  
	} else if( strcmp(parameter, "AIN") == 0 ) {
	  s32 ain;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (ain=get_dec(word)) < 0 || ain >= MIDIO_PATCH_NUM_AIN  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid AIN pin for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports = 0;
	    int bit;
	    for(bit=0; bit<16; ++bit) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int enable = get_dec(word);
	      if( enable < 0 )
		break;
	      if( enable >= 1 )
		enabled_ports |= (1 << bit);
	    }

	    if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %d'\n", parameter, ain);
#endif
	    } else {
	      u8 events[2];
	      int i;
	      for(i=0; i<2; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (events[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Event format for parameter '%s %d'\n", parameter, ain);
#endif
		  break;
		} else {
		  events[i] &= 0x7f;
		}
	      }

	      if( i == 2 ) {
		// finally a valid line!
		midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[ain];
		ain_cfg->enabled_ports = enabled_ports;
		ain_cfg->evnt0 = events[0];
		ain_cfg->evnt1 = events[1];
	      }
	    }
	    
	  }	  
	} else if( strcmp(parameter, "AINSER") == 0 ) {
	  s32 ain;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (ain=get_dec(word)) < 0 || ain >= MIDIO_PATCH_NUM_AINSER  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid AINSER pin for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports = 0;
	    int bit;
	    for(bit=0; bit<16; ++bit) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int enable = get_dec(word);
	      if( enable < 0 )
		break;
	      if( enable >= 1 )
		enabled_ports |= (1 << bit);
	    }

	    if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %d'\n", parameter, ain);
#endif
	    } else {
	      u8 events[2];
	      int i;
	      for(i=0; i<2; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (events[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Event format for parameter '%s %d'\n", parameter, ain);
#endif
		  break;
		} else {
		  events[i] &= 0x7f;
		}
	      }

	      if( i == 2 ) {
		// finally a valid line!
		midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[ain];
		ain_cfg->enabled_ports = enabled_ports;
		ain_cfg->evnt0 = events[0];
		ain_cfg->evnt1 = events[1];
	      }
	    }
	    
	  }	  
	} else if( strcmp(parameter, "MATRIX") == 0 ) {
	  s32 matrix;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (matrix=get_dec(word)) < 1 || matrix > MIDIO_PATCH_NUM_MATRIX  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid matrix number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --matrix;

	    s32 enabled_ports = 0;
	    int bit;
	    for(bit=0; bit<16; ++bit) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int enable = get_dec(word);
	      if( enable < 0 )
		break;
	      if( enable >= 1 )
		enabled_ports |= (1 << bit);
	    }

	    if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %d'\n", parameter, matrix);
#endif
	    } else {
	      // we have to read 5 additional values
	      int values[5];
	      const char value_name[5][10] = { "Mode", "Channel", "Arg", "SR_DIN", "SR_DOUT" };
	      int i;
	      for(i=0; i<5; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (values[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, matrix);
#endif
		  break;
		}
	      }

	      if( i == 5 ) {
		// finally a valid line!
		midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[matrix];
		m->enabled_ports = enabled_ports;
		m->mode = values[0];
		m->chn = values[1];
		m->arg = values[2];
		m->sr_din = values[3];
		m->sr_dout = values[4];
	      }
	    }
	  }	  
	} else if( strcmp(parameter, "MATRIX_MAP_CHN") == 0 ) {
	  s32 matrix;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (matrix=get_dec(word)) < 1 || matrix > MIDIO_PATCH_NUM_MATRIX  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid matrix number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --matrix;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int row;
	    if( (row=get_dec(word)) < 1 || row > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid row number for parameter '%s'\n", parameter);
#endif
	    } else {
	      // user counts from 1...
	      --row;

	      int chn[8];
	      int col;
	      for(col=0; col<8; ++col) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (chn[col]=get_dec(word)) < 1 || chn[col] > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid channel number for parameter '%s'\n", parameter);
#endif
		  break;
		}
	      }

	      // valid line?
	      if( col == 8 ) {
		midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[matrix];
		for(col=0; col<8; ++col) {
		  m->map_chn[row*8+col] = chn[col];
		}
	      }
	    }
	  }
	} else if( strcmp(parameter, "MATRIX_MAP_EVNT1") == 0 ) {
	  s32 matrix;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (matrix=get_dec(word)) < 1 || matrix > MIDIO_PATCH_NUM_MATRIX  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid matrix number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --matrix;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int row;
	    if( (row=get_dec(word)) < 1 || row > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid row number for parameter '%s'\n", parameter);
#endif
	    } else {
	      // user counts from 1...
	      --row;

	      int evnt1[8];
	      int col;
	      for(col=0; col<8; ++col) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (evnt1[col]=get_dec(word)) < 0 || evnt1[col] > 127 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid evnt1 number for parameter '%s'\n", parameter);
#endif
		  break;
		}
	      }

	      // valid line?
	      if( col == 8 ) {
		midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[matrix];
		for(col=0; col<8; ++col) {
		  m->map_evnt1[row*8+col] = evnt1[col];
		}
	      }
	    }
	  }
	} else if( strcmp(parameter, "ROUTER") == 0 ) {
	  s32 node;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (node=get_dec(word)) < 1 || node > MIDI_ROUTER_NUM_NODES  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid node number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --node;

	    // we have to read 4 additional values
	    int values[4];
	    const char value_name[4][10] = { "SrcPort", "Channel", "DstPort", "Channel" };
	    int i;
	    for(i=0; i<4; ++i) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (values[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, node);
#endif
		break;
	      }
	    }
	    
	    if( i == 4 ) {
	      // finally a valid line!
	      midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[node];
	      n->src_port = values[0];
	      n->src_chn = values[1];
	      n->dst_port = values[2];
	      n->dst_chn = values[3];
	    }
	  }
	} else if( strcmp(parameter, "ForwardIO") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.flags.FORWARD_IO = value;
	  }
	} else if( strcmp(parameter, "InverseDIN") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.flags.INVERSE_DIN = value;
	  }
	} else if( strcmp(parameter, "InverseDOUT") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.flags.INVERSE_DOUT = value;
	  }
	} else if( strcmp(parameter, "AltProgramChange") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.flags.ALT_PROGCHNG = value;
	  }
	} else if( strcmp(parameter, "DebounceCtr") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.debounce_ctr = value;
	  }
	} else if( strcmp(parameter, "GlobalChannel") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.global_chn = value;
	  }
	} else if( strcmp(parameter, "AllNotesOffChannel") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.all_notes_off_chn = value;
	  }

	} else if( strcmp(parameter, "MidiFilePlayMode") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 || SEQ_MidiPlayModeSet(value) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid play mode for parameter '%s'\n", parameter);
#endif
	  }

	} else if( strcmp(parameter, "BPM_Preset") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid BPM for parameter '%s'\n", parameter);
#endif
	  }
	  SEQ_BPM_Set((float)value);

	} else if( strcmp(parameter, "BPM_Mode") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 || SEQ_ClockModeSet(value) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid BPM Mode for parameter '%s'\n", parameter);
#endif
	  }

	} else if( strcmp(parameter, "MidiFilePlayPorts") == 0 ) {
	  s32 enabled_ports = 0;
	  int bit;
	  for(bit=0; bit<17; ++bit) {
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int enable = get_dec(word);
	    if( enable < 0 )
	      break;
	    if( enable >= 1 )
	      enabled_ports |= (1 << bit);
	  }

	  if( bit != 17 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
#endif
	  } else {
	    seq_play_enable_dout = enabled_ports & 1;
	    seq_play_enabled_ports = enabled_ports >> 1;
	  }	  

	} else if( strcmp(parameter, "MidiFileRecPorts") == 0 ) {
	  s32 enabled_ports = 0;
	  int bit;
	  for(bit=0; bit<17; ++bit) {
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int enable = get_dec(word);
	    if( enable < 0 )
	      break;
	    if( enable >= 1 )
	      enabled_ports |= (1 << bit);
	  }

	  if( bit != 17 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
#endif
	  } else {
	    seq_rec_enable_din = enabled_ports & 1;
	    seq_rec_enabled_ports = enabled_ports >> 1;
	  }	  

	} else if( strcmp(parameter, "MidiFileClkOutPorts") == 0 ) {
	  s32 enabled_ports = 0;
	  int bit;
	  for(bit=0; bit<16; ++bit) {
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int enable = get_dec(word);
	    if( enable < 0 )
	      break;
	    if( enable >= 1 )
	      enabled_ports |= (1 << bit);
	  }

	  if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
#endif
	  } else {
	    MIDI_ROUTER_MIDIClockOutSet(USB0, (enabled_ports & 0x0001) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(USB1, (enabled_ports & 0x0002) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(USB2, (enabled_ports & 0x0004) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(USB3, (enabled_ports & 0x0008) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(UART0, (enabled_ports & 0x0010) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(UART1, (enabled_ports & 0x0020) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(UART2, (enabled_ports & 0x0040) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(UART3, (enabled_ports & 0x0080) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(IIC0, (enabled_ports & 0x0100) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(IIC1, (enabled_ports & 0x0200) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(IIC2, (enabled_ports & 0x0400) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(IIC3, (enabled_ports & 0x0800) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(OSC0, (enabled_ports & 0x1000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(OSC1, (enabled_ports & 0x2000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(OSC2, (enabled_ports & 0x4000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockOutSet(OSC3, (enabled_ports & 0x8000) ? 1 : 0);
	  }	  

	} else if( strcmp(parameter, "MidiFileClkInPorts") == 0 ) {
	  s32 enabled_ports = 0;
	  int bit;
	  for(bit=0; bit<16; ++bit) {
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int enable = get_dec(word);
	    if( enable < 0 )
	      break;
	    if( enable >= 1 )
	      enabled_ports |= (1 << bit);
	  }

	  if( bit != 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
#endif
	  } else {
	    MIDI_ROUTER_MIDIClockInSet(USB0, (enabled_ports & 0x0001) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(USB1, (enabled_ports & 0x0002) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(USB2, (enabled_ports & 0x0004) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(USB3, (enabled_ports & 0x0008) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(UART0, (enabled_ports & 0x0010) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(UART1, (enabled_ports & 0x0020) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(UART2, (enabled_ports & 0x0040) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(UART3, (enabled_ports & 0x0080) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(IIC0, (enabled_ports & 0x0100) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(IIC1, (enabled_ports & 0x0200) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(IIC2, (enabled_ports & 0x0400) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(IIC3, (enabled_ports & 0x0800) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(OSC0, (enabled_ports & 0x1000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(OSC1, (enabled_ports & 0x2000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(OSC2, (enabled_ports & 0x4000) ? 1 : 0);
	    MIDI_ROUTER_MIDIClockInSet(OSC3, (enabled_ports & 0x8000) ? 1 : 0);
	  }	  

#if !defined(MIOS32_FAMILY_EMULATION)
	} else if( strcmp(parameter, "ETH_LocalIp") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_IP_AddressSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Netmask") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_NetmaskSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Gateway") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_GatewaySet(value);
	  }
	} else {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "ETH_Dhcp") == 0 ) {
	    UIP_TASK_DHCP_EnableSet((value >= 1) ? 1 : 0);
	  } else if( strcmp(parameter, "OSC_RemoteIp") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      u32 ip;
	      if( !(ip=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	      } else {
		OSC_SERVER_RemoteIP_Set(con, ip);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_RemotePort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_RemotePortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_LocalPort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_LocalPortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_TransferMode") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid transfer mode number for parameter '%s'\n", parameter);
	      } else {
		OSC_CLIENT_TransferModeSet(con, value);
	      }
	    }
#endif /* !defined(MIOS32_FAMILY_EMULATION) */
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    // changed error level from 1 to 2 here, since people are sometimes confused about these messages
	    // on file format changes
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MIDIO_FILE_P] ERROR no space or semicolon separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= FILE_ReadClose(&file);

#if !defined(MIOS32_FAMILY_EMULATION)
  // OSC_SERVER_Init(0) has to be called after all settings have been done!
  OSC_SERVER_Init(0);
#endif

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_P] ERROR while reading file, status: %d\n", status);
#endif
    return MIDIO_FILE_P_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 MIDIO_FILE_P_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  {
    sprintf(line_buffer, "\n\n#DIN;SR.Pin;USB1;USB2;USB3;USB4;OUT1;OUT2;OUT3;OUT4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Mode;OnEv0;OnEv1;OnEv2;OffEv0;OffEv1;OffEv2\n");
    FLUSH_BUFFER;

    int din;
    midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[0];
    for(din=0; din<MIDIO_PATCH_NUM_DIN; ++din, ++din_cfg) {
      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (din_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "DIN;%d.D%d;%s;%d;0x%02X;0x%02X;0x%02X;0x%02x;0x%02x;0x%02x\n",
	      (din / 8) + 1,
	      din % 8,
	      ports_bin,
	      din_cfg->mode,
	      din_cfg->evnt0_on | 0x80, din_cfg->evnt1_on, din_cfg->evnt2_on,
	      din_cfg->evnt0_off | 0x80, din_cfg->evnt1_off, din_cfg->evnt2_off);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#DOUT;SR.Pin;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Evnt0;Evnt1\n");
    FLUSH_BUFFER;

    int dout;
    midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[0];
    for(dout=0; dout<MIDIO_PATCH_NUM_DOUT; ++dout, ++dout_cfg) {
      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (dout_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "DOUT;%d.D%d;%s;0x%02X;0x%02X\n",
	      (dout / 8) + 1,
	      7 - (dout % 8),
	      ports_bin,
	      dout_cfg->evnt0 | 0x80, dout_cfg->evnt1);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#AIN;Pin;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Evnt0;Evnt1\n");
    FLUSH_BUFFER;

    int ain;
    midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[0];
    for(ain=0; ain<MIDIO_PATCH_NUM_AIN; ++ain, ++ain_cfg) {
      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (ain_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "AIN;%d;%s;0x%02X;0x%02X\n",
	      ain,
	      ports_bin,
	      ain_cfg->evnt0 | 0x80, ain_cfg->evnt1);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#AINSER;Pin;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Evnt0;Evnt1\n");
    FLUSH_BUFFER;

    int ain;
    midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[0];
    for(ain=0; ain<MIDIO_PATCH_NUM_AINSER; ++ain, ++ain_cfg) {
      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (ain_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "AINSER;%d;%s;0x%02X;0x%02X\n",
	      ain,
	      ports_bin,
	      ain_cfg->evnt0 | 0x80, ain_cfg->evnt1);
      FLUSH_BUFFER;
    }
  }

  {
    int matrix;
    midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[0];
    for(matrix=0; matrix<MIDIO_PATCH_NUM_MATRIX; ++matrix, ++m) {
      sprintf(line_buffer, "\n\n#MATRIX;Number;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
      FLUSH_BUFFER;
      sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Mode;Chn;Arg;DIN_SR;DOUT_SR\n");
      FLUSH_BUFFER;

      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (m->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "MATRIX;%d;%s;%d;%d;0x%02X;%d;%d\n",
	      matrix+1,
	      ports_bin,
	      m->mode,
	      m->chn,
	      m->arg,
	      m->sr_din,
	      m->sr_dout);
      FLUSH_BUFFER;

      sprintf(line_buffer, "\n#MATRIX_MAP_CHN;Number;Row;C1;C2;C3;C4;C5;C6;C7;C8\n");
      FLUSH_BUFFER;
      int row;
      for(row=0; row<8; ++row) {
	int ix = row*8;

	sprintf(line_buffer, "MATRIX_MAP_CHN;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
		matrix+1,
		row+1,
		m->map_chn[ix+0],
		m->map_chn[ix+1],
		m->map_chn[ix+2],
		m->map_chn[ix+3],
		m->map_chn[ix+4],
		m->map_chn[ix+5],
		m->map_chn[ix+6],
		m->map_chn[ix+7]);
	FLUSH_BUFFER;
      }

      sprintf(line_buffer, "#MATRIX_MAP_EVNT1;Number;Row;C1;C2;C3;C4;C5;C6;C7;C8\n");
      FLUSH_BUFFER;
      for(row=0; row<8; ++row) {
	int ix = row*8;

	sprintf(line_buffer, "MATRIX_MAP_EVNT1;%d;%d;0x%02X;0x%02X;0x%02X;0x%02X;0x%02X;0x%02X;0x%02X;0x%02X\n",
		matrix+1,
		row+1,
		m->map_evnt1[ix+0],
		m->map_evnt1[ix+1],
		m->map_evnt1[ix+2],
		m->map_evnt1[ix+3],
		m->map_evnt1[ix+4],
		m->map_evnt1[ix+5],
		m->map_evnt1[ix+6],
		m->map_evnt1[ix+7]);
	FLUSH_BUFFER;
      }
    }
  }

  {
    sprintf(line_buffer, "\n\n#ROUTER;Node;SrcPort;Chn.;DstPort;Chn.\n");
    FLUSH_BUFFER;

    int node;
    midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      sprintf(line_buffer, "ROUTER;%d;0x%02X;%d;0x%02X;%d\n",
	      node+1,
	      n->src_port,
	      n->src_chn,
	      n->dst_port,
	      n->dst_chn);
      FLUSH_BUFFER;
    }
  }

#if !defined(MIOS32_FAMILY_EMULATION)
  sprintf(line_buffer, "\n\n# Ethernet Setup\n");
  FLUSH_BUFFER;
  {
    u32 value = UIP_TASK_IP_AddressGet();
    sprintf(line_buffer, "ETH_LocalIp;%d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_NetmaskGet();
    sprintf(line_buffer, "ETH_Netmask;%d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_GatewayGet();
    sprintf(line_buffer, "ETH_Gateway;%d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "ETH_Dhcp;%d\n", UIP_TASK_DHCP_EnableGet());
  FLUSH_BUFFER;

  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    u32 value = OSC_SERVER_RemoteIP_Get(con);
    sprintf(line_buffer, "OSC_RemoteIp;%d;%d.%d.%d.%d\n",
	    con,
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_RemotePort;%d;%d\n", con, OSC_SERVER_RemotePortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_LocalPort;%d;%d\n", con, OSC_SERVER_LocalPortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_TransferMode;%d;%d\n", con, OSC_CLIENT_TransferModeGet(con));
    FLUSH_BUFFER;
  }
#endif

  sprintf(line_buffer, "\n\n# Misc. Configuration\n");
  FLUSH_BUFFER;

  sprintf(line_buffer, "ForwardIO;%d\n", midio_patch_cfg.flags.FORWARD_IO);
  FLUSH_BUFFER;
  sprintf(line_buffer, "InverseDIN;%d\n", midio_patch_cfg.flags.INVERSE_DIN);
  FLUSH_BUFFER;
  sprintf(line_buffer, "InverseDOUT;%d\n", midio_patch_cfg.flags.INVERSE_DOUT);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AltProgramChange;%d\n", midio_patch_cfg.flags.ALT_PROGCHNG);
  FLUSH_BUFFER;
  sprintf(line_buffer, "DebounceCtr;%d\n", midio_patch_cfg.debounce_ctr);
  FLUSH_BUFFER;
  sprintf(line_buffer, "GlobalChannel;%d\n", midio_patch_cfg.global_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AllNotesOffChannel;%d\n", midio_patch_cfg.all_notes_off_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "MidiFilePlayMode;%d\n", SEQ_MidiPlayModeGet());
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Preset;%d\n", (int)SEQ_BPM_Get());
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Mode;%d\n", SEQ_ClockModeGet());
  FLUSH_BUFFER;

  {
    sprintf(line_buffer, "\n\n#MidiFilePlayPorts;DOUT;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4\n");
    FLUSH_BUFFER;

    char ports_bin[40];
    int bit;
    for(bit=0; bit<16; ++bit) {
      ports_bin[2*bit+0] = (seq_play_enabled_ports & (1 << bit)) ? '1' : '0';
      ports_bin[2*bit+1] = ';';
    }
    ports_bin[2*16-1] = 0; // removes also last semicolon

    sprintf(line_buffer, "MidiFilePlayPorts;%c;%s\n",
	    seq_play_enable_dout ? '1' : '0',
	    ports_bin);
    FLUSH_BUFFER;
  }

  {
    sprintf(line_buffer, "\n\n#MidiFileRecPorts;DIN;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4\n");
    FLUSH_BUFFER;

    char ports_bin[40];
    int bit;
    for(bit=0; bit<16; ++bit) {
      ports_bin[2*bit+0] = (seq_rec_enabled_ports & (1 << bit)) ? '1' : '0';
      ports_bin[2*bit+1] = ';';
    }
    ports_bin[2*16-1] = 0; // removes also last semicolon

    sprintf(line_buffer, "MidiFileRecPorts;%c;%s\n",
	    seq_rec_enable_din ? '1' : '0',
	    ports_bin);
    FLUSH_BUFFER;
  }

  {
    sprintf(line_buffer, "\n\n#MidiFileClkOutPorts;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4\n");
    FLUSH_BUFFER;

    sprintf(line_buffer, "MidiFileClkOutPorts;"); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(USB0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(USB1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(USB2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(USB3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(UART0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(UART1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(UART2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(UART3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(IIC0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(IIC1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(IIC2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(IIC3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(OSC0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(OSC1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockOutGet(OSC2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c\n", MIDI_ROUTER_MIDIClockOutGet(OSC3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
  }

  {
    sprintf(line_buffer, "\n\n#MidiFileClkInPorts;USB1;USB2;USB3;USB4;IN1;IN2;IN3;IN4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4\n");
    FLUSH_BUFFER;

    sprintf(line_buffer, "MidiFileClkInPorts;"); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(USB0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(USB1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(USB2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(USB3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(UART0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(UART1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(UART2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(UART3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(IIC0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(IIC1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(IIC2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(IIC3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(OSC0) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(OSC1) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c;", MIDI_ROUTER_MIDIClockInGet(OSC2) >= 1 ? '1' : '0'); FLUSH_BUFFER;
    sprintf(line_buffer, "%c\n", MIDI_ROUTER_MIDIClockInGet(OSC3) >= 1 ? '1' : '0'); FLUSH_BUFFER;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into patch file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Write(char *filename)
{
  midio_file_p_info_t *info = &midio_file_p_info;

  // store current file name in global variable for UI
  memcpy(midio_file_p_patch_name, filename, MIDIO_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.MIO", MIDIO_FILES_PATH, midio_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Open patch '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_P] Failed to open/create patch file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= MIDIO_FILE_P_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] patch file written with status %d\n", status);
#endif

  return (status < 0) ? MIDIO_FILE_P_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends patch data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Debug(void)
{
  return MIDIO_FILE_P_Write_Hlp(0); // send to debug terminal
}
