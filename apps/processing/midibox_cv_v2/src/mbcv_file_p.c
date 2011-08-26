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
#include <limits.h>

#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_patch.h"
#include "mbcv_map.h"

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

#define MBCV_FILES_PATH "/"
//#define MBCV_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} mbcv_file_p_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mbcv_file_p_info_t mbcv_file_p_info;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
char mbcv_file_p_patch_name[MBCV_FILE_P_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Init(u32 mode)
{
  // invalidate file info
  MBCV_FILE_P_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads patch file
// Called from MBCV_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Load(char *filename)
{
  s32 error;
  error = MBCV_FILE_P_Read(filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_P] Tried to open patch %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads patch file
// Called from MBCV_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Unload(void)
{
  mbcv_file_p_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if current patch file valid
// Returns 0 if current patch file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Valid(void)
{
  return mbcv_file_p_info.valid;
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
// help function which parses an integer value
// returns != -INT_MIN if value is valid
/////////////////////////////////////////////////////////////////////////////
static s32 get_int(char *word)
{
  if( word == NULL )
    return INT_MIN;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return INT_MIN;

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
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Read(char *filename)
{
  s32 status = 0;
  mbcv_file_p_info_t *info = &mbcv_file_p_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(mbcv_file_p_patch_name, filename, MBCV_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.CV2", MBCV_FILES_PATH, mbcv_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_P] Open patch '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_P] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read patch values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MBCV_FILE_P] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
      char *brkt;
      char *parameter;

      if( (parameter = remove_quotes(strtok_r(line_buffer, separators, &brkt))) ) {

	if( *parameter == 0 || *parameter == '#' ) {
	  // ignore comments and empty lines
	} else if( strcmp(parameter, "CV") == 0 ) {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  int cv;
	  if( (cv=get_dec(word)) < 1 || cv > MBCV_PATCH_NUM_CV ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid CV channel number parameter '%s'\n", parameter);
#endif
	  } else {
	    --cv; // user counts from 1

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
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid MIDI port format for parameter '%s %d'\n", parameter, cv+1);
#endif
	    } else {
	      int values[10];
	      int i;
	      for(i=0; i<11; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		u8 failed = 0;
		if( i == 8 || i == 9 )
		  failed = (values[i]=get_int(word)) == INT_MIN;
		else
		  failed = (values[i]=get_dec(word)) < 0;
		if( failed ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_P] ERROR invalid value format for parameter '%s %d'\n", parameter, cv+1);
#endif
		  break;
		}
	      }

	      if( i == 11 ) {
		// finally a valid line!
		mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[cv];
		cv_cfg->enabled_ports = enabled_ports;
		cv_cfg->chn = values[0];
		cv_cfg->midi_mode.event = values[1];
		cv_cfg->midi_mode.LEGATO = values[2] ? 1 : 0;
		cv_cfg->midi_mode.POLY = values[3] ? 1 : 0;
		if( values[4] )
		  mbcv_patch_gate_inverted[cv>>3] |= (1 << (cv&7));
		else
		  mbcv_patch_gate_inverted[cv>>3] &= ~(1 << (cv&7));
		MBCV_MAP_PitchRangeSet(cv, values[5]);
		cv_cfg->split_l = values[6];
		cv_cfg->split_u = values[7];
		cv_cfg->transpose_oct = values[8];
		cv_cfg->transpose_semi = values[9];
		cv_cfg->cc_number = values[10];
	      }
	    }
	  }
	} else if( strcmp(parameter, "ROUTER") == 0 ) {
	  s32 node;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (node=get_dec(word)) < 1 || node > MBCV_PATCH_NUM_ROUTER  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid node number for parameter '%s'\n", parameter);
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
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, node);
#endif
		break;
	      }
	    }
	    
	    if( i == 4 ) {
	      // finally a valid line!
	      mbcv_patch_router_entry_t *n = (mbcv_patch_router_entry_t *)&mbcv_patch_router[node];
	      n->src_port = values[0];
	      n->src_chn = values[1];
	      n->dst_port = values[2];
	      n->dst_chn = values[3];
	    }
	  }

	} else if( strcmp(parameter, "AOUT_Type") == 0 ) {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  int aout_type;
	  for(aout_type=0; aout_type<AOUT_NUM_IF; ++aout_type) {
	    if( strcmp(word, MBCV_MAP_IfNameGet(aout_type)) == 0 )
	      break;
	  }

	  if( aout_type == AOUT_NUM_IF ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid AOUT interface name for parameter '%s': %s\n", parameter, word);
#endif
	  } else {
	    MBCV_MAP_IfSet(aout_type);
	  }
	} else if( strcmp(parameter, "AOUT_PinMode") == 0 ) {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  int cv;
	  if( (cv=get_dec(word)) < 1 || cv > MBCV_PATCH_NUM_CV ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid CV channel number parameter '%s'\n", parameter);
#endif
	  } else {
	    --cv; // user counts from 1

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    int curve;
	    if( (curve=get_dec(word)) < 0 || curve >= MBCV_MAP_NUM_CURVES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR wrong curve %d for parameter '%s', CV channel %d\n", curve, parameter, cv);
#endif
	    } else {
	      char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      int slewrate;
	      if( (slewrate=get_dec(word)) < 0 || slewrate >= MBCV_MAP_NUM_CURVES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBCV_FILE_P] ERROR wrong slewrate %d for parameter '%s', CV channel %d\n", curve, slewrate, cv);
#endif
	      } else {
		MBCV_MAP_CurveSet(cv, curve);
		MBCV_MAP_SlewRateSet(cv, slewrate);
	      }
	    }
	  }
	} else if( strcmp(parameter, "ExtClkPulsewidth") == 0 ) {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  int pulsewidth;
	  if( (pulsewidth=get_dec(word)) < 0 || pulsewidth > 255 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid pulsewidth %d for parameter '%s'\n", pulsewidth, parameter);
#endif
	  } else {
	    mbcv_patch_cfg.ext_clk_pulsewidth = pulsewidth;
	  }
	} else if( strcmp(parameter, "ExtClkDivider") == 0 ) {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  int clkdiv;
	  if( (clkdiv=get_dec(word)) < 0 || clkdiv > 255 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid clockdivider %d for parameter '%s'\n", clkdiv, parameter);
#endif
	  } else {
	    mbcv_patch_cfg.ext_clk_divider = clkdiv;
	  }

#if !defined(MIOS32_FAMILY_EMULATION)
	} else if( strcmp(parameter, "ETH_LocalIp") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_IP_AddressSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Netmask") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_NetmaskSet(value);
	  }
	} else if( strcmp(parameter, "ETH_Gateway") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_GatewaySet(value);
	  }
	} else {
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "ETH_Dhcp") == 0 ) {
	    UIP_TASK_DHCP_EnableSet((value >= 1) ? 1 : 0);
	  } else if( strcmp(parameter, "OSC_RemoteIp") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      u32 ip;
	      if( !(ip=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	      } else {
		OSC_SERVER_RemoteIP_Set(con, ip);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_RemotePort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_RemotePortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_LocalPort") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_LocalPortSet(con, value);
	      }
	    }
	  } else if( strcmp(parameter, "OSC_TransferMode") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = remove_quotes(strtok_r(NULL, separators, &brkt));
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid transfer mode number for parameter '%s'\n", parameter);
	      } else {
		OSC_CLIENT_TransferModeSet(con, value);
	      }
	    }
#endif /* !defined(MIOS32_FAMILY_EMULATION) */
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    // changed error level from 1 to 2 here, since people are sometimes confused about these messages
	    // on file format changes
	    DEBUG_MSG("[MBCV_FILE_P] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MBCV_FILE_P] ERROR no space or semicolon separator in following line: %s", line_buffer);
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
    DEBUG_MSG("[MBCV_FILE_P] ERROR while reading file, status: %d\n", status);
#endif
    return MBCV_FILE_P_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 MBCV_FILE_P_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  {
    sprintf(line_buffer, "#CV;Pin;USB1;USB2;USB3;USB4;OUT1;OUT2;OUT3;OUT4;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "RES1;RES2;RES3;RES4;OSC1;OSC2;OSC3;OSC4;Chn;Event;");
    FLUSH_BUFFER;
    sprintf(line_buffer, "Legato;Poly;InvGate;PitchRng;SplitL;SplitU;TrOctave;TrSemitones;CCNumber\n");
    FLUSH_BUFFER;

    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      char ports_bin[40];
      int bit;
      for(bit=0; bit<16; ++bit) {
	ports_bin[2*bit+0] = (cv_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
	ports_bin[2*bit+1] = ';';
      }
      ports_bin[2*16-1] = 0; // removes also last semicolon

      sprintf(line_buffer, "CV;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
	      cv+1,
	      ports_bin,
	      cv_cfg->chn,
	      cv_cfg->midi_mode.event,
	      cv_cfg->midi_mode.LEGATO,
	      cv_cfg->midi_mode.POLY,
	      (mbcv_patch_gate_inverted[cv>>3] & (1 << (cv&7))) ? 1 : 0,
	      MBCV_MAP_PitchRangeGet(cv),
	      cv_cfg->split_l,
	      cv_cfg->split_u,
	      cv_cfg->transpose_oct,
	      cv_cfg->transpose_semi,
	      cv_cfg->cc_number);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#PinMode;Curve;SlewRate\n");
    FLUSH_BUFFER;

    int cv;
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv) {
      sprintf(line_buffer, "AOUT_PinMode;%d;%d;%d\n", cv+1, MBCV_MAP_CurveGet(cv), (int)MBCV_MAP_SlewRateGet(cv));
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n#ROUTER;Node;SrcPort;Chn.;DstPort;Chn.\n");
    FLUSH_BUFFER;

    int node;
    mbcv_patch_router_entry_t *n = (mbcv_patch_router_entry_t *)&mbcv_patch_router[0];
    for(node=0; node<MBCV_PATCH_NUM_ROUTER; ++node, ++n) {
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

  sprintf(line_buffer, "AOUT_Type;%s\n", (char *)MBCV_MAP_IfNameGet(MBCV_MAP_IfGet()));
  FLUSH_BUFFER;

  sprintf(line_buffer, "ExtClkPulsewidth %d\n", mbcv_patch_cfg.ext_clk_pulsewidth);
  FLUSH_BUFFER;

  sprintf(line_buffer, "ExtClkDivider %d\n", mbcv_patch_cfg.ext_clk_divider);
  FLUSH_BUFFER;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into patch file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Write(char *filename)
{
  mbcv_file_p_info_t *info = &mbcv_file_p_info;

  // store current file name in global variable for UI
  memcpy(mbcv_file_p_patch_name, filename, MBCV_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.CV2", MBCV_FILES_PATH, mbcv_file_p_patch_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_P] Open patch '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_P] Failed to open/create patch file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= MBCV_FILE_P_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();


  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_P] patch file written with status %d\n", status);
#endif

  return (status < 0) ? MBCV_FILE_P_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends patch data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Debug(void)
{
  return MBCV_FILE_P_Write_Hlp(0); // send to debug terminal
}
