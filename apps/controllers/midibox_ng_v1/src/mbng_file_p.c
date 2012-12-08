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
#include "mbng_event.h"

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
// returns -1000000000 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1000000000;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1000000000;

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
// help function which parses a quoted string
// returns >= 0 if string is valid
// returns < 0 if string is invalid
/////////////////////////////////////////////////////////////////////////////
static char *getQuotedString(char **brkt)
{
  char *value_str;

  if( *brkt == NULL )
    return NULL;

  value_str = *brkt;
  {
    int quote_started = 0;

    char *search_str = *brkt;
    for(; *search_str != 0 && (*search_str == ' ' || *search_str == '\t'); ++search_str);

    if( *search_str == '\'' || *search_str == '"' ) {
      quote_started = 1;
      ++search_str;
    }

    value_str = search_str;

    if( quote_started ) {
      for(; *search_str != 0 && *search_str != '\'' && *search_str != '\"'; ++search_str);
    } else {
      for(; *search_str != 0 && *search_str != ' ' && *search_str != '\t'; ++search_str);
    }

    if( *search_str != 0 ) {
      *search_str = 0;
      ++search_str;
    }

    *brkt = search_str;
  }

  // end of command line reached?
  if( (*brkt)[0] == 0 )
    *brkt = NULL;

  return value_str[0] ? value_str : NULL;
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses an extended parameter with the syntax
// 'parameter=value_str'
// returns >= 0 if parameter is valid
// returns <0 if parameter is invalid or no more parameters found
/////////////////////////////////////////////////////////////////////////////
static s32 parseExtendedParameter(char *cmd, char **parameter, char **value_str, char **brkt)
{
  const char *separators = " \t";

  // parameter name + '='
  if( !(*parameter = strtok_r(NULL, separators, brkt)) ) {
    return -1; // no additional parameter
  }

  // store this information, because it stores the information that the end of the command line has been reached
  u8 brkt_was_NULL = *brkt == NULL;

  // since strstr() doesn't work for unknown reasons (maybe newlib issue)
  u8 eq_found = 0;
  char *search_str = *parameter;

  for(; *search_str != 0 && !eq_found; ++search_str) {
    if( *search_str == '=' ) {
      eq_found = 1;
      *search_str = 0;
      if( search_str[1] != 0 ) {
	*brkt = (char *)&search_str[1];
	(*brkt)[strlen(*brkt)] = brkt_was_NULL ? 0 : ' ';
      }
    }
  }

  // if '=' wasn't found, check if there are spaces between
  if( !eq_found && *brkt != NULL ) {
    // since strstr() doesn't work for unknown reasons (maybe newlib issue)
    char *search_str = *brkt;
    for(; *search_str != 0 && !eq_found; ++search_str) {
      if( *search_str == '=' ) {
	eq_found = 1;
	*brkt = (char *)&search_str[1];
      }
    }
  }

  if( *brkt == NULL || (*brkt)[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] ERROR: missing value after %s ... %s\n", cmd, *parameter);
#endif
    return -2; // missing value
  } else if( !eq_found ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] ERROR: missing '=' after %s ... %s\n", cmd, *parameter);
#endif
    return -1;
  }

  // we can't use strtok_r here, since we have to consider quotes...
  *value_str = getQuotedString(brkt);

  // end of command line reached?
  if( brkt_was_NULL || (*brkt)[0] == 0 )
    *brkt = NULL;

  if( *value_str == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] ERROR: missing value after %s ... %s\n", cmd, *parameter);
#endif
    return -2; // missing value
  }

#if 0
  DEBUG_MSG("%s -> '%s'\n", *parameter, *value_str);
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses a EVENT definitions and adds them to the pool
// returns >= 0 if command is valid
// returns <0 if command is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseEvent(char *cmd, char *brkt)
{
  mbng_event_item_t item;
  char *event = (char *)&cmd[6]; // remove "EVENT_"

  MBNG_EVENT_ItemInit(&item);
  item.id = MBNG_EVENT_ItemIdFromControllerStrGet(event);

  // extra: if button, invert it by default to avoid confusion if inverted=1 not set (DINs are low-active)
  if( item.id == MBNG_EVENT_CONTROLLER_BUTTON || item.id == MBNG_EVENT_CONTROLLER_BUTTON_MATRIX )
      item.flags.DIN.inverse = 1;

#define STREAM_MAX_SIZE 128
  u8 stream[STREAM_MAX_SIZE];
  item.stream = stream;
  item.stream_size = 0;

#define LABEL_MAX_SIZE 41
  char label[LABEL_MAX_SIZE];
  item.label = label;
  label[0] = 0;

  if( (item.id & 0xf000) == MBNG_EVENT_CONTROLLER_DISABLED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] ERROR: EVENT_%s item not supported!\n", event);
#endif
    return -1;
  }

  // parse the parameters
  char *parameter;
  char *value_str;
  while( parseExtendedParameter(cmd, &parameter, &value_str, &brkt) >= 0 ) { 
    const char *separator_colon = ":";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    if( strcasecmp(parameter, "id") == 0 ) {
      int id;
      if( (id=get_dec(value_str)) < 0 || id > 0xfff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid ID in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	item.id = (item.id & 0xf000) | id;
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "id") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 0 || value > 0xfff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid ID in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	item.id = (item.id & 0xf000) | value;
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "type") == 0 ) {
      mbng_event_type_t event_type = MBNG_EVENT_ItemTypeFromStrGet(value_str);
      if( event_type == -1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid event type in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	item.flags.general.type = event_type;

	switch( event_type ) {
	case MBNG_EVENT_TYPE_NOTE_OFF:
	case MBNG_EVENT_TYPE_NOTE_ON:
	case MBNG_EVENT_TYPE_POLY_PRESSURE:
	case MBNG_EVENT_TYPE_CC: {
	  item.stream_size = 2;
	  item.stream[0] = 0x80 | (event_type << 4);
	  item.stream[1] = 0x30;
	} break;

	case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
	case MBNG_EVENT_TYPE_AFTERTOUCH:
	case MBNG_EVENT_TYPE_PITCHBEND: {
	  item.stream_size = 1;
	  item.stream[0] = 0x80 | (event_type << 4);
	} break;

	case MBNG_EVENT_TYPE_SYSEX: {
	  item.stream_size = 2;
	  item.stream[0] = 0x00; // SysEx type
	  item.stream[1] = 0x00; // Value Pos
	} break;

	case MBNG_EVENT_TYPE_RPN:
	case MBNG_EVENT_TYPE_NRPN: {
	  item.stream_size = 3;
	  item.stream[0] = 0x00; // number
	  item.stream[1] = 0x00;
	  item.stream[2] = 0x00; // value format
	} break;

	case MBNG_EVENT_TYPE_META: {
	  item.stream_size = 2;
	  item.stream[0] = 0x00; // Meta Number
	  item.stream[1] = 0x00;
	} break;

	default:
	  item.stream_size = 0;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "chn") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 1 || value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid MIDI channel in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( item.flags.general.type == MBNG_EVENT_TYPE_SYSEX || item.flags.general.type == MBNG_EVENT_TYPE_META ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] WARNING: no MIDI channel expected for EVENT_%s due to type: %s\n", event, MBNG_EVENT_ItemTypeStrGet(&item));
#endif
	} else {
	  // no extra check if event_type already defined...
	  stream[0] = (stream[0] & 0xf0) | (value-1);
	}
      }

    } else if( strcasecmp(parameter, "key") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 0 || value > 127 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid key number in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( item.flags.general.type != MBNG_EVENT_TYPE_NOTE_OFF &&
	    item.flags.general.type != MBNG_EVENT_TYPE_NOTE_ON &&
	    item.flags.general.type != MBNG_EVENT_TYPE_POLY_PRESSURE ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] WARNING: no key number expected for EVENT_%s due to type: %s\n", event, MBNG_EVENT_ItemTypeStrGet(&item));
#endif
	} else {
	  // no extra check if event_type already defined...
	  stream[1] = value;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "cc") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 0 || value > 127 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid CC number in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( item.flags.general.type != MBNG_EVENT_TYPE_CC ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] WARNING: no CC number expected for EVENT_%s due to type: %s\n", event, MBNG_EVENT_ItemTypeStrGet(&item));
#endif
	} else {
	  // no extra check if event_type already defined...
	  stream[1] = value;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "rpn") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 0 || value >= 16384 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid RPN number in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( item.flags.general.type != MBNG_EVENT_TYPE_RPN ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] WARNING: no RPN number expected for EVENT_%s due to type: %s\n", event, MBNG_EVENT_ItemTypeStrGet(&item));
#endif
	} else {
	  // no extra check if event_type already defined...
	  stream[0] = value & 0xff;
	  stream[1] = value >> 8;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "rpn_format") == 0 ) {
      mbng_event_nrpn_format_t nrpn_format = MBNG_EVENT_ItemNrpnFormatFromStrGet(value_str);
      if( nrpn_format == -1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid RPN format in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	// no extra check if event_type already defined...
	stream[2] = nrpn_format;
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "nrpn") == 0 ) {
      int value;
      if( (value=get_dec(value_str)) < 0 || value >= 16384 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid NRPN number in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( item.flags.general.type != MBNG_EVENT_TYPE_NRPN ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] WARNING: no NRPN number expected for EVENT_%s due to type: %s\n", event, MBNG_EVENT_ItemTypeStrGet(&item));
#endif
	} else {
	  // no extra check if event_type already defined...
	  stream[0] = value & 0xff;
	  stream[1] = value >> 8;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "nrpn_format") == 0 ) {
      mbng_event_nrpn_format_t nrpn_format = MBNG_EVENT_ItemNrpnFormatFromStrGet(value_str);
      if( nrpn_format == -1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid NRPN format in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	// no extra check if event_type already defined...
	stream[2] = nrpn_format;
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "range") == 0 ) {
      char *values_str = value_str;
      char *brkt_local;
      int values[2];
      if( !(values_str = strtok_r(value_str, separator_colon, &brkt_local)) ||
	  (values[0]=get_dec(values_str)) <= -1000000000 ||
	  !(values_str = strtok_r(NULL, separator_colon, &brkt_local)) ||
	  (values[1]=get_dec(values_str)) <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid range format in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( values[0] < -16384 || values[0] > 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid min value in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	} else if( values[1] < -16384 || values[1] > 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid max value in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	} else {
	  item.min = values[0];
	  item.max = values[1];
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "ports") == 0 ) {
      int value;
      if( (value=get_bin(value_str, 16)) < 0 || value > 0xffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid port mask in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	item.enabled_ports = value;
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "lcd_pos") == 0 ) {
      char *values_str = value_str;
      char *brkt_local;
      int values[3];
      if( !(values_str = strtok_r(value_str, separator_colon, &brkt_local)) ||
	  (values[0]=get_dec(values_str)) < 0 ||
	  !(values_str = strtok_r(NULL, separator_colon, &brkt_local)) ||
	  (values[1]=get_dec(values_str)) < 0 ||
	  !(values_str = strtok_r(NULL, separator_colon, &brkt_local)) ||
	  (values[2]=get_dec(values_str)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid LCD position format in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	if( values[0] < 1 || values[0] >= 256 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid LCD number (first item) in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	} else if( values[1] < 1 || values[1] >= 64 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid LCD X position (second item) in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	} else if( values[2] < 1 || values[2] >= 4 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid LCD Y position (third item) in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	} else {
	  item.lcd = values[0] - 1;
	  item.lcd_pos = ((values[1]-1) & 0x3f) | ((values[2]-1) & 0x3) << 6;
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "label") == 0 ) {
      if( strlen(value_str) >= LABEL_MAX_SIZE ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: string to long in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
	return -1;
      } else {
	strcpy(item.label, value_str);
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_P] WARNING: unsupported parameter in EVENT_%s ... %s=%s\n", event, parameter, value_str);
#endif
      // just continue to keep files compatible
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  MBNG_EVENT_ItemPrint(&item);
#endif

  if( MBNG_EVENT_ItemAdd(&item) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_P] ERROR: couldn't add EVENT_%s ... id=%d: out of memory!\n", event, item.id & 0xfff);
#endif
    return -2;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses ENC definitions
// returns >= 0 if command is valid
// returns <0 if command is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseEnc(char *cmd, char *brkt)
{
  // parse the parameters
  int num = 0;
  int sr = 0;
  int pin_a = 0;
  mios32_enc_type_t enc_type = DISABLED;

  char *parameter;
  char *value_str;
  while( parseExtendedParameter(cmd, &parameter, &value_str, &brkt) >= 0 ) { 
    const char *separator_colon = ":";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    if( strcasecmp(parameter, "n") == 0 ) {
      if( (num=get_dec(value_str)) < 1 || num > MIOS32_ENC_NUM_MAX ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid encoder number for %s ... %s=%s' (1..%d)\n", cmd, parameter, value_str, MIOS32_ENC_NUM_MAX-1);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr") == 0 ) {
      if( (sr=get_dec(value_str)) < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "pins") == 0 ) {
      char *values_str = value_str;
      char *brkt_local;
      int values[2];
      if( !(values_str = strtok_r(value_str, separator_colon, &brkt_local)) ||
	  (values[0]=get_dec(values_str)) < 0 ||
	  !(values_str = strtok_r(NULL, separator_colon, &brkt_local)) ||
	  (values[1]=get_dec(values_str)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid pin format for %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
      } else {
	if( values[0] >= 8 ||  (values[0] & 1) ||
	    values[1] >= 8 || !(values[1] & 1) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_P] ERROR: invalid pins for %s n=%d ... %s=%s (expecting 0:1, 2:3, 4:5 or 6:7)\n", cmd, num, parameter, value_str);
#endif
	  return -1; // invalid parameter
	} else {
	  pin_a = values[0];
	}
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "type") == 0 ) {
      if( strcasecmp(value_str, "disabled") == 0 ) {
	enc_type = DISABLED;
      } else if( strcasecmp(value_str, "non_detented") == 0 ) {
	enc_type = NON_DETENTED;
      } else if( strcasecmp(value_str, "detented1") == 0 ) {
	enc_type = DETENTED1;
      } else if( strcasecmp(value_str, "detented2") == 0 ) {
	enc_type = DETENTED2;
      } else if( strcasecmp(value_str, "detented3") == 0 ) {
	enc_type = DETENTED3;
      } else if( strcasecmp(value_str, "detented4") == 0 ) {
	enc_type = DETENTED4;
      } else if( strcasecmp(value_str, "detented5") == 0 ) {
	enc_type = DETENTED5;
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid type for %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_P] WARNING: unsupported parameter in %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
      // just continue to keep files compatible
    }
  }

  if( num >= 1 ) {
    mios32_enc_config_t enc_config = {
      .cfg.type=enc_type,
      .cfg.speed=NORMAL,
      .cfg.speed_par=0,
      .cfg.sr=sr,
      .cfg.pos=pin_a
    };

    MIOS32_ENC_ConfigSet(num, enc_config); // counting from 1, because SCS menu encoder is assigned to 0
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses DIN_MATRIX definitions
// returns >= 0 if command is valid
// returns <0 if command is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseDinMatrix(char *cmd, char *brkt)
{
  // parse the parameters
  int num = 0;
  int rows = 0;
  int inverted = 0;
  int sr_dout_sel1 = 0;
  int sr_dout_sel2 = 0;
  int sr_din1 = 0;
  int sr_din2 = 0;

  char *parameter;
  char *value_str;
  while( parseExtendedParameter(cmd, &parameter, &value_str, &brkt) >= 0 ) { 

    ////////////////////////////////////////////////////////////////////////////////////////////////
    if( strcasecmp(parameter, "n") == 0 ) {
      if( (num=get_dec(value_str)) < 1 || num > MBNG_PATCH_NUM_MATRIX_DIN ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid DIN matrix number for %s ... %s=%s' (1..%d)\n", cmd, parameter, value_str, MBNG_PATCH_NUM_MATRIX_DIN);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "rows") == 0 ) {
      if( (rows=get_dec(value_str)) < 0 || (rows != 4 && rows != 8 && rows != 16) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid row number for %s n=%d ... %s=%s (only 4, 8 or 16 allowed)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "inverted") == 0 ) {
      if( (inverted=get_dec(value_str)) < 0 || inverted > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid inverted value for %s n=%d ... %s=%s (only 0 or 1 allowed)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_sel1") == 0 ) {
      if( (sr_dout_sel1=get_dec(value_str)) < 0 || sr_dout_sel1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_sel2") == 0 ) {
      if( (sr_dout_sel2=get_dec(value_str)) < 0 || sr_dout_sel2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_din1") == 0 ) {
      if( (sr_din1=get_dec(value_str)) < 0 || sr_din1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_din2") == 0 ) {
      if( (sr_din2=get_dec(value_str)) < 0 || sr_din2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_P] WARNING: unsupported parameter in %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
      // just continue to keep files compatible
    }
  }

  if( num >= 1 ) {
    mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[num-1];
    m->num_rows = rows;
    m->inverted = inverted;
    m->sr_dout_sel1 = sr_dout_sel1;
    m->sr_dout_sel2 = sr_dout_sel2;
    m->sr_din1 = sr_din1;
    m->sr_din2 = sr_din2;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses DOUT_MATRIX definitions
// returns >= 0 if command is valid
// returns <0 if command is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseDoutMatrix(char *cmd, char *brkt)
{
  // parse the parameters
  int num = 0;
  int rows = 0;
  int inverted = 0;
  int sr_dout_sel1 = 0;
  int sr_dout_sel2 = 0;
  int sr_dout_r1 = 0;
  int sr_dout_r2 = 0;
  int sr_dout_g1 = 0;
  int sr_dout_g2 = 0;
  int sr_dout_b1 = 0;
  int sr_dout_b2 = 0;

  char *parameter;
  char *value_str;
  while( parseExtendedParameter(cmd, &parameter, &value_str, &brkt) >= 0 ) { 

    ////////////////////////////////////////////////////////////////////////////////////////////////
    if( strcasecmp(parameter, "n") == 0 ) {
      if( (num=get_dec(value_str)) < 1 || num > MBNG_PATCH_NUM_MATRIX_DIN ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid DIN matrix number for %s ... %s=%s' (1..%d)\n", cmd, parameter, value_str, MBNG_PATCH_NUM_MATRIX_DIN);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "rows") == 0 ) {
      if( (rows=get_dec(value_str)) < 0 || (rows != 4 && rows != 8 && rows != 16) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid row number for %s n=%d ... %s=%s (only 4, 8 or 16 allowed)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "inverted") == 0 ) {
      if( (inverted=get_dec(value_str)) < 0 || inverted > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid inverted value for %s n=%d ... %s=%s (only 0 or 1 allowed)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_sel1") == 0 ) {
      if( (sr_dout_sel1=get_dec(value_str)) < 0 || sr_dout_sel1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_sel2") == 0 ) {
      if( (sr_dout_sel2=get_dec(value_str)) < 0 || sr_dout_sel2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_r1") == 0 ) {
      if( (sr_dout_r1=get_dec(value_str)) < 0 || sr_dout_r1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_r2") == 0 ) {
      if( (sr_dout_r2=get_dec(value_str)) < 0 || sr_dout_r2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_g1") == 0 ) {
      if( (sr_dout_g1=get_dec(value_str)) < 0 || sr_dout_g1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_g2") == 0 ) {
      if( (sr_dout_g2=get_dec(value_str)) < 0 || sr_dout_g2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_b1") == 0 ) {
      if( (sr_dout_b1=get_dec(value_str)) < 0 || sr_dout_b1 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "sr_dout_b2") == 0 ) {
      if( (sr_dout_b2=get_dec(value_str)) < 0 || sr_dout_b2 > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid SR number for %s n=%d ... %s=%s (1..%d)\n", cmd, num, parameter, value_str, MIOS32_SRIO_NUM_SR);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_P] WARNING: unsupported parameter in %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
      // just continue to keep files compatible
    }
  }

  if( num >= 1 ) {
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[num-1];
    m->num_rows = rows;
    m->inverted = inverted;
    m->sr_dout_sel1 = sr_dout_sel1;
    m->sr_dout_sel2 = sr_dout_sel2;
    m->sr_dout_r1 = sr_dout_r1;
    m->sr_dout_r2 = sr_dout_r2;
    m->sr_dout_g1 = sr_dout_g1;
    m->sr_dout_g2 = sr_dout_g2;
    m->sr_dout_b1 = sr_dout_b1;
    m->sr_dout_b2 = sr_dout_b2;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses ROUTER definitions
// returns >= 0 if command is valid
// returns <0 if command is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseRouter(char *cmd, char *brkt)
{
  // parse the parameters
  int num = 0;
  int src_port = 0x10;
  int src_chn  = 0;
  int dst_port = 0x10;
  int dst_chn  = 0;

  char *parameter;
  char *value_str;
  while( parseExtendedParameter(cmd, &parameter, &value_str, &brkt) >= 0 ) { 

    ////////////////////////////////////////////////////////////////////////////////////////////////
    if( strcasecmp(parameter, "n") == 0 ) {
      if( (num=get_dec(value_str)) < 1 || num > MIDI_ROUTER_NUM_NODES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid router node number for %s ... %s=%s' (1..%d)\n", cmd, parameter, value_str, MIDI_ROUTER_NUM_NODES);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "src_port") == 0 ) {
      if( (src_port=get_dec(value_str)) < 0 || src_port > 0xff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid source port for %s n=%d ... %s=%s (0x00..0xff)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "src_chn") == 0 ) {
      if( (src_chn=get_dec(value_str)) < 0 || src_chn > 17 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid source channel for %s n=%d ... %s=%s (0x00..0xff)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "dst_port") == 0 ) {
      if( (dst_port=get_dec(value_str)) < 0 || dst_port > 0xff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid source port for %s n=%d ... %s=%s (0x00..0xff)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else if( strcasecmp(parameter, "dst_chn") == 0 ) {
      if( (dst_chn=get_dec(value_str)) < 0 || dst_chn > 17 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_P] ERROR invalid source channel for %s n=%d ... %s=%s (0x00..0xff)\n", cmd, num, parameter, value_str);
#endif
	return -1; // invalid parameter
      }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_P] WARNING: unsupported parameter in %s n=%d ... %s=%s\n", cmd, num, parameter, value_str);
#endif
      // just continue to keep files compatible
    }
  }

  if( num >= 1 ) {
    midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[num-1];
    n->src_port = src_port;
    n->src_chn = src_chn;
    n->dst_port = dst_port;
    n->dst_chn = dst_chn;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads the patch file content (again)
// returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_P_Read(char *filename)
{
  s32 status = 0;
  mbng_file_p_info_t *info = &mbng_file_p_info;
  file_t file;
  u8 got_first_event_item = 0;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(mbng_file_p_patch_name, filename, MBNG_FILE_P_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGP", MBNG_FILES_PATH, mbng_file_p_patch_name);

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
  char line_buffer[200];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 200);

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
	} else if( strncmp(parameter, "EVENT_", 6) == 0 ) {
	  if( !got_first_event_item ) {
	    got_first_event_item = 1;
	    MBNG_EVENT_PoolClear();
	  }
	  parseEvent(parameter, brkt);
	} else if( strcmp(parameter, "ENC") == 0 ) {
	  parseEnc(parameter, brkt);
	} else if( strcmp(parameter, "DIN_MATRIX") == 0 ) {
	  parseDinMatrix(parameter, brkt);
	} else if( strcmp(parameter, "DOUT_MATRIX") == 0 ) {
	  parseDoutMatrix(parameter, brkt);
	} else if( strcmp(parameter, "ROUTER") == 0 ) {
	  parseRouter(parameter, brkt);

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

#if DEBUG_VERBOSE_LEVEL >= 1
  if( got_first_event_item ) {
    DEBUG_MSG("[MBNG_FILE_P] Event Pool Number of Items: %d", MBNG_EVENT_PoolNumItemsGet());
    u32 pool_size = MBNG_EVENT_PoolSizeGet();
    u32 pool_max_size = MBNG_EVENT_PoolMaxSizeGet();
    DEBUG_MSG("[MBNG_FILE_P] Event Pool Allocation: %d of %d bytes (%d%%)",
	      pool_size, pool_max_size, (100*pool_size)/pool_max_size);
  }
#endif

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
    sprintf(line_buffer, "# EVENT_* (event definitions)\n");
    FLUSH_BUFFER;

    int num_items = MBNG_EVENT_PoolNumItemsGet();
    int item_ix;
    for(item_ix=0; item_ix<num_items; ++item_ix) {
      mbng_event_item_t item;
      if( MBNG_EVENT_ItemGet(item_ix, &item) < 0 )
	continue;

      sprintf(line_buffer, "EVENT_%-6s id=%3d  type=%-6s",
	      MBNG_EVENT_ItemControllerStrGet(&item),
	      item.id & 0xfff,
	      MBNG_EVENT_ItemTypeStrGet(&item));
      FLUSH_BUFFER;

      switch( item.flags.general.type ) {
      case MBNG_EVENT_TYPE_NOTE_OFF:
      case MBNG_EVENT_TYPE_NOTE_ON:
      case MBNG_EVENT_TYPE_POLY_PRESSURE: {
	if( item.stream_size >= 2 ) {
	  sprintf(line_buffer, " chn=%2d key=%3d", (item.stream[0] & 0xf)+1, item.stream[1]);
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_CC: {
	if( item.stream_size >= 2 ) {
	  sprintf(line_buffer, " chn=%2d cc=%3d ", (item.stream[0] & 0xf)+1, item.stream[1]);
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
      case MBNG_EVENT_TYPE_AFTERTOUCH:
      case MBNG_EVENT_TYPE_PITCHBEND: {
	if( item.stream_size >= 2 ) {
	  sprintf(line_buffer, " chn=%2d", (item.stream[0] & 0xf)+1);
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_SYSEX: {
	if( item.stream_size >= 4 ) {
	  sprintf(line_buffer, " sysex_type=%d sysex_value_pos=%d stream=\"", item.stream[0], item.stream[1]); // it isn't required to dump the sysex length
	  FLUSH_BUFFER;

	  int pos;
	  for(pos=0; pos<(item.stream_size-3); ++pos) {
	    sprintf(line_buffer, "%s0x%02x", (pos == 0) ? "" : " ", item.stream[3+pos]);
	    FLUSH_BUFFER;
	  }
	  sprintf(line_buffer, "\"");
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_RPN: {
	if( item.stream_size >= 2 ) {
	  sprintf(line_buffer, " rpn=%d rpn_format=%d", item.stream[0] | (int)(item.stream[1] << 7), MBNG_EVENT_ItemNrpnFormatStrGet(&item));
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_NRPN: {
	if( item.stream_size >= 3 ) {
	  sprintf(line_buffer, " nrpn=%d nrpn_format=%s", item.stream[0] | (int)(item.stream[1] << 7), MBNG_EVENT_ItemNrpnFormatStrGet(&item));
	  FLUSH_BUFFER;
	}
      } break;

      case MBNG_EVENT_TYPE_META: {
	if( item.stream_size >= 2 ) {
	  sprintf(line_buffer, " meta_id=0x%04x", item.stream[0] | (int)(item.stream[1] << 8));
	  FLUSH_BUFFER;
	}
      } break;
      }

      {
	char ports_bin[17];
	int bit;
	for(bit=0; bit<16; ++bit) {
	  ports_bin[bit] = (item.enabled_ports & (1 << bit)) ? '1' : '0';
	}
	ports_bin[16] = 0;

	sprintf(line_buffer, "  range=%3d:%3d  ports=%s", item.min, item.max, ports_bin);
	FLUSH_BUFFER;
      }

      sprintf(line_buffer, "  lcd_pos=%d:%d:%d label=\"%s\"\n", item.lcd+1, (item.lcd_pos%64)+1, (item.lcd_pos/64)+1, item.label ? item.label : "");
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n# ENC (encoder definitions)\n");
    FLUSH_BUFFER;

    int enc;
    for(enc=1; enc<MIOS32_ENC_NUM_MAX; ++enc) {
      mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);

      char enc_type[20];
      switch( enc_config.cfg.type ) { // see mios32_enc.h for available types
      case 0xff: strcpy(enc_type, "non_detented"); break;
      case 0xaa: strcpy(enc_type, "detented1"); break;
      case 0x22: strcpy(enc_type, "detented2"); break;
      case 0x88: strcpy(enc_type, "detented3"); break;
      case 0xa5: strcpy(enc_type, "detented4"); break;
      case 0x5a: strcpy(enc_type, "detented5"); break;
      default: strcpy(enc_type, "disabled"); break;
      }

      sprintf(line_buffer, "ENC n=%3d   sr=%2d  pins=%d:%d   type=%s\n",
	      enc,
	      enc_config.cfg.sr,
	      enc_config.cfg.pos,
	      enc_config.cfg.pos+1,
	      enc_type);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n# DIN_MATRIX definitions\n");
    FLUSH_BUFFER;

    int matrix;
    mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {

      sprintf(line_buffer, "DIN_MATRIX n=%2d   rows=%d  inverted=%d  sr_dout_sel1=%2d sr_dout_sel2=%2d  sr_din1=%2d sr_din2=%2d\n",
	      matrix+1,
	      m->num_rows,
	      m->inverted,
	      m->sr_dout_sel1,
	      m->sr_dout_sel2,
	      m->sr_din1,
	      m->sr_din2);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n# DOUT_MATRIX definitions\n");
    FLUSH_BUFFER;

    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {

      sprintf(line_buffer, "DOUT_MATRIX n=%2d   rows=%d  inverted=%d  sr_dout_sel1=%2d sr_dout_sel2=%2d  sr_dout_r1=%2d sr_dout_r2=%2d  sr_dout_g1=%2d sr_dout_g2=%2d  sr_dout_b1=%2d sr_dout_b2=%2d\n",
	      matrix+1,
	      m->num_rows,
	      m->inverted,
	      m->sr_dout_sel1,
	      m->sr_dout_sel2,
	      m->sr_dout_r1,
	      m->sr_dout_r2,
	      m->sr_dout_g1,
	      m->sr_dout_g2,
	      m->sr_dout_b1,
	      m->sr_dout_b2);
      FLUSH_BUFFER;
    }
  }

  {
    sprintf(line_buffer, "\n\n# ROUTER definitions (Note: chn=0 disables, chn=17 selects all channels)\n");
    FLUSH_BUFFER;

    int node;
    midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      sprintf(line_buffer, "ROUTER n=%2d   src_port=0x%02x  src_chn=%2d   dst_port=0x%02x  dst_chn=%2d\n",
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

    sprintf(line_buffer, "OSC_TransferMode %d %d\n", con, OSC_CLIENT_TransferModeGet(con));
    FLUSH_BUFFER;
  }
#endif

  sprintf(line_buffer, "\n\n# Misc. Configuration\n");
  FLUSH_BUFFER;

  sprintf(line_buffer, "DebounceCtr %d\n", mbng_patch_cfg.debounce_ctr);
  FLUSH_BUFFER;
  sprintf(line_buffer, "GlobalChannel %d\n", mbng_patch_cfg.global_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "AllNotesOffChannel %d\n", mbng_patch_cfg.all_notes_off_chn);
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Preset %d\n", (int)SEQ_BPM_Get());
  FLUSH_BUFFER;
  sprintf(line_buffer, "BPM_Mode %d\n", SEQ_BPM_ModeGet());
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
  sprintf(filepath, "%s%s.NGP", MBNG_FILES_PATH, mbng_file_p_patch_name);

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
