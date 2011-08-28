// $Id$
/*
 * Utility page
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
#include <file.h>
#include "tasks.h"

#include "seq_ui.h"

#include "seq_core.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 2

#ifndef DEBUG_MSG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


// file version (on format changes just increment to invalidate old formats)
#define DUMP_FILE_VERSION 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 undo_tracks;
static u16 copy_tracks;

static const char undo_filename[] = "/UNDO.V4T";
static const char copy_filename[] = "/COPY.V4T";


/////////////////////////////////////////////////////////////////////////////
// Dumps all tracks into a file
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_UTIL_WriteTracks(char *filename)
{
  s32 status = 0;

  if( !FILE_VolumeAvailable() )
    return -1; // no SD Card available

  MUTEX_SDCARD_TAKE;

  if( (status=FILE_WriteOpen((char *)filename, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_UI_UTIL_WriteTracks] Failed to open/create %s, status: %d\n", filename, status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
  } else {

    // dump file version
    status |= FILE_WriteByte(DUMP_FILE_VERSION);

    // dump tracks into file
    u8 track=0;
    for(track=0; track<SEQ_CORE_NUM_TRACKS && status >= 0; ++track) {
      // layouts
      if( status >= 0 )
	status |= FILE_WriteHWord(SEQ_PAR_NumStepsGet(track));
      if( status >= 0 )
	status |= FILE_WriteByte(SEQ_PAR_NumLayersGet(track));
      if( status >= 0 )
	status |= FILE_WriteByte(SEQ_PAR_NumInstrumentsGet(track));

      if( status >= 0 )
	status |= FILE_WriteHWord(SEQ_TRG_NumStepsGet(track));
      if( status >= 0 )
	status |= FILE_WriteByte(SEQ_TRG_NumLayersGet(track));
      if( status >= 0 )
	status |= FILE_WriteByte(SEQ_TRG_NumInstrumentsGet(track));

      // parameter layer
      if( status >= 0 )
	status |= FILE_WriteBuffer((u8 *)&seq_par_layer_value[track], SEQ_PAR_MAX_BYTES);

      // trigger layer
      if( status >= 0 )
	status |= FILE_WriteBuffer((u8 *)&seq_trg_layer_value[track], SEQ_TRG_MAX_BYTES);

      // CCs
      int cc;
      for(cc=0; cc<128; ++cc)
	if( status >= 0 )
	  status |= FILE_WriteByte(SEQ_CC_Get(track, cc));
    }

    // close file
    status |= FILE_WriteClose();

    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_UI_UTIL_WriteTracks] Failed to write into %s!\n", filename);
#endif
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Reads tracks from a file and copies them to given tracks
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_UTIL_ReadTracks(char *filename, u16 src_tracks, u16 dst_tracks)
{
  s32 status = 0;
  file_t file;

  if( !FILE_VolumeAvailable() )
    return -1; // no SD Card available

  // determine first source track
  u8 src_track = 0;
  while( !(src_tracks & (1 << src_track)) )
    ++src_track;
  if( src_track >= 16 )
    return 0; // no source track selected -> no copy required

  // determine first destination track
  u8 dst_track = 0;
  while( !(dst_tracks & (1 << dst_track)) )
    ++dst_track;
  if( dst_track >= 16 )
    return 0; // no destination track selected -> no copy required

  MUTEX_SDCARD_TAKE;

  if( (status=FILE_ReadOpen(&file, (char *)filename)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_UI_UTIL_ReadTracks] Failed to open %s, status: %d\n", filename, status);
#endif
    FILE_ReadClose(&file);
  } else {

    // check file version
    u8 version;
    status |= FILE_ReadByte(&version);

    if( status >= 0 && version != DUMP_FILE_VERSION ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_UI_UTIL_ReadTracks] file format of %s expired - ignoring content!\n", filename);
#endif
      status = -1;
    }

    // reads tracks from file
    u8 track=0;
    for(track=0; track<SEQ_CORE_NUM_TRACKS &&
	         status >= 0 &&
	         src_track < SEQ_CORE_NUM_TRACKS &&
	         dst_track < SEQ_CORE_NUM_TRACKS; ++track) {
      u16 num_p_steps;
      status |= FILE_ReadHWord(&num_p_steps);
      u8 num_p_layers;
      status |= FILE_ReadByte(&num_p_layers);
      u8 num_p_instruments;
      status |= FILE_ReadByte(&num_p_instruments);

      u16 num_t_steps;
      status |= FILE_ReadHWord(&num_t_steps);
      u8 num_t_layers;
      status |= FILE_ReadByte(&num_t_layers);
      u8 num_t_instruments;
      status |= FILE_ReadByte(&num_t_instruments);

      if( status >= 0 && track == src_track ) {
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[SEQ_UI_UTIL_ReadTracks] T%d -> T%d!\n", src_track+1, dst_track+1);
#endif	
	// initialize layers
	status |= SEQ_PAR_TrackInit(dst_track, num_p_steps, num_p_layers, num_p_instruments);
	status |= SEQ_TRG_TrackInit(dst_track, num_t_steps, num_t_layers, num_t_instruments);

	// read parameter layer
	if( status >= 0 )
	  status |= FILE_ReadBuffer((u8 *)&seq_par_layer_value[dst_track], SEQ_PAR_MAX_BYTES);

	// read trigger layer
	if( status >= 0 )
	  status |= FILE_ReadBuffer((u8 *)&seq_trg_layer_value[dst_track], SEQ_TRG_MAX_BYTES);

	// read CCs
	int cc;
	for(cc=0; cc<128; ++cc)
	  if( status >= 0 ) {
	    u8 value;
	    status |= FILE_ReadByte(&value);
	    SEQ_CC_Set(dst_track, cc, value);
	  }

	// switch to next src/dst track
	do {
	  ++src_track;
	} while( !(src_tracks & (1 << src_track)) );
	do {
	  ++dst_track;
	} while( !(dst_tracks & (1 << dst_track)) );
      } else {
	// dummy read to skip track
	u8 value;
	u32 num_bytes = SEQ_PAR_MAX_BYTES + SEQ_TRG_MAX_BYTES + 128;
	int i;
	for(i=0; i<num_bytes && status >= 0; ++i)
	  status |= FILE_ReadByte(&value);
      }
    }

    // close file
    status |= FILE_ReadClose(&file);

    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_UI_UTIL_WriteTracks] Failed to write into %s!\n", filename);
#endif
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Updates the UnDo buffer
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_UTIL_UndoUpdate(void)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("UNDO Write Begin\n");
#endif
  s32 status = SEQ_UI_UTIL_WriteTracks((char *)undo_filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("UNDO Write End\n");
#endif

  if( status >= 0 )
    undo_tracks = ui_selected_tracks;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Copy selected sequences to SD Card
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_Copy(void)
{
  if( !FILE_VolumeAvailable() )
    return -1; // no SD Card available

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("COPY Write Begin\n");
#endif
  s32 status = SEQ_UI_UTIL_WriteTracks((char *)copy_filename);
  if( status >= 0 )
    copy_tracks = ui_selected_tracks;
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("COPY Write End\n");
#endif
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copy from SD Card into selected sequences
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_Paste(void)
{
  if( !FILE_VolumeAvailable() )
    return -1; // no SD Card available

  if( !copy_tracks )
    return 0; // no tracks copied yet

  SEQ_UI_UTIL_UndoUpdate();

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("COPY Read Begin\n");
#endif
  SEQ_UI_UTIL_ReadTracks((char *)copy_filename, copy_tracks, ui_selected_tracks);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("COPY Read End\n");
#endif
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear selected sequences
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_Clear(void)
{
  SEQ_UI_UTIL_UndoUpdate();

  // copy preset
  u8 only_layers = seq_core_options.PASTE_CLR_ALL ? 0 : 1;
  u8 all_triggers_cleared = 0;
  u8 init_assignments = 1;

  u8 track;
  for(track=0;track<SEQ_CORE_NUM_TRACKS; ++track)
    if( ui_selected_tracks & (1 << track) )
      SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Undo last operation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_Undo(void)
{
  if( !FILE_VolumeAvailable() )
    return -1; // no SD Card available
  
  // exit if undo buffer not filled
  if( !undo_tracks )
    return 0; // no error

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("UNDO Read Begin\n");
#endif
  // read back from UNDO buffer
  SEQ_UI_UTIL_ReadTracks((char *)undo_filename, undo_tracks, undo_tracks);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("UNDO Read End\n");
#endif

  return 0; // no error
}
