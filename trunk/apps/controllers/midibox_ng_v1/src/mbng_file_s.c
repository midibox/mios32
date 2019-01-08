// $Id$
//! \defgroup MBNG_FILE_S
//! Snapshot File access functions
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
#include "tasks.h"
#include "app.h"

#include <string.h>
#include <md5.h>

#include "file.h"
#include "mbng_file.h"
#include "mbng_file_s.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
//! for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1

// format of the .NGS file (32bit) - has to be changed whenever the .NGS structure changes!
#define NGS_FILE_FORMAT_NUMBER 0
#if MBNG_FILE_S_NUM_SNAPSHOTS != 128
# error "MBNG_FILE_S_NUM_SNAPSHOTS != 128 requires a new NGS_FILE_FORMAT_NUMBER!"
#endif


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
} mbng_file_s_info_t;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static mbng_file_s_info_t mbng_file_s_info;

static u8 current_snapshot;

static u8 delayed_snapshot_ctr;

/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////
char mbng_file_s_patch_name[MBNG_FILE_S_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_S_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Loads snapshot file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been connected
//! \param filename the filename which should be read
//! \param snapshot if < 0: load last snapshot, if >= 0: load given snapshot
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Load(char *filename, int snapshot)
{
  s32 error;
  error = MBNG_FILE_S_Read(filename, snapshot);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_S] Tried to open snapshot file %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! Unloads snapshot file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been disconnected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Unload(void)
{
  mbng_file_s_info.valid = 0;
  current_snapshot = 0;
  delayed_snapshot_ctr = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \returns 1 if current snapshot file valid
//! \returns 0 if current snapshot file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Valid(void)
{
  return mbng_file_s_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
//! \returns current snapshot
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_SnapshotGet(void)
{
  return current_snapshot;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the current snapshot (but doesn't load it!)\n
//! Used by SCS
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_SnapshotSet(u8 snapshot)
{
  if( snapshot >= MBNG_FILE_S_NUM_SNAPSHOTS )
    return -1; // invalid snapshot

  current_snapshot = snapshot;
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! reads the snapshot file content (again)
//! \param filename the filename which should be read
//! \param snapshot if < 0: load last snapshot, if >= 0: load given snapshot
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
//! \returns 0 if snapshot not in file
//! \returns 1 if snapshot loaded from file
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Read(char *filename, int snapshot)
{
  if( snapshot >= MBNG_FILE_S_NUM_SNAPSHOTS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] Invalid snapshot cannot be loaded: %d!\n", snapshot);
#endif
    return -3;
  }

  if( mbng_file_s_patch_name != NULL ) {
    MBNG_FILE_S_Periodic_1s(1); // enforce snapshot store if requested
  }

  s32 status = 0;
  mbng_file_s_info_t *info = &mbng_file_s_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(mbng_file_s_patch_name, filename, MBNG_FILE_S_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGS", MBNG_FILES_PATH, mbng_file_s_patch_name);
  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_S] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read file
  u32 format;
  if( (status=FILE_ReadWord(&format)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] ERROR: failed while reading %s\n", filepath);
#endif
    FILE_ReadClose(&file);
    return -1;
  }

  if( format != NGS_FILE_FORMAT_NUMBER ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] ERROR: .NGS file format not supported!\n");
#endif
    FILE_ReadClose(&file);
    return -2;
  }

  if( snapshot < 0 ) {
    u32 tmp;
    if( (status=FILE_ReadWord(&tmp)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_S] ERROR: failed while reading %s\n", filepath);
#endif
      FILE_ReadClose(&file);
      return -1;
    }
    current_snapshot = tmp;
  } else {
    current_snapshot = snapshot;
  }

  // get snapshot pos
  u32 snapshot_pos;
  if( (status=FILE_ReadSeek(8 + current_snapshot*4)) < 0 ||
      (status=FILE_ReadWord(&snapshot_pos)) < 0 ||
      (snapshot_pos > 0 && (status=FILE_ReadSeek(snapshot_pos)) < 0) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] failed to retrieve snapshot position in %s!\n", filepath);
#endif
    FILE_ReadClose(&file);
    return -1;
  }

  if( snapshot_pos == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] snapshot #%d not stored in %s yet\n", current_snapshot, filepath);
#endif
  } else {
    u32 snapshot_len;
    if( (status=FILE_ReadWord(&snapshot_len)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_S] failed to retrieve snapshot #%d length in %s!\n", current_snapshot, filepath);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_S] loading snapshot #%d from %s\n", current_snapshot, filepath);
#endif

      snapshot_len -= 4;
      int pos;
      for(pos=0; pos<snapshot_len; pos+=5) {
	u16 id;
	u16 value;
	u8 secondary_value;
	if( (status=FILE_ReadHWord(&id)) < 0 ||
	    (status=FILE_ReadHWord(&value)) < 0 ||
	    (status=FILE_ReadByte(&secondary_value)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_S] failed to retrieve value in snapshot #%d of %s yet!\n", current_snapshot, filepath);
#endif
	  break;
	} else {
#if DEBUG_VERBOSE_LEVEL >= 2
	  DEBUG_MSG("[MBNG_FILE_S] id=%s:%d value=%d svalue=%d%s\n",
		    MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, value, secondary_value);
#endif
	  u32 found_items = 0;
	  mbng_event_item_t item;
	  u32 continue_id_ix = 0;
	  while( MBNG_EVENT_ItemSearchById(id, &item, &continue_id_ix) >= 0 ) {
	    ++found_items;

	    if( !item.flags.no_dump ) {
	      item.secondary_value = secondary_value;
	      u8 from_midi = 1;
	      u8 fwd_enabled = 1;
	      MBNG_EVENT_ItemReceive(&item, value, from_midi, fwd_enabled);
	    }

	    if( continue_id_ix == 0 )
	      break;
	  }

	  if( !found_items ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_S] WARNING: id=%s:%d not found in event pool!\n",
		      MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
#endif
	  }
	}
      }
    }
  }

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_S_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return (snapshot_pos == 0) ? 0 : 1; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! writes data into snapshot file
//! \param filename the filename which should be written
//! \param snapshot if < 0: generate new file, if >= 0: write into given snapshot
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Write(char *filename, int snapshot)
{
  if( snapshot >= MBNG_FILE_S_NUM_SNAPSHOTS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] Invalid snapshot cannot be stored: %d!\n", snapshot);
#endif
    return -3;
  }

  mbng_file_s_info_t *info = &mbng_file_s_info;

  // store current file name in global variable for UI
  memcpy(mbng_file_s_patch_name, filename, MBNG_FILE_S_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGS", MBNG_FILES_PATH, mbng_file_s_patch_name);

  // write snapshot

  s32 status = 0;
  if( snapshot < 0 || FILE_FileExists(filepath) < 1 ) {
    current_snapshot = 0;

    if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_S] Failed to create snapshot file, status: %d\n", status);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      info->valid = 0;
      return status;
    }

    // format
    status |= FILE_WriteWord(NGS_FILE_FORMAT_NUMBER);

    // last snapshot
    status |= FILE_WriteWord(current_snapshot);

    // all snapshots are empty
    int i;
    for(i=0; i<MBNG_FILE_S_NUM_SNAPSHOTS; ++i) {
      status |= FILE_WriteWord(0);
    }
    FILE_WriteClose(); // important to free memory given by malloc
  }

  if( snapshot >= 0 ) {
    current_snapshot = snapshot;
    u32 snapshot_pos = 0;

    {
      file_t file;

      if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[MBNG_FILE_S] failed to open file, status: %d\n", status);
#endif
	return status;
      }

      // read file
      u32 format;
      if( (status=FILE_ReadWord(&format)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_S] ERROR: failed while reading %s\n", filepath);
#endif
	FILE_ReadClose(&file);
	return -1;
      }

      if( format != NGS_FILE_FORMAT_NUMBER ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_S] ERROR: .NGS file format not supported!\n");
#endif
	FILE_ReadClose(&file);
	return -2;
      }

      if( (status=FILE_ReadSeek(8 + current_snapshot*4)) < 0 ||
	  (status=FILE_ReadWord(&snapshot_pos)) < 0 ||
	  (snapshot_pos > 0 && (status=FILE_ReadSeek(snapshot_pos)) < 0) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_S] failed to retrieve snapshot position in %s!\n", filepath);
#endif
	FILE_ReadClose(&file);
	return -1;
      }

      // create new snapshot record if it doesn't exist yet
      if( snapshot_pos == 0 ) {
	snapshot_pos = FILE_ReadGetCurrentSize();
      } else {
	u32 snapshot_len;
	if( (status=FILE_ReadWord(&snapshot_len)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_S] failed to retrieve snapshot #%d length in %s yet!\n", current_snapshot, filepath);
#endif
	} else {
	  if( snapshot_len != (4+5*MBNG_EVENT_PoolNumItemsGet()) ) {
	    // new snapshot
#if DEBUG_VERBOSE_LEVEL >= 2
	    DEBUG_MSG("[MBNG_FILE_S] snapshot size mismatch - creating new record!!\n");
#endif
	    snapshot_pos = FILE_ReadGetCurrentSize();	    
	  }
	}
      }
      FILE_ReadClose(&file);
    }

    s32 status = 0;
    if( (status=FILE_WriteOpen(filepath, 0)) < 0 ) { // don't create!
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_S] Failed to open snapshot file, status: %d\n", status);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      info->valid = 0;
      return status;
    }

    // write snapshot length
    status |= FILE_WriteSeek(snapshot_pos);
    int num_items = MBNG_EVENT_PoolNumItemsGet();
    if( status >= 0 )
      status |= FILE_WriteWord(4+5*num_items);

    // write snapshot values
    if( status >= 0 ) {
      u32 continue_ix = 0;
      do {
	u16 id;
	s16 value;
	u8 secondary_value;

	if( MBNG_EVENT_ItemRetrieveValues(&id, &value, &secondary_value, &continue_ix) < 0 ) {
	  break;
	} else {
	  status |= FILE_WriteHWord(id);
	  status |= FILE_WriteHWord((u16)value);
	  status |= FILE_WriteByte(secondary_value);
	}
      } while( status >= 0 && continue_ix );
    }

    // enter record into bank structure
    if( status >= 0 ) {
      status |= FILE_WriteSeek(4);
      status |= FILE_WriteWord(current_snapshot);
      status |= FILE_WriteSeek(8 + snapshot*4);
      status |= FILE_WriteWord(snapshot_pos);
    }

    // close file
    status |= FILE_WriteClose();
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_S] ERROR: failed while writing %s!\n", filepath);
#endif
    return status;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Requests a delayed snapshot
//! \param delay_s the delay in seconds - will be take if there is no ongoing request
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_RequestDelayedSnapshot(u8 delay_s)
{
  if( !delayed_snapshot_ctr ) {
    delayed_snapshot_ctr = delay_s;
  }
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Should be called periodically each second to check for a delayed snapshot
//! \param force if set to != 0, an ongoing request will be enforced
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_S_Periodic_1s(u8 force)
{
  if( delayed_snapshot_ctr && (force || --delayed_snapshot_ctr == 0) ) {
    delayed_snapshot_ctr = 0;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("[MBNG_FILE_S] delayed snapshot stored\n");
      MUTEX_MIDIOUT_GIVE;
    }
    
    MUTEX_SDCARD_TAKE;
    MBNG_FILE_S_Write(mbng_file_s_patch_name, MBNG_FILE_S_SnapshotGet());
    MUTEX_SDCARD_GIVE;
  }

  return 0; // no error
}

//! \}
