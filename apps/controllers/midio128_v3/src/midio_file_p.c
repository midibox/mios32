// $Id$
/*
 * Global Config File access functions
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

#include "file.h"
#include "midio_file.h"
#include "midio_file_p.h"
#include "midio_patch.h"

#if !defined(MIOS32_FAMILY_EMULATION)
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"
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
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Init(u32 mode)
{
  // invalidate file info
  MIDIO_FILE_P_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads global config file
// Called from MIDIO_FILE_PheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Load(void)
{
  s32 error;
  error = MIDIO_FILE_P_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Tried to open global config file, status: %d\n", error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads global config file
// Called from MIDIO_FILE_PheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Unload(void)
{
  midio_file_p_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if global config file valid
// Returns 0 if global config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Valid(void)
{
  return midio_file_p_info.valid;
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
    if( (word=strtok_r(NULL, ".", &brkt)) ) {
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
// reads the global config file content (again)
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Read(void)
{
  s32 status = 0;
  midio_file_p_info_t *info = &midio_file_p_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMIDIO_P.V3", MIDIO_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Open global config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_P] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read global config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MIDIO_FILE_P] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
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
	  char *word = strtok_r(NULL, separators, &brkt);
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
	      word = strtok_r(NULL, separators, &brkt);
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
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid port number for parameter '%s'\n", parameter);
	      } else {
		OSC_SERVER_LocalPortSet(con, value);
	      }
	    }
#if 0
	  } else if( strcmp(parameter, "OSC_TransferMode") == 0 ) {
	    if( value > OSC_SERVER_NUM_CONNECTIONS ) {
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid connection number for parameter '%s'\n", parameter);
	    } else {
	      u8 con = value;
	      word = strtok_r(NULL, separators, &brkt);
	      if( (value=get_dec(word)) < 0 ) {
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid transfer mode number for parameter '%s'\n", parameter);
	      } else {
		SEQ_MIDI_OSC_TransferModeSet(con, value);
	      }
	    }
#endif
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
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MIDIO_FILE_P] ERROR no space separator in following line: %s", line_buffer);
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

#if !defined(MIOS32_FAMILY_EMULATION)
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
    sprintf(line_buffer, "OSC_RemoteIp %d %d.%d.%d.%d\n",
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

#if 0
    sprintf(line_buffer, "OSC_TransferMode %d %d\n", con, MIDIO_MIDI_OSC_TransferModeGet(con));
    FLUSH_BUFFER;
#endif
  }
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// writes data into global config file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Write(void)
{
  midio_file_p_info_t *info = &midio_file_p_info;

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMIDIO_P.V3", MIDIO_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_P] Open global config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_P] Failed to open/create global config file, status: %d\n", status);
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
  DEBUG_MSG("[MIDIO_FILE_P] global config file written with status %d\n", status);
#endif

  return (status < 0) ? MIDIO_FILE_P_ERR_WRITE : 0;

}

/////////////////////////////////////////////////////////////////////////////
// sends global config data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_P_Debug(void)
{
  return MIDIO_FILE_P_Write_Hlp(0); // send to debug terminal
}
