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
#include "app.h"

#include <string.h>
#include <limits.h>

#include <midi_router.h>

#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_patch.h"
#include "mbcv_map.h"
#include "mbcv_lre.h"

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
// reads the patch file content (again)
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_P_Read(char *filename)
{
  MbCvEnvironment* env = APP_GetEnv();
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
	} else if( strcasecmp(parameter, "ROUTER") == 0 ) {
	  s32 node;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (node=get_dec(word)) < 1 || node > MIDI_ROUTER_NUM_NODES ) {
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
		DEBUG_MSG("[MBCV_FILE_P] ERROR invalid %s number for parameter '%s %d'\n", value_name[i], parameter, node+1);
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

	} else if( strcasecmp(parameter, "CV_UpdateRateFactor") == 0 ) {
	  s32 factor;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (factor=get_dec(word)) < 1 || factor > APP_CV_UPDATE_RATE_FACTOR_MAX ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR '%s' should be between 1..%d\n", parameter, APP_CV_UPDATE_RATE_FACTOR_MAX);
#endif
	  } else {
	    APP_CvUpdateRateFactorSet(factor);
	  }

	} else if( strcasecmp(parameter, "EXTCLK_Divider") == 0 ) {
	  s32 clk;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (clk=get_dec(word)) < 1 || clk > env->mbCvClock.externalClockDivider.size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid clock number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --clk;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    s32 value;
	    if( (value=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid divider value for parameter '%s %d'\n", parameter, value);
#endif
	    } else {
	      env->mbCvClock.externalClockDivider[clk] = value;
	    }
	  }
	} else if( strcasecmp(parameter, "EXTCLK_PulseWidth") == 0 ) {
	  s32 clk;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (clk=get_dec(word)) < 1 || clk > env->mbCvClock.externalClockPulseWidth.size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid clock number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --clk;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    s32 value;
	    if( (value=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid pulsewidth value for parameter '%s %d'\n", parameter, value);
#endif
	    } else {
	      env->mbCvClock.externalClockPulseWidth[clk] = value;
	    }
	  }
	} else if( strcasecmp(parameter, "SCOPE_Source") == 0 ) {
	  s32 scope;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (scope=get_dec(word)) < 1 || scope > env->mbCvScope.size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid scope number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --scope;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    s32 value;
	    if( (value=get_dec(word)) < 0 || value >= MBCV_SCOPE_NUM_SOURCES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid source value for parameter '%s %d'\n", parameter, value);
#endif
	    } else {
	      env->mbCvScope[scope].setSource(value);
	    }
	  }
	} else if( strcasecmp(parameter, "SCOPE_Trigger") == 0 ) {
	  s32 scope;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (scope=get_dec(word)) < 1 || scope > env->mbCvScope.size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid scope number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --scope;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    s32 value;
	    if( (value=get_dec(word)) < 0 || value >= MBCV_SCOPE_NUM_TRIGGERS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid trigger value for parameter '%s %d'\n", parameter, value);
#endif
	    } else {
	      env->mbCvScope[scope].setTrigger(value);
	    }
	  }
	} else if( strcasecmp(parameter, "SCOPE_OversamplingFactor") == 0 ) {
	  s32 scope;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (scope=get_dec(word)) < 1 || scope > env->mbCvScope.size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid scope number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --scope;

	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    s32 value;
	    if( (value=get_dec(word)) < 0 || value >= 255 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid oversampling value for parameter '%s %d'\n", parameter, value);
#endif
	    } else {
	      env->mbCvScope[scope].setOversamplingFactor(value);
	    }
	  }
	} else if( strcasecmp(parameter, "ENC_Cfg") == 0 ) {
	  s32 bank;
	  char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	  if( (bank=get_dec(word)) < 1 || bank > MBCV_LRE_NUM_BANKS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid bank number for parameter '%s'\n", parameter);
#endif
	  } else {
	    // user counts from 1...
	    --bank;

	    s32 enc;
	    char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
	    if( (enc=get_dec(word)) < 1 || enc > MBCV_LRE_NUM_ENC ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_P] ERROR invalid encoder number for parameter '%s'\n", parameter);
#endif
	    } else {
	      // user counts from 1...
	      --enc;
		
	      // we have to read 3 additional values
	      int values[3];
	      const char value_name[4][10] = { "NRPN", "Min", "Max" };
	      int i;
	      for(i=0; i<3; ++i) {
		char *word = remove_quotes(strtok_r(NULL, separators, &brkt));
		if( (values[i]=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_P] ERROR invalid %s number for parameter '%s %d %d'\n", value_name[i], parameter, bank+1, enc+1);
#endif
		  break;
		}
	      }
	    
	      if( i == 3 ) {
		// finally a valid line!
		mbcv_lre_enc_cfg_t e;
		e = MBCV_LRE_EncCfgGet(enc, bank);
		e.nrpn = values[0];
		e.min = values[1];
		e.max = values[2];
		MBCV_LRE_EncCfgSet(enc, bank, e);
	      }
	    }
	  }

#if !defined(MIOS32_FAMILY_EMULATION)
	} else if( strcasecmp(parameter, "ETH_LocalIp") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_IP_AddressSet(value);
	  }
	} else if( strcasecmp(parameter, "ETH_Netmask") == 0 ) {
	  u32 value;
	  if( !(value=get_ip(brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_P] ERROR invalid IP format for parameter '%s'\n", parameter);
#endif
	  } else {
	    UIP_TASK_NetmaskSet(value);
	  }
	} else if( strcasecmp(parameter, "ETH_Gateway") == 0 ) {
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
	  } else if( strcasecmp(parameter, "ETH_Dhcp") == 0 ) {
	    UIP_TASK_DHCP_EnableSet((value >= 1) ? 1 : 0);
	  } else if( strcasecmp(parameter, "OSC_RemoteIp") == 0 ) {
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
	  } else if( strcasecmp(parameter, "OSC_RemotePort") == 0 ) {
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
	  } else if( strcasecmp(parameter, "OSC_LocalPort") == 0 ) {
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
	  } else if( strcasecmp(parameter, "OSC_TransferMode") == 0 ) {
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
  MbCvEnvironment* env = APP_GetEnv();
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  {
    sprintf(line_buffer, "\n\n# ROUTER <Node> <SrcPort> <Chn.> <DstPort> <Chn.>\n");
    FLUSH_BUFFER;

    int node;
    midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      sprintf(line_buffer, "ROUTER %d 0x%02X %d 0x%02X %d\n",
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
    sprintf(line_buffer, "ETH_LocalIp %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_NetmaskGet();
    sprintf(line_buffer, "ETH_Netmask %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  {
    u32 value = UIP_TASK_GatewayGet();
    sprintf(line_buffer, "ETH_Gateway %d.%d.%d.%d\n",
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;
  }

  sprintf(line_buffer, "ETH_Dhcp %d\n", UIP_TASK_DHCP_EnableGet());
  FLUSH_BUFFER;

  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    u32 value = OSC_SERVER_RemoteIP_Get(con);
    sprintf(line_buffer, "OSC_RemoteIp %d;%d.%d.%d.%d\n",
	    con,
	    (value >> 24) & 0xff,
	    (value >> 16) & 0xff,
	    (value >>  8) & 0xff,
	    (value >>  0) & 0xff);
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_RemotePort %d %d\n", con, OSC_SERVER_RemotePortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_LocalPort %d %d\n", con, OSC_SERVER_LocalPortGet(con));
    FLUSH_BUFFER;

    sprintf(line_buffer, "OSC_TransferMode %d %d\n", con, OSC_CLIENT_TransferModeGet(con));
    FLUSH_BUFFER;
  }
#endif

  sprintf(line_buffer, "\n\n# CV Configuration\n");
  FLUSH_BUFFER;

  sprintf(line_buffer, "\n# CV Update Rate (500 Hz * factor)\n");
  FLUSH_BUFFER;
  sprintf(line_buffer, "CV_UpdateRateFactor %d\n", APP_CvUpdateRateFactorGet());
  FLUSH_BUFFER;

  sprintf(line_buffer, "\n\n# External Clock Outputs (available at DOUT_DinSyncSR D1..D7)\n");
  FLUSH_BUFFER;
  {
    u8 *externalClockDividerPtr = env->mbCvClock.externalClockDivider.first();
    for(int clk=0; clk < env->mbCvClock.externalClockDivider.size; ++clk, ++externalClockDividerPtr) {
      sprintf(line_buffer, "EXTCLK_Divider %d %d\n", clk+1, *externalClockDividerPtr);
      FLUSH_BUFFER;
    }
  }
  {
    u8 *externalClockPulseWidthPtr = env->mbCvClock.externalClockPulseWidth.first();
    for(int clk=0; clk < env->mbCvClock.externalClockPulseWidth.size; ++clk, ++externalClockPulseWidthPtr) {
      sprintf(line_buffer, "EXTCLK_PulseWidth %d %d\n", clk+1, *externalClockPulseWidthPtr);
      FLUSH_BUFFER;
    }
  }

  sprintf(line_buffer, "\n\n# Scopes\n");
  FLUSH_BUFFER;
  {
    MbCvScope *scopePtr = env->mbCvScope.first();
    for(int scope=0; scope < env->mbCvScope.size; ++scope, ++scopePtr) {
      sprintf(line_buffer, "SCOPE_Source %d %d\n", scope+1, scopePtr->getSource());
      FLUSH_BUFFER;
      sprintf(line_buffer, "SCOPE_Trigger %d %d\n", scope+1, scopePtr->getTrigger());
      FLUSH_BUFFER;
      sprintf(line_buffer, "SCOPE_OversamplingFactor %d %d\n", scope+1, scopePtr->getOversamplingFactor());
      FLUSH_BUFFER;
    }
  }

  sprintf(line_buffer, "\n\n# LRE Encoder Assignments\n");
  FLUSH_BUFFER;
  sprintf(line_buffer, "# ENC_Cfg <bank> <enc> <nrpn> <min> <max>\n");
  FLUSH_BUFFER;
  {
    int bank;
    for(bank=0; bank<MBCV_LRE_NUM_BANKS; ++bank) {
      int enc;
      for(enc=0; enc<MBCV_LRE_NUM_ENC; ++enc) {
	mbcv_lre_enc_cfg_t e;
	e = MBCV_LRE_EncCfgGet(enc, bank);
	sprintf(line_buffer, "ENC_Cfg %d %2d 0x%04x %d %d\n", bank+1, enc+1, e.nrpn, e.min, e.max);
	FLUSH_BUFFER;
      }
    }
  }

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
