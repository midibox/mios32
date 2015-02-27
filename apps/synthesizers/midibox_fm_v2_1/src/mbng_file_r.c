// $Id: mbng_file_r.c 2045 2014-09-04 19:36:25Z tk $ //! \defgroup
// MBNG_FILE_R //! Config File access functions //! //! NOTE: before
// accessing the SD Card, the upper level function should //! 
// synchronize with the SD Card semaphore!  //!  MUTEX_SDCARD_TAKE; //
// to take the semaphore //!  MUTEX_SDCARD_GIVE; // to release the
// semaphore //! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <uip_task.h>
#include <midi_port.h>

#include "tasks.h"
#include "file.h"
#include "mbng_file.h"
#include "mbng_file_r.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_lcd.h"


/////////////////////////////////////////////////////////////////////////////
//! for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
//! for performance measurements
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_FILE_HANDLER_PERFORMANCE 0


/////////////////////////////////////////////////////////////////////////////
//! Nesting vars
/////////////////////////////////////////////////////////////////////////////
#define IF_MAX_NESTING_LEVEL 16
static u8 nesting_level;
static u8 if_state[IF_MAX_NESTING_LEVEL];


/////////////////////////////////////////////////////////////////////////////
//! Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MBNG_FILES_PATH "/"
//#define MBNG_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
  file_t r_file;
} mbng_file_r_info_t;


// read request handling
typedef union {
  u32 ALL;

  struct {
    s16 value;
    u32 section:8;
    u32 load:1;
    u32 notify_done:1;
  };
} mbng_file_r_req_t;

// script variables
typedef struct {
  s16 value;
  u8 section;
  u8 bank;
} mbng_file_r_var_t;

// script items
typedef union {
  u32 ALL;

  struct {
    mbng_event_item_id_t id:16;
    u32 valid:1;
    u32 is_hw_id:1;
  };
} mbng_file_r_item_id_t;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static mbng_file_r_info_t mbng_file_r_info;

static u16 mbng_file_r_delay_ctr;
static mbng_file_r_req_t mbng_file_r_req;

static mbng_file_r_var_t vars;

static const char *separators = " \t";
static const char *separator_colon = ":";


/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////
char mbng_file_r_script_name[MBNG_FILE_R_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_R_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Loads script
//! Called from MBNG_FILE_RheckSDCard() when the SD card has been connected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Load(char *filename)
{
  s32 error;
  error = MBNG_FILE_R_Read(filename, 0);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R] Tried to open script %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! Unloads script file
//! Called from MBNG_FILE_RheckSDCard() when the SD card has been disconnected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Unload(void)
{
  mbng_file_r_info.valid = 0;
  mbng_file_r_req.ALL = 0;
  mbng_file_r_delay_ctr = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \Returns 1 if current script file valid
//! \Returns 0 if current script file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Valid(void)
{
  return mbng_file_r_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
//! Set/Get functions for variables
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_VarSectionSet(u8 section)
{
  vars.section = section;
  return 0; // no error
}

s32 MBNG_FILE_R_VarSectionGet()
{
  return vars.section;
}

s32 MBNG_FILE_R_VarValueSet(u8 value)
{
  vars.value = value;
  return 0; // no error
}

s32 MBNG_FILE_R_VarValueGet()
{
  return vars.value;
}


/////////////////////////////////////////////////////////////////////////////
//! This is a local implementation of strtok_r (original one taken from newlib-1.20-0)
//! It ignores delim inside squared brackets [ ... ]
/////////////////////////////////////////////////////////////////////////////
static char *ngr_strtok_r(register char *s, register const char *delim, char **lasts)
{
  register char *spanp;
  register int c, sc;
  char *tok;
  int tk_nested_squarebrackets_ctr = 0;

  if (s == NULL && (s = *lasts) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
cont:
  c = *s++;
  for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {		/* no non-delimiter characters */
    *lasts = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */

  if( c == '[' )
    ++tk_nested_squarebrackets_ctr;

  for (;;) {
    c = *s++;

    if( c == '[' ) {
      ++tk_nested_squarebrackets_ctr;
    } else if( c == ']' && tk_nested_squarebrackets_ctr > 0 ) {
      --tk_nested_squarebrackets_ctr;
    } else if( c == 0 || tk_nested_squarebrackets_ctr == 0 ) {
      spanp = (char *)delim;
      do {
	if ((sc = *spanp++) == c) {
	  if (c == 0)
	    s = NULL;
	  else
	    s[-1] = 0;
	  *lasts = s;
	  return (tok);
	}
      } while (sc != 0);
    }
  }
  /* NOTREACHED */
}

/////////////////////////////////////////////////////////////////////////////
//! help function which removes whitespaces at the begin and end of a string
/////////////////////////////////////////////////////////////////////////////
static char *trim(char *word)
{
  if( word == NULL )
    return NULL;

  // remove at string begin
  while( *word != 0 && (*word == ' ' || *word == '\t') )
    ++word;

  if( *word == 0 )
    return word;

  // inc until we reach the end of string
  char *word_end = word;
  while( *word_end != 0 ) ++word_end;

  // remove at string end
  --word_end;
  while( *word_end == ' ' || *word_end == '\t' )
    --word_end;

  word_end[1] = 0;

  return word;
}

/////////////////////////////////////////////////////////////////////////////
//! help function which removes the quotes of an argument (e.g. .csv file format)
//! can be cascaded with ngr_strtok_r
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
//! help function which parses a decimal or hex value
//! \returns >= 0 if value is valid
//! \returns -1000000000 if value is invalid
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
//! help function which parses an id (xxx:<number>)
//! \returns > 0 if id is valid
//! \returns == 0 if id is invalid
/////////////////////////////////////////////////////////////////////////////
static mbng_file_r_item_id_t parseId(char *parameter)
{
  char *brkt;

  mbng_file_r_item_id_t id;
  id.ALL = 0;

  // by default we assume that the id addresses a hw_id
  id.is_hw_id = 1;

  if( strncasecmp(parameter, "(id)", 4) == 0 ) {
    id.is_hw_id = 0;
    parameter += 4;
  } else if( strncasecmp(parameter, "(hw_id)", 7) == 0 ) {
    // normaly not necessary, but this covers the case that somebody explicitely specifies (hw_id)
    id.is_hw_id = 1;
    parameter += 7;
  }

  char *value_str;
  if( !(value_str = ngr_strtok_r(parameter, separator_colon, &brkt)) ||
      (id.id=MBNG_EVENT_ItemIdFromControllerStrGet(value_str)) == MBNG_EVENT_CONTROLLER_DISABLED ) {
    return id;
  }

  int id_lower = 0;
  value_str = brkt;
  if( (id_lower=get_dec(value_str)) < 1 || id_lower > 0xfff ) {
    return id;
  } else {
    id.id |= id_lower;
  }

  id.valid = 1;

  return id;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which determine a value of a condition
//! \returns >= 0 if value is valid
//! \returns -1000000000 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseValue(u32 line, char *command, char *value_str)
{
  if( value_str == NULL || value_str[0] == 0 )
    return -1000000000;

  if( value_str[0] == '[' ) {
    // math operation
    int len = strlen(value_str);
    if( value_str[len-1] != ']' ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid syntax for math operation:", line);
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: '%s' should end with ']' in '%s' command!", line, value_str, command);
#endif
      return -1000000000;
    }

    // remove [ and ]
    value_str[len-1] = 0;
    ++value_str;
    len -= 2;

    // separate operands
    char *lOperand = value_str;
    char *rOperand = value_str;
    char operator = '?';
    {
      int i;
      for(i=0; i<len; ++i, ++rOperand) {
	if( *rOperand == '+' ||
	    *rOperand == '-' ||
	    *rOperand == '*' ||
	    *rOperand == '/' ||
	    *rOperand == '%' ||
	    *rOperand == '&' ||
	    *rOperand == '|' ||
	    *rOperand == '^' ) {
	  operator = *rOperand;
	  *rOperand = 0;
	  ++rOperand;
	  break;
	}
      }

      if( operator == '?' ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: no operator in math operation '%s'!", line, value_str);
#endif
	return -1000000000;
      }
    }

    //DEBUG_MSG("Calc: %s %c %s\n", lOperand, operator, rOperand);

    // get left side value (recursively)
    s32 lValue = parseValue(line, command, trim(lOperand));
    if( lValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid left side operand '%s' in '%s' command!", line, lOperand, command);
#endif
      return -1000000000;
    }

    // get right side value (recursively)
    s32 rValue = parseValue(line, command, trim(rOperand));
    if( rValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid right side operand '%s' in '%s' command!", line, rOperand, command);
#endif
      return -1000000000;
    }

    switch( operator ) {
    case '+': return lValue + rValue;
    case '-': return lValue - rValue;
    case '*': return lValue * rValue;
    case '/': return lValue / rValue;
    case '%': return lValue % rValue;
    case '&': return lValue & rValue;
    case '|': return lValue | rValue;
    case '^': return lValue ^ rValue;
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: unsupported operator in math operation '%s'!", line, value_str);
#endif
    return -1000000000;
  }


  if( value_str[0] == '^' ) {
    if( strcasecmp((char *)&value_str[1], "SECTION") == 0 ) {
      return vars.section;
    } else if( strcasecmp((char *)&value_str[1], "VALUE") == 0 ) {
      return vars.value;
    } else if( strcasecmp((char *)&value_str[1], "BANK") == 0 ) {
      return MBNG_EVENT_SelectedBankGet();
    } else if( strcasecmp((char *)&value_str[1], "DEV") == 0 ) {
      return mbng_patch_cfg.sysex_dev;
    } else if( strcasecmp((char *)&value_str[1], "PAT") == 0 ) {
      return mbng_patch_cfg.sysex_pat;
    } else if( strcasecmp((char *)&value_str[1], "BNK") == 0 ) {
      return mbng_patch_cfg.sysex_bnk;
    } else if( strcasecmp((char *)&value_str[1], "INS") == 0 ) {
      return mbng_patch_cfg.sysex_ins;
    } else if( strcasecmp((char *)&value_str[1], "CHN") == 0 ) {
      return mbng_patch_cfg.sysex_chn;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid or unsupported variable:", line);
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: '%s' in '%s' command!", line, value_str, command);
#endif
      return -1000000000;
    }
  }

  mbng_file_r_item_id_t id = parseId(value_str);
  if( id.valid ) {
    // search for items with matching ID
    mbng_event_item_t item;
    u32 continue_ix = 0;
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) >= 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) >= 0) ) {
      return item.value;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: '%s' not found in event pool by '%s' command!", line, value_str, command);
#endif
      return -1000000000;
    }
  }

  return get_dec(value_str);
}

/////////////////////////////////////////////////////////////////////////////
//! Checks if line is empty (space, tab, # allowed
//! \returns 1 if line is empty
//! \returns 0 if line is not empty
/////////////////////////////////////////////////////////////////////////////
s32 lineIsEmpty(char *line)
{
  while( *line ) {
    if( *line == '#' )
      return 1; // line is empty (allow comment)

    if( *line != ' ' && *line != '\t' )
      return 0; // line is not empty

    ++line;
  }

  return 1; // line is empty
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses an IF condition
//! \returns >= 0 if condition is valid
//! \returns < 0 if condition is invalid
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseCondition(u32 line, char *command, char **brkt)
{
  char *lvalue_str, *condition_str, *rvalue_str;

  if( !(lvalue_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: missing left value of expression in '%s' command!", line, command);
#endif
    return -1;
  }

  if( !(condition_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: missing condition of expression in '%s' command!", line, command);
#endif
    return -2;
  }

  if( !(rvalue_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: missing right value of expression in '%s' command!", line, command);
#endif
    return -3;
  }

  s32 lvalue = parseValue(line, command, lvalue_str);
  if( lvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid left value '%s' in '%s' command!", line, lvalue_str, command);
#endif
    return -4;
  }

  s32 rvalue = parseValue(line, command, rvalue_str);
  if( rvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid right value '%s' in '%s' command!", line, rvalue_str, command);
#endif
    return -5;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_C:%d] condition: %s %s %s (%d %s %d)\n", line, lvalue_str, condition_str, rvalue_str, lvalue, condition_str, rvalue);
#endif

  if( strcasecmp(condition_str, "==") == 0 ) {
    return lvalue == rvalue;
  } else if( strcasecmp(condition_str, "!=") == 0 ) {
    return lvalue != rvalue;
  } else if( strcasecmp(condition_str, ">=") == 0 ) {
    return lvalue >= rvalue;
  } else if( strcasecmp(condition_str, "<=") == 0 ) {
    return lvalue <= rvalue;
  } else if( strcasecmp(condition_str, ">") == 0 ) {
    return lvalue > rvalue;
  } else if( strcasecmp(condition_str, "<") == 0 ) {
    return lvalue < rvalue;
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid or unsupported condition '%s' in '%s' command!", line, condition_str, command);
#endif

  return -10; // invalid condition
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SEND command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSend(u32 line, char *command, char **brkt)
{
  char *event_str;
  if( !(event_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: missing event type in '%s' command!", line, command);
#endif
    return -1;
  }

  mbng_event_type_t event_type;
  if( (event_type=MBNG_EVENT_ItemTypeFromStrGet(event_str)) == MBNG_EVENT_TYPE_UNDEFINED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: unknown event type '%s' in '%s' command!", line, event_str, command);
#endif
    return -1;
  }


  char *port_str;
  if( !(port_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: missing MIDI port in '%s' command!", line, command);
#endif
    return -1;
  }

  mios32_midi_port_t port;
  {
    int out_port = 0xff;
    int port_ix;

    for(port_ix=0; port_ix<MIDI_PORT_OutNumGet(); ++port_ix) {
      // terminate port name at first space
      char port_name[10];
      strcpy(port_name, MIDI_PORT_OutNameGet(port_ix));
      int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;
    
      if( strcasecmp(port_str, port_name) == 0 ) {
	out_port = MIDI_PORT_OutPortGet(port_ix);
	break;
      }
    }
    
    if( out_port == 0xff && ((out_port=parseValue(line, command, port_str)) < 0 || out_port > 0xff) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid MIDI port '%s' in '%s' command!", line, port_str, command);
#endif
      return -1;
    }

    port = (mios32_midi_port_t)out_port;
  }



  int num_values = 0;
#define STREAM_MAX_SIZE 256
  u8 stream[STREAM_MAX_SIZE];
  int stream_size = 0;

  switch( event_type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       num_values = 3; break;
  case MBNG_EVENT_TYPE_NOTE_ON:        num_values = 3; break;
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  num_values = 3; break;
  case MBNG_EVENT_TYPE_CC:             num_values = 3; break;
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: num_values = 2; break;
  case MBNG_EVENT_TYPE_AFTERTOUCH:     num_values = 2; break;
  case MBNG_EVENT_TYPE_PITCHBEND:      num_values = 2; break;
  case MBNG_EVENT_TYPE_NRPN:           num_values = 3; break;

  case MBNG_EVENT_TYPE_SYSEX: { // extra handling
    char *stream_str;
    char *brkt_local = *brkt;
    u8 *stream_pos = (u8 *)&stream[0];
    // check for STREAM_MAX_SIZE-2, since a meta entry allocates up to 3 bytes
    while( stream_size < (STREAM_MAX_SIZE-2) && (stream_str = ngr_strtok_r(NULL, separators, &brkt_local)) ) {
      if( *stream_str == '^' ) {
	mbng_event_sysex_var_t sysex_var = MBNG_EVENT_ItemSysExVarFromStrGet((char *)&stream_str[1]);
	if( sysex_var == MBNG_EVENT_SYSEX_VAR_UNDEFINED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unknown SysEx variable '%s' in command '%s'\n", line, stream_str, command);
#endif
	  return -1;
	} else {
	  *stream_pos = 0xff; // meta indicator
	  ++stream_pos;
	  ++stream_size;
	  *stream_pos = (u8)sysex_var;
	}
      } else {
	int value;
	if( (value=parseValue(line, command, stream_str)) < 0 || value > 0xff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid SysEx value '%s' in command '%s', expecting 0..255 (0x00..0xff)\n", line, stream_str, command);
#endif
	  return -1;
	} else {
	  *stream_pos = (u8)value;
	}
      }

      ++stream_pos;
      ++stream_size;
    }

    if( !stream_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing SysEx string in command '%s'\n", line, command);
#endif
      return -1;
    }

  } break;

  case MBNG_EVENT_TYPE_META: {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: use EXEC_META to send meta events!", line);
#endif
      return -1;
  }
  }

  s16 values[3];
  int i;
  for(i=0; i<num_values; ++i) {
    char *value_str;
    if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value for '%s' event in '%s' command!", line, event_str, command);
#endif
      return -1;
    }

    s32 value;
    if( (value=parseValue(line, command, value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value for '%s' event in '%s' command!", line, event_str, command);
#endif
      return -1;
    }

    values[i] = value;
  }

  MUTEX_MIDIOUT_TAKE;
  switch( event_type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       MIOS32_MIDI_SendNoteOff(port, (values[0]-1) & 0xf, values[1] & 0x7f, values[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_NOTE_ON:        MIOS32_MIDI_SendNoteOn(port, (values[0]-1) & 0xf, values[1] & 0x7f, values[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  MIOS32_MIDI_SendPolyPressure(port, (values[0]-1) & 0xf, values[1] & 0x7f, values[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_CC:             MIOS32_MIDI_SendCC(port, (values[0]-1), values[1] & 0x7f, values[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: MIOS32_MIDI_SendProgramChange(port, (values[0]-1) & 0xf, values[1] & 0x7f); break;
  case MBNG_EVENT_TYPE_AFTERTOUCH:     MIOS32_MIDI_SendAftertouch(port, (values[0]-1) & 0xf, values[1] & 0x7f); break;
  case MBNG_EVENT_TYPE_PITCHBEND:      MIOS32_MIDI_SendPitchBend(port, (values[0]-1) & 0xf, (values[1] + 8192) & 0x3fff); break;
  case MBNG_EVENT_TYPE_NRPN:           MBNG_EVENT_SendOptimizedNRPN(port, (values[0]-1) & 0xf, values[1], values[2], 0); break; // msb_only == 0

  case MBNG_EVENT_TYPE_SYSEX: {
    if( !stream_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unexpected condition for 'send SysEx' - please inform TK!", line);
#endif
      return -1;
    }

    // send from a dummy item
    mbng_event_item_t item;
    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
    item.stream = stream;
    item.stream_size = stream_size;
    item.value = vars.value;
    MBNG_EVENT_SendSysExStream(port, &item);
  } break;

  //case MBNG_EVENT_TYPE_META: // extra handling with EXEC_META
  }
  MUTEX_MIDIOUT_GIVE;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a EXEC_META command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseExecMeta(u32 line, char *command, char **brkt)
{
  u8 stream[10];

  mbng_event_item_t item;
  MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
  item.stream = stream;
  item.stream_size = 0;

  char *meta_str;
  if( !(meta_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing meta type in '%s' command!", line, command);
#endif
    return -1;
  }

  char *values_str = meta_str;
  char *brkt_local;

  mbng_event_meta_type_t meta_type;
  if( !(values_str = ngr_strtok_r(meta_str, separator_colon, &brkt_local)) ||
      (meta_type = MBNG_EVENT_ItemMetaTypeFromStrGet(meta_str)) == MBNG_EVENT_META_TYPE_UNDEFINED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unknown or invalid meta type in '%s' command!\n", line, command);
#endif
    return -1;
  }
  item.stream[item.stream_size++] = meta_type;

  u8 num_bytes = MBNG_EVENT_ItemMetaNumBytesGet(meta_type);
  
  if( num_bytes > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unexpected required number of meta bytes (%d) - please inform TK!", line, num_bytes);
#endif
    return -1;
  }

  int i;
  for(i=0; i<num_bytes; ++i) {
    int value = 0;
    if( !(values_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(value=get_dec(values_str)) < 0 || value > 255 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: expecting %d values for meta type %s\n", line, num_bytes, MBNG_EVENT_ItemMetaTypeStrGet(meta_type));
#endif
      return -1;
    }

    item.stream[item.stream_size++] = value;
  }

  if( strlen(brkt_local) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] WARNING: more values specified than expected for meta type %s\n", line, MBNG_EVENT_ItemMetaTypeStrGet(meta_type));
#endif
  }

  // parse optional value
  s32 value = vars.value;
  if( (values_str = ngr_strtok_r(NULL, separators, brkt)) ) {

    if( (value=get_dec(values_str)) < -16384 || value > 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid (optional) value for meta type %s\n", line, MBNG_EVENT_ItemMetaTypeStrGet(meta_type));
#endif
      return -1;
    }
  }
  item.value = (s16)value;

  MUTEX_MIDIOUT_TAKE;
  MBNG_EVENT_ExecMeta(&item);
  MUTEX_MIDIOUT_GIVE;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSet(u32 line, char *command, char **brkt, u8 send_event)
{
  char *dst_str = NULL;
  char *value_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  u8 is_var = (*brkt)[0] == '^';

  if( is_var ) {
    if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) || strlen(dst_str) < 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing variable in '%s ^<var>' command!\n", line, command);
#endif
      return -1;
    }

  } else {
    if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
      return -1;
    }

    id = parseId(dst_str);
    if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
      return -1;
    }
  }

  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
    return -1;
  }

  s32 value = 0;
  if( (value=parseValue(line, command, value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s %s' command (expecting -16384..16383!\n", line, command, dst_str, value_str);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

  if( is_var ) {
    if( strcasecmp((char *)&dst_str[1], "SECTION") == 0 ) {
      vars.section = value;
    } else if( strcasecmp((char *)&dst_str[1], "VALUE") == 0 ) {
      vars.value = value;
    } else if( strcasecmp((char *)&dst_str[1], "BANK") == 0 ) {
      MBNG_EVENT_SelectedBankSet(value);
    } else if( strcasecmp((char *)&dst_str[1], "DEV") == 0 ) {
      mbng_patch_cfg.sysex_dev = value;
    } else if( strcasecmp((char *)&dst_str[1], "PAT") == 0 ) {
      mbng_patch_cfg.sysex_pat = value;
    } else if( strcasecmp((char *)&dst_str[1], "BNK") == 0 ) {
      mbng_patch_cfg.sysex_bnk = value;
    } else if( strcasecmp((char *)&dst_str[1], "INS") == 0 ) {
      mbng_patch_cfg.sysex_ins = value;
    } else if( strcasecmp((char *)&dst_str[1], "CHN") == 0 ) {
      mbng_patch_cfg.sysex_chn = value;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_C:%d] ERROR: invalid or unsupported variable '%s' in '%s' command!", line, dst_str, command);
#endif
      return -1;
    }
  } else {
    // search for items with matching ID
    mbng_event_item_t item;
    u32 continue_ix = 0;
    u32 num_set = 0;
    do {
      if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	  (!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
	break;
      } else {
	++num_set;

	// notify item
	item.value = value;
	MBNG_EVENT_ItemReceive(&item, item.value, 1, 0); // "from_midi" and forwarding disabled

	// also send an event (SET command) --- use CHANGE command to avoid sending
	if( send_event ) {
	  if( MBNG_EVENT_NotifySendValue(&item) == 2 )
	    break; // stop has been requested
	}
      }
    } while( continue_ix );

    if( !num_set ) {
      if( !id.is_hw_id ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s %d' failed - item not found!\n", line, command, dst_str, value);
#endif
	return -1;
      }

      // hw_id: send to dummy item    
      MBNG_EVENT_ItemInit(&item, id.id);
      item.flags.active = 1;
      item.value = value;
      MBNG_EVENT_ItemSendVirtual(&item, item.id);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET_RGB command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSetRgb(u32 line, char *command, char **brkt)
{
  char *dst_str = NULL;
  char *value_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
    return -1;
  }

  id = parseId(dst_str);
  if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
    return -1;
  }

  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
    return -1;
  }

  s32 value = 0;
  int r = 0;
  int g = 0;
  int b = 0;

  {
    char *brkt_local;
    if( !(value_str = ngr_strtok_r(value_str, separator_colon, &brkt_local)) ||
	(r=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(g=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(b=get_dec(value_str)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid rgb format in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    if( r < 0 || r >= 16 ||
	g < 0 || g >= 16 ||
	b < 0 || b >= 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid rgb values in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

  // search for items with matching ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  u32 num_set = 0;
  do {
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
      break;
    } else {
      ++num_set;

      // notify item
      item.rgb.r = r;
      item.rgb.g = g;
      item.rgb.b = b;

      MBNG_EVENT_ItemReceive(&item, item.value, 1, 0); // "from_midi" and forwarding disabled

      if( MBNG_EVENT_NotifySendValue(&item) == 2 )
	break; // stop has been requested
    }
  } while( continue_ix );

  if( !num_set ) {
    if( !id.is_hw_id ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s %d' failed - item not found!\n", line, command, dst_str, value);
#endif
      return -1;
    }

    // hw_id: send to dummy item    
    MBNG_EVENT_ItemInit(&item, id.id);
    item.flags.active = 1;
    item.value = 127;
    item.rgb.r = r;
    item.rgb.g = g;
    item.rgb.b = b;
    MBNG_EVENT_ItemSendVirtual(&item, item.id);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET_LOCK command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSetLock(u32 line, char *command, char **brkt)
{
  char *dst_str = NULL;
  char *value_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
    return -1;
  }

  id = parseId(dst_str);
  if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
    return -1;
  }

  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
    return -1;
  }

  s32 value = 0;
  if( (value=parseValue(line, command, value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s %s' command (expecting -16384..16383!\n", line, command, dst_str, value_str);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

  // search for items with matching ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  u32 num_set = 0;
  do {
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
      break;
    } else {
      ++num_set;

      // lock/unlock
      MBNG_EVENT_ItemSetLock(&item, value);
    }
  } while( continue_ix );

  if( !num_set ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s %d' failed - item not found!\n", line, command, dst_str, value);
#endif
    return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a TRIGGER command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseTrigger(u32 line, char *command, char **brkt)
{
  char *dst_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
    return -1;
  }

  id = parseId(dst_str);
  if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
    return -1;
  }

  // search for items with matching ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  u32 num_set = 0;
  do {
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
      break;
    } else {
      ++num_set;

      // notify item
      if( MBNG_EVENT_NotifySendValue(&item) == 2 )
	break; // stop has been requested
    }
  } while( continue_ix );

  if( !num_set ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s' failed - item not found!\n", line, command, dst_str);
#endif
    return -1;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET_ACTIVE command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSetActive(u32 line, char *command, char **brkt)
{
  char *dst_str = NULL;
  char *value_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
    return -1;
  }

  id = parseId(dst_str);
  if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
    return -1;
  }

  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
    return -1;
  }

  s32 value = 0;
  if( (value=parseValue(line, command, value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s %s' command (expecting -16384..16383!\n", line, command, dst_str, value_str);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

  // search for items with matching ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  u32 num_set = 0;
  do {
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
      break;
    } else {
      ++num_set;

      // activate/deactivate
      MBNG_EVENT_ItemSetActive(&item, value);
    }
  } while( continue_ix );

  if( !num_set ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s %d' failed - item not found!\n", line, command, dst_str, value);
#endif
    return -1;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET_MIN and SET_MAX command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSetMinMax(u32 line, char *command, char **brkt, u8 set_max)
{
  char *dst_str = NULL;
  char *value_str = NULL;
  mbng_file_r_item_id_t id; id.ALL = 0;

  if( !(dst_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing id in '%s' command!\n", line, command);
#endif
    return -1;
  }

  id = parseId(dst_str);
  if( !id.valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid id '%s %s'!\n", line, command, dst_str);
#endif
    return -1;
  }

  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
    return -1;
  }

  s32 value = 0;
  if( (value=parseValue(line, command, value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s %s' command (expecting -16384..16383!\n", line, command, dst_str, value_str);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

  // search for items with matching ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  u32 num_set = 0;
  do {
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) < 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) < 0) ) {
      break;
    } else {
      ++num_set;

      // change min/max value
      if( set_max )
	item.max = value;
      else
	item.min = value;

      MBNG_EVENT_ItemModify(&item);
    }
  } while( continue_ix );

  if( !num_set ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] '%s %s %d' failed - item not found!\n", line, command, dst_str, value);
#endif
    return -1;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! help function which parses a DELAY_MS command
//! returns > 0 if a delay has been requested
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseDelay(u32 line, char *command, char **brkt)
{
  char *value_str;
  s32 value;
  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s' command!\n", line, command);
#endif
    return -1;
  }

  if( (value=parseValue(line, command, value_str)) < 0 || value > 100000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s' command (expecting 0..100000)!\n", line, command, value_str);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] DELAY_MS %d\n", line, value);
#endif

  return value; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Parses a .NGR command line
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Parser(u32 line, char *line_buffer, u8 *if_state, u8 *nesting_level)
{
  s32 status = 0;

  // sscanf consumes too much memory, therefore we parse directly
  char *brkt;
  char *parameter;

  if( (parameter = remove_quotes(ngr_strtok_r(line_buffer, separators, &brkt))) ) {

    if( *parameter == 0 || *parameter == '#' ) {
      // ignore comments and empty lines
      return 0;
    }

    if( strcasecmp(parameter, "IF") == 0 ) {
      if( !if_state ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' not supported from terminal command line!\n", line, parameter);
#endif
	return -1;
      }

      if( *nesting_level >= IF_MAX_NESTING_LEVEL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: max nesting level (%d) for if commands reached!\n", line, IF_MAX_NESTING_LEVEL);
#endif
	return 2; // due to error
      } else {
	++(*nesting_level);

	if( *nesting_level >= 2 && if_state[*nesting_level-2] == 0 ) { // this IF is executed inside a non-matching block
	  if_state[*nesting_level-1] = 0;
	} else {
	  s32 match = parseCondition(line, parameter, &brkt);
	  if( match < 0 ) {
	    return 2; // exit due to error
	  } else {
	    if_state[*nesting_level-1] = match ? 1 : 0;
	  }
	}
      }
      return 0; // read next line
    }

    if( strcasecmp(parameter, "ELSEIF") == 0 || strcasecmp(parameter, "ELSIF") == 0 ) {
      if( !if_state ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' not supported from terminal command line!\n", line, parameter);
#endif
	return -1;
      }

      if( *nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unexpected %s statement!\n", line, parameter);
#endif
      } else {
	if( *nesting_level >= 2 && if_state[*nesting_level-2] == 0 ) { // this ELSIF is executed inside a non-matching block
	  if_state[*nesting_level-1] = 0;
	} else {
	  if( if_state[*nesting_level-1] == 0 ) { // no matching IF condition yet?
	    s32 match = parseCondition(line, parameter, &brkt);
	    if( match < 0 ) {
	      return 2; // exit due to error
	    } else {
	      if_state[*nesting_level-1] = match ? 1 : 0;
	    }
	  } else {
	    if_state[*nesting_level-1] = 2; // IF has been processed
	  }
	}
      }
      return 0; // read next line
    }

    if( strcasecmp(parameter, "ENDIF") == 0 ) {
      if( !if_state ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' not supported from terminal command line!\n", line, parameter);
#endif
	return -1;
      }

      if( !lineIsEmpty(brkt) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: no program code allowed in the same line as ENDIF!\n", line);
#endif
	return -1;
      }

      if( *nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unexpected %s statement!\n", line, parameter);
#endif
      } else {
	--(*nesting_level);
      }
      return 0; // read next line
    }

    if( strcasecmp(parameter, "ELSE") == 0 ) {
      if( !if_state ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' not supported from terminal command line!\n", line, parameter);
#endif
	return -1;
      }
      if( !lineIsEmpty(brkt) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: no program code allowed in the same line as ELSE!\n", line);
#endif
	return -1;
      }

      if( *nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unexpected %s statement!\n", line, parameter);
#endif
      } else {
	if( *nesting_level >= 2 && if_state[*nesting_level-2] == 0 ) { // this ELSIF is executed inside a non-matching block
	  if_state[*nesting_level-1] = 0;
	} else {
	  if( if_state[*nesting_level-1] == 0 ) { // no matching IF condition yet?
	    if_state[*nesting_level-1] = 1; // matching condition
	  } else {
	    if_state[*nesting_level-1] = 2; // IF has been processed
	  }
	}
      }
      return 0; // read next line
    }

    if( !if_state || *nesting_level == 0 || if_state[*nesting_level-1] == 1 ) {
      if( strcasecmp(parameter,
 "LCD") == 0 ) {
	char *str = brkt;
	if( !(str=remove_quotes(str)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing string after LCD message!\n", line);
#endif
	} else {
	  // print from a dummy item
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
	  item.label = str;
	  MBNG_LCD_PrintItemLabel(&item, NULL, 0);
	}
      } else if( strcasecmp(parameter, "LOG") == 0 ) {
	char *str = brkt;
	if( !(str=remove_quotes(str)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing string after LOG message!\n", line);
#endif
	} else {
	  MIOS32_MIDI_SendDebugString(str);
	}
      } else if( strcasecmp(parameter, "SEND") == 0 ) {
	parseSend(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "EXEC_META") == 0 ) {
	parseExecMeta(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "EXIT") == 0 ) {
	if( nesting_level )
	  *nesting_level = 0; // doesn't matter anymore
	return 1;
      } else if( strcasecmp(parameter, "SET") == 0 ) {
	parseSet(line, parameter, &brkt, 1); // and send
      } else if( strcasecmp(parameter, "CHANGE") == 0 ) {
	parseSet(line, parameter, &brkt, 0); // don't send
      } else if( strcasecmp(parameter, "SET_RGB") == 0 ) {
	parseSetRgb(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "SET_LOCK") == 0 ) {
	parseSetLock(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "SET_ACTIVE") == 0 ) {
	parseSetActive(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "SET_MIN") == 0 ) {
	parseSetMinMax(line, parameter, &brkt, 0); // min
      } else if( strcasecmp(parameter, "SET_MAX") == 0 ) {
	parseSetMinMax(line, parameter, &brkt, 1); // max
      } else if( strcasecmp(parameter, "TRIGGER") == 0 ) {
	parseTrigger(line, parameter, &brkt);
      } else if( strcasecmp(parameter, "DELAY_MS") == 0 ) {
	s32 delay = parseDelay(line, parameter, &brkt);
	mbng_file_r_delay_ctr = (delay >= 0 ) ? delay : 0;
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	// changed error to warning, since people are sometimes confused about these messages
	// on file format changes
	DEBUG_MSG("[MBNG_FILE_R:%d] WARNING: unknown command: %s", line, line_buffer);
#endif
      }
    } else {
#if DEBUG_VERBOSE_LEVEL >= 4
      // no real error, can for example happen in .csv file
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR no space or semicolon separator in following line: %s", line, line_buffer);
#endif
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! reads the config file content (again)
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Read(char *filename, u8 cont_script)
{
  s32 status = 0;
  mbng_file_r_info_t *info = &mbng_file_r_info;

  if( !info->valid ) {
    // store current file name in global variable for UI
    memcpy(mbng_file_r_script_name, filename, MBNG_FILE_R_FILENAME_LEN+1);

    char filepath[MAX_PATH];
    sprintf(filepath, "%s%s.NGR", MBNG_FILES_PATH, mbng_file_r_script_name);

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_R] Open config '%s'\n", filepath);
#endif

    if( (status=FILE_ReadOpen(&info->r_file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R] %s (optional run script) not found\n", filepath);
#endif
      return status;
    }
  } else {
    if( (status=FILE_ReadReOpen(&info->r_file)) < 0 ||
	(!cont_script && (status=FILE_ReadSeek(0)) < 0) ) {
      info->valid = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R] ERROR: %s can't be re-opened!\n", mbng_file_r_script_name);
#endif
      return status;
    }
  }

  // allocate 1024 bytes from heap
  u32 line_buffer_size = 1024;
  char *line_buffer = pvPortMalloc(line_buffer_size);
  u32 line_buffer_len = 0;
  if( !line_buffer ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R] FATAL: out of heap memory!\n");
#endif
    return -1;
  }

  // read commands
  s32 exit = 0;
  u32 line = 0;
  if( !cont_script )
    nesting_level = 0;
  do {
    ++line;
    status=FILE_ReadLine((u8 *)(line_buffer+line_buffer_len), line_buffer_size-line_buffer_len);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      if( line_buffer_len )
	MIOS32_MIDI_SendDebugString("+++");
      MIOS32_MIDI_SendDebugString(line_buffer);
#endif

      // concatenate?
      u32 new_len = strlen(line_buffer);
      // remove spaces
      while( new_len >= 1 && line_buffer[new_len-1] == ' ' ) {
	line_buffer[new_len-1] = 0;
	--new_len;
      }
      if( new_len >= 1 && line_buffer[new_len-1] == '\\' ) {
	line_buffer[new_len-1] = ' ';
	line_buffer[new_len] = 0;
	line_buffer_len = new_len;
	continue; // read next line
      } else {
	line_buffer_len = 0; // for next round we start at 0 again
      }

      exit = MBNG_FILE_R_Parser(line, line_buffer, if_state, &nesting_level);
    }

  } while( !exit && !mbng_file_r_delay_ctr && status >= 1 );

  // release memory from heap
  vPortFree(line_buffer);

#if DEBUG_VERBOSE_LEVEL >= 1
  if( !mbng_file_r_delay_ctr ) {
    if( exit >= 2 ) {
      DEBUG_MSG("[MBNG_FILE_R:%d] stopped script execution due to previous error!\n", line);
    } else if( nesting_level > 0 ) {
      DEBUG_MSG("[MBNG_FILE_R:%d] WARNING: missing ENDIF command!\n", line);
    }
  }
#endif

  // close file
  status |= FILE_ReadClose(&info->r_file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_R_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! request to read the script file from run thread
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_ReadRequest(char *filename, u8 section, s16 value, u8 notify_done)
{
  // new file?
  if( filename ) {
    mbng_file_r_info.valid = 0; // Ensure that it will be opened

    // store current file name in global variable for UI
    memcpy(mbng_file_r_script_name, filename, MBNG_FILE_R_FILENAME_LEN+1);
  }

  // set request (and value)
  mbng_file_r_req.section = section;
  mbng_file_r_req.value = value;
  mbng_file_r_req.notify_done = notify_done;
  mbng_file_r_req.load = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! checks for a read request, should be called periodically each mS
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_CheckRequest(void)
{
  // load requested? reset delay counter
  if( mbng_file_r_req.load )
    mbng_file_r_delay_ctr = 0;

  // check if script has to be continued after a delay
  u8 cont_script = 0;
  if( mbng_file_r_delay_ctr ) {
    if( --mbng_file_r_delay_ctr ) {
      return 0; // wait until 0 is reached
    }
    cont_script = 1; // continue with script execution
  }

  // read request for run file?
  if( cont_script || mbng_file_r_req.load ) {
    mbng_file_r_req.load = 0;

#if DEBUG_FILE_HANDLER_PERFORMANCE
    MIOS32_STOPWATCH_Reset();
#endif

    vars.section = mbng_file_r_req.section;
    vars.value = mbng_file_r_req.value;

    MUTEX_MIDIOUT_TAKE;
    MUTEX_SDCARD_TAKE;
    MBNG_FILE_R_Read(mbng_file_r_script_name, cont_script);
    MUTEX_SDCARD_GIVE;
    MUTEX_MIDIOUT_GIVE;

#if DEBUG_FILE_HANDLER_PERFORMANCE
    u32 cycles = MIOS32_STOPWATCH_ValueGet();
    if( cycles == 0xffffffff )
      DEBUG_MSG("[PERF NGR] overrun!\n");
    else
      DEBUG_MSG("[PERF NGR] %5d.%d mS\n", cycles/10, cycles%10);
#endif

    if( mbng_file_r_req.notify_done && !mbng_file_r_delay_ctr ) {
      MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("%s.NGR with ^section==%d ^value==%d processed.", mbng_file_r_script_name, mbng_file_r_req.section, mbng_file_r_req.value);
      MUTEX_MIDIOUT_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Stops the execution of a currently running script
//! \returns < 0 on errors, 0 if script not running, 1 if script running (or requested)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_RunStop(void)
{
  if( mbng_file_r_delay_ctr || mbng_file_r_req.load ) {
    mbng_file_r_delay_ctr = 0;
    mbng_file_r_req.load = 0;

    return 1; // script was running
  }

  return 0; // script wasn't running
}

//! \}
