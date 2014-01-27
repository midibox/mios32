// $Id$
/*
 * MIDI File Access Routines
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
#include "tasks.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <string.h>

#include "file.h"
#include "midio_file.h"
#include "mid_file.h"
#include "seq.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the .mid files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MID_FILES_PATH ""
//#define MID_FILES_PATH "MyMidi/"

// max path length
#define MAX_PATH 100


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the file position and length
static u32 midifile_pos;
static u32 midifile_len;
static char ui_midifile_name[MAX_PATH];

// MIDI file references
static file_t midifile_fi;

// for recording
static u8 record_mode;
static char record_filename[13];
static u32 record_last_tick;
static mios32_midi_port_t record_last_port;
static mios32_midi_package_t record_last_event;
static u32 record_trk_header_filepos;
static u32 record_trk_size;

static u32 record_sysex_delta_tick;
static u8 record_sysex_buffer_pos;
#define SYSEX_BUFFER_SIZE 64
static u8 record_sysex_buffer[SYSEX_BUFFER_SIZE];

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_Init(u32 mode)
{
  // initial midifile name and size
  ui_midifile_name[0] = 0;
  midifile_len = 0;

  // recording disabled
  record_mode = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the filename of the .mid file for the user interface
// Contains an error message if file wasn't loaded correctly
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_UI_NameGet(void)
{
  return ui_midifile_name;
}


/////////////////////////////////////////////////////////////////////////////
// Clears the current midi file name
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_UI_NameClear(void)
{
  ui_midifile_name[0] = 0;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns the .mid file next to the given filename
// returns < 0 on errors
// returns 1 if a new file has been found, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_FindNext(char *filename, char *next_file)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = FILE_FindNextFile(MID_FILES_PATH,
			     filename,
			     "MID",
			     next_file);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    MIDIO_FILE_StatusMsgSet("SDCard Err!");
  } else if( status == 0 ) {
    MIDIO_FILE_StatusMsgSet("No .MID File");
  } else {
    MIDIO_FILE_StatusMsgSet(NULL);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the .mid file previous to the given filename
// returns < 0 on errors
// returns 1 if a new file has been found, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_FindPrev(char *filename, char *prev_file)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = FILE_FindPreviousFile(MID_FILES_PATH,
				 filename,
				 "MID",
				 prev_file);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    MIDIO_FILE_StatusMsgSet("SDCard Err!");
  } else if( status == 0 ) {
    MIDIO_FILE_StatusMsgSet("No .MID File");
  } else {
    MIDIO_FILE_StatusMsgSet(NULL);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Open a .mid file with given filename
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_open(char *filename)
{
  char filepath[MAX_PATH];
  strncpy(filepath, MID_FILES_PATH, MAX_PATH);
  strncat(filepath, filename, MAX_PATH);
  
  MUTEX_SDCARD_TAKE;
  s32 status = FILE_ReadOpen(&midifile_fi, filepath);
  FILE_ReadClose(&midifile_fi); // close again - file will be reopened by read handler
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MID_FILE] failed to open file, status: %d\n", status);
#endif
    ui_midifile_name[0] = 0; // disable file
  } else {

    // got it
    midifile_pos = 0;
    midifile_len = midifile_fi.fsize;

    strncpy(ui_midifile_name, filepath, MAX_PATH);
    ui_midifile_name[MAX_PATH-1] = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MID_FILE] opened '%s' of length %u\n", filepath, midifile_len);
#endif
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 MID_FILE_read(void *buffer, u32 len)
{
  s32 status;

  if( !ui_midifile_name[0] )
    return FILE_ERR_NO_FILE;

  MUTEX_SDCARD_TAKE;
  if( (status=FILE_ReadReOpen(&midifile_fi)) >= 0 ) {
    status = FILE_ReadBuffer(buffer, len);
    FILE_ReadClose(&midifile_fi);
  }
  MUTEX_SDCARD_GIVE;

  return (status >= 0) ? len : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_eof(void)
{
  if( midifile_pos >= midifile_len || !FILE_SDCardAvailable() )
    return 1; // end of file reached or SD card disconnected

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_seek(u32 pos)
{
  s32 status;

  if( !ui_midifile_name[0] )
    return -1; // end of file reached

  midifile_pos = pos;

  MUTEX_SDCARD_TAKE;

  if( midifile_pos >= midifile_len )
    status = -1; // end of file reached
  else {
    if( (status=FILE_ReadReOpen(&midifile_fi)) >= 0 ) {
      status = FILE_ReadSeek(pos);
      FILE_ReadClose(&midifile_fi);
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// help functions
/////////////////////////////////////////////////////////////////////////////
static s32 MID_FILE_WriteWord(u32 word, u8 len)
{
  int i;
  s32 status = 0;

  // ensure big endian coding, therefore byte writes
  for(i=0; i<len; ++i)
    status |= FILE_WriteByte((u8)(word >> (8*(len-1-i))));

  return (status < 0) ? status : len;
}


static s32 MID_FILE_WriteVarLen(u32 value)
{
  // based on code example from MIDI file spec
  s32 status = 0;
  u32 buffer;

  buffer = value & 0x7f;
  while( (value >>= 7) > 0 ) {
    buffer <<= 8;
    buffer |= 0x80 | (value & 0x7f);
  }

  int num_bytes = 0;
  while( 1 ) {
    ++num_bytes;
    status |= FILE_WriteByte((u8)(buffer & 0xff));
    if( buffer & 0x80 )
      buffer >>= 8;
    else
      break;
  }

  return (status < 0) ? status : num_bytes;
}


static s32 MID_FILE_WriteSysEx(u8 *buffer, u32 len)
{
  s32 status = 0;

#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("[MID_FILE:%u] SysEx (%d bytes)\n", SEQ_BPM_TickGet(), len);
  MIOS32_MIDI_SendDebugHexDump(buffer, len);
#endif

  if( len < 1 ) // just to ensure...
    return -1;

  // delta tick
  record_trk_size += MID_FILE_WriteVarLen(record_sysex_delta_tick);
  record_sysex_delta_tick = 0;

  // on new SysEx stream: F0 <length> <bytes to be transmitted after F0>
  // on continued SysEx stream: F7 <length> <all bytes to be transmitted>
  int pos = 0;
  if( buffer[0] == 0xf0 ) {
    status |= FILE_WriteByte(0xf0);
    ++record_trk_size;
    ++pos;
    record_trk_size += MID_FILE_WriteVarLen(len - 1);
  } else {
    status |= FILE_WriteByte(0xf7);
    ++record_trk_size;
    record_trk_size += MID_FILE_WriteVarLen(len);
  }

  while( pos < len ) {
    status |= FILE_WriteByte(buffer[pos++]);
    ++record_trk_size;
  }

  return (status < 0) ? status : len;
}


/////////////////////////////////////////////////////////////////////////////
// Flush SysEx buffer
/////////////////////////////////////////////////////////////////////////////
static s32 MID_FILE_FlushSysEx(void)
{
  s32 status = 0;

  if( record_sysex_buffer_pos ) {
    MUTEX_SDCARD_TAKE;
    status |= MID_FILE_WriteSysEx(record_sysex_buffer, record_sysex_buffer_pos);
    record_sysex_buffer_pos = 0;
    MUTEX_SDCARD_GIVE;
  }
  
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// called from when a MIDI event is received
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_Receive(mios32_midi_port_t port, mios32_midi_package_t package)
{
  s32 status = 0;

  // ignore if recording mode not enabled
  if( !MID_FILE_RecordingEnabled() )
    return 0;

  // port enabled?
  if( port == DEFAULT ) {
    if( !seq_rec_enable_din )
      return 0;
  } else {
    // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
    if( port < USB0 || port > OSC3 )
      return 0;
    u16 port_ix = ((port-USB0) >> 2) | (port & 0x03);
    u16 port_mask = (1 << port_ix);
    if( !(seq_rec_enabled_ports & port_mask) )
      return 0;
  }

  // ignore realtime events (like MIDI clock)
  if( package.evnt0 >= 0xf8 )
    return 0;

  u32 seq_tick = SEQ_BPM_TickGet();

#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("[MID_FILE:%u] P:%s  M:%02X %02X %02X\n",
	    seq_tick,
	    MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(port)),
	    package.evnt0, package.evnt1, package.evnt2);
#endif

  u32 word = 0;
  u8 num_bytes = 0;
  switch( package.event ) {
    case NoteOff:
    case NoteOn:
    case PolyPressure:
    case CC:
    case PitchBend:
      word = ((u32)package.evnt0 << 16) | ((u32)package.evnt1 << 8) | ((u32)package.evnt2 << 0);
      num_bytes = 3;
      break;

    case ProgramChange:
    case Aftertouch:
      word = ((u32)package.evnt0 << 8) | ((u32)package.evnt1 << 0);
      num_bytes = 2;
      break;
  }

  if( num_bytes ) {
    // remaining data in SysEx buffer?
    status |= MID_FILE_FlushSysEx();

    int delta = seq_tick - record_last_tick;
    if( delta < 0 )
      delta = 0;

    MUTEX_SDCARD_TAKE;
    record_trk_size += MID_FILE_WriteVarLen(delta);
    record_trk_size += MID_FILE_WriteWord(word, num_bytes);
    MUTEX_SDCARD_GIVE;

    record_last_tick = seq_tick;
    record_last_port = port;
    record_last_event = package;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// called when a SysEx byte is received
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  s32 status = 0;

  // ignore if recording mode not enabled
  if( !MID_FILE_RecordingEnabled() )
    return 0;

  // port enabled?
  if( port == DEFAULT ) {
    if( !seq_rec_enable_din )
      return 0;
  } else {
    // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
    if( port < USB0 || port > OSC3 )
      return 0;
    u16 port_ix = ((port-USB0) >> 2) | (port & 0x03);
    u16 port_mask = (1 << port_ix);
    if( !(seq_rec_enabled_ports & port_mask) )
      return 0;
  }

  // ignore realtime events (like MIDI clock)
  if( midi_in >= 0xf8 )
    return 0;

  u32 seq_tick = SEQ_BPM_TickGet();

  // start of SysEx stream?
  if( midi_in == 0xf0 ) {
    // flush remaining data in buffer if available.
    status |= MID_FILE_FlushSysEx();

    // start new stream
    record_sysex_buffer[0] = 0xf0;
    record_sysex_buffer_pos = 1;

    // determine tick
    int delta = seq_tick - record_last_tick;
    if( delta < 0 )
      delta = 0;

    record_sysex_delta_tick = delta;

    record_last_tick = seq_tick;
    record_last_port = port;
    {
      mios32_midi_package_t package;
      package.ALL = 0;
      package.type = 0xf;
      package.evnt0 = 0xf0;
      record_last_event = package;
    }
  } else {
    // end of buffer already reached? -> flush it
    if( record_sysex_buffer_pos == SYSEX_BUFFER_SIZE ) {
      status |= MID_FILE_FlushSysEx();
    }

    // store new byte
    record_sysex_buffer[record_sysex_buffer_pos++] = midi_in;

    // if this was 0xf7 (end of stream)? Flush SysEx buffer
    if( midi_in == 0xf7 ) {
      status |= MID_FILE_FlushSysEx();
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// enters/exits record mode
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_SetRecordMode(u8 enable)
{
  s32 status = 0;

  if( (!record_mode && !enable) || (record_mode && enable) )
    return 0; // nothing to do

  record_mode = enable;

  MUTEX_SDCARD_TAKE;

  if( record_mode ) {
    // start recording
    record_trk_size = 0;
    record_last_tick = 0;
    record_last_port = 0;
    record_last_event.ALL = 0;
    record_sysex_buffer_pos = 0;

    // determine filename
    u32 rec_num = 0;
    while( 1 ) {
      ++rec_num;
      sprintf(record_filename, "REC%d.MID", rec_num);
      if( (status=FILE_FileExists(record_filename)) <= 0 )
	break;
    }

    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MID_FILE] Recording failed, status: %d\n", status);
#endif
    } else if( (status=FILE_WriteOpen(record_filename, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MID_FILE] Failed to open/create %s, status: %d\n", record_filename, status);
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MID_FILE] Recording to '%s' started\n", record_filename);
#endif
      // write file header
      u32 header_size = 6;
      status |= FILE_WriteBuffer((u8*)"MThd", 4);
      status |= MID_FILE_WriteWord(header_size, 4);
      status |= MID_FILE_WriteWord(1, 2); // MIDI File Format
      status |= MID_FILE_WriteWord(1, 2); // Number of Tracks
      status |= MID_FILE_WriteWord(SEQ_BPM_PPQN_Get(), 2); // PPQN

      // write Track header
      record_trk_header_filepos = FILE_WriteGetCurrentSize();
      status |= FILE_WriteBuffer((u8*)"MTrk", 4);
      status |= MID_FILE_WriteWord(record_trk_size, 4); // Placeholder

      // set initial BPM
      record_trk_size += MID_FILE_WriteVarLen(0); // initial delta
      record_trk_size += MID_FILE_WriteWord(0xff, 1); // meta event
      record_trk_size += MID_FILE_WriteWord(0x51, 1); // set tempo
      record_trk_size += MID_FILE_WriteVarLen(3);     // 3 bytes
      u32 tempo_us = (u32)(1E6 / (SEQ_BPM_EffectiveGet() / 60.0));
      record_trk_size += MID_FILE_WriteWord(tempo_us, 3);
    }

  } else {
    // if ongoing SysEx: store remaining bytes
    status |= MID_FILE_FlushSysEx();

    // set end of track marker at the end of the bar (to allow proper looping)
    {
      u32 ticks_per_bar = 4 * SEQ_BPM_PPQN_Get();
      u32 delta_bar = ticks_per_bar - (record_last_tick % ticks_per_bar);
      u32 seq_tick = record_last_tick + delta_bar;

      int delta = seq_tick - record_last_tick;
      if( delta < 0 )
	delta = 0;

      record_trk_size += MID_FILE_WriteVarLen(delta);
      record_trk_size += MID_FILE_WriteWord(0xff, 1); // meta event
      record_trk_size += MID_FILE_WriteWord(0x2f, 1); // End of Track
      record_trk_size += MID_FILE_WriteWord(0x00, 1);
    }

    // stop recording
    status |= FILE_WriteClose();

    if( record_trk_size ) {
      // switch back to first byte of track and write final track size
      if( (status=FILE_WriteOpen(record_filename, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MID_FILE] Failed to open %s again, status: %d\n", record_filename, status);
#endif
	MIDIO_FILE_StatusMsgSet("SDCard Err!");
	status = -3; // file re-open error
      } else {
	status |= FILE_WriteSeek(record_trk_header_filepos + 4);
	status |= MID_FILE_WriteWord(record_trk_size, 4);
	status |= FILE_WriteClose();

	// take over new filename, but pause sequencer
	SEQ_SetPauseMode(1);
	SEQ_PlayFile(record_filename);
	MIDIO_FILE_StatusMsgSet(NULL);
      }
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MID_FILE] Recording to '%s' stopped. Track size: %d bytes\n", record_filename, record_trk_size);
#endif
  }

  MUTEX_SDCARD_GIVE;

  // disable recording on error
  if( status < 0 ) {
    record_mode = 0;
    MIDIO_FILE_StatusMsgSet("SDCard Err!");
    SEQ_BPM_Stop();
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// returns 1 if record mode enabled
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_RecordingEnabled(void)
{
  return record_mode;
}


/////////////////////////////////////////////////////////////////////////////
// returns the record filename (up to 12 chars)
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_RecordingFilename(void)
{
  return record_filename;
}


/////////////////////////////////////////////////////////////////////////////
// returns timestamp of last recorded MIDI event
/////////////////////////////////////////////////////////////////////////////
u32 MID_FILE_LastRecordedTick(void)
{
  return record_last_tick;
}

/////////////////////////////////////////////////////////////////////////////
// returns the last recorded MIDI port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t MID_FILE_LastRecordedPort(void)
{
  return record_last_port;
}

/////////////////////////////////////////////////////////////////////////////
// returns the last recorded MIDI event
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MID_FILE_LastRecordedEvent(void)
{
  return record_last_event;
}
