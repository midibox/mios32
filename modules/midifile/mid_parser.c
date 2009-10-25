// $Id$
/*
 * MIDI file player
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

#include "mid_parser.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging via MIOS terminal
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u32  initial_file_pos;
  u32  initial_tick;
  u32  file_pos;
  u32  chunk_end;
  u32  tick;
  u8   running_status;
} midi_track_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static u32 MID_PARSER_ReadWord(u8 len);
static u32 MID_PARSER_ReadVarLen(u32 *pos);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 midifile_format;
static u16 midifile_ppqn;

static u8 midi_tracks_num;
static midi_track_t midi_tracks[MID_PARSER_MAX_TRACKS];

// callback functions
static u32 (*mid_parser_read_callback)(void *buffer, u32 len);
static s32 (*mid_parser_eof_callback)(void);
static s32 (*mid_parser_seek_callback)(u32 pos);
static s32 (*mid_parser_playevent_callback)(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 (*mid_parser_playmeta_callback)(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MID_PARSER_Init(u32 mode)
{
  // initial values
  midi_tracks_num = 0;

  mid_parser_read_callback = NULL;
  mid_parser_eof_callback = NULL;
  mid_parser_seek_callback = NULL;
  mid_parser_playevent_callback = NULL;
  mid_parser_playmeta_callback = NULL;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Various installation routines for callbacks
/////////////////////////////////////////////////////////////////////////////
s32 MID_PARSER_InstallFileCallbacks(void *mid_parser_read, void *mid_parser_eof, void *mid_parser_seek)
{
  mid_parser_read_callback = mid_parser_read;
  mid_parser_eof_callback = mid_parser_eof;
  mid_parser_seek_callback = mid_parser_seek;
  return 0; // no error
}

s32 MID_PARSER_InstallEventCallbacks(void *mid_parser_playevent, void *mid_parser_playmeta)
{
  mid_parser_playevent_callback = mid_parser_playevent;
  mid_parser_playmeta_callback = mid_parser_playmeta;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Opens a .mid file and parses for available header/track chunks
/////////////////////////////////////////////////////////////////////////////
s32 MID_PARSER_Read(void)
{
  u8 chunk_type[4];
  u32 chunk_len;

  if( mid_parser_read_callback == NULL ||
      mid_parser_eof_callback == NULL ||
      mid_parser_seek_callback == NULL )
    return -1; // missing callback functions

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MID_PARSER] reading file\n\r");
#endif

  // reset file position
  mid_parser_seek_callback(0);
  u32 file_pos = 0;

  midi_tracks_num = 0;
  u16 num_tracks = 0;

  // read chunks
  while( !mid_parser_eof_callback() ) {
    file_pos += mid_parser_read_callback(chunk_type, 4);

    if( mid_parser_eof_callback() )
      break; // unexpected: end of file reached

    chunk_len = MID_PARSER_ReadWord(4);
    file_pos += 4;

    if( mid_parser_eof_callback() )
      break; // unexpected: end of file reached

    if( memcmp(chunk_type, "MThd", 4) == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MID_PARSER] Found Header with size: %u\n\r", chunk_len);
#endif
      if( chunk_len != 6 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MID_PARSER] invalid header size - skip!\n\r");
#endif
      } else {
	midifile_format = (u16)MID_PARSER_ReadWord(2);
	num_tracks = (u16)MID_PARSER_ReadWord(2);
	midifile_ppqn = (u16)MID_PARSER_ReadWord(2);
	file_pos += 6;
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MID_PARSER] MIDI file format: %u\n\r", midifile_format);
	DEBUG_MSG("[MID_PARSER] Number of tracks: %u\n\r", num_tracks);
	DEBUG_MSG("[MID_PARSER] ppqn (n per quarter note): %u\n\r", midifile_ppqn);
#endif
      }
    } else if( memcmp(chunk_type, "MTrk", 4) == 0 ) {
      if( midi_tracks_num >= MID_PARSER_MAX_TRACKS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MID_PARSER] Found Track with size: %u must be ignored due to MID_PARSER_MAX_TRACKS\n\r", chunk_len);
#endif
      } else {
	u32 num_bytes = 0;
	u32 delta = (u32)MID_PARSER_ReadVarLen(&num_bytes);
	file_pos += num_bytes;
	chunk_len -= num_bytes;

	midi_track_t *mt = &midi_tracks[midi_tracks_num];
	mt->initial_file_pos = file_pos;
	mt->initial_tick = delta;
	mt->file_pos = file_pos;
	mt->chunk_end = file_pos + chunk_len - 1;
	mt->tick = delta;
	mt->running_status = 0x80;
	++midi_tracks_num;

#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MID_PARSER] Found Track %d with size: %u, starting at tick: %u\n\r", midi_tracks_num, chunk_len + num_bytes, mt->tick);
#endif
      }

      // switch to next track
      file_pos += chunk_len;
      mid_parser_seek_callback(file_pos);
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MID_PARSER] Found unknown chunk '%c%c%c%c' of size %u\n\r", 
	     chunk_type[0], chunk_type[1], chunk_type[2], chunk_type[3],
	     chunk_len);
      return -1;
#endif
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MID_PARSER] Number of Tracks: %u (expected: %u)\n\r", midi_tracks_num, num_tracks);
#endif    

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns MIDI file format after MID_PARSER_Read() has been called
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PARSER_FormatGet(void)
{
  return (s32)midifile_format;
}

/////////////////////////////////////////////////////////////////////////////
// returns ppqn resolution after MID_PARSER_Read() has been called
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PARSER_PPQN_Get(void)
{
  return (s32)midifile_ppqn;
}


/////////////////////////////////////////////////////////////////////////////
// returns number of tracks after MID_PARSER_Read() has been called
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_PARSER_TrackNumGet(void)
{
  return (s32)midi_tracks_num;
}


/////////////////////////////////////////////////////////////////////////////
// prefetches MIDI events from the MIDI file for a given number of MIDI ticks
// returns < 0 on errors
// returns > 0 if tracks are still playing
// returns 0 if song is finished
/////////////////////////////////////////////////////////////////////////////
s32 MID_PARSER_FetchEvents(u32 tick_offset, u32 num_ticks)
{
  if( mid_parser_read_callback == NULL ||
      mid_parser_eof_callback == NULL ||
      mid_parser_seek_callback == NULL )
    return -1; // missing callback functions

  u8 num_tracks_running = 0;
  u8 track = 0;
  midi_track_t *mt = &midi_tracks[0];
  for(track=0; track<midi_tracks_num; ++mt, ++track) {
    while( mt->file_pos < mt->chunk_end ) {

      // this track is still running
      ++num_tracks_running;

      // exit if next tick is not within given timeframe
      if( mt->tick >= (tick_offset + num_ticks) )
	break;

      // set file pos
      mid_parser_seek_callback(mt->file_pos);

      // get event
      u8 event;
      mt->file_pos += mid_parser_read_callback(&event, 1);

      if( event == 0xf0 ) { // SysEx event
	u32 length = (u32)MID_PARSER_ReadVarLen(&mt->file_pos);
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[MID_PARSER:%d:%u] SysEx event with %u bytes\n\r", track, mt->tick, length);
#endif

	// TODO: use optimized packages for SysEx!
	mios32_midi_package_t midi_package;
	midi_package.type = 0xf; // single bytes will be transmitted
	int i;
	for(i=0; i<length; ++i) {
	  u8 evnt0;
	  mt->file_pos += mid_parser_read_callback(&evnt0, 1);
	  midi_package.evnt0 = evnt0;
	  if( mid_parser_playevent_callback != NULL )
	    mid_parser_playevent_callback(track, midi_package, mt->tick);
	}
      } else if( event == 0xf7 ) { // "Escaped" event (allows to send any MIDI data)
	u32 length = (u32)MID_PARSER_ReadVarLen(&mt->file_pos);
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[MID_PARSER:%d:%u] Escaped event with %u bytes\n\r", track, mt->tick, length);
#endif
	mios32_midi_package_t midi_package;
	midi_package.type = 0xf; // single bytes will be transmitted
	int i;
	for(i=0; i<length; ++i) {
	  u8 evnt0;
	  mt->file_pos += mid_parser_read_callback(&evnt0, 1);
	  midi_package.evnt0 = evnt0;
	  if( mid_parser_playevent_callback != NULL )
	    mid_parser_playevent_callback(track, midi_package, mt->tick);
	}
      } else if( event == 0xff ) { // Meta Event
	u8 meta;
	mt->file_pos += mid_parser_read_callback(&meta, 1);
	u32 length = (u32)MID_PARSER_ReadVarLen(&mt->file_pos);

	// try to allocate memory for the temporary buffer
	u8 *buffer;
	if( mid_parser_playmeta_callback != NULL && (buffer=(u8 *)pvPortMalloc(length+1)) != NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[MID_PARSER:%d:%u] Meta Event 0x%02x with %u bytes\n\r", track, mt->tick, meta, length);
#endif
	  if( length ) {
	    // copy bytes into buffer
	    mt->file_pos += mid_parser_read_callback(buffer, length);
	  }
	  buffer[length] = 0; // terminate with 0 for the case that a string has been transfered
	  
	  // -> forward to callback function
	  mid_parser_playmeta_callback(track, meta, length, buffer, mt->tick);

	  // free buffer
	  vPortFree(buffer);
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MID_PARSER:%d:%u] Meta Event 0x%02x with %u bytes - ERROR: couldn't allocate memory\n\r", track, mt->tick, meta, length);
#endif
	  // no free memory: dummy reads
	  int i;
	  u8 dummy;
	  for(i=0; i<length; ++i)
	    mt->file_pos += mid_parser_read_callback(&dummy, 1);
	}
      } else { // common MIDI event
	mios32_midi_package_t midi_package;

	if( event & 0x80 ) {
	  mt->running_status = event;
	  midi_package.evnt0 = event;
	  u8 evnt1;
	  mt->file_pos += mid_parser_read_callback(&evnt1, 1);
	  midi_package.evnt1 = evnt1;
	} else {
	  midi_package.evnt0 = mt->running_status;
	  midi_package.evnt1 = event;
	}
	midi_package.type = midi_package.event;

	switch( midi_package.event ) {
	  case NoteOff:
	  case NoteOn:
	  case PolyPressure:
	  case CC:
	  case PitchBend:
	  {
	    u8 evnt2;
	    mt->file_pos += mid_parser_read_callback(&evnt2, 1);
	    midi_package.evnt2 = evnt2;

	    if( mid_parser_playevent_callback != NULL )
	      mid_parser_playevent_callback(track, midi_package, mt->tick);
#if DEBUG_VERBOSE_LEVEL >= 3
	    DEBUG_MSG("[MID_PARSER:%d:%u] %02x%02x%02x\n\r", track, mt->tick, midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
#endif
	  }
	  break;
	  case ProgramChange:
	  case Aftertouch:
	    if( mid_parser_playevent_callback != NULL )
	      mid_parser_playevent_callback(track, midi_package, mt->tick);
#if DEBUG_VERBOSE_LEVEL >= 3
	    DEBUG_MSG("[MID_PARSER:%d:%u] %02x%02x\n\r", track, mt->tick, midi_package.evnt0, midi_package.evnt1);
#endif
	    break;
	  default:
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MID_PARSER:%d:%u] ooops? got 0xf0 status in MIDI event stream!\n\r", track, mt->tick);
#endif
	    break;
	}
      }

      // get delta length to next event if end of track hasn't been reached yet
      if( mt->file_pos < mt->chunk_end ) {
	u32 delta = (u32)MID_PARSER_ReadVarLen(&mt->file_pos);
	mt->tick += delta;
      }
    }
  }

  return num_tracks_running;
}



/////////////////////////////////////////////////////////////////////////////
// Help function: reads a byte/hword/word from the .mid file
/////////////////////////////////////////////////////////////////////////////
static u32 MID_PARSER_ReadWord(u8 len)
{
  int word = 0;
  int i;

  for(i=0; i<len; ++i) {
    // due to unknown endianess of the host processor, we have to read byte by byte!
    u8 byte;
    mid_parser_read_callback(&byte, 1);
    word = (word << 8) | byte;
  }

  return word;
}

/////////////////////////////////////////////////////////////////////////////
// Help function: reads a variable-length number from the .mid file
// based on code example in MIDI file spec
/////////////////////////////////////////////////////////////////////////////
static u32 MID_PARSER_ReadVarLen(u32 *pos)
{
  u32 value;
  u8 c;

  *pos += mid_parser_read_callback(&c, 1);
  if( (value = c) & 0x80 ) {
    value &= 0x7f;

    do {
      *pos += mid_parser_read_callback(&c, 1);
      value = (value << 7) | (c & 0x7f);
    } while( c & 0x80 );
  }

  return value;
}


/////////////////////////////////////////////////////////////////////////////
// Restarts a song w/o reading the .mid file chunks again (saves time)
/////////////////////////////////////////////////////////////////////////////
s32 MID_PARSER_RestartSong(void)
{
  // restore saved filepos/tick values
  u8 track = 0;
  midi_track_t *mt = &midi_tracks[0];
  for(track=0; track<midi_tracks_num; ++mt, ++track) {
    mt->file_pos = mt->initial_file_pos;
    mt->tick = mt->initial_tick;
    mt->running_status = 0x80;
  }

  return 0; // no error
}

