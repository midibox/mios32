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
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
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
#include "mbng_file.h"
#include "mbng_file_p.h"
#include "mbng_patch.h"

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

#define MBNG_FILES_PATH "/"
//#define MBNG_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} mbng_file_p_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mbng_file_p_info_t mbng_file_p_info;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
char mbng_file_p_patch_name[MBNG_FILE_P_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_P_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads patch file
// Called from MBNG_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Load(char *filename)
{
  s32 error;
  error = MBNG_FILE_P_Read(filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_P] Tried to open patch %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads patch file
// Called from MBNG_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Unload(void)
{
  mbng_file_p_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if current patch file valid
// Returns 0 if current patch file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Valid(void)
{
  return mbng_file_p_info.valid;
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
// returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Read(char *filename)
{
  s32 status = 0;
  mbng_file_p_info_t *info = &mbng_file_p_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(mbng_file_p_patch_name, filename, MBNG_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NG1", MBNG_FILES_PATH, mbng_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_P] Open patch '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_P] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read patch values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MBNG_FILE_P] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
      char *brkt;
      char *parameter;

      if( (parameter = remove_quotes(strtok_r(line_buffer, separators, &brkt))) ) {

	if( *parameter == 0 || *parameter == '#' ) {
	  // ignore comments and empty lines
	} else if( strcmp(parameter, "ENC") == 0 ) {
	  s32 enc;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (enc=get_dec(word)) < 1 || enc >= MIOS32_ENC_NUM_MAX ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid encoder number for parameter '%s %d' (1..%d)\n", parameter, enc, MIOS32_ENC_NUM_MAX-1);
#endif
	  } else {
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int sr;
	    if( (sr=get_dec(word)) < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number %d for parameter '%s %d' (1..%d)\n", sr, parameter, enc, MIOS32_SRIO_NUM_SR);
#endif
	    } else {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int pin;
	      if( (pin=get_dec(word)) < 0 || pin > 8 || (pin&1) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid DIN pin number %d for parameter '%s %d' (0,2,4,6)\n", pin, parameter, enc);
#endif
	      } else {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		mios32_enc_type_t enc_type = DISABLED;
		if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_P] ERROR missing encoder type for parameter '%s %d' (0,2,4,6)\n", parameter, enc);
#endif
		  continue;
		} else if( strcmp(word, "DISABLED") == 0 ) {
		  enc_type = DISABLED;
		} else if( strcmp(word, "NON_DETENTED") == 0 ) {
		  enc_type = NON_DETENTED;
		} else if( strcmp(word, "DETENTED1") == 0 ) {
		  enc_type = DETENTED1;
		} else if( strcmp(word, "DETENTED2") == 0 ) {
		  enc_type = DETENTED2;
		} else if( strcmp(word, "DETENTED3") == 0 ) {
		  enc_type = DETENTED3;
		} else if( strcmp(word, "DETENTED4") == 0 ) {
		  enc_type = DETENTED4;
		} else if( strcmp(word, "DETENTED5") == 0 ) {
		  enc_type = DETENTED5;
		} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_P] ERROR invalid encoder type '%s' for parameter '%s %d' (0,2,4,6)\n", word, parameter, enc);
#endif
		  continue;
		}

		mios32_enc_config_t enc_config = {
		  .cfg.type=enc_type,
		  .cfg.speed=NORMAL,
		  .cfg.speed_par=0,
		  .cfg.sr=sr,
		  .cfg.pos=pin
		};

		MIOS32_ENC_ConfigSet(enc, enc_config); // counting from 1, because SCS menu encoder is assigned to 0
	      }
	    }
	  }
	} else if( strcmp(parameter, "MATRIX") == 0 ) {
	  s32 matrix;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (matrix=get_dec(word)) < 1 || matrix > MBNG_PATCH_NUM_MATRIX  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid matrix number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --matrix;

	    // we have to read 2 additional values
	    int values[2];
	    const char value_name[2][10] = { "SR_DIN", "SR_DOUT" };
	    int i;
	    for(i=0; i<2; ++i) {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (values[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, matrix);
#endif
		break;
	      }
	    }

	    if( i == 2 ) {
	      // finally a valid line!
	      mbng_patch_matrix_entry_t *m = (mbng_patch_matrix_entry_t *)&mbng_patch_matrix[matrix];
	      m->sr_din = values[0];
	      m->sr_dout = values[1];
	    }
	  }
	} else if( strcmp(parameter, "ROUTER") == 0 ) {
	  s32 node;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (node=get_dec(word)) < 1 || node > MIDI_ROUTER_NUM_NODES  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid node number for parameter '%s'\n", parameter);
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
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, node);
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
	} else if( strcmp(parameter, "DebounceCtr") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    mbng_patch_cfg.debounce_ctr = value;
	  }
	} else if( strcmp(parameter, "GlobalChannel") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    mbng_patch_cfg.global_chn = value;
	  }
	} else if( strcmp(parameter, "AllNotesOffChannel") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    mbng_patch_cfg.all_notes_off_chn = value;
	  }

	} else if( strcmp(parameter, "BPM_Preset") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid BPM for parameter '%s'\n", parameter);
#endif
	  }
	  SEQ_BPM_Set((float)value);

	} else if( strcmp(parameter, "BPM_Mode") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 || SEQ_BPM_ModeSet(value) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid BPM Mode for parameter '%s'\n", parameter);
#endif
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
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
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
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid MIDI port format for parameter '%s'\n", parameter);
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
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_IP_AddressSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Netmask") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_NetmaskSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Gateway") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_GatewaySet(value);
	  }
	} else {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_P] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "ETH_Dhcp") == 0 ) {
	    UIP_TASK_DHCP_EnableSet((value >= 1) ? 1 : 0);
	  } else if( strcmp(parameter, "OSC_RemoteIp") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBNG_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      u32 ip;
	      if( !(ip=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	      } else {
		OSC_SERVER_RemoteIP_Set(con, ip);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_RemotePort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBNG_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_RemotePortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_LocalPort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBNG_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_LocalPortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_TransferMode") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBNG_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBNG_FILE_P] ERROR invalid transfer mode number for parameter '%s'\n", parameter);
	      } else {
		OSC_CLIENT_TransferModeSet(con, value);
	      }
	    }
#endif /* !defined(MIOS32_FAMILY_EMULATION) */
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    // changed error level from 1 to 2 here, since people are sometimes confused about these messages
	    // on file format changes
	    DEBUG_MSG("[MBNG_FILE_P] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MBNG_FILE_P] ERROR no space or semicolon separator in following line: %s", line_buffer);
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
    DEBUG_MSG("[MBNG_FILE_P] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_P_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_FILE_P_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  {
    sprintf(line_buffer, "\n\n#ENC;SR;Pin;Type\n");
    FLUSH_BUFFER;

    int enc;
    for(enc=1; enc<MIOS32_ENC_NUM_MAX; ++enc) {
      mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);

      char enc_type[20];
      switch( enc_config.cfg.type ) { // see mios32_enc.h for available types
      case 0xff: strcpy(enc_type, "NON_DETENTED"); break;
      case 0xaa: strcpy(enc_type, "DETENTED1"); break;
      case 0x22: strcpy(enc_type, "DETENTED2"); break;
      case 0x88: strcpy(enc_type, "DETENTED3"); break;
      case 0xa5: strcpy(enc_type, "DETENTED4"); break;
      case 0x5a: strcpy(enc_type, "DETENTED5"); break;
      default: strcpy(enc_type, "DISABLED"); break;
      }

      sprintf(line_buffer, "ENC;%d;%d;%d;%s\n",
	      enc,
	      enc_config.cfg.sr,
	      enc_config.cfg.pos,
	      enc_type);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#MATRIX;DIN_SR;DOUT_SR\n");
    FLUSH_BUFFER;

    int matrix;
    mbng_patch_matrix_entry_t *m = (mbng_patch_matrix_entry_t *)&mbng_patch_matrix[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX; ++matrix, ++m) {

      sprintf(line_buffer, "MATRIX;%d;%d;%d\n",
	      matrix+1,
	      m->sr_din,
	      m->sr_dout);
      FLUSH_BUFFER;
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

  sprintf(line_buffer, "DebounceCtr;%d\n", mbng_patch_cfg.debounce_ctr);
  FLUSH_BUFFER;
  sprintf(line_buffer, "GlobalChannel;%d\n", mbng_patch_cfg.global_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AllNotesOffChannel;%d\n", mbng_patch_cfg.all_notes_off_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Preset;%d\n", (int)SEQ_BPM_Get());
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Mode;%d\n", SEQ_BPM_ModeGet());
  FLUSH_BUFFER;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into patch file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Write(char *filename)
{
  mbng_file_p_info_t *info = &mbng_file_p_info;

  // store current file name in global variable for UI
  memcpy(mbng_file_p_patch_name, filename, MBNG_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NG1", MBNG_FILES_PATH, mbng_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_P] Open patch '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] Failed to open/create patch file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= MBNG_FILE_P_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_P] patch file written with status %d\n", status);
#endif

  return (status < 0) ? MBNG_FILE_P_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends patch data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Debug(void)
{
  return MBNG_FILE_P_Write_Hlp(0); // send to debug terminal
}
