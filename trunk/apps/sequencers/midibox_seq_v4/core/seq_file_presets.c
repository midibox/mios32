// $Id: seq_file_bm.c 2634 2019-01-05 16:51:12Z tk $
/*
 * Preset Files access functions
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
#include "seq_file_presets.h"

#include "seq_layer.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the preset files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_PRESET_FILES_PATH     "/PRESETS"
#define SEQ_PRESET_TRKLABEL_PATH  SEQ_PRESET_FILES_PATH "/TRKLABEL.V4P"
#define SEQ_PRESET_TRKCATS_PATH   SEQ_PRESET_FILES_PATH "/TRKCATS.V4P"
#define SEQ_PRESET_TRKDRUMS_PATH  SEQ_PRESET_FILES_PATH "/TRKDRUMS.V4P"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  u8 num_trk_labels;
  u8 num_trk_categories;
  u8 num_trk_drums;
} seq_file_presets_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_presets_info_t seq_file_presets_info;

static const char *default_trk_labels[] = {
  "Vintage",
  "Synthline",
  "Bassline",
  "Pads",
  "Chords",
  "Lovely Arps",
  "Electr. Drums",
  "Heavy Beats",
  "Simple Beats",
  "CC Tracks",
  "Experiments",
  "Live Played",
  "Transposer"
};


static const char *default_trk_categories[] = {
  "Synth",
  "Piano",
  "Bass",
  "Drums",
  "Break",
  "MBSID",
  "MBFM",
  "Ctrl"
};


// must match with seq_layer_preset_table_drum_notes in seq_layer.c
static const char default_trk_drums[16][6] = {
  " BD  ",
  " SD  ",
  " CH  ",
  " PH  ",
  " OH  ",
  " MA  ",
  " CP  ",
  " CY  ",
  " LT  ",
  " MT  ",
  " HT  ",
  " RS  ",
  " CB  ",
  "Smp1 ",
  "Smp2 ",
  "Smp3 "
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_Init(u32 mode)
{
  // invalidate file info
  SEQ_FILE_PRESETS_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads preset files
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_Load(void)
{
  s32 status = 0;

  SEQ_FILE_PRESETS_Unload();

  // create default files (if they don't exist)
  SEQ_FILE_PRESETS_CreateDefaults();

  // count preset entries for each file
  if( (status=SEQ_FILE_PRESETS_TrkLabel_Read(0, NULL)) > 0 ) {
    seq_file_presets_info.num_trk_labels = status;
  }

  if( (status=SEQ_FILE_PRESETS_TrkCategory_Read(0, NULL)) > 0 ) {
    seq_file_presets_info.num_trk_categories = status;
  }

  if( (status=SEQ_FILE_PRESETS_TrkDrum_Read(0, NULL, NULL, 1)) > 0 ) {
    seq_file_presets_info.num_trk_drums = status;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads preset files
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_Unload(void)
{
  seq_file_presets_info.num_trk_labels = 0;
  seq_file_presets_info.num_trk_categories = 0;
  seq_file_presets_info.num_trk_drums = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if preset file valid
// Returns 0 if preset file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_Valid(void)
{
  return seq_file_presets_info.num_trk_labels > 0 || seq_file_presets_info.num_trk_categories > 0 || seq_file_presets_info.num_trk_drums > 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns number of presets
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkLabel_NumGet(void)
{
  return seq_file_presets_info.num_trk_labels;
}


/////////////////////////////////////////////////////////////////////////////
// help function which reads a given preset from file (if preset_ix == 0: counts all entries)
// returns < 0 on errors (error codes are documented in seq_file.h), otherwise number of read entries
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_PRESETS_Hlp_Read(const char* filename, u8 preset_ix, char *dst, int max_len, u8 *value, u8 init_layer_preset_notes)
{

  s32 status = 0;
  file_t file;

  if( (status=FILE_ReadOpen(&file, (char *)filename)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_PRESETS] failed to open '%s', status: %d\n", filename, status);
#endif
    return SEQ_FILE_PRESETS_ERR_READ;
  }

  u32 ix = 0;
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 128);

    if( status > 1 ) {

      if( line_buffer[0] == '#' ) {
	// ignore comments
      } else {
	size_t len = strlen(line_buffer);

	// comma separated?
	int pos;
	char *brkt = NULL;
	for(pos=0; pos<len; ++pos) {
	  if( line_buffer[pos] == ',' ) {
	    line_buffer[pos] = 0;
	    len = pos;
	    brkt = &line_buffer[pos+1];
	    break;
	  }
	}

	while( (len >= 1) && (line_buffer[len-1] == ' ') ) { // strip
	  line_buffer[len-1] = 0;
	  --len;
	}

	if( len > max_len ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_FILE_PRESETS:%s] WARNING: line too long, %d chars max: %s", filename, max_len, line_buffer);
#endif
	}

	++ix;

	if( ix >= 256 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_FILE_PRESETS:%s] WARNING: maximum number of items (256) reached!", filename);
#endif
	  break;
	}

	if( dst ) {
	  strncpy(dst, line_buffer, max_len+1);
	}

	if( value && !brkt ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_FILE_PRESETS:%s] WARNING: no comma separated value found in %s", filename, line_buffer);
#endif
	} else if( brkt ) {
	  char *next;
	  long l = strtol(brkt, &next, 0);

	  if( brkt == next || l < 0 || l >= 256 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_PRESETS:%s] WARNING: invalid value: %s", filename, line_buffer);
#endif
	  } else {
	    if( value ) {
	      *value = l;
	    }

	    if( init_layer_preset_notes ) {
	      SEQ_LAYER_PresetDrumNoteSet(ix-1, l);
	    }
	  }
	}

	if( ix == preset_ix ) // consider that 0 will return number of all labels
	  break;
      }
    }
  } while( status >= 1 );

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_PRESETS] errors while parsing '%s'\n", filename);
#endif
    return SEQ_FILE_PRESETS_ERR_FORMAT;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  if( preset_ix == 0 ) {
    DEBUG_MSG("[SEQ_FILE_PRESETS] found %d items in '%s'\n", ix, filename);
  }
#endif

  return ix; // no error
}



/////////////////////////////////////////////////////////////////////////////
// reads a given preset from file (if preset_ix == 0: counts all entries)
// dst should be able to store up to 16 chars (15 chars + zero limiter)
// returns < 0 on errors (error codes are documented in seq_file.h), otherwise number of read entries
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkLabel_Read(u8 preset_ix, char *dst)
{
  return SEQ_FILE_PRESETS_Hlp_Read(SEQ_PRESET_TRKLABEL_PATH, preset_ix, dst, 15, NULL, 0);
}


/////////////////////////////////////////////////////////////////////////////
// returns number of presets (if preset_ix == 0: counts all entries)
// returns < 0 on errors (error codes are documented in seq_file.h), otherwise number of read entries
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkCategory_NumGet(void)
{
  return seq_file_presets_info.num_trk_categories;
}

/////////////////////////////////////////////////////////////////////////////
// reads a given preset from file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkCategory_Read(u8 preset_ix, char *dst)
{
  return SEQ_FILE_PRESETS_Hlp_Read(SEQ_PRESET_TRKCATS_PATH, preset_ix, dst, 5, NULL, 0);
}


/////////////////////////////////////////////////////////////////////////////
// returns number of presets
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkDrum_NumGet(void)
{
  return seq_file_presets_info.num_trk_drums;
}

/////////////////////////////////////////////////////////////////////////////
// reads a given preset from file (if preset_ix == 0: counts all entries)
// returns < 0 on errors (error codes are documented in seq_file.h), otherwise number of read entries
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PRESETS_TrkDrum_Read(u8 preset_ix, char *dst, u8 *note, u8 init_layer_preset_notes)
{
  return SEQ_FILE_PRESETS_Hlp_Read(SEQ_PRESET_TRKDRUMS_PATH, preset_ix, dst, 5, note, init_layer_preset_notes);
}



/////////////////////////////////////////////////////////////////////////////
// creates default content of preset files
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
extern s32 SEQ_FILE_PRESETS_CreateDefaults(void)
{
  s32 status = 0;

  char line_buffer[200];
#define FLUSH_BUFFER { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  status = FILE_MakeDir(SEQ_PRESET_FILES_PATH); // create directory if it doesn't exist
  status = FILE_DirExists(SEQ_PRESET_FILES_PATH);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_PRESETS] ERROR couldn't create " SEQ_PRESET_FILES_PATH "\n");
#endif
    return SEQ_FILE_PRESETS_ERR_NO_FILE;
  }

  if( FILE_FileExists(SEQ_PRESET_TRKLABEL_PATH) < 1 ) {
    if( (status=FILE_WriteOpen(SEQ_PRESET_TRKLABEL_PATH, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_PRESETS] Failed to open/create '" SEQ_PRESET_TRKLABEL_PATH "', status: %d\n", status);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      return SEQ_FILE_PRESETS_ERR_WRITE;
    }

    sprintf(line_buffer, "##########################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Following labels can be selected for tracks and patterns\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Each Label can have 15 characters max!\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "##########################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "#------------->\n");
    FLUSH_BUFFER;

    int i;
    u32 num = sizeof(default_trk_labels) / 4;
    for(i=0; i<num; ++i) {
      sprintf(line_buffer, "%s\n", default_trk_labels[i]);
      FLUSH_BUFFER;
    }

    // close file
    status |= FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
    if( status < 0 ) {
      DEBUG_MSG("[SEQ_FILE_PRESETS] ERROR couldn't create " SEQ_PRESET_TRKLABEL_PATH "\n");
    } else {
      DEBUG_MSG("[SEQ_FILE_PRESETS] created " SEQ_PRESET_TRKLABEL_PATH " with %d items\n", num);
    }
#endif
  }


  if( FILE_FileExists(SEQ_PRESET_TRKCATS_PATH) < 1 ) {
    if( (status=FILE_WriteOpen(SEQ_PRESET_TRKCATS_PATH, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_PRESETS] Failed to open/create '" SEQ_PRESET_TRKCATS_PATH "', status: %d\n", status);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      return SEQ_FILE_PRESETS_ERR_WRITE;
    }

    sprintf(line_buffer, "##############################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Following categories can be selected for tracks and patterns\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Each category name can have 5 characters max!\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "##############################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "#--->\n");
    FLUSH_BUFFER;

    int i;
    u32 num = sizeof(default_trk_categories) / 4;
    for(i=0; i<num; ++i) {
      sprintf(line_buffer, "%s\n", default_trk_categories[i]);
      FLUSH_BUFFER;
    }

    // close file
    status |= FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
    if( status < 0 ) {
      DEBUG_MSG("[SEQ_FILE_PRESETS] ERROR couldn't create " SEQ_PRESET_TRKCATS_PATH "\n");
    } else {
      DEBUG_MSG("[SEQ_FILE_PRESETS] created " SEQ_PRESET_TRKCATS_PATH " with %d items\n", num);
    }
#endif
  }


  if( FILE_FileExists(SEQ_PRESET_TRKDRUMS_PATH) < 1 ) {
    if( (status=FILE_WriteOpen(SEQ_PRESET_TRKDRUMS_PATH, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_PRESETS] Failed to open/create '" SEQ_PRESET_TRKDRUMS_PATH "', status: %d\n", status);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      return SEQ_FILE_PRESETS_ERR_WRITE;
    }

    sprintf(line_buffer, "##############################################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Following drums can be selected in track instrument configuration.\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# The first 16 definitions are taken by default whenever track is initialized.\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "# Each item contains a name and the note number (separated with a comma)\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "##############################################################################\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "\n");
    FLUSH_BUFFER;
    sprintf(line_buffer, "#--->,note\n");
    FLUSH_BUFFER;

    int i;
    u32 num = sizeof(default_trk_drums) / 6;
    for(i=0; i<num; ++i) {
      sprintf(line_buffer, "%s,%d\n", default_trk_drums[i], SEQ_LAYER_PresetDrumNoteGet(i));
      FLUSH_BUFFER;
    }

    // close file
    status |= FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
    if( status < 0 ) {
      DEBUG_MSG("[SEQ_FILE_PRESETS] ERROR couldn't create " SEQ_PRESET_TRKDRUMS_PATH "\n");
    } else {
      DEBUG_MSG("[SEQ_FILE_PRESETS] created " SEQ_PRESET_TRKDRUMS_PATH " with %d items\n", num);
    }
#endif
  }

  return status;
}
