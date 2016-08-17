// $Id$
//! \defgroup MBNG_FILE_R
//! Run File access functions
//! 
//! NOTE: before accessing the SD Card, the upper level function should
//! synchronize with the SD Card semaphore!
//!   MUTEX_SDCARD_TAKE; // to take the semaphore
//!   MUTEX_SDCARD_GIVE; // to release the semaphore
//! \{
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
#include <seq_bpm.h>
#include <seq_midi_out.h>

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
//! Tokenize the source file?
/////////////////////////////////////////////////////////////////////////////
#if defined(MIOS32_FAMILY_STM32F4xx)
# define NGR_TOKENIZED 1
#else
# define NGR_TOKENIZED 0
#endif


/////////////////////////////////////////////////////////////////////////////
//! Nesting vars
/////////////////////////////////////////////////////////////////////////////
#define IF_MAX_NESTING_LEVEL 16
static u8 nesting_level;
static u8 if_state[IF_MAX_NESTING_LEVEL];


/////////////////////////////////////////////////////////////////////////////
//! Defines and variables for the tokenizer
/////////////////////////////////////////////////////////////////////////////
#if NGR_TOKENIZED
#define NGR_TOKEN_MEM_SIZE 16384
static u8 ngr_token_mem[NGR_TOKEN_MEM_SIZE];
static u32 ngr_token_mem_end;
static u8 if_offset[IF_MAX_NESTING_LEVEL];
#endif

static u32 ngr_token_mem_run_pos; // used by some debug messages

static u32 disable_tokenized_ngr; // can be changed from .NGC to *disable* tokenized NGR for compatibility checks

// the tokens are sometimes also used for other purposes (e.g. TOKEN_COND_* used in parseCondition()) if NGR_TOKENIZED not set!
typedef enum {
  TOKEN_NOP             = 0,

  TOKEN_EXIT            = 0x01,
  TOKEN_DELAY_MS        = 0x02,
  TOKEN_LCD             = 0x03, // followed by 0-terminated string
  TOKEN_LOG             = 0x04, // followed by 0-terminated string
  TOKEN_SEND            = 0x05, // followed by port, event_type, stream_len and stream values
  TOKEN_SEND_SEQ        = 0x06, // followed by delay, length, port, event_type, stream_len, stream values
  TOKEN_EXEC_META       = 0x07, // followed by multiple bytes (depending on meta type) + (TOKEN_VALUE_*)
  TOKEN_SET             = 0x08, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_CHANGE          = 0x09, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_TRIGGER         = 0x0a, // followed by 3 bytes (TOKEN_VALUE_*)
  TOKEN_SET_RGB         = 0x0b, // followed by 5 bytes (TOKEN_VALUE_*) + id + rgb value
  TOKEN_SET_HSV         = 0x0c, // followed by 7 bytes (TOKEN_VALUE_*) + id + hsv value
  TOKEN_SET_LOCK        = 0x0d, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_SET_ACTIVE      = 0x0e, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_SET_NO_DUMP     = 0x0f, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_SET_MIN         = 0x10, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)
  TOKEN_SET_MAX         = 0x11, // followed by 3 bytes (TOKEN_VALUE_*) + operation (x bytes)

  TOKEN_IF              = 0x80, // +2 bytes for jump offset
  TOKEN_ELSE            = 0x81, // +2 bytes for jump offset
  TOKEN_ELSEIF          = 0x82, // +2 bytes for jump offset
  TOKEN_ENDIF           = 0x84,

  TOKEN_COND_EQ         = 0x90,
  TOKEN_COND_NE         = 0x91,
  TOKEN_COND_GTEQ       = 0x92,
  TOKEN_COND_LTEQ       = 0x93,
  TOKEN_COND_GT         = 0x94,
  TOKEN_COND_LT         = 0x95,

#define TOKEN_MATH_BEGIN TOKEN_MATH_PLUS // for parseTokenizedValue()
  TOKEN_MATH_PLUS       = 0xa0,
  TOKEN_MATH_MINUS      = 0xa1,
  TOKEN_MATH_MUL        = 0xa2,
  TOKEN_MATH_DIV        = 0xa3,
  TOKEN_MATH_REMAIN     = 0xa4,
  TOKEN_MATH_AND        = 0xa5,
  TOKEN_MATH_OR         = 0xa6,
  TOKEN_MATH_XOR        = 0xa7,
#define TOKEN_MATH_END TOKEN_MATH_XOR // for parseTokenizedValue()

  TOKEN_VALUE_CONST8    = 0xb0, // +1 byte for the constant value
  TOKEN_VALUE_CONST16   = 0xb1, // +1 byte for the constant value
  TOKEN_VALUE_ID        = 0xb2, // +2 bytes for ID
  TOKEN_VALUE_HW_ID     = 0xb3, // +2 bytes for ID

  TOKEN_VALUE_SECTION   = 0xb4,
  TOKEN_VALUE_VALUE     = 0xb5,
  TOKEN_VALUE_BANK      = 0xb6,
  TOKEN_VALUE_SYSEX_DEV = 0xb7,
  TOKEN_VALUE_SYSEX_PAT = 0xb8,
  TOKEN_VALUE_SYSEX_BNK = 0xb9,
  TOKEN_VALUE_SYSEX_INS = 0xba,
  TOKEN_VALUE_SYSEX_CHN = 0xbb,

} ngr_token_t;


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
  unsigned valid: 1;      // file is accessible
  unsigned tokenized: 1;  // file has already been tokenized
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

  u8 tokenize_req = 0;
#if NGR_TOKENIZED
  if( !disable_tokenized_ngr ) {
    tokenize_req = 1; // called if file not valid yet
  }
#endif

  error = MBNG_FILE_R_Read(filename, 0, tokenize_req);
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
  mbng_file_r_info.tokenized = 0;
  mbng_file_r_req.ALL = 0;
  mbng_file_r_delay_ctr = 0;

#if NGR_TOKENIZED
  ngr_token_mem_run_pos = 0;
  ngr_token_mem_end = 0;
#endif

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
//! Allows to enable/disable tokenized NGR
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_TokenizedNgrSet(u8 enable)
{
  disable_tokenized_ngr = enable ? 0 : 1;

  if( !enable ) {
    MBNG_FILE_R_Unload();
  }

  return 0; // no error
}

s32 MBNG_FILE_R_TokenizedNgrGet(void)
{
  return disable_tokenized_ngr ? 0 : 1;
}


/////////////////////////////////////////////////////////////////////////////
//! Prints information about the token memory (used in terminal.c)
//! returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_TokenMemPrint(void)
{
#if !NGR_TOKENIZED
  DEBUG_MSG("ERROR: tokenized .NGR scripts not supported by this processor!");
  return -1;
#else
  DEBUG_MSG("Token processing is: %s", disable_tokenized_ngr ? "disabled" : "enabled");
  DEBUG_MSG("Token memory allocation: %d of %d bytes", ngr_token_mem_end, NGR_TOKEN_MEM_SIZE);
  DEBUG_MSG("%s.NGR file is: %s", mbng_file_r_script_name, mbng_file_r_info.valid ? "valid" : "invalid");
  DEBUG_MSG("Tokens are: %s", mbng_file_r_info.tokenized ? "valid" : "invalid");

  if( ngr_token_mem_end > 0 ) {
    MIOS32_MIDI_SendDebugHexDump(ngr_token_mem, ngr_token_mem_end);
  }

  return 0; // no error
#endif
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


#if NGR_TOKENIZED
/////////////////////////////////////////////////////////////////////////////
//! Adds a token to the ngr memory
//! returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_PushToken(ngr_token_t token, u8 line)
{
  if( ngr_token_mem_end >= (NGR_TOKEN_MEM_SIZE-1) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: no free token memory anymore (%d bytes allocated!)", line, NGR_TOKEN_MEM_SIZE);
#endif
    return -1;
  }

  ngr_token_mem[ngr_token_mem_end++] = (u8)token;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Pushes a string to the ngr memory
//! returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_PushString(char *str, u8 line)
{
  for(; *str != 0; ++str) {
    if( MBNG_FILE_R_PushToken(*str, line) < 0 )
      return -1;
  }

  return MBNG_FILE_R_PushToken(0, line);
}

/////////////////////////////////////////////////////////////////////////////
//! Pushes a memory region to the ngr memory
//! returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_PushMemory(u8 *src, u32 src_size, u8 line)
{
  if( (ngr_token_mem_end+src_size) > NGR_TOKEN_MEM_SIZE ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: no free token memory anymore (%d bytes allocated!)", line, NGR_TOKEN_MEM_SIZE);
#endif
    return -1;
  }

  u8 *dst = (u8 *)&ngr_token_mem[ngr_token_mem_end];
  int i;
  for(i=0; i<src_size; ++i) {
    *(dst++) = *(src++);
  }
  ngr_token_mem_end += src_size;

  return 0; // no error
}
#endif


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
static s32 parseValue(u32 line, char *command, char *value_str, u8 tokenize_req)
{
  if( value_str == NULL || value_str[0] == 0 )
    return -1000000000;

  if( value_str[0] == '[' ) {
    // math operation
    int len = strlen(value_str);
    if( value_str[len-1] != ']' ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid syntax for math operation:", line);
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' should end with ']' in '%s' command!", line, value_str, command);
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
    ngr_token_t math_token = TOKEN_NOP;
    {
      int i;
      for(i=0; i<len; ++i, ++rOperand) {
	switch( *rOperand ) {
	case '+': math_token = TOKEN_MATH_PLUS; break;
	case '-': math_token = TOKEN_MATH_MINUS; break;
	case '*': math_token = TOKEN_MATH_MUL; break;
	case '/': math_token = TOKEN_MATH_DIV; break;
	case '%': math_token = TOKEN_MATH_REMAIN; break;
	case '&': math_token = TOKEN_MATH_AND; break;
	case '|': math_token = TOKEN_MATH_OR; break;
	case '^': math_token = TOKEN_MATH_XOR; break;
	}

	if( math_token != TOKEN_NOP ) {
#if NGR_TOKENIZED
	  if( tokenize_req ) { // store token
	    if( MBNG_FILE_R_PushToken(math_token, line) < 0 )
	      return -1000000000; // exit due to error
	  }
#endif

	  *rOperand = 0;
	  ++rOperand;
	  break;
	}
      }

      if( math_token == TOKEN_NOP ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: no operator in math operation '%s'!", line, value_str);
#endif
	return -1000000000;
      }
    }

    //DEBUG_MSG("Calc: %s %c %s\n", lOperand, operator, rOperand);

    // get left side value (recursively)
    s32 lValue = parseValue(line, command, trim(lOperand), tokenize_req);
    if( lValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid left side operand '%s' in '%s' command!", line, lOperand, command);
#endif
      return -1000000000;
    }

    // get right side value (recursively)
    s32 rValue = parseValue(line, command, trim(rOperand), tokenize_req);
    if( rValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid right side operand '%s' in '%s' command!", line, rOperand, command);
#endif
      return -1000000000;
    }

    switch( math_token ) {
    case TOKEN_MATH_PLUS:   return lValue + rValue;
    case TOKEN_MATH_MINUS:  return lValue - rValue;
    case TOKEN_MATH_MUL:    return lValue * rValue;
    case TOKEN_MATH_DIV:    return lValue / rValue;
    case TOKEN_MATH_REMAIN: return lValue % rValue;
    case TOKEN_MATH_AND:    return lValue & rValue;
    case TOKEN_MATH_OR:     return lValue | rValue;
    case TOKEN_MATH_XOR:    return lValue ^ rValue;
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unsupported operator in math operation '%s'!", line, value_str);
#endif
    return -1000000000;
  }


  if( value_str[0] == '^' ) {
    if( strcasecmp((char *)&value_str[1], "SECTION") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SECTION, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return vars.section;
    } else if( strcasecmp((char *)&value_str[1], "VALUE") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_VALUE, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return vars.value;
    } else if( strcasecmp((char *)&value_str[1], "BANK") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_BANK, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return MBNG_EVENT_SelectedBankGet();
    } else if( strcasecmp((char *)&value_str[1], "DEV") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SYSEX_DEV, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return mbng_patch_cfg.sysex_dev;
    } else if( strcasecmp((char *)&value_str[1], "PAT") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SYSEX_PAT, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return mbng_patch_cfg.sysex_pat;
    } else if( strcasecmp((char *)&value_str[1], "BNK") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SYSEX_BNK, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return mbng_patch_cfg.sysex_bnk;
    } else if( strcasecmp((char *)&value_str[1], "INS") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SYSEX_INS, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return mbng_patch_cfg.sysex_ins;
    } else if( strcasecmp((char *)&value_str[1], "CHN") == 0 ) {
#if NGR_TOKENIZED
      if( tokenize_req ) { // store token
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_SYSEX_CHN, line) < 0 )
	  return -1000000000; // exit due to error
      }
#endif
      return mbng_patch_cfg.sysex_chn;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid or unsupported variable:", line);
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' in '%s' command!", line, value_str, command);
#endif
      return -1000000000;
    }
  }

  mbng_file_r_item_id_t id = parseId(value_str);
  if( id.valid ) {
#if NGR_TOKENIZED
    if( tokenize_req ) { // store token
      if( MBNG_FILE_R_PushToken(id.is_hw_id ? TOKEN_VALUE_HW_ID : TOKEN_VALUE_ID, line) < 0 ||
	  MBNG_FILE_R_PushToken((id.id >> 0) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((id.id >> 8) & 0xff, line) < 0 )
	return -1000000000; // exit due to error
    }
#endif

    // search for items with matching ID
    mbng_event_item_t item;
    u32 continue_ix = 0;
    if( (id.is_hw_id && MBNG_EVENT_ItemSearchByHwId(id.id, &item, &continue_ix) >= 0) ||
	(!id.is_hw_id && MBNG_EVENT_ItemSearchById(id.id, &item, &continue_ix) >= 0) ) {
      return item.value;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: '%s' not found in event pool by '%s' command!", line, value_str, command);
#endif
      return -1000000000;
    }
  }

  s32 value = get_dec(value_str);
#if NGR_TOKENIZED
    if( tokenize_req ) { // store token
      if( value < 0 )
	return -1000000000; // exit due to error
      if( value > 65535 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: constant value '%s' too big, it should be in the range of 0..65535!", line, value_str);
#endif
	return -1000000000;
      }

      if( value & 0xff00 ) {
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_CONST16, line) < 0 ||
	    MBNG_FILE_R_PushToken((value >> 0) & 0xff, line) < 0 ||
	    MBNG_FILE_R_PushToken((value >> 8) & 0xff, line) < 0 )
	  return -1000000000; // exit due to error
      } else {
	if( MBNG_FILE_R_PushToken(TOKEN_VALUE_CONST8, line) < 0 ||
	    MBNG_FILE_R_PushToken((value >> 0) & 0xff, line) < 0 )
	  return -1000000000; // exit due to error
      }
    }
#endif

  return value;
}


#if NGR_TOKENIZED
/////////////////////////////////////////////////////////////////////////////
//! help function which determine a tokenized value of a tokenized condition
//! \returns >= 0 if value is valid
//! \returns -1000000000 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 parseTokenizedValue(void)
{
  u32 init_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
  ngr_token_t token = ngr_token_mem[ngr_token_mem_run_pos++];

  if( token >= TOKEN_MATH_BEGIN && token <= TOKEN_MATH_END ) {
    u32 prev_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
    s32 lValue = parseTokenizedValue();
    if( lValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: invalid left value at mem pos 0x%x!", prev_ngr_token_mem_run_pos);
#endif
      return -4;
    }

    prev_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
    s32 rValue = parseTokenizedValue();
    if( rValue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: invalid right value at mem pos 0x%x!", prev_ngr_token_mem_run_pos);
#endif
      return -5;
    }

    switch( token ) {
    case TOKEN_MATH_PLUS:   return lValue + rValue;
    case TOKEN_MATH_MINUS:  return lValue - rValue;
    case TOKEN_MATH_MUL:    return lValue * rValue;
    case TOKEN_MATH_DIV:    return lValue / rValue;
    case TOKEN_MATH_REMAIN: return lValue % rValue;
    case TOKEN_MATH_AND:    return lValue & rValue;
    case TOKEN_MATH_OR:     return lValue | rValue;
    case TOKEN_MATH_XOR:    return lValue ^ rValue;
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: unsupported operator in math operation at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif
    return -1000000000;
  }

  switch( token ) {
  case TOKEN_VALUE_SECTION:   return vars.section;
  case TOKEN_VALUE_VALUE:     return vars.value;
  case TOKEN_VALUE_BANK:      return MBNG_EVENT_SelectedBankGet();
  case TOKEN_VALUE_SYSEX_DEV: return mbng_patch_cfg.sysex_dev;
  case TOKEN_VALUE_SYSEX_PAT: return mbng_patch_cfg.sysex_pat;
  case TOKEN_VALUE_SYSEX_BNK: return mbng_patch_cfg.sysex_bnk;
  case TOKEN_VALUE_SYSEX_INS: return mbng_patch_cfg.sysex_ins;
  case TOKEN_VALUE_SYSEX_CHN: return mbng_patch_cfg.sysex_chn;

  case TOKEN_VALUE_CONST8: {
    u16 value = (u16)ngr_token_mem[ngr_token_mem_run_pos++];
    return value;
  } break;

  case TOKEN_VALUE_CONST16: {
    u16 value = (u16)ngr_token_mem[ngr_token_mem_run_pos++];
    value |= ((u16)ngr_token_mem[ngr_token_mem_run_pos++] << 8);
    return value;
  } break;

  case TOKEN_VALUE_ID:
  case TOKEN_VALUE_HW_ID: {
    u8 is_hw_id = token == TOKEN_VALUE_HW_ID;
    u16 id = (u16)ngr_token_mem[ngr_token_mem_run_pos++];
    id |= ((u16)ngr_token_mem[ngr_token_mem_run_pos++] << 8);
    mbng_event_item_t item;
    u32 continue_ix = 0;
    if( (is_hw_id && MBNG_EVENT_ItemSearchByHwId(id, &item, &continue_ix) >= 0) ||
	(!is_hw_id && MBNG_EVENT_ItemSearchById(id, &item, &continue_ix) >= 0) ) {
      return item.value;
    }
    DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: (%s)%s:%d not found in event pool at mem pos 0x%x!", 
	      is_hw_id ? "hw_id" : "id", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, init_ngr_token_mem_run_pos);
    return -1000000000;
  } break;
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: unsupported value at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif
  return -1000000000;
}
#endif


/////////////////////////////////////////////////////////////////////////////
//! help function which parses an IF condition
//! \returns >= 0 if condition is valid
//! \returns < 0 if condition is invalid
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseCondition(u32 line, char *command, char **brkt, u8 tokenize_req)
{
  char *lvalue_str, *condition_str, *rvalue_str;

  if( !(lvalue_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing left value of expression in '%s' command!", line, command);
#endif
    return -1;
  }

  if( !(condition_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing condition of expression in '%s' command!", line, command);
#endif
    return -2;
  }

  if( !(rvalue_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing right value of expression in '%s' command!", line, command);
#endif
    return -3;
  }

  ngr_token_t cond_token = TOKEN_NOP;
  if( strcasecmp(condition_str, "==") == 0 ) {
    cond_token = TOKEN_COND_EQ;
  } else if( strcasecmp(condition_str, "!=") == 0 ) {
    cond_token = TOKEN_COND_NE;
  } else if( strcasecmp(condition_str, ">=") == 0 ) {
    cond_token = TOKEN_COND_GTEQ;
  } else if( strcasecmp(condition_str, "<=") == 0 ) {
    cond_token = TOKEN_COND_LTEQ;
  } else if( strcasecmp(condition_str, ">") == 0 ) {
    cond_token = TOKEN_COND_GT;
  } else if( strcasecmp(condition_str, "<") == 0 ) {
    cond_token = TOKEN_COND_LT;
  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid or unsupported condition '%s' in '%s' command!", line, condition_str, command);
#endif
    return -10;
  }

#if NGR_TOKENIZED
  if( tokenize_req ) {
    if( MBNG_FILE_R_PushToken(cond_token, line) < 0 )
      return -11; // exit due to error
  }
#endif

  s32 lvalue = parseValue(line, command, lvalue_str, tokenize_req);
  if( lvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid left value '%s' in '%s' command!", line, lvalue_str, command);
#endif
    return -4;
  }

  s32 rvalue = parseValue(line, command, rvalue_str, tokenize_req);
  if( rvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid right value '%s' in '%s' command!", line, rvalue_str, command);
#endif
    return -5;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] condition: %s %s %s (%d %s %d)\n", line, lvalue_str, condition_str, rvalue_str, lvalue, condition_str, rvalue);
#endif

  if( cond_token == TOKEN_COND_EQ   ) return lvalue == rvalue;
  if( cond_token == TOKEN_COND_NE   ) return lvalue != rvalue;
  if( cond_token == TOKEN_COND_GTEQ ) return lvalue >= rvalue;
  if( cond_token == TOKEN_COND_LTEQ ) return lvalue <= rvalue;
  if( cond_token == TOKEN_COND_GT   ) return lvalue >  rvalue;
  if( cond_token == TOKEN_COND_LT   ) return lvalue <  rvalue;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid or unsupported condition '%s' in '%s' command!", line, condition_str, command);
#endif

  return -10; // invalid condition
}


#if NGR_TOKENIZED
/////////////////////////////////////////////////////////////////////////////
//! help function which parses an IF condition token
//! \returns >= 0 if condition is valid
//! \returns < 0 if condition is invalid
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseTokenizedCondition(void)
{
  u32 init_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
  ngr_token_t cond_token = ngr_token_mem[ngr_token_mem_run_pos++];

  u32 prev_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
  s32 lvalue = parseTokenizedValue();
  if( lvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: invalid left value at mem pos 0x%x!", prev_ngr_token_mem_run_pos);
#endif
    return -4;
  }

  prev_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
  s32 rvalue = parseTokenizedValue();
  if( rvalue <= -1000000000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: invalid right value at mem pos 0x%x!", prev_ngr_token_mem_run_pos);
#endif
    return -5;
  }

  //DEBUG_MSG("[parseTokenizedCondition]: cond %02x lvalue %04x rvalue %04x\n", cond_token, lvalue, rvalue);

  if( cond_token == TOKEN_COND_EQ   ) return lvalue == rvalue;
  if( cond_token == TOKEN_COND_NE   ) return lvalue != rvalue;
  if( cond_token == TOKEN_COND_GTEQ ) return lvalue >= rvalue;
  if( cond_token == TOKEN_COND_LTEQ ) return lvalue <= rvalue;
  if( cond_token == TOKEN_COND_GT   ) return lvalue >  rvalue;
  if( cond_token == TOKEN_COND_LT   ) return lvalue <  rvalue;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: invalid condition at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif

  return -10; // invalid condition
}
#endif


/////////////////////////////////////////////////////////////////////////////
//! help function which executes a tokenized SEND command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 sendTokenized(mios32_midi_port_t port, mbng_event_type_t event_type, u8 *stream, u32 stream_size)
{
  MUTEX_MIDIOUT_TAKE;
  switch( event_type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       MIOS32_MIDI_SendNoteOff(port, (stream[0]-1) & 0xf, stream[1] & 0x7f, stream[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_NOTE_ON:        MIOS32_MIDI_SendNoteOn(port, (stream[0]-1) & 0xf, stream[1] & 0x7f, stream[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_NOTE_ON_OFF:    MIOS32_MIDI_SendNoteOn(port, (stream[0]-1) & 0xf, stream[1] & 0x7f, stream[2] & 0x7f); MIOS32_MIDI_SendNoteOn(port, (stream[0]-1) & 0xf, stream[1] & 0x7f, 0); break;
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  MIOS32_MIDI_SendPolyPressure(port, (stream[0]-1) & 0xf, stream[1] & 0x7f, stream[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_CC:             MIOS32_MIDI_SendCC(port, (stream[0]-1), stream[1] & 0x7f, stream[2] & 0x7f); break;
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: MIOS32_MIDI_SendProgramChange(port, (stream[0]-1) & 0xf, stream[1] & 0x7f); break;
  case MBNG_EVENT_TYPE_AFTERTOUCH:     MIOS32_MIDI_SendAftertouch(port, (stream[0]-1) & 0xf, stream[1] & 0x7f); break;
  case MBNG_EVENT_TYPE_PITCHBEND:      MIOS32_MIDI_SendPitchBend(port, (stream[0]-1) & 0xf, (stream[1] + 8192) & 0x3fff); break;
  case MBNG_EVENT_TYPE_NRPN:           MBNG_EVENT_SendOptimizedNRPN(port, (stream[0]-1) & 0xf, stream[1], stream[2], 0); break; // msb_only == 0

  case MBNG_EVENT_TYPE_SYSEX: {
    if( !stream_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: unexpected condition for 'send SysEx' at mem pos 0x%x!", ngr_token_mem_run_pos);
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

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SEND command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSend(u32 line, char *command, char **brkt, u8 tokenize_req)
{
  char *event_str;
  if( !(event_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing event type in '%s' command!", line, command);
#endif
    return -1;
  }

  mbng_event_type_t event_type;
  if( (event_type=MBNG_EVENT_ItemTypeFromStrGet(event_str)) == MBNG_EVENT_TYPE_UNDEFINED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unknown event type '%s' in '%s' command!", line, event_str, command);
#endif
    return -1;
  }


  char *port_str;
  if( !(port_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing MIDI port in '%s' command!", line, command);
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
    
    if( out_port == 0xff && ((out_port=parseValue(line, command, port_str, tokenize_req)) < 0 || out_port > 0xff) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid MIDI port '%s' in '%s' command!", line, port_str, command);
#endif
      return -1;
    }

    port = (mios32_midi_port_t)out_port;
  }

#if NGR_TOKENIZED
  u32 memo_stream_size_pos = 0;
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(TOKEN_SEND, line) < 0 ||
	MBNG_FILE_R_PushToken(event_type, line) < 0 ||
	MBNG_FILE_R_PushToken(port, line) < 0 )
      return -1;

    memo_stream_size_pos = ngr_token_mem_end; // will be inserted at the end of this function
    if( MBNG_FILE_R_PushToken(0, line) < 0 ) // dummy value which will be replaced
      return -1;
  }
#endif


#define STREAM_MAX_SIZE 256
  u8 stream[STREAM_MAX_SIZE];
  int stream_size = 0;

  if( event_type == MBNG_EVENT_TYPE_SYSEX ) { // extra handling
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
#if NGR_TOKENIZED
	if( tokenize_req ) {
	  if( MBNG_FILE_R_PushToken(TOKEN_VALUE_CONST8, line) < 0 ||
	      MBNG_FILE_R_PushToken(0xff, line) < 0 ||
	      MBNG_FILE_R_PushToken(TOKEN_VALUE_CONST8, line) < 0 ||
	      MBNG_FILE_R_PushToken(sysex_var, line) < 0 )
	    return -1;
	}
#endif
      } else {
	int value;
	if( (value=parseValue(line, command, stream_str, tokenize_req)) < 0 || value > 0xff ) {
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

#if NGR_TOKENIZED
    if( tokenize_req && memo_stream_size_pos ) {
      // insert of stream
      ngr_token_mem[memo_stream_size_pos] = stream_size;
    }
#endif

  } else {
    int num_values = 0;

    switch( event_type ) {
    case MBNG_EVENT_TYPE_NOTE_OFF:       num_values = 3; break;
    case MBNG_EVENT_TYPE_NOTE_ON:        num_values = 3; break;
    case MBNG_EVENT_TYPE_NOTE_ON_OFF:    num_values = 3; break;
    case MBNG_EVENT_TYPE_POLY_PRESSURE:  num_values = 3; break;
    case MBNG_EVENT_TYPE_CC:             num_values = 3; break;
    case MBNG_EVENT_TYPE_PROGRAM_CHANGE: num_values = 2; break;
    case MBNG_EVENT_TYPE_AFTERTOUCH:     num_values = 2; break;
    case MBNG_EVENT_TYPE_PITCHBEND:      num_values = 2; break;
    case MBNG_EVENT_TYPE_NRPN:           num_values = 3; break;
    case MBNG_EVENT_TYPE_META: {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: use EXEC_META to send meta events!", line);
#endif
      return -1;
    }
    }

    // for non-sysex
    if( !num_values ) {
      return -1; // ???
    } else {
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
	if( (value=parseValue(line, command, value_str, tokenize_req)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value for '%s' event in '%s' command!", line, event_str, command);
#endif
	  return -1;
	}
      
	stream[i] = value;
      }
      stream_size = num_values;

#if NGR_TOKENIZED
      if( tokenize_req && memo_stream_size_pos ) {
	// insert of stream
	ngr_token_mem[memo_stream_size_pos] = stream_size;
      }
#endif
    }
  }

#if NGR_TOKENIZED
  if( !tokenize_req )
#endif
  {
    sendTokenized(port, event_type, stream, stream_size);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which executes a tokenized SEND command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 sendSeqTokenized(u16 delay, u16 len, mios32_midi_port_t port, mbng_event_type_t event_type, u8 *stream, u32 stream_size)
{
  mios32_midi_package_t p;
  p.ALL = 0;
  p.chn = (stream[0]-1) & 0xf;
  p.evnt1 = stream[1] & 0x7f;
  p.evnt2 = stream[2] & 0x7f;

  switch( event_type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       p.type = p.event = NoteOff; break;
  case MBNG_EVENT_TYPE_NOTE_ON:        p.type = p.event = NoteOn; break;
  case MBNG_EVENT_TYPE_NOTE_ON_OFF:    p.type = p.event = NoteOn; break;
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  p.type = p.event = PolyPressure; break;
  case MBNG_EVENT_TYPE_CC:             p.type = p.event = CC; break;
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: p.type = p.event = ProgramChange; break;
  case MBNG_EVENT_TYPE_AFTERTOUCH:     p.type = p.event = Aftertouch; break;
  case MBNG_EVENT_TYPE_PITCHBEND:      p.type = p.event = PitchBend; break;
  //case MBNG_EVENT_TYPE_NRPN: // not supported
  //case MBNG_EVENT_TYPE_SYSEX: // not supported
  //case MBNG_EVENT_TYPE_META: // not supported
    return 0;
  }

  MUTEX_MIDIOUT_TAKE;
  if( !SEQ_BPM_IsRunning() ) {
    // send immediately
    MIOS32_MIDI_SendPackage(port, p);

    if( event_type == MBNG_EVENT_TYPE_NOTE_ON_OFF ) {
      p.velocity = 0;
      MIOS32_MIDI_SendPackage(port, p);
    }
  } else {
    u32 bpm_tick = SEQ_BPM_TickGet() + delay;
    if( event_type == MBNG_EVENT_TYPE_NOTE_ON_OFF ) {
      SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, len);
    } else if( event_type == MBNG_EVENT_TYPE_NOTE_ON ) {
      SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_OnEvent, bpm_tick, len);
    } else if( event_type == MBNG_EVENT_TYPE_NOTE_OFF ) {
      SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_OffEvent, bpm_tick, len);
    } else {
      SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_CCEvent, bpm_tick, len);
    }
  }
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SEND_SEQ command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseSendSeq(u32 line, char *command, char **brkt, u8 tokenize_req)
{
#if NGR_TOKENIZED
  u32 memo_stream_size_pos = 0;
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(TOKEN_SEND_SEQ, line) < 0 )
      return  -1;
  }
#endif

  char *value_str;
  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing delay after '%s' command!\n", line, command);
#endif
    return -1;
  }

  int delay;
  if( (delay=parseValue(line, command, value_str, tokenize_req)) < 0 || delay > 65535 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid delay value in '%s' command (expecting 0..65535)!\n", line, command);
#endif
    return -1;
  }


  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing delay after '%s' command!\n", line, command);
#endif
    return -1;
  }

  int len;
  if( (len=parseValue(line, command, value_str, tokenize_req)) < 0 || len > 65535 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid length value in '%s' command (expecting 0..65535)!\n", line, command);
#endif
    return -1;
  }


  char *event_str;
  if( !(event_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing event type in '%s' command!", line, command);
#endif
    return -1;
  }

  mbng_event_type_t event_type;
  if( (event_type=MBNG_EVENT_ItemTypeFromStrGet(event_str)) == MBNG_EVENT_TYPE_UNDEFINED ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: unknown event type '%s' in '%s' command!", line, event_str, command);
#endif
    return -1;
  }


  char *port_str;
  if( !(port_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing MIDI port in '%s' command!", line, command);
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
    
    if( out_port == 0xff && ((out_port=parseValue(line, command, port_str, tokenize_req)) < 0 || out_port > 0xff) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid MIDI port '%s' in '%s' command!", line, port_str, command);
#endif
      return -1;
    }

    port = (mios32_midi_port_t)out_port;
  }

#if NGR_TOKENIZED
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(event_type, line) < 0 ||
	MBNG_FILE_R_PushToken(port, line) < 0 )
      return -1;

    memo_stream_size_pos = ngr_token_mem_end; // will be inserted at the end of this function
    if( MBNG_FILE_R_PushToken(0, line) < 0 ) // dummy value which will be replaced
      return -1;
  }
#endif


#define STREAM_MAX_SIZE 256
  u8 stream[STREAM_MAX_SIZE];
  int stream_size = 0;

  if( event_type == MBNG_EVENT_TYPE_SYSEX ) { // extra handling
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: SEND_SEQ doesn't support SYSEX events!", line);
#endif
    return -1;
  } else {
    int num_values = 0;

    switch( event_type ) {
    case MBNG_EVENT_TYPE_NOTE_OFF:       num_values = 3; break;
    case MBNG_EVENT_TYPE_NOTE_ON:        num_values = 3; break;
    case MBNG_EVENT_TYPE_NOTE_ON_OFF:    num_values = 3; break;
    case MBNG_EVENT_TYPE_POLY_PRESSURE:  num_values = 3; break;
    case MBNG_EVENT_TYPE_CC:             num_values = 3; break;
    case MBNG_EVENT_TYPE_PROGRAM_CHANGE: num_values = 2; break;
    case MBNG_EVENT_TYPE_AFTERTOUCH:     num_values = 2; break;
    case MBNG_EVENT_TYPE_PITCHBEND:      num_values = 2; break;
    case MBNG_EVENT_TYPE_NRPN: {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: SEND_SEQ doesn't support NRPN events!", line);
#endif
      return -1;
    }

    case MBNG_EVENT_TYPE_META: {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: use EXEC_META to send meta events!", line);
#endif
      return -1;
    }
    }

    // for non-sysex
    if( !num_values ) {
      return -1; // ???
    } else {
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
	if( (value=parseValue(line, command, value_str, tokenize_req)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value for '%s' event in '%s' command!", line, event_str, command);
#endif
	  return -1;
	}
      
	stream[i] = value;
      }
      stream_size = num_values;

#if NGR_TOKENIZED
      if( tokenize_req && memo_stream_size_pos ) {
	// insert of stream
	ngr_token_mem[memo_stream_size_pos] = stream_size;
      }
#endif
    }
  }

#if NGR_TOKENIZED
  if( !tokenize_req )
#endif
  {
    sendSeqTokenized(delay, len, port, event_type, stream, stream_size);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a EXEC_META command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseExecMeta(u32 line, char *command, char **brkt, u8 tokenize_req)
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
	(value=get_dec(values_str)) < 0 || value > 0xff ) {
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

#if NGR_TOKENIZED
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(TOKEN_EXEC_META, line) < 0 ||
	MBNG_FILE_R_PushToken(item.stream_size, line) < 0 )
      return -1;

    int i;
    for(i=0; i<item.stream_size; ++i) {
      if( MBNG_FILE_R_PushToken(item.stream[i], line) < 0 )
	return -1;
    }

    // parse optional value
    if( (values_str = ngr_strtok_r(NULL, separators, brkt)) ) {
      s32 value;
      if( (value=parseValue(line, command, values_str, tokenize_req)) < -16384 || value > 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid (optional) value for meta type %s\n", line, MBNG_EVENT_ItemMetaTypeStrGet(meta_type));
#endif
	return -1;
      }
    } else {
      // take ^value
      if( MBNG_FILE_R_PushToken(TOKEN_VALUE_VALUE, line) < 0 )
	return -1;
    }
  } else
#endif
  {
    // parse optional value
    s32 value = vars.value;
    if( (values_str = ngr_strtok_r(NULL, separators, brkt)) ) {

      if( (value=parseValue(line, command, values_str, tokenize_req)) < -16384 || value > 16383 ) {
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
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! help functions for various commands
/////////////////////////////////////////////////////////////////////////////
static s32 execSET(mbng_event_item_t *item, s32 value)
{
  item->value = value;
  MBNG_EVENT_ItemReceive(item, item->value, 1, 0); // "from_midi" and forwarding disabled

  if( MBNG_EVENT_NotifySendValue(item) == 2 )
    return 2; // stop has been requested

  return 0; // no error
}

static s32 execCHANGE(mbng_event_item_t *item, s32 value)
{
  item->value = value;
  return MBNG_EVENT_ItemReceive(item, item->value, 1, 0); // "from_midi" and forwarding disabled
}

static s32 execCHANGE_Virtual(mbng_event_item_id_t id, s32 value) // also used for SET
{
  // hw_id: send to dummy item    
  mbng_event_item_t item;
  MBNG_EVENT_ItemInit(&item, id);
  item.flags.active = 1;
  item.value = value;
  return MBNG_EVENT_ItemSendVirtual(&item, item.id);
}

static s32 execTRIGGER(mbng_event_item_t *item, s32 value)
{
  if( MBNG_EVENT_NotifySendValue(item) == 2 )
    return 2; // stop has been requested

  return 0; // no error
}

static s32 execSET_RGB(mbng_event_item_t *item, s32 value)
{
  item->rgb.ALL = value;
  MBNG_EVENT_ItemModify(item);
  MBNG_EVENT_ItemReceive(item, item->value, 1, 0); // "from_midi" and forwarding disabled

  if( MBNG_EVENT_NotifySendValue(item) == 2 )
    return 2; // stop has been requested

  return 0; // no error
}

static s32 execSET_HSV(mbng_event_item_t *item, s32 value)
{
  item->hsv.ALL = (u32)value;
  MBNG_EVENT_ItemModify(item);
  MBNG_EVENT_ItemReceive(item, item->value, 1, 0); // "from_midi" and forwarding disabled

  if( MBNG_EVENT_NotifySendValue(item) == 2 )
    return 2; // stop has been requested

  return 0; // no error
}

static s32 execSET_LOCK(mbng_event_item_t *item, s32 value)
{
  return MBNG_EVENT_ItemSetLock(item, value);
}

static s32 execSET_ACTIVE(mbng_event_item_t *item, s32 value)
{
  return MBNG_EVENT_ItemSetActive(item, value);
}

static s32 execSET_NO_DUMP(mbng_event_item_t *item, s32 value)
{
  return MBNG_EVENT_ItemSetNoDump(item, value);
}

static s32 execSET_MIN(mbng_event_item_t *item, s32 value)
{
  item->min = value;
  return MBNG_EVENT_ItemModify(item);
}

static s32 execSET_MAX(mbng_event_item_t *item, s32 value)
{
  item->max = value;
  return MBNG_EVENT_ItemModify(item);
}


/////////////////////////////////////////////////////////////////////////////
//! help function which SETs a value based on a token
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 setTokenizedValue(ngr_token_t token, u16 id, s32 value, s32 (*exec_function)(mbng_event_item_t *item, s32 value), s32 (*exec_virtual_function)(mbng_event_item_id_t id, s32 value))
{
  switch( token ) {
  case TOKEN_VALUE_SECTION:   vars.section = value; break;
  case TOKEN_VALUE_VALUE:     vars.value = value; break;
  case TOKEN_VALUE_BANK:      MBNG_EVENT_SelectedBankSet(value); break;
  case TOKEN_VALUE_SYSEX_DEV: mbng_patch_cfg.sysex_dev = value; break;
  case TOKEN_VALUE_SYSEX_PAT: mbng_patch_cfg.sysex_pat = value; break;
  case TOKEN_VALUE_SYSEX_BNK: mbng_patch_cfg.sysex_bnk = value; break;
  case TOKEN_VALUE_SYSEX_INS: mbng_patch_cfg.sysex_ins = value; break;
  case TOKEN_VALUE_SYSEX_CHN: mbng_patch_cfg.sysex_chn = value; break;

  case TOKEN_VALUE_ID:
  case TOKEN_VALUE_HW_ID: {
    u8 is_hw_id = token == TOKEN_VALUE_HW_ID;

    // search for items with matching ID
    mbng_event_item_t item;
    u32 continue_ix = 0;
    u32 num_exec_items = 0;
    do {
      if( (is_hw_id && MBNG_EVENT_ItemSearchByHwId(id, &item, &continue_ix) < 0) ||
	  (!is_hw_id && MBNG_EVENT_ItemSearchById(id, &item, &continue_ix) < 0) ) {
	break;
      } else {
	++num_exec_items;

	// pass to item
	if( exec_function(&item, value) > 0 )
	  break; // stop has been requested
      }
    } while( continue_ix );

    if( !num_exec_items ) {
      if( !is_hw_id ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: (%s)%s:%d not found in event pool at mem pos 0x%x (must be hw_id)!", 
		  is_hw_id ? "hw_id" : "id", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, ngr_token_mem_run_pos);
#endif
	return -1;
      }

      // pass to virtual item
      if( exec_virtual_function )
	exec_virtual_function(id, value);
    }

  } break;
  }

  return 0; // no error
}


#if NGR_TOKENIZED
/////////////////////////////////////////////////////////////////////////////
//! help function which executes a token
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 execToken(ngr_token_t command_token, u8 if_condition_matching, s32 (*exec_function)(mbng_event_item_t *item, s32 value), s32 (*exec_virtual_function)(mbng_event_item_id_t id, s32 value))
{
  ngr_token_t value_token = ngr_token_mem[ngr_token_mem_run_pos++];

  u16 id = 0;
  if( value_token == TOKEN_VALUE_ID || value_token == TOKEN_VALUE_HW_ID ) {
    id = (u16)ngr_token_mem[ngr_token_mem_run_pos++];
    id |= (u16)ngr_token_mem[ngr_token_mem_run_pos++] << 8;
  }
	
  s32 value = 0;
  switch( command_token ) {

  case TOKEN_TRIGGER:
    break; // no additional parameter

  case TOKEN_SET_RGB:
    value  = (u16)ngr_token_mem[ngr_token_mem_run_pos++];
    value |= ((u16)ngr_token_mem[ngr_token_mem_run_pos++] << 8);
    break;

  case TOKEN_SET_HSV:
    value  = (u32)ngr_token_mem[ngr_token_mem_run_pos++];
    value |= ((u32)ngr_token_mem[ngr_token_mem_run_pos++] << 8);
    value |= ((u32)ngr_token_mem[ngr_token_mem_run_pos++] << 16);
    value |= ((u32)ngr_token_mem[ngr_token_mem_run_pos++] << 24);
    break;

  default:
    value = parseTokenizedValue();
  }

  if( if_condition_matching && value >= -16384 ) {
    setTokenizedValue(value_token, id, value, exec_function, exec_virtual_function);
  }

  return 0; // no error
}
#endif


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a SET command
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseCommand(u32 line, char *command, char **brkt, u8 tokenize_req, ngr_token_t command_token, s32 (*exec_function)(mbng_event_item_t *item, s32 value), s32 (*exec_virtual_function)(mbng_event_item_id_t id, s32 value))
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

  ngr_token_t value_token = TOKEN_NOP;
  if( is_var ) {
    if( strcasecmp((char *)&dst_str[1], "SECTION") == 0 ) {
      value_token = TOKEN_VALUE_SECTION;
    } else if( strcasecmp((char *)&dst_str[1], "VALUE") == 0 ) {
      value_token = TOKEN_VALUE_VALUE;
    } else if( strcasecmp((char *)&dst_str[1], "BANK") == 0 ) {
      value_token = TOKEN_VALUE_BANK;
    } else if( strcasecmp((char *)&dst_str[1], "DEV") == 0 ) {
      value_token = TOKEN_VALUE_SYSEX_DEV;
    } else if( strcasecmp((char *)&dst_str[1], "PAT") == 0 ) {
      value_token = TOKEN_VALUE_SYSEX_PAT;
    } else if( strcasecmp((char *)&dst_str[1], "BNK") == 0 ) {
      value_token = TOKEN_VALUE_SYSEX_BNK;
    } else if( strcasecmp((char *)&dst_str[1], "INS") == 0 ) {
      value_token = TOKEN_VALUE_SYSEX_INS;
    } else if( strcasecmp((char *)&dst_str[1], "CHN") == 0 ) {
      value_token = TOKEN_VALUE_SYSEX_CHN;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid or unsupported variable '%s' in '%s' command!", line, dst_str, command);
#endif
      return -1;
    }
  } else {
    value_token = id.is_hw_id ? TOKEN_VALUE_HW_ID : TOKEN_VALUE_ID;
  }

#if NGR_TOKENIZED
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(command_token, line) < 0 ||
	MBNG_FILE_R_PushToken(value_token, line) < 0 )
      return -1;

    if( value_token == TOKEN_VALUE_ID || value_token == TOKEN_VALUE_HW_ID ) {
      if( MBNG_FILE_R_PushToken((id.id >> 0) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((id.id >> 8) & 0xff, line) < 0 )
	return -1;
    }
  }
#endif

  s32 value = 0;
  switch( command_token ) {
  /////////////////////////////////////////////////////////////////////////////
  case TOKEN_SET_RGB: {
    int r = 0;
    int g = 0;
    int b = 0;

    if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    char *brkt_local;
    if( !(value_str = ngr_strtok_r(value_str, separator_colon, &brkt_local)) ||
	(r=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(g=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(b=get_dec(value_str)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid RGB format in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    if( r < 0 || r >= 16 ||
	g < 0 || g >= 16 ||
	b < 0 || b >= 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid RGB values in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    mbng_event_rgb_t rgb;
    rgb.ALL = 0;
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;

    value = rgb.ALL;

#if NGR_TOKENIZED
    if( tokenize_req ) { // store token
      if( MBNG_FILE_R_PushToken((rgb.ALL >> 0) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((rgb.ALL >> 8) & 0xff, line) < 0 )
	return -1;
    }
#endif
  } break;

  /////////////////////////////////////////////////////////////////////////////
  case TOKEN_SET_HSV: {
    int h = 0;
    int s = 0;
    int v = 0;

    if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    char *brkt_local;
    if( !(value_str = ngr_strtok_r(value_str, separator_colon, &brkt_local)) ||
	(h=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(s=get_dec(value_str)) < 0 ||
	!(value_str = ngr_strtok_r(NULL, separator_colon, &brkt_local)) ||
	(v=get_dec(value_str)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid HSV format in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    if( h < 0 || h >= 360 ||
	s < 0 || s >= 101 ||
	v < 0 || v >= 101 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid HSV values in '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    mbng_event_hsv_t hsv;
    hsv.ALL = 0;
    hsv.h = h;
    hsv.s = s;
    hsv.v = v;

    value = hsv.ALL;

#if NGR_TOKENIZED
    if( tokenize_req ) { // store token
      if( MBNG_FILE_R_PushToken((hsv.ALL >> 0) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((hsv.ALL >> 8) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((hsv.ALL >> 16) & 0xff, line) < 0 ||
	  MBNG_FILE_R_PushToken((hsv.ALL >> 24) & 0xff, line) < 0 )
	return -1;
    }
#endif
  } break;

  /////////////////////////////////////////////////////////////////////////////
  case TOKEN_TRIGGER:
    break; // no value

  /////////////////////////////////////////////////////////////////////////////
  default: // all other command tokens
    if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s %s' command!\n", line, command, dst_str);
#endif
      return -1;
    }

    if( (value=parseValue(line, command, value_str, tokenize_req)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: invalid value in '%s %s %s' command (expecting -16384..16383!\n", line, command, dst_str, value_str);
#endif
      return -1;
    }
  }
  /////////////////////////////////////////////////////////////////////////////


#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] %s = %d\n", line, dst_str, value);
#endif

#if NGR_TOKENIZED
  if( !tokenize_req )
#endif
  {
    setTokenizedValue(value_token, id.id, value, exec_function, exec_virtual_function);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! help function which parses a DELAY_MS command
//! returns > 0 if a delay has been requested
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseDelay(u32 line, char *command, char **brkt, u8 tokenize_req)
{
  char *value_str;
  s32 value;
  if( !(value_str = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing value after '%s' command!\n", line, command);
#endif
    return -1;
  }

#if NGR_TOKENIZED
  if( tokenize_req ) { // store token
    if( MBNG_FILE_R_PushToken(TOKEN_DELAY_MS, line) < 0 )
      return -1;
  }
#endif

  if( (value=parseValue(line, command, value_str, tokenize_req)) < 0 || value > 100000 ) {
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
//! help function which parses a LOAD command
//! returns > 0 if a valid filename has been specified
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_R_Read - this will blow up the stack usage too much!
s32 parseLoad(u32 line, char *command, char **brkt, char *load_filename, u8 tokenize_req)
{
  char *filename;
  if( !(filename = ngr_strtok_r(NULL, separators, brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing filename after '%s' command!\n", line, command);
#endif
    return -1;
  }

  int i;
  for(i=0; i<8; ++i) {
    if( filename[i] == '.' ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: remove file extension after '.' in '%s' command !\n", line, command);
#endif
      return -2;
    }
    if( filename[i] == 0 )
      break;
  }

  if( i >= 8 && filename[i] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: too long filename specified in '%s' command !\n", line, command);
#endif
    return -3;
  }

  // take over filename
  strncpy(load_filename, filename, 8);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R:%d] LOAD %s\n", line, load_filename);
#endif

  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//! Parses a .NGR command line
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Parser(u32 line, char *line_buffer, u8 *if_state, u8 *nesting_level, char *load_filename, u8 tokenize_req)
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


    /////////////////////////////////////////////////////////////////////////
    //
    // IF/ELSE/ELSEIF/ENDIF
    //
    /////////////////////////////////////////////////////////////////////////
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

	if( !tokenize_req && *nesting_level >= 2 && if_state[*nesting_level-2] != 1 ) { // this IF is executed inside a non-matching block
	  if_state[*nesting_level-1] = 0;
	} else {
#if NGR_TOKENIZED
	  if( tokenize_req ) { // store token
	    if( MBNG_FILE_R_PushToken(TOKEN_IF, line) < 0 ||
		MBNG_FILE_R_PushToken(0, line) < 0 || // placeholder - jump offset will be determined during second pass
		MBNG_FILE_R_PushToken(0, line) < 0 ) // placeholder
	      return 2; // exit due to error
	  }
#endif

	  s32 match = parseCondition(line, parameter, &brkt, tokenize_req);

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
	if( !tokenize_req && *nesting_level >= 2 && if_state[*nesting_level-2] != 1 ) { // this ELSIF is executed inside a non-matching block
	  if_state[*nesting_level-1] = 0;
	} else {
	  if( tokenize_req || if_state[*nesting_level-1] == 0 ) { // no matching IF condition yet?

#if NGR_TOKENIZED
	    if( tokenize_req ) { // store token
	      if( MBNG_FILE_R_PushToken(TOKEN_ELSEIF, line) < 0 ||
		MBNG_FILE_R_PushToken(0, line) < 0 || // placeholder - jump offset will be determined during second pass
		MBNG_FILE_R_PushToken(0, line) < 0 ) // placeholder
		return 2; // exit due to error
	    }
#endif

	    s32 match = parseCondition(line, parameter, &brkt, tokenize_req);
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
#if NGR_TOKENIZED
	if( tokenize_req ) { // store token
	  if( MBNG_FILE_R_PushToken(TOKEN_ENDIF, line) < 0 )
	    return 2; // exit due to error
	}
#endif

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
#if NGR_TOKENIZED
	if( tokenize_req ) { // store token
	  if( MBNG_FILE_R_PushToken(TOKEN_ELSE, line) < 0 ||
	      MBNG_FILE_R_PushToken(0, line) < 0 || // placeholder - jump offset will be determined during second pass
	      MBNG_FILE_R_PushToken(0, line) < 0 ) // placeholder
	    return 2; // exit due to error
	}
#endif

	if( *nesting_level >= 2 && if_state[*nesting_level-2] != 1 ) { // this ELSE is executed inside a non-matching block
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


    /////////////////////////////////////////////////////////////////////////
    //
    // COMMANDS
    //
    /////////////////////////////////////////////////////////////////////////
    if( !if_state || tokenize_req || *nesting_level == 0 || if_state[*nesting_level-1] == 1 ) {
      if( strcasecmp(parameter, "LCD") == 0 ) {
	char *str = brkt;
	if( !(str=remove_quotes(str)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing string after LCD message!\n", line);
#endif
	} else {
#if NGR_TOKENIZED
	  if( tokenize_req ) {
	    MBNG_FILE_R_PushToken(TOKEN_LCD, line);
	    MBNG_FILE_R_PushString(str, line);
	  } else
#endif
	  {
	    // print from a dummy item
	    mbng_event_item_t item;
	    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
	    item.label = str;
	    MBNG_LCD_PrintItemLabel(&item, NULL, 0);
	  }
	}
      } else if( strcasecmp(parameter, "LOG") == 0 ) {
	char *str = brkt;
	if( !(str=remove_quotes(str)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: missing string after LOG message!\n", line);
#endif
	} else {
#if NGR_TOKENIZED
	  if( tokenize_req ) {
	    MBNG_FILE_R_PushToken(TOKEN_LOG, line);
	    MBNG_FILE_R_PushString(str, line);
	  } else
#endif
	  {
	    MIOS32_MIDI_SendDebugString(str);
	  }
	}
      } else if( strcasecmp(parameter, "SEND") == 0 ) {
	parseSend(line, parameter, &brkt, tokenize_req);
      } else if( strcasecmp(parameter, "SEND_SEQ") == 0 ) {
	parseSendSeq(line, parameter, &brkt, tokenize_req);
      } else if( strcasecmp(parameter, "EXEC_META") == 0 ) {
	parseExecMeta(line, parameter, &brkt, tokenize_req);
      } else if( strcasecmp(parameter, "EXIT") == 0 ) {
#if NGR_TOKENIZED
	if( tokenize_req ) {
	  MBNG_FILE_R_PushToken(TOKEN_EXIT, line);
	} else
#endif
        {
	  if( nesting_level )
	    *nesting_level = 0; // doesn't matter anymore
	  return 1;
	}
      } else if( strcasecmp(parameter, "SET") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET, execSET, execCHANGE_Virtual);
      } else if( strcasecmp(parameter, "CHANGE") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_CHANGE, execCHANGE, execCHANGE_Virtual);
      } else if( strcasecmp(parameter, "SET_RGB") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_RGB, execSET_RGB, NULL);
      } else if( strcasecmp(parameter, "SET_HSV") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_HSV, execSET_HSV, NULL);
      } else if( strcasecmp(parameter, "SET_LOCK") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_LOCK, execSET_LOCK, NULL);
      } else if( strcasecmp(parameter, "SET_ACTIVE") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_ACTIVE, execSET_ACTIVE, NULL);
      } else if( strcasecmp(parameter, "SET_NO_DUMP") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_NO_DUMP, execSET_NO_DUMP, NULL);
      } else if( strcasecmp(parameter, "SET_MIN") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_MIN, execSET_MIN, NULL);
      } else if( strcasecmp(parameter, "SET_MAX") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_SET_MAX, execSET_MAX, NULL);
      } else if( strcasecmp(parameter, "TRIGGER") == 0 ) {
	parseCommand(line, parameter, &brkt, tokenize_req, TOKEN_TRIGGER, execTRIGGER, NULL);
      } else if( strcasecmp(parameter, "DELAY_MS") == 0 ) {
	s32 delay = parseDelay(line, parameter, &brkt, tokenize_req);
#if NGR_TOKENIZED
	if( !tokenize_req )
#endif
	  mbng_file_r_delay_ctr = (delay >= 0 ) ? delay : 0;
      } else if( strcasecmp(parameter, "LOAD") == 0 ) {
#if NGR_TOKENIZED
	if( tokenize_req ) {
	  DEBUG_MSG("[MBNG_FILE_R:%d] ERROR: the LOAD command is not supported anymore!", line); // let's see if somebody really needs this...
	} else
#endif
	{
	  if( parseLoad(line, parameter, &brkt, load_filename, tokenize_req) > 0 ) {
	    return 1;
	  }
	}
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
//! Executes the tokenized content of a NGR file
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Exec(u8 cont_script, u8 determine_if_offsets)
{
#if !NGR_TOKENIZED
# if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNG_FILE_R] ERROR: MBNG_FILE_R_Exec has been called, although tokens are not supported!");
# endif
  return -1; // not supported!
#else

  if( determine_if_offsets )
    return -1; // 2nd pass not supported yet...

  if( !cont_script ) {
    ngr_token_mem_run_pos = 0;
    nesting_level = 0;
  }

  while( ngr_token_mem_run_pos < ngr_token_mem_end ) {
    u32 init_ngr_token_mem_run_pos = ngr_token_mem_run_pos;
    ngr_token_t command_token = ngr_token_mem[ngr_token_mem_run_pos++];
    u8 if_condition_matching = nesting_level == 0 || if_state[nesting_level-1] == 1;

    //DEBUG_MSG("[MBNG_FILE_R_Exec:%d] %02x\n", init_ngr_token_mem_run_pos, token);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    switch( command_token ) {

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_IF: {
      if_offset[nesting_level] = init_ngr_token_mem_run_pos;
      ngr_token_mem_run_pos += 2; // offset not used yet!

      if( nesting_level >= IF_MAX_NESTING_LEVEL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: max nesting level (%d) for if commands reached!\n", IF_MAX_NESTING_LEVEL);
#endif
	return -2; // due to error
      } else {
	++nesting_level;

	if( nesting_level >= 2 && if_state[nesting_level-2] != 1 ) { // this IF is executed inside a non-matching block
	  if_state[nesting_level-1] = 0;
	  parseTokenizedCondition(); // dummy
	} else {
	  s32 match = parseTokenizedCondition();

	  if( match < 0 ) {
	    return -2; // exit due to error
	  } else {
	    if_state[nesting_level-1] = match ? 1 : 0;
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_ELSEIF: {
      if( nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: tried to execute an unexpected ELSEIF token at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif
      } else {
	ngr_token_mem_run_pos += 2; // offset not used yet!

	if( nesting_level >= 2 && if_state[nesting_level-2] != 1 ) { // this ELSIF is executed inside a non-matching block
	  if_state[nesting_level-1] = 0;
	  parseTokenizedCondition(); // dummy
	} else {
	  if( if_state[nesting_level-1] == 0 ) { // no matching IF condition yet?
	    s32 match = parseTokenizedCondition();
	    if( match < 0 ) {
	      return -2; // exit due to error
	    } else {
	      if_state[nesting_level-1] = match ? 1 : 0;
	    }
	  } else {
	    parseTokenizedCondition(); // dummy
	    if_state[nesting_level-1] = 2; // IF has been processed
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_ELSE: {
      if( nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: tried to execute an unexpected ELSE token at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif
      } else {
	ngr_token_mem_run_pos += 2; // offset not used yet!

	if( nesting_level >= 2 && if_state[nesting_level-2] != 1 ) { // this ELSE is executed inside a non-matching block
	  if_state[nesting_level-1] = 0;
	} else {
	  if( if_state[nesting_level-1] == 0 ) { // no matching IF condition yet?
	    if_state[nesting_level-1] = 1; // matching condition
	  } else {
	    if_state[nesting_level-1] = 2; // IF has been processed
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_ENDIF: {
      if( nesting_level == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: tried to execute an unexpected ENDIF token at mem pos 0x%x!", init_ngr_token_mem_run_pos);
#endif
      } else {
	--nesting_level;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_EXIT: {
      if( if_condition_matching ) {
	nesting_level = 0; // doesn't matter anymore
	return 1;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_DELAY_MS: {
      s32 value = parseTokenizedValue();
      if( if_condition_matching && value >= 0 ) {
	mbng_file_r_delay_ctr = value;
	return 1;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_LCD: {
      if( if_condition_matching ) {
	// print from a dummy item
	mbng_event_item_t item;
	MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
	item.label = (char *)&ngr_token_mem[ngr_token_mem_run_pos];
	MUTEX_LCD_TAKE;
	MBNG_LCD_PrintItemLabel(&item, NULL, 0);
	MUTEX_LCD_GIVE;
      }

      while( ngr_token_mem[ngr_token_mem_run_pos] != 0 && ngr_token_mem_run_pos < ngr_token_mem_end )
	++ngr_token_mem_run_pos;
      ++ngr_token_mem_run_pos;
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_LOG: {
      if( if_condition_matching ) {
	MIOS32_MIDI_SendDebugString((char *)&ngr_token_mem[ngr_token_mem_run_pos]);
      }

      while( ngr_token_mem[ngr_token_mem_run_pos] != 0 && ngr_token_mem_run_pos < ngr_token_mem_end )
	++ngr_token_mem_run_pos;
      ++ngr_token_mem_run_pos;
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_SEND: {
      mbng_event_type_t event_type = ngr_token_mem[ngr_token_mem_run_pos++];
      mios32_midi_port_t port = ngr_token_mem[ngr_token_mem_run_pos++];
      u8 stream_size = ngr_token_mem[ngr_token_mem_run_pos++];

#define STREAM_MAX_SIZE 256
      u8 stream[STREAM_MAX_SIZE];

      int i;
      for(i=0; i<stream_size && ngr_token_mem_run_pos < ngr_token_mem_end; ++i) {
	s32 value = parseTokenizedValue();
	stream[i] = value;
      }

      if( if_condition_matching ) {
	MUTEX_MIDIOUT_TAKE;
	sendTokenized(port, event_type, stream, stream_size);
	MUTEX_MIDIOUT_GIVE;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_SEND_SEQ: {
      s32 delay = parseTokenizedValue();
      s32 len = parseTokenizedValue();
      mbng_event_type_t event_type = ngr_token_mem[ngr_token_mem_run_pos++];
      mios32_midi_port_t port = ngr_token_mem[ngr_token_mem_run_pos++];
      u8 stream_size = ngr_token_mem[ngr_token_mem_run_pos++];

#define STREAM_MAX_SIZE 256
      u8 stream[STREAM_MAX_SIZE];

      int i;
      for(i=0; i<stream_size && ngr_token_mem_run_pos < ngr_token_mem_end; ++i) {
	s32 value = parseTokenizedValue();
	stream[i] = value;
      }

      if( if_condition_matching ) {
	MUTEX_MIDIOUT_TAKE;
	sendSeqTokenized(delay, len, port, event_type, stream, stream_size);
	MUTEX_MIDIOUT_GIVE;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_EXEC_META: {
      mbng_event_item_t item;
      MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
      u8 stream[10];
      item.stream = stream;

      item.stream_size = ngr_token_mem[ngr_token_mem_run_pos++];
      if( item.stream_size > 10 )
	item.stream_size = 10; // just to ensure...

      int i;
      for(i=0; i<item.stream_size; ++i) {
	item.stream[i] = ngr_token_mem[ngr_token_mem_run_pos++];
      }

      s32 value = parseTokenizedValue();
      item.value = value;

      if( if_condition_matching ) {
	MUTEX_MIDIOUT_TAKE;
	MBNG_EVENT_ExecMeta(&item);
	MUTEX_MIDIOUT_GIVE;

	if( item.stream[0] == MBNG_EVENT_META_TYPE_RUN_SECTION )
	  return 1; // prevent recursion
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case TOKEN_SET:         execToken(command_token, if_condition_matching, execSET, execCHANGE_Virtual); break;
    case TOKEN_CHANGE:      execToken(command_token, if_condition_matching, execCHANGE, execCHANGE_Virtual); break;
    case TOKEN_TRIGGER:     execToken(command_token, if_condition_matching, execTRIGGER, NULL); break;
    case TOKEN_SET_RGB:     execToken(command_token, if_condition_matching, execSET_RGB, NULL); break;
    case TOKEN_SET_HSV:     execToken(command_token, if_condition_matching, execSET_HSV, NULL); break;
    case TOKEN_SET_LOCK:    execToken(command_token, if_condition_matching, execSET_LOCK, NULL); break;
    case TOKEN_SET_ACTIVE:  execToken(command_token, if_condition_matching, execSET_ACTIVE, NULL); break;
    case TOKEN_SET_NO_DUMP: execToken(command_token, if_condition_matching, execSET_NO_DUMP, NULL); break;
    case TOKEN_SET_MIN:     execToken(command_token, if_condition_matching, execSET_MIN, NULL); break;
    case TOKEN_SET_MAX:     execToken(command_token, if_condition_matching, execSET_MAX, NULL); break;

    /////////////////////////////////////////////////////////////////////////
    default:
      // should never happen...
# if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R_Exec] ERROR: tried to execute invalid token 0x%02x at mem pos 0x%x!", command_token, init_ngr_token_mem_run_pos);
# endif
      mbng_file_r_info.valid = 0;
      mbng_file_r_info.tokenized = 0;
      return -1;
    }
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! reads the config file content (again)
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Read(char *filename, u8 cont_script, u8 tokenize_req)
{
  s32 status = 0;
  mbng_file_r_info_t *info = &mbng_file_r_info;

  char load_filename[9];

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

#if NGR_TOKENIZED
  if( tokenize_req && !cont_script ) {
    ngr_token_mem_end = 0;
    ngr_token_mem_run_pos = 0;
  }
#endif

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

      load_filename[0] = 0; // invalidate filename

      // parse line
      exit = MBNG_FILE_R_Parser(line, line_buffer, if_state, &nesting_level, load_filename, tokenize_req);
    }

  } while( !exit && !mbng_file_r_delay_ctr && status >= 1 );

  // release memory from heap
  vPortFree(line_buffer);

#if DEBUG_VERBOSE_LEVEL >= 1
  if( !mbng_file_r_delay_ctr ) {
    if( exit >= 2 ) {
      DEBUG_MSG("[MBNG_FILE_R:%d] stopped script execution due to previous error!\n", line);
    } else if( !load_filename[0] && nesting_level > 0 ) {
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

#if NGR_TOKENIZED
  if( tokenize_req ) {
    // tokens valid as well
    info->tokenized = 1;

    // run 2nd pass to insert jump offsets after IF/ELSE/ELSEIF
    MBNG_FILE_R_Exec(0, 1); // cont_script, determine_if_offsets
  }
#endif

  if( load_filename[0] ) {
    s32 status = MBNG_PATCH_Load(load_filename);

    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_R] ERROR while loading patch %s\n", load_filename);
#endif
    }
  }

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

#if NGR_TOKENIZED
  if( !disable_tokenized_ngr && mbng_file_r_info.valid && mbng_file_r_info.tokenized ) {
    mbng_file_r_req.load = 0;
    vars.section = section;
    vars.value = value;

    MUTEX_MIDIOUT_TAKE;
    MBNG_FILE_R_Exec(0, 0);
    MUTEX_MIDIOUT_GIVE;
  } else
#endif
  {
    mbng_file_r_req.load = 1;
  }

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

#if NGR_TOKENIZED
    if( disable_tokenized_ngr || !mbng_file_r_info.valid || !mbng_file_r_info.tokenized )
#endif
    {
      u8 tokenize_req = 0;
#if NGR_TOKENIZED
      if( !disable_tokenized_ngr ) {
	tokenize_req = 1; // called if file not valid yet
	cont_script = 0; // just to ensure...
      }
#endif

      MUTEX_MIDIOUT_TAKE;
      MUTEX_SDCARD_TAKE;
      MBNG_FILE_R_Read(mbng_file_r_script_name, cont_script, tokenize_req);
      MUTEX_SDCARD_GIVE;
      MUTEX_MIDIOUT_GIVE;
    }

#if NGR_TOKENIZED
    if( mbng_file_r_info.valid && mbng_file_r_info.tokenized ) {
      MUTEX_MIDIOUT_TAKE;
      MBNG_FILE_R_Exec(cont_script, 0);
      MUTEX_MIDIOUT_GIVE;
    }
#endif

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
