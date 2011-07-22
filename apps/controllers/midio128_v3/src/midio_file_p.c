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
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
	} else if( strcmp(parameter, "DIN") == 0 ) {
	  s32 sr;
	  char *word = strtok_r(NULL, separators, &brkt);
	  if( (sr=get_sr(word)) < 0 || sr >= MIDIO_PATCH_NUM_DIN  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid SR.Pin format for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports;
	    char *word = strtok_r(NULL, separators, &brkt);
	    if( (enabled_ports=get_bin(word, 16)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, sr%8);
#endif
	    } else {
	      s32 mode;
	      char *word = strtok_r(NULL, separators, &brkt);
	      if( (mode=get_dec(word)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid Mode format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, sr%8);
#endif
	      } else {
		u8 events[6];
		int i;
		for(i=0; i<6; ++i) {
		  char *word = strtok_r(NULL, separators, &brkt);
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
	  char *word = strtok_r(NULL, separators, &brkt);
	  if( (sr=get_sr(word)) < 0 || sr >= MIDIO_PATCH_NUM_DIN  ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid SR.Pin format for parameter '%s'\n", parameter);
#endif
	  } else {
	    s32 enabled_ports;
	    char *word = strtok_r(NULL, separators, &brkt);
	    if( (enabled_ports=get_bin(word, 16)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid MIDI port format for parameter '%s %2d.D%d'\n", parameter, (sr/8)+1, 7-(sr%8));
#endif
	    } else {
	      u8 events[2];
	      int i;
	      for(i=0; i<2; ++i) {
		char *word = strtok_r(NULL, separators, &brkt);
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
	} else if( strcmp(parameter, "MergerMode") == 0 ) {
	  u32 value;
	  if( (value=get_dec(brkt)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MIDIO_FILE_P] ERROR invalid format for parameter '%s'\n", parameter);
#endif
	  } else {
	    midio_patch_cfg.flags.MERGER_MODE = value;
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

  {
    //    SR/Pin MIDI OUT ports   mode     ON Event        OFF event
    // DIN  1.D0 1111111111111111   0   0x90 0x30 0x7F  0x90 0x30 0x00
    sprintf(line_buffer, "\n\n#  SR/Pin MIDI OUT ports   mode     ON Event        OFF event\n");
    FLUSH_BUFFER;

    int din;
    midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[0];
    for(din=0; din<MIDIO_PATCH_NUM_DIN; ++din, ++din_cfg) {
      char ports_bin[17];
      int bit;
      for(bit=0; bit<16; ++bit)
	ports_bin[bit] = (din_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
      ports_bin[16] = 0;

      sprintf(line_buffer, "DIN %2d.D%d %s  %2d   0x%02X 0x%02X 0x%02X  0x%02x 0x%02x 0x%02x\n",
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
    //     SR/Pin MIDI IN ports       Event
    // DOUT  1.D7 1111111111111111  0x90 0x30
    sprintf(line_buffer, "\n\n#   SR/Pin MIDI IN ports       Event\n");
    FLUSH_BUFFER;

    int dout;
    midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[0];
    for(dout=0; dout<MIDIO_PATCH_NUM_DOUT; ++dout, ++dout_cfg) {
      char ports_bin[17];
      int bit;
      for(bit=0; bit<16; ++bit)
	ports_bin[bit] = (dout_cfg->enabled_ports & (1 << bit)) ? '1' : '0';
      ports_bin[16] = 0;

      sprintf(line_buffer, "DOUT %2d.D%d %s  0x%02X 0x%02X\n",
	      (dout / 8) + 1,
	      7 - (dout % 8),
	      ports_bin,
	      dout_cfg->evnt0 | 0x80, dout_cfg->evnt1);
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

  sprintf(line_buffer, "\n\n# Misc. Configuration\n");
  FLUSH_BUFFER;

  sprintf(line_buffer, "MergerMode %d\n", midio_patch_cfg.flags.MERGER_MODE);
  FLUSH_BUFFER;
  sprintf(line_buffer, "ForwardIO %d\n", midio_patch_cfg.flags.FORWARD_IO);
  FLUSH_BUFFER;
  sprintf(line_buffer, "InverseDIN %d\n", midio_patch_cfg.flags.INVERSE_DIN);
  FLUSH_BUFFER;
  sprintf(line_buffer, "InverseDOUT %d\n", midio_patch_cfg.flags.INVERSE_DOUT);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AltProgramChange %d\n", midio_patch_cfg.flags.ALT_PROGCHNG);
  FLUSH_BUFFER;
  sprintf(line_buffer, "DebounceCtr %d\n", midio_patch_cfg.debounce_ctr);
  FLUSH_BUFFER;
  sprintf(line_buffer, "GlobalChannel %d\n", midio_patch_cfg.global_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AllNotesOffChannel %d\n", midio_patch_cfg.all_notes_off_chn);
  FLUSH_BUFFER;


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
