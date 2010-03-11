// $Id$
/*
 * Groove File access functions
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

#include <string.h>

#include "seq_file.h"
#include "seq_file_g.h"

#include "seq_groove.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} seq_file_g_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_g_info_t seq_file_g_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Init(u32 mode)
{
  // invalidate file info
  SEQ_FILE_G_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Load(void)
{
  s32 error;
  error = SEQ_FILE_G_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_G] Tried to open config file, status: %d\n", error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Unload(void)
{
  seq_file_g_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Valid(void)
{
  return seq_file_g_info.valid;
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
    return -256; // modification for SEQ_FILE_G: return -256 if value invalid, since we read signed bytes

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// reads the config file content (again)
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Read(void)
{
  s32 status = 0;
  seq_file_g_info_t *info = &seq_file_g_info;
  seq_file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_G.V4", SEQ_FILE_SESSION_PATH, seq_file_session_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_G] Open config file '%s'\n", filepath);
#endif

  if( (status=SEQ_FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_G] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  char line_buffer[128];
  do {
    status=SEQ_FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_G] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == '#' ) {
	  // ignore comments
	} else {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_G] ERROR invalid value for parameter '%s'\n", parameter);
#endif
	  } else if( strcmp(parameter, "NumSteps") == 0 ) {
	    u8 groove = value;
	    if( groove < SEQ_GROOVE_NUM_TEMPLATES ) {
	      word = strtok_r(NULL, separators, &brkt);
	      int num_steps = get_dec(word);
	      if( num_steps > 0 )
		seq_groove_templates[groove].num_steps = num_steps;
	    }
	  } else if( strcmp(parameter, "Delay") == 0 ) {
	    u8 groove = value;
	    if( groove < SEQ_GROOVE_NUM_TEMPLATES ) {
	      int i;
	      for(i=0; i<16; ++i) {
		word = strtok_r(NULL, separators, &brkt);
		seq_groove_templates[groove].add_step_delay[i] = get_dec(word);
	      }
	    }
	  } else if( strcmp(parameter, "Length") == 0 ) {
	    u8 groove = value;
	    if( groove < SEQ_GROOVE_NUM_TEMPLATES ) {
	      int i;
	      for(i=0; i<16; ++i) {
		word = strtok_r(NULL, separators, &brkt);
		seq_groove_templates[groove].add_step_length[i] = get_dec(word);
	      }
	    }
	  } else if( strcmp(parameter, "Velocity") == 0 ) {
	    u8 groove = value;
	    if( groove < SEQ_GROOVE_NUM_TEMPLATES ) {
	      int i;
	      for(i=0; i<16; ++i) {
		word = strtok_r(NULL, separators, &brkt);
		seq_groove_templates[groove].add_step_velocity[i] = get_dec(word);
	      }
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown parameter: %s", line_buffer);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_G] ERROR no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= SEQ_FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_G] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_G_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_G_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[200];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= SEQ_FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  // write groove templates
  u8 groove;
  seq_groove_entry_t *g = &seq_groove_templates[0];
  for(groove=0; groove<SEQ_GROOVE_NUM_TEMPLATES; ++groove, ++g) {
    u8 step;

    sprintf(line_buffer, "NumSteps %d %d\n", groove, g->num_steps);
    FLUSH_BUFFER;

    sprintf(line_buffer, "Delay %2d     ", groove);
    for(step=0; step<16; ++step) {
      sprintf((char *)(line_buffer+strlen(line_buffer)), " %4d", g->add_step_delay[step]);
    }
    sprintf((char *)(line_buffer+strlen(line_buffer)), "\n");
    FLUSH_BUFFER;

    sprintf(line_buffer, "\nLength %2d    ", groove);
    for(step=0; step<16; ++step) {
      sprintf((char *)(line_buffer+strlen(line_buffer)), " %4d", g->add_step_length[step]);
    }
    sprintf((char *)(line_buffer+strlen(line_buffer)), "\n");
    FLUSH_BUFFER;

    sprintf(line_buffer, "\nVelocity %2d  ", groove);
    for(step=0; step<16; ++step) {
      sprintf((char *)(line_buffer+strlen(line_buffer)), " %4d", g->add_step_velocity[step]);
    }
    sprintf((char *)(line_buffer+strlen(line_buffer)), "\n");
    FLUSH_BUFFER;
  }

return status;
}

/////////////////////////////////////////////////////////////////////////////
// writes the groove file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Write(void)
{
  seq_file_g_info_t *info = &seq_file_g_info;

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_G.V4", SEQ_FILE_SESSION_PATH, seq_file_session_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_G] Open config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_G] Failed to open/create config file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= SEQ_FILE_G_Write_Hlp(1);

  // close file
  status |= SEQ_FILE_WriteClose();

  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_G] config file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_G_ERR_WRITE : 0;
}

/////////////////////////////////////////////////////////////////////////////
// sends groove data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_G_Debug(void)
{
  return SEQ_FILE_G_Write_Hlp(0); // send to debug terminal
}
