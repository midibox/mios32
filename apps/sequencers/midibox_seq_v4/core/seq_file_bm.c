// $Id$
/*
 * Bookmark File access functions
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

#include "file.h"
#include "seq_file.h"
#include "seq_file_bm.h"

#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBSEQ files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH "/"
//#define SEQ_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} seq_file_bm_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// 0: session
// 1: global
static seq_file_bm_info_t seq_file_bm_info[2];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Init(u32 mode)
{
  // invalidate file info
  SEQ_FILE_BM_Unload(0);
  SEQ_FILE_BM_Unload(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Load(char *session, u8 global)
{
  s32 error;
  error = SEQ_FILE_BM_Read(session, global);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_BM] Tried to open config file, status: %d\n", error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Unload(u8 global)
{
  seq_file_bm_info[global].valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Valid(u8 global)
{
  return seq_file_bm_info[global].valid;
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -256 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -256; // modification for SEQ_FILE_BM: return -256 if value invalid, since we read signed bytes

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// reads the config file content (again)
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Read(char *session, u8 global)
{
  s32 status = 0;
  seq_file_bm_info_t *info = &seq_file_bm_info[global];
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  if( global )
    sprintf(filepath, "%sMBSEQ_BM.V4", SEQ_FILES_PATH);
  else
    sprintf(filepath, "%s/%s/MBSEQ_BM.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_BM] Open config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_BM] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  u8 current_bookmark = 0;
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_BM] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	u8 parameter_enabled = 1;
	if( *parameter == '-' ) {
	  parameter_enabled = 0;
	  ++parameter;
	} else if( *parameter == '+' ) {
	  ++parameter;
	}

	if( *parameter == '#' ) {
	  // ignore comments
	} else if( strcmp(parameter, "Slot") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 1 || value > SEQ_UI_BOOKMARKS_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid Slot number %d\n", value);
#endif
	  } else {
	    current_bookmark = value - 1;
	  }
	} else if( strcmp(parameter, "Name") == 0 ) {
	  //char *word = strtok_r(NULL, separators, &brkt);
	  seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];
	  //strncpy(bm->name, word, 6);
	  //strncpy(bm->name, (char *)&line_buffer+5, 6); // allow spaces...
	  strncpy(bm->name, brkt, 6); // allow spaces...
	} else if( strcmp(parameter, "ParLayer") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  int layer = word[0] - 'A';
	  if( layer < 0 || layer > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid ParLayer '%s' in Slot %d\n", word, current_bookmark+1);
#endif
	  } else {
	    seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];
	    bm->par_layer = layer;
	    bm->enable.PAR_LAYER = parameter_enabled;
	  }
	} else if( strcmp(parameter, "TrgLayer") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  int layer = word[0] - 'A';
	  if( layer < 0 || layer > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid TrgLayer '%s' in Slot %d\n", word, current_bookmark+1);
#endif
	  } else {
	    seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];
	    bm->trg_layer = layer;
	    bm->enable.TRG_LAYER = parameter_enabled;
	  }
	} else if( strcmp(parameter, "Tracks") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  if( strlen(word) < 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid Tracks Parameter '%s' in Slot %d\n", word, current_bookmark+1);
#endif
	  } else {
	    seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];
	    bm->tracks = 0;
	    int i;
	    for(i=0; i<16; ++i)
	      if( word[i] == '1' )
		bm->tracks |= (1 << i);
	    bm->enable.TRACKS = parameter_enabled;
	  }
	} else if( strcmp(parameter, "Mutes") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  if( strlen(word) < 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid Mutes Parameter '%s' in Slot %d\n", word, current_bookmark+1);
#endif
	  } else {
	    seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];
	    bm->mutes = 0;
	    int i;
	    for(i=0; i<16; ++i)
	      if( word[i] == '1' )
		bm->mutes |= (1 << i);
	    bm->enable.MUTES = parameter_enabled;
	  }
	} else if( strcmp(parameter, "Page") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];

#ifdef MBSEQV4L
	  seq_ui_page_t page = SEQ_UI_PAGE_NONE;
#else
	  seq_ui_page_t page = SEQ_UI_PAGES_CfgNameSearch(word);
	  if( page == SEQ_UI_PAGE_NONE ) {
	    s32 value = get_dec(word);
	    if( value <= 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid value '%s' for parameter '%s' in Slot %d\n", word, parameter, current_bookmark);
#endif
	    } else {
	      page = SEQ_UI_PAGES_OldBmIndexSearch(value);
	    }
	  }
#endif

	  if( page == SEQ_UI_PAGE_NONE ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid value '%s' for parameter '%s' in Slot %d\n", word, parameter, current_bookmark);
#endif
	  } else {
	    bm->page = page;
	    bm->enable.PAGE = parameter_enabled;
	  }
	} else {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  seq_ui_bookmark_t *bm = &seq_ui_bookmarks[current_bookmark];

	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_BM] ERROR invalid value '%s', for parameter '%s' in Slot %d\n", word, parameter, current_bookmark);
#endif
	  } else if( strcmp(parameter, "Group") == 0 ) {
	    bm->group = value - 1;
	    bm->enable.GROUP = parameter_enabled;
	  } else if( strcmp(parameter, "Instrument") == 0 ) {
	    bm->instrument = value - 1;
	    bm->enable.INSTRUMENT = parameter_enabled;
	  } else if( strcmp(parameter, "StepView") == 0 ) {
	    bm->step_view = value - 1;
	    bm->enable.STEP_VIEW = parameter_enabled;
	  } else if( strcmp(parameter, "Step") == 0 ) {
	    bm->step = value - 1;
	    bm->enable.STEP = parameter_enabled;
	  } else if( strcmp(parameter, "EditView") == 0 ) {
	    bm->edit_view = value;
	    bm->enable.EDIT_VIEW = parameter_enabled;
	  } else if( strcmp(parameter, "Solo") == 0 ) {
	    bm->flags.SOLO = value ? 1 : 0;
	    bm->enable.SOLO = parameter_enabled;
	  } else if( strcmp(parameter, "All") == 0 ) {
	    bm->flags.CHANGE_ALL_STEPS = value ? 1 : 0;
	    bm->enable.CHANGE_ALL_STEPS = parameter_enabled;
	  } else if( strcmp(parameter, "Fast") == 0 ) {
	    bm->flags.FAST = value ? 1 : 0;
	    bm->enable.FAST = parameter_enabled;
	  } else if( strcmp(parameter, "Metronome") == 0 ) {
	    bm->flags.METRONOME = value ? 1 : 0;
	    bm->enable.METRONOME = parameter_enabled;
	  } else if( strcmp(parameter, "LoopMode") == 0 ) {
	    bm->flags.LOOP = value ? 1 : 0;
	    bm->enable.LOOP = parameter_enabled;
	  } else if( strcmp(parameter, "FollowMode") == 0 ) {
	    bm->flags.FOLLOW = value ? 1 : 0;
	    bm->enable.FOLLOW = parameter_enabled;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown parameter: %s in Slot %d", line_buffer, current_bookmark);
#endif
	  }
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_BM] ERROR no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_BM] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_BM_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function to write data into file or send to debug terminal
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_BM_Write_Hlp(u8 write_to_file, u8 global)
{
  s32 status = 0;
  char line_buffer[200];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  u8 from_bookmark = global ? 0 : 8;
  u8 to_bookmark = global ? 7 : (SEQ_UI_BOOKMARKS_NUM-1);

  // write bookmarks
  u8 bookmark;
  seq_ui_bookmark_t *bm = &seq_ui_bookmarks[from_bookmark];
  for(bookmark=from_bookmark; bookmark<=to_bookmark; ++bookmark, ++bm) {
    int i;

    sprintf(line_buffer, "####################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "Slot %d\n", bookmark+1);
    FLUSH_BUFFER;
    sprintf(line_buffer, "Name %s\n", bm->name);
    FLUSH_BUFFER;
    sprintf(line_buffer, "####################\n");
    FLUSH_BUFFER;

#ifdef MBSEQV4L
    sprintf(line_buffer, "%cPage %s\n", bm->enable.PAGE ? '+' : '-', bm->page);
#else
    sprintf(line_buffer, "%cPage %s\n", bm->enable.PAGE ? '+' : '-', SEQ_UI_PAGES_CfgNameGet(bm->page));
#endif
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cGroup %d\n", bm->enable.GROUP ? '+' : '-', bm->group+1);
    FLUSH_BUFFER;

    sprintf(line_buffer, "%cTracks xxxxxxxxxxxxxxxx\n", bm->enable.TRACKS ? '+' : '-');
    for(i=0; i<16; ++i)
      line_buffer[8+i] = (bm->tracks & (1 << i)) ? '1' : '0';
    FLUSH_BUFFER;

    sprintf(line_buffer, "%cMutes xxxxxxxxxxxxxxxx\n", bm->enable.MUTES ? '+' : '-');
    for(i=0; i<16; ++i)
      line_buffer[7+i] = (bm->mutes & (1 << i)) ? '1' : '0';
    FLUSH_BUFFER;

    sprintf(line_buffer, "%cParLayer %c\n", bm->enable.PAR_LAYER ? '+' : '-', 'A'+bm->par_layer);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cTrgLayer %c\n", bm->enable.TRG_LAYER ? '+' : '-', 'A'+bm->trg_layer);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cInstrument %d\n", bm->enable.INSTRUMENT ? '+' : '-', bm->instrument+1);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cStepView %d\n", bm->enable.STEP_VIEW ? '+' : '-', (int)bm->step_view+1);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cStep %d\n", bm->enable.STEP ? '+' : '-', (int)bm->step+1);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cEditView %d\n", bm->enable.EDIT_VIEW ? '+' : '-', bm->edit_view);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cSolo %d\n", bm->enable.SOLO ? '+' : '-', bm->flags.SOLO);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cAll %d\n", bm->enable.CHANGE_ALL_STEPS ? '+' : '-', bm->flags.CHANGE_ALL_STEPS);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cFast %d\n", bm->enable.FAST ? '+' : '-', bm->flags.FAST);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cMetronome %d\n", bm->enable.METRONOME ? '+' : '-', bm->flags.METRONOME);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cLoopMode %d\n", bm->enable.LOOP ? '+' : '-', bm->flags.LOOP);
    FLUSH_BUFFER;
    sprintf(line_buffer, "%cFollowMode %d\n", bm->enable.FOLLOW ? '+' : '-', bm->flags.FOLLOW);
    FLUSH_BUFFER;


    sprintf(line_buffer, "\n");
    FLUSH_BUFFER;
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// writes the groove file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Write(char* session, u8 global)
{
  seq_file_bm_info_t *info = &seq_file_bm_info[global];

  char filepath[MAX_PATH];
  if( global )
    sprintf(filepath, "%sMBSEQ_BM.V4", SEQ_FILES_PATH);
  else
    sprintf(filepath, "%s/%s/MBSEQ_BM.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_BM] Open config file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_BM] Failed to open/create config file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= SEQ_FILE_BM_Write_Hlp(1, global);

  // close file
  status |= FILE_WriteClose();

  // check if file is valid
  if( status >= 0 )
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_BM] config file written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_BM_ERR_WRITE : 0;
}

/////////////////////////////////////////////////////////////////////////////
// sends groove data to debug terminal
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_BM_Debug(u8 global)
{
  return SEQ_FILE_BM_Write_Hlp(0, global); // send to debug terminal
}
