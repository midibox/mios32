// MBLoopa Core Logic
// (c) Hawkeye 2015
//
// This unit provides the looper core data structures and the menu navigation and multi-clip support
//
// When recording clips, these are directly stored to a MIDI file, we cannot loop record (so what)
// After recording a clip, it will be instantly re-scanned from SD to gather key informations,
// like tick length, noteroll information, and so on.
//
// We cannot store all sequencer notes in memory, but we can pre-fetch notes for every midi-file
// from SD-Card, so that we might be able to play eight tracks (unmuted) with no sync issues.
// Track looping can be implemented likewise - if our SEQ Handler detects, that we are close
// to the end of a clip, and that clip is set to loop, we can resupply the seq_midi_out buffers...
//
// In playback, all clips will start to play back, regardless of their mute state.
// Also, all clips will loop by default (it is called loopA, anyways)
// This emulates the behaviour of the MBSEQ.
//
// The Loopa itself has a beatloop minimum sync range (default 16 steps). All playback clip regions
// will have lengths of multiples of this, when modifying start and stop positions (for looping),
// it is made sure, that the output clip has a length of n*beatloop
// (will be filled up with silence at the end, if necessary)
//
// Recording occurs directly to SD and can be of unlimited length. After recording, a new MID file
// with an increased number (e.g. C_3_0004.MID -> clip 4, recording take 5) is stored and reloaded
// and length-adjusted to n*beatloop requirements.
//
// If the beatloop value is changed in the UI, the clip endpoints are automatically adjusted.
//
// =================================================================================================


#include <mios32.h>
#include <mid_parser.h>
#include <FreeRTOS.h>
#include <seq_midi_out.h>
#include <seq_bpm.h>
#include <midi_port.h>
#include <midi_router.h>
#include <string.h>

#include "tasks.h"
#include "file.h"
#include "loopa.h"
#include "seq.h"
#include "mid_file.h"
#include "hardware.h"
#include "screen.h"
#include "voxelspace.h"


// --  Local types ---

typedef struct
{
   u32  initial_file_pos;
   u32  initial_tick;
   u32  file_pos;
   u32  chunk_end;
   u32  tick;
   u8   running_status;
} midi_track_t;


// --- Consts ---

const int PREFETCH_TIME_MS = 100;


// --- Global vars ---

static s32 (*clipPlayEventCallback)(u8 clipNumber, mios32_midi_package_t midi_package, u32 tick) = 0;     // fetchClipEvents() callback
static s32 (*clipMetaEventCallback)(u8 clipNumber, u8 meta, u32 len, u8 *buffer, u32 tick) = 0; // fetchClipEvents() callback
static u8 meta_buffer[MID_PARSER_META_BUFFER_SIZE];

u16 sessionNumber_ = 0;         // currently active session number (directory e.g. /SESSIONS/0001)
u8 baseView_ = 0;               // if not in baseView, we are in single clipView
u8 displayMode2d_ = 0;          // if not in 2d display mode, we are rendering voxelspace
u8 selectedClipNumber_ = 0;     // currently active or last active clip number (1-8)
u16 beatLoopSteps_ = 16;        // number of steps for one beatloop (adjustable)
u8 isRecording_ = 0;            // set, if currently recording
u8 stopRecordingRequest_ = 0;   // if set, the sequencer will stop recording at an even "beatLoopStep" interval, to create loopable clips
s8 reloadClipNumberRequest_ = -1; // if != -1, reload the given clip number after sequencer stop (usually, a track has been recorded, then!)

u16 seqPlayEnabledPorts_;
u16 seqRecEnabledPorts_;

static u32 clipFilePos_[8];     // file position within midi file (for 8 clips)
static u32 clipFileLen_[8];     // file length of midi file (for 8 clips)
u8 clipFileAvailable_[8];       // true, if clip midi file is available (for 8 clips)
static file_t clipFileFi_[8];   // clip file reference handles, only valid if clipFileAvailable is true (for 8 clips)
u16 clipMidiFileFormat_[8];     // clip midi file format (for 8 clips)
u16 clipMidiFilePPQN_[8];       // pulses per quarter note (for 8 clips)
midi_track_t clipMidiTrack_[8]; // max one midi track per clip mid file supported (for 8 clips)

u32 clipEventNumber_[8];        // number of events in the respective clip
u32 clipTicks_[8];              // number of available ticks in the respective clip
u8 clipMute_[8];                // mute state of each clip

static u8 ffwdSilentMode_;      // for FFWD function
static u32 nextPrefetch_[8];    // next tick at which the prefetch should take place for each clip
static u32 prefetchOffset_[8];  // already prefetched ticks for each clip
static u8 seqClkLocked;         // lock BPM, so that it can't be changed from MIDI player

// =================================================================================================


/**
 * Help function: convert tick number to step number
 * @return step
 *
 */
u16 tickToStep(u32 tick)
{
   return tick / (SEQ_BPM_PPQN_Get() / 4);
}
// -------------------------------------------------------------------------------------------------


/**
 * Reads <len> bytes from the .mid file into <buffer>
 * @return number of read bytes
 *
 */
u32 clipFileRead(u8 clipNumber, void *buffer, u32 len)
{
   s32 status;

   if (!clipFileAvailable_[clipNumber])
      return FILE_ERR_NO_FILE;

   MUTEX_SDCARD_TAKE;
   if ((status=FILE_ReadReOpen(&clipFileFi_[clipNumber])) >= 0)
   {
      status = FILE_ReadBuffer(buffer, len);
      FILE_ReadClose(&clipFileFi_[clipNumber]);
   }
   MUTEX_SDCARD_GIVE;

   return (status >= 0) ? len : 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * @return 1, if end of file reached
 *
 */
s32 clipFileEOF(u8 clipNumber)
{
   if (clipFilePos_[clipNumber] >= clipFileLen_[clipNumber] || !FILE_SDCardAvailable())
      return 1; // end of file reached or SD card disconnected

   return 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Sets file pointer to a specific position
 * @return -1, if end of file reached
 *
 */
s32 clipFileSeek(u8 clipNumber, u32 pos)
{
   s32 status;

   if (!clipFileAvailable_[clipNumber])
      return -1; // end of file reached

   clipFilePos_[clipNumber] = pos;

   MUTEX_SDCARD_TAKE;

   if (clipFilePos_[clipNumber] >= clipFileLen_[clipNumber])
      status = -1; // end of file reached
   else
   {
      if ((status=FILE_ReadReOpen(&clipFileFi_[clipNumber])) >= 0)
      {
         status = FILE_ReadSeek(pos);
         FILE_ReadClose(&clipFileFi_[clipNumber]);
      }
   }

   MUTEX_SDCARD_GIVE;

   return status;
}
// -------------------------------------------------------------------------------------------------


/**
 * Help function: reads a byte/hword/word from the .mid file
 *
 */
static u32 clipReadWord(u8 clipNumber, u8 len)
{
   int word = 0;
   int i;

   for (i=0; i<len; ++i)
   {
      // due to unknown endianess of the host processor, we have to read byte by byte!
      u8 byte;
      clipFileRead(clipNumber, &byte, 1);
      word = (word << 8) | byte;
   }

   return word;
}
// -------------------------------------------------------------------------------------------------


/**
 * Help function: reads a variable-length number from the .mid file
 * based on code example in MIDI file spec
 *
 */
static u32 clipReadVarLen(u8 clipNumber, u32* pos)
{
   u32 value;
   u8 c;

   *pos += clipFileRead(clipNumber, &c, 1);
   if ((value = c) & 0x80)
   {
      value &= 0x7f;

      do
      {
         *pos += clipFileRead(clipNumber, &c, 1);
         value = (value << 7) | (c & 0x7f);
      } while (c & 0x80);
   }

   return value;
}
// -------------------------------------------------------------------------------------------------


/**
 * Restarts a clip w/o reading the .mid file chunks again (saves time)
 * Resets the clip start position + prefetch positions.
 * Usually called when (re)starting a whole song.
 *
 */
s32 clipRestart(u8 clipNumber)
{
   if (!clipFileAvailable_[clipNumber])
      return FILE_ERR_NO_FILE;

   DEBUG_MSG("[clipRestart] clip %d", clipNumber);

   // restore saved filepos/tick values
   midi_track_t *mt = &clipMidiTrack_[clipNumber];

   mt->file_pos = mt->initial_file_pos; // XXX: Set prescanned loop-start point file position here
   mt->tick = mt->initial_tick;
   mt->running_status = 0x80;

   nextPrefetch_[clipNumber] = 0;
   prefetchOffset_[clipNumber] = 0;

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------



/**
 * Rewinds a clip after a loop, not resetting the mt->tick position and prefetch offsets,
 * to stay in sync with the current song tick position.
 * Called, when looping a clip.
 *
 */
s32 clipRewind(u8 clipNumber)
{
   if (!clipFileAvailable_[clipNumber])
      return FILE_ERR_NO_FILE;

   DEBUG_MSG("[clipRewind] clip %d", clipNumber);

   // restore saved filepos value
   midi_track_t *mt = &clipMidiTrack_[clipNumber];

   mt->file_pos = mt->initial_file_pos; // XXX: Set prescanned loop-start point file position here
   mt->running_status = 0x80;

   return 0; // no error;
}
// -------------------------------------------------------------------------------------------------



/**
 * Toggle the mute state of a clip
 * (XXX: this must beat-synced, later on)
 * (XXX: also, hanging notes must be avoided)
 *
 */
void toggleMute(u8 clipNumber)
{
   u8 state = clipMute_[clipNumber];

   // If old state mute=1, after state change, the led must light up, so ledstate=old mute state
   switch (clipNumber)
   {
   case 0: MIOS32_DOUT_PinSet(led_clip1, state); break;
   case 1: MIOS32_DOUT_PinSet(led_clip2, state); break;
   case 2: MIOS32_DOUT_PinSet(led_clip3, state); break;
   case 3: MIOS32_DOUT_PinSet(led_clip4, state); break;
   case 4: MIOS32_DOUT_PinSet(led_clip5, state); break;
   case 5: MIOS32_DOUT_PinSet(led_clip6, state); break;
   case 6: MIOS32_DOUT_PinSet(led_clip7, state); break;
   case 7: MIOS32_DOUT_PinSet(led_clip8, state); break;
   }

   // invert the mute state
   clipMute_[clipNumber] = !state;
}
// -------------------------------------------------------------------------------------------------


/**
 * Initially open the current clip .mid file of a clipNumber (associated with current sessionNumber)
 * and parse for available header/track chunks.

 * Multiple recordings into that clip slot will produce higher -0000.MID suffixes, the highest
 * number is loaded.
 *
 * @return 0, if no errors occured
 *
 */
s32 clipFileOpen(u8 clipNumber)
{
   // invalidate current file
   clipFileAvailable_[clipNumber] = 0;

   static char filepath[48];

   u32 rec_num = 0;
   MUTEX_SDCARD_TAKE;
   while (1)
   {
      sprintf(filepath, "/SESSIONS/%04d/C_%d-%04d.MID", sessionNumber_, clipNumber, rec_num);
      if (FILE_FileExists(filepath) <= 0)
         break;
      ++rec_num;
   }
   MUTEX_SDCARD_GIVE;

   sprintf(filepath, "/SESSIONS/%04d/C_%d-%04d.MID", sessionNumber_, clipNumber, rec_num-1);

   MUTEX_SDCARD_TAKE;
   s32 status = FILE_ReadOpen(&clipFileFi_[clipNumber], filepath);
   FILE_ReadClose(&clipFileFi_[clipNumber]); // close again - file will be reopened by read handler
   MUTEX_SDCARD_GIVE;

   if (status != 0)
   {
      DEBUG_MSG("[clipFileOpen] failed to open file %s, status: %d\n", filepath, status);
      return status;
   }

   // File is available

   clipFilePos_[clipNumber] = 0;
   clipFileLen_[clipNumber] = clipFileFi_[clipNumber].fsize;
   clipFileAvailable_[clipNumber] = 1;

   DEBUG_MSG("[clipFileOpen] opened '%s' of length %u\n", filepath, clipFileLen_[clipNumber]);

   // Parse header...
   u8 chunk_type[4];
   u32 chunk_len;

   DEBUG_MSG("[clipFileOpen] reading file\n\r");

   // reset file position
   clipFileSeek(clipNumber, 0);
   u32 file_pos = 0;
   u16 num_tracks = 0;

   // read chunks
   while (!clipFileEOF(clipNumber))
   {
      file_pos += clipFileRead(clipNumber, chunk_type, 4);

      if (clipFileEOF(clipNumber))
         break; // unexpected: end of file reached

      chunk_len = clipReadWord(clipNumber, 4);
      file_pos += 4;

      DEBUG_MSG("[clipFileOpen] chunk len %u", chunk_len);


      if (clipFileEOF(clipNumber))
         break; // unexpected: end of file reached

      if (memcmp(chunk_type, "MThd", 4) == 0)
      {
         DEBUG_MSG("[clipFileOpen] Found Header with size: %u\n\r", chunk_len);
         if( chunk_len != 6 )
         {
            DEBUG_MSG("[clipFileOpen] invalid header size - skip!\n\r");
         }
         else
         {
            clipMidiFileFormat_[clipNumber] = (u16)clipReadWord(clipNumber, 2);
            u16 tempTracksNum = (u16)clipReadWord(clipNumber, 2);
            clipMidiFilePPQN_[clipNumber] = (u16)clipReadWord(clipNumber, 2);
            file_pos += 6;
            DEBUG_MSG("[clipFileOpen] MIDI file format: %u\n\r", clipMidiFileFormat_[clipNumber]);
            DEBUG_MSG("[clipFileOpen] Number of tracks: %u\n\r", tempTracksNum);
            DEBUG_MSG("[clipFileOpen] ppqn (n per quarter note): %u\n\r", clipMidiFilePPQN_[clipNumber]);
         }
      }
      else if (memcmp(chunk_type, "MTrk", 4) == 0)
      {
         if (num_tracks >= 1)
         {
            DEBUG_MSG("[clipFileOpen] Found Track with size: %u must be ignored due to MID_PARSER_MAX_TRACKS\n\r", chunk_len);
         }
         else
         {
            u32 num_bytes = 0;
            u32 delta = (u32)clipReadVarLen(clipNumber, &num_bytes);
            file_pos += num_bytes;
            chunk_len -= num_bytes;

            midi_track_t *mt = &clipMidiTrack_[clipNumber];
            mt->initial_file_pos = file_pos;
            mt->initial_tick = delta;
            mt->file_pos = file_pos;
            mt->chunk_end = file_pos + chunk_len - 1;
            mt->tick = delta;
            mt->running_status = 0x80;
            ++num_tracks;

            DEBUG_MSG("[clipFileOpen] Found Track %d with size: %u, initial_tick: %u, starting at tick: %u\n\r", num_tracks, chunk_len + num_bytes, mt->initial_tick, mt->tick);

            break;
         }

         // switch to next track
         file_pos += chunk_len;
         clipFileSeek(clipNumber, file_pos);
      }
      else
      {
         DEBUG_MSG("[clipFileOpen] Found unknown chunk '%c%c%c%c' of size %u\n\r",
                   chunk_type[0], chunk_type[1], chunk_type[2], chunk_type[3],
                   chunk_len);

         if (num_tracks >= 1 )
         {
            // Found at least one track, all is good
            break;
         }
         else
         {
            // Invalidate file, no tracks found
            clipFileAvailable_[clipNumber] = 0;
            return -1;
         }
      }
   }

   clipFileAvailable_[clipNumber] = 1;

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Callback method to count a loaded play event in memory.
 * Triggered by MID_PARSER_FetchEvents during loadClip
 *
 */
static s32 countPlayEvent(u8 clipNumber, mios32_midi_package_t midi_package, u32 tick)
{
   if (midi_package.event == NoteOff || midi_package.velocity == 0)
      DEBUG_MSG("[countPlayEvent] @tick %u note off", tick);
   else if (midi_package.event == NoteOn)
      DEBUG_MSG("[countPlayEvent] @tick %u note on", tick);

   clipEventNumber_[clipNumber]++;

   if (tick > clipTicks_[clipNumber])
      clipTicks_[clipNumber] = tick;

   return 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Callback method to count a loaded play event in memory.
 * Triggered by MID_PARSER_FetchEvents during loadClip
 *
 */
static s32 countMetaEvent(u8 clipNumber, u8 meta, u32 len, u8 *buffer, u32 tick)
{
   clipEventNumber_[clipNumber]++;

   if (tick > clipTicks_[clipNumber])
      clipTicks_[clipNumber] = tick;

   return 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Prefetches MIDI events from the given clip number file for a given number of MIDI ticks
 *
 * tick_offsets are scaled to clip loop ranges
 *
 * returns < 0 on errors
 * returns > 0 if tracks are still playing
 * returns 0 if song is finished
 *
 */
s32 clipFetchEvents(u8 clipNumber, u32 tick_offset, u32 num_ticks)
{
   u8 stillRunning = 0;

   if (clipFileAvailable_[clipNumber] == 0)
      return 1; // fake for compatibility reasons

   midi_track_t *mt = &clipMidiTrack_[clipNumber];

   DEBUG_MSG("[fetch events] clip %d fpos %d endpos %d | tick_offs: %d", clipNumber, mt->file_pos, mt->chunk_end, tick_offset);

   // Special case - if we are at the end of our midi file, loop back to start
   // XXX: later on - skip to preindexed loop start point, for now, just rewind
   if (mt->file_pos >= mt->chunk_end)
      clipRewind(clipNumber);

   while (mt->file_pos < mt->chunk_end)
   {
      stillRunning = 1;

      // exit, if next tick is not within given timeframe
      if (mt->tick >= tick_offset + num_ticks)
         break;

      // re-set to current file pos
      clipFileSeek(clipNumber, mt->file_pos);

      // get event
      u8 event;
      mt->file_pos += clipFileRead(clipNumber, &event, 1);

      if (event == 0xf0)
      {
         // SysEx event
         u32 length = (u32)clipReadVarLen(clipNumber, &mt->file_pos);
         #if DEBUG_VERBOSE_LEVEL >= 3
         DEBUG_MSG("[MID_PARSER:%d:%u] SysEx event with %u bytes\n\r", track, mt->tick, length);
         #endif

         // TODO: use optimized packages for SysEx!
         mios32_midi_package_t midi_package;
         midi_package.type = 0xf; // single bytes will be transmitted

         // initial 0xf0
         midi_package.evnt0 = 0xf0;
         if (clipPlayEventCallback != NULL)
            clipPlayEventCallback(clipNumber, midi_package, mt->tick);

         // remaining bytes
         int i;
         for (i=0; i<length; ++i)
         {
            u8 evnt0;
            mt->file_pos += clipFileRead(clipNumber, &evnt0, 1);
            midi_package.evnt0 = evnt0;
            if (clipPlayEventCallback != NULL)
               clipPlayEventCallback(clipNumber, midi_package, mt->tick);
         }
      }
      else if (event == 0xf7)
      {
         // "Escaped" event (allows to send any MIDI data)
         u32 length = (u32)clipReadVarLen(clipNumber, &mt->file_pos);
         #if DEBUG_VERBOSE_LEVEL >= 3
         DEBUG_MSG("[MID_PARSER:%d:%u] Escaped event with %u bytes\n\r", track, mt->tick, length);
         #endif

         mios32_midi_package_t midi_package;
         midi_package.type = 0xf; // single bytes will be transmitted
         int i;
         for (i=0; i<length; ++i)
         {
            u8 evnt0;
            mt->file_pos += clipFileRead(clipNumber, &evnt0, 1);
            midi_package.evnt0 = evnt0;
            if (clipPlayEventCallback != NULL)
               clipPlayEventCallback(clipNumber, midi_package, mt->tick);
         }
      }
      else if (event == 0xff)
      {
         // Meta Event
         u8 meta;
         mt->file_pos += clipFileRead(clipNumber, &meta, 1); // Meta type 1 byte, e.g. 0x2f "EndOfTrack"
         u32 length = (u32)clipReadVarLen(clipNumber, &mt->file_pos);

         if (clipMetaEventCallback != NULL)
         {
            u32 buflen = length;
            if (buflen > (MID_PARSER_META_BUFFER_SIZE-1))
            {
               buflen = MID_PARSER_META_BUFFER_SIZE - 1;
               DEBUG_MSG("[MID_PARSER:%d:%u] Meta Event 0x%02x with %u bytes - cut at %u bytes!\n\r", clipNumber, mt->tick, meta, length, buflen);
            }
            else
            {
               DEBUG_MSG("[MID_PARSER:%d:%u] Meta Event 0x%02x with %u bytes\n\r", clipNumber, mt->tick, meta, buflen);
            }

            if (buflen)
            {
               // copy bytes into buffer
               mt->file_pos += clipFileRead(clipNumber, meta_buffer, buflen);

               if (length > buflen)
               {
                  // no free memory: dummy reads
                  int i;
                  u8 dummy;
                  for(i=buflen; i<length; ++i)
                     mt->file_pos += clipFileRead(clipNumber, &dummy, 1);
               }
            }

            meta_buffer[buflen] = 0; // terminate with 0 for the case that a string has been transfered

            // -> forward to callback function
            clipMetaEventCallback(clipNumber, meta, buflen, meta_buffer, mt->tick);
         }
      }
      else
      {
         // common MIDI event
         mios32_midi_package_t midi_package;

         if (event & 0x80)
         {
            mt->running_status = event;
            midi_package.evnt0 = event;
            u8 evnt1;
            mt->file_pos += clipFileRead(clipNumber, &evnt1, 1);
            midi_package.evnt1 = evnt1;
         }
         else
         {
            midi_package.evnt0 = mt->running_status;
            midi_package.evnt1 = event;
         }
         midi_package.type = midi_package.event;

         switch (midi_package.event)
         {
         case NoteOff:
         case NoteOn:
         case PolyPressure:
         case CC:
         case PitchBend:
            {
               u8 evnt2;
               mt->file_pos += clipFileRead(clipNumber, &evnt2, 1);
               midi_package.evnt2 = evnt2;

               if (clipPlayEventCallback != NULL)
                  clipPlayEventCallback(clipNumber, midi_package, mt->tick);

               #if DEBUG_VERBOSE_LEVEL >= 3
               DEBUG_MSG("[MID_PARSER:%d:%u] %02x%02x%02x\n\r", clipNumber, mt->tick, midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
               #endif
            }
            break;
         case ProgramChange:
         case Aftertouch:
            if (clipPlayEventCallback != NULL)
               clipPlayEventCallback(clipNumber, midi_package, mt->tick);

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

      // get delta ticks to next event, if end of track hasn't been reached yet
      if (mt->file_pos < mt->chunk_end)
      {
         u32 delta = (u32)clipReadVarLen(clipNumber, &mt->file_pos);
         mt->tick += delta;
      }
   }

   return stillRunning;
}
// -------------------------------------------------------------------------------------------------


/**
 * Scan a clip and read the number of events and midi ticks contained
 *
 */
void clipScan(u8 clipNumber)
{
   clipEventNumber_[clipNumber] = 0;
   clipTicks_[clipNumber] = 0;

   // Install "count-only" callback
   clipPlayEventCallback = countPlayEvent;
   clipMetaEventCallback = countMetaEvent;

   clipFetchEvents(clipNumber, 0, 0xFFFFFFF);
}
// -------------------------------------------------------------------------------------------------


/**
 * (Re)load a session clip. It will have changed after recording.
 * The clip is also automatically unmuted, if it was muted before and has enough data.
 * If no clip was found, the local clipData will be empty
 *
 */
void loadClip(u8 clipNumber)
{
   clipFileOpen(clipNumber);
   clipScan(clipNumber);

   DEBUG_MSG("loadClip(): counted %d events and %d ticks (= %d steps) for clip %d.", clipEventNumber_[clipNumber], clipTicks_[clipNumber], tickToStep(clipTicks_[clipNumber]), clipNumber);

   if (clipEventNumber_[clipNumber] < 3)
   {
      DEBUG_MSG("loadClip(): disabling clip: not enough events (%d events)", clipEventNumber_[clipNumber]);
      clipTicks_[clipNumber] = 0;
      clipEventNumber_[clipNumber] = 0;
      clipFileAvailable_[clipNumber] = 0;
   }

   screenSetClipLength(clipNumber, tickToStep(clipTicks_[clipNumber]));
   screenSetClipPosition(clipNumber, 0);

   // unmute available clip
   clipMute_[clipNumber] = clipFileAvailable_[clipNumber];
   toggleMute(clipNumber);
}
// -------------------------------------------------------------------------------------------------


/**
 * Load new session data
 * also load all stored clips into memory
 *
 */
void loadSession(u16 newSessionNumber)
{
   sessionNumber_ = newSessionNumber;

   u8 clip;
   for (clip = 0; clip < 8; clip++)
      loadClip(clip);

   selectedClipNumber_ = 0;
   screenSetClipSelected(selectedClipNumber_);
   screenFormattedFlashMessage("Loaded Session %d", newSessionNumber);
}
// -------------------------------------------------------------------------------------------------


/**
 * First callback from app - render Loopa Startup logo on screen
 *
 */
void loopaStartup()
{
   screenShowLoopaLogo(1);
}
// -------------------------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////////////////////////
// LOOPA Sequencer Section
////////////////////////////////////////////////////////////////////////////////////////////////////

static s32  seqPlayOffEvents(void);
static s32  seqReset(u8 play_off_events);
static s32  seqSongPos(u16 new_song_pos);
static void seqUpdateBeatLEDs(u32 bpm_tick);
static s32  seqTick(u32 bpm_tick);
static s32  hookMIDISendPackage(mios32_midi_port_t port, mios32_midi_package_t package);
static s32  seqPlayEvent(u8 clipNumber, mios32_midi_package_t midi_package, u32 tick);
static s32  seqIgnoreMetaEvent(u8 clipNumber, u8 meta, u32 len, u8 *buffer, u32 tick);



/**
 * Initialize Loopa SEQ
 *
 */
s32 seqInit()
{
   // play mode
   seqClkLocked = 0;

   // play over USB0 and UART0/1
   seqPlayEnabledPorts_ = 0x01 | (0x03 << 4);

   // record over USB0 and UART0/1
   seqRecEnabledPorts_ = 0x01 | (0x03 << 4);

   // reset sequencer
   seqReset(0);

   // init BPM generator
   SEQ_BPM_Init(0);
   SEQ_BPM_Set(120.0);

   // scheduler should send packages to private hook
   SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(hookMIDISendPackage);

   // install seqPlayEvent callback for clipFetchEvents()
   clipPlayEventCallback = seqPlayEvent;
   clipMetaEventCallback = seqIgnoreMetaEvent;

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Get Clock Mode
 * adds a fourth mode which locks the BPM so that it can't be modified by the MIDI file
 *
 */
u8 seqClockModeGet(void)
{
   if (seqClkLocked)
      return 3;

   return SEQ_BPM_ModeGet();
}
// -------------------------------------------------------------------------------------------------


/**
 * Set Clock Mode
 * adds a fourth mode which locks the BPM so that it can't be modified by the MIDI file
 *
 */
s32 seqClockModeSet(u8 mode)
{
  if (mode > 3)
     return -1; // invalid mode

  if (mode == 3)
  {
     SEQ_BPM_ModeSet(SEQ_BPM_MODE_Master);
     seqClkLocked = 1;
  }
  else
  {
     SEQ_BPM_ModeSet(mode);
     seqClkLocked = 0;
  }

  return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * This main sequencer handler is called periodically to poll the clock/current tick
 * from BPM generator
 *
 */
s32 seqHandler(void)
{
   // handle BPM requests

   // note: don't remove any request check - clocks won't be propagated
   // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
   if (SEQ_BPM_ChkReqStop())
   {
      seqPlayOffEvents();
      MID_FILE_SetRecordMode(0);
      MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
   }

   if (SEQ_BPM_ChkReqCont())
   {
      MIDI_ROUTER_SendMIDIClockEvent(0xfb, 0);
   }

   if (SEQ_BPM_ChkReqStart())
   {
      MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);
      seqReset(1);
      seqSongPos(0);
   }

   u16 new_song_pos;
   if (SEQ_BPM_ChkReqSongPos(&new_song_pos))
   {
      seqSongPos(new_song_pos);
   }


   u32 bpm_tick;
   if (SEQ_BPM_ChkReqClk(&bpm_tick) > 0)
   {
      seqUpdateBeatLEDs(bpm_tick);

      // set initial BPM according to MIDI spec
      if (bpm_tick == 0 && !seqClkLocked)
         SEQ_BPM_Set(120.0);

      if (bpm_tick == 0) // send start (again) to synchronize with new MIDI songs
         MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);

      seqTick(bpm_tick);

      // Perform synchronized recording stop (tracklength a multiple of beatsteps)
      if (stopRecordingRequest_)
      {
         if (tickToStep(bpm_tick) % beatLoopSteps_ == 0)
         {
            SEQ_BPM_Stop();          // stop sequencer
            MIOS32_DOUT_PinSet(led_armrecord, 0);

            screenFormattedFlashMessage("Stopped Recording");
            MID_FILE_SetRecordMode(0);
            stopRecordingRequest_ = 0;

            loadClip(reloadClipNumberRequest_);
            reloadClipNumberRequest_ = -1;
         }
         else
         {
            u8 remainSteps = 16 - (tickToStep(bpm_tick) % beatLoopSteps_);
            screenFormattedFlashMessage("Stop Recording in %d", remainSteps);
         }
      }
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * This function plays all "off" events
 * Should be called on sequencer reset/restart/pause to avoid hanging notes
 *
 */
static s32 seqPlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // send Note Off to all channels
  // TODO: howto handle different ports?
  // TODO: should we also send Note Off events? Or should we trace Note On events and send Off if required?
  int chn;
  mios32_midi_package_t midi_package;
  midi_package.type = CC;
  midi_package.event = CC;
  midi_package.evnt2 = 0;

  for (chn=0; chn<16; ++chn)
  {
     midi_package.chn = chn;
     midi_package.evnt1 = 123; // All Notes Off
     hookMIDISendPackage(UART0, midi_package);
     midi_package.evnt1 = 121; // Controller Reset
     hookMIDISendPackage(UART0, midi_package);
  }

  return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Reset song position of sequencer
 *
 */
s32 seqReset(u8 play_off_events)
{
   // install seqPlayEvent callback for clipFetchEvents()
   clipPlayEventCallback = seqPlayEvent;

   // since timebase has been changed, ensure that Off-Events are played
   // (otherwise they will be played much later...)
   if (play_off_events)
      seqPlayOffEvents();

   // release  FFWD mode
   ffwdSilentMode_ = 0;

   u8 i;
   for (i = 0; i < 8; i++)
      clipRestart(i);

   // set initial BPM (according to MIDI file spec)
   SEQ_BPM_PPQN_Set(384); // not specified
   //SEQ_BPM_Set(120.0);
   // will be done at tick 0 to avoid overwrite in record mode!


   // reset BPM tick
   SEQ_BPM_TickSet(0);

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Sets new song position (new_song_pos resolution: 16th notes)
 *
 */
static s32 seqSongPos(u16 new_song_pos)
{
   if (MID_FILE_RecordingEnabled())
      return 0; // nothing to do

   u32 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

   portENTER_CRITICAL();

   // set new tick value
   SEQ_BPM_TickSet(new_tick);

   DEBUG_MSG("[SEQ] Setting new song position %u (-> %u ticks)\n", new_song_pos, new_tick);

   // since timebase has been changed, ensure that Off-Events are played
   // (otherwise they will be played much later...)
   seqPlayOffEvents();

   // restart song
   //MID_PARSER_RestartSong();

   // Rewind clips (they have been scanned for length before)
   u8 i;
   for (i = 0; i < 8; i++)
      clipRestart(i);

   /* XXX TODO
   if( new_song_pos > 1 )
   {
      // (silently) fast forward to requested position
      ffwdSilentMode = 1;
      MID_PARSER_FetchEvents(0, new_tick-1);
      ffwd_silent_mode = 0;
   }

   // when do we expect the next prefetch:
   nextPrefetch_ = new_tick;
   prefetchOffset_ = new_tick;
   */

   portEXIT_CRITICAL();

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Update BEAT LEDs / Voxel space beat line / Clip positions
 *
 */
static void seqUpdateBeatLEDs(u32 bpm_tick)
{
   static u8 lastLEDstate = 255;

   u16 ticksPerStep = SEQ_BPM_PPQN_Get() / 4;

   u8 beatled = (bpm_tick / ticksPerStep) % 4;

   if (beatled != lastLEDstate)
   {
      lastLEDstate = beatled;

      switch (beatled)
      {
      case 0:
         MIOS32_DOUT_PinSet(led_beat0, 1);
         MIOS32_DOUT_PinSet(led_beat1, 0);
         MIOS32_DOUT_PinSet(led_beat2, 0);
         MIOS32_DOUT_PinSet(led_beat3, 0);
         voxelTickLine();
         break;
      case 1:
         MIOS32_DOUT_PinSet(led_beat0, 0);
         MIOS32_DOUT_PinSet(led_beat1, 1);
         MIOS32_DOUT_PinSet(led_beat2, 0);
         MIOS32_DOUT_PinSet(led_beat3, 0);
         break;
      case 2:
         MIOS32_DOUT_PinSet(led_beat0, 0);
         MIOS32_DOUT_PinSet(led_beat1, 0);
         MIOS32_DOUT_PinSet(led_beat2, 1);
         MIOS32_DOUT_PinSet(led_beat3, 0);
         break;
      case 3:
         MIOS32_DOUT_PinSet(led_beat0, 0);
         MIOS32_DOUT_PinSet(led_beat1, 0);
         MIOS32_DOUT_PinSet(led_beat2, 0);
         MIOS32_DOUT_PinSet(led_beat3, 1);
         break;
      }

      // New step, Update clip positions
      u8 i;
      for (i = 0; i < 8; i++)
      {
         DEBUG_MSG("[SetClipPos] clip: %u bpm_tick: %u clipTicks_[clip]: %u ticksPerStep: %u", i, bpm_tick, clipTicks_[i], ticksPerStep);
         screenSetClipPosition(i, ((u32) (bpm_tick % clipTicks_[i]) / ticksPerStep));
      }

      // Set global song step (non-wrapping), e.g. for recording clips
      screenSetSongStep(bpm_tick / ticksPerStep);
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * Perform a single bpm tick
 *
 */
static s32 seqTick(u32 bpm_tick)
{
   // send MIDI clock depending on ppqn
   if( (bpm_tick % (SEQ_BPM_PPQN_Get()/24)) == 0)
   {
      // DEBUG_MSG("Tick %d, SEQ BPM PPQN/24 %d", bpm_tick, SEQ_BPM_PPQN_Get()/24);
      MIDI_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);
   }


   u8 clipNumber;
   for (clipNumber = 0; clipNumber < 8; clipNumber++)
   {
      if (clipFileAvailable_[clipNumber])
      {
         // DEBUG_MSG("clip: %d bpm_tick: %d nextPrefetch: %d", clipNumber, bpm_tick, nextPrefetch_[clipNumber]);

         if (bpm_tick >= nextPrefetch_[clipNumber])
         {
            // get number of prefetch ticks depending on current BPM
            u32 prefetch_ticks = SEQ_BPM_TicksFor_mS(PREFETCH_TIME_MS);

            if (bpm_tick >= prefetchOffset_[clipNumber])
            {
               DEBUG_MSG("[PREFETCH] BUFFER UNDERRUN");
               // buffer underrun - fetch more!
               prefetch_ticks += (bpm_tick - prefetchOffset_[clipNumber]);
               nextPrefetch_[clipNumber] = bpm_tick; // ASAP
            }
            else if ((prefetchOffset_[clipNumber] - bpm_tick) < prefetch_ticks)
            {
               DEBUG_MSG("[PREFETCH] nearly a buffer underrun");
               // close to a buffer underrun - fetch more!
               prefetch_ticks *= 2;
               nextPrefetch_[clipNumber] = bpm_tick; // ASAP
            }
            else
            {
               nextPrefetch_[clipNumber] += prefetch_ticks;
            }

            DEBUG_MSG("[SEQ] Prefetch for clip %d started at tick %u (prefetching %u..%u)\n", clipNumber, bpm_tick, prefetchOffset_[clipNumber], prefetchOffset_[clipNumber]+prefetch_ticks-1);

            clipFetchEvents(clipNumber, prefetchOffset_[clipNumber], prefetch_ticks);
            prefetchOffset_[clipNumber] += prefetch_ticks;

            DEBUG_MSG("[SEQ] Prefetch for clip %d finished at tick %u\n", clipNumber, SEQ_BPM_TickGet());
         }
      }
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Schedule a MIDI event to be played at a given tick
 *
 */
static s32 seqPlayEvent(u8 clipNumber, mios32_midi_package_t midi_package, u32 tick)
{
   // DEBUG_MSG("[seqPlayEvent] silent:%d type:%d", ffwdSilentMode_, midi_package.type);

   // ignore all events in silent mode (for SEQ_SongPos function)
   // we could implement a more intelligent parser, which stores the sent CC/program change, etc...
   // and sends the last received values before restarting the song...
   if (ffwdSilentMode_)
      return 0;

   // In order to support an unlimited SysEx stream length, we pass them as single bytes directly w/o the sequencer!
   if (midi_package.type == 0xf)
   {
      hookMIDISendPackage(clipNumber, midi_package);
      return 0;
   }

   seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnEvent;
   if (midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0))
      event_type = SEQ_MIDI_OUT_OffEvent;

   // output events on "clipNumber" port
   u32 status = 0;
   status |= SEQ_MIDI_OUT_Send(clipNumber, midi_package, event_type, tick, 0);

   return status;
}
// -------------------------------------------------------------------------------------------------


/**
 * Ignore track meta events
 *
 */
static s32 seqIgnoreMetaEvent(u8 clipNumber, u8 meta, u32 len, u8 *buffer, u32 tick)
{
   return 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Realtime output hook: called exactly, when the MIDI scheduler has a package to send
 *
 */
static s32 hookMIDISendPackage(mios32_midi_port_t clipNumber, mios32_midi_package_t package)
{
   // XXX: Map clipNumber to configured output port/channel for this clip

   // DEBUG_MSG("hook clipNumber %d mute of this channel %d", clipNumber, clipMute_[clipNumber]);

   if (clipNumber > 7 || (!clipMute_[clipNumber]))
   {
      // realtime events are already scheduled by MIDI_ROUTER_SendMIDIClockEvent()
      if (package.evnt0 >= 0xf8)
      {
         MIOS32_MIDI_SendPackage(UART0, package);
      }
      else
      {
         // forward to enabled MIDI ports
         int i;
         u16 mask = 1;

         for (i=0; i<16; ++i, mask <<= 1)
         {
            if (seqPlayEnabledPorts_ & mask)
            {
               // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
               mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
               MIOS32_MIDI_SendPackage(port, package);
            }
         }
      }

      // Voxelspace note rendering
      if (package.event == NoteOn && package.velocity > 0)
         voxelNoteOn(package.note, package.velocity);

      if (package.event == NoteOff || (package.event == NoteOn && package.velocity == 0))
         voxelNoteOff(package.note);
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Handle a stop request (with beatstep synced recording stop)
 *
 */
void handleStop()
{
   if (!isRecording_)
   {
      screenFormattedFlashMessage("Stopped Playing");
      SEQ_BPM_Stop();          // stop sequencer

      MIOS32_DOUT_PinSet(led_startstop, 0);
   }
   else
      stopRecordingRequest_ = 1;

   isRecording_ = 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Control the play/stop button function
 *
 */
s32 seqPlayStopButton(void)
{
  if (SEQ_BPM_IsRunning())
  {
     handleStop();
  }
  else
  {
     SEQ_BPM_CheckAutoMaster();

     // reset sequencer
     seqReset(1);

     // start sequencer
     SEQ_BPM_Start();

     MIOS32_DOUT_PinSet(led_startstop, 1);
     MIOS32_DOUT_PinSet(led_armrecord, 0);

     screenFormattedFlashMessage("Play");
  }

  return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * To control the rec/stop button function
 *
 */
s32 seqRecStopButton(void)
{
  if( SEQ_BPM_IsRunning() )
  {
     handleStop();
  }
  else
  {
     // if in auto mode and BPM generator is clocked in slave mode:
     // change to master mode
     SEQ_BPM_CheckAutoMaster();

     // enter record mode
     if (MID_FILE_SetRecordMode(1) >= 0)
     {
        reloadClipNumberRequest_ = selectedClipNumber_;
        isRecording_ = 1;

        // reset sequencer
        seqReset(1);

        // start sequencer
        SEQ_BPM_Start();

        MIOS32_DOUT_PinSet(led_startstop, 1);
        MIOS32_DOUT_PinSet(led_armrecord, 1);

        screenFormattedFlashMessage("Recording Clip %d", selectedClipNumber_ + 1);
     }
  }

  return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Fast forward (XXX: not yet implemented)
 *
 */
s32 seqFFwdButton(void)
{
   u32 tick = SEQ_BPM_TickGet();
   u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
   u32 ticks_per_measure = ticks_per_step * 16;

   int measure = tick / ticks_per_measure;
   int song_pos = 16 * (measure + 1);
   if (song_pos > 65535)
      song_pos = 65535;

   return seqSongPos(song_pos);
}
// -------------------------------------------------------------------------------------------------


/**
 * Rewind (XXX: not yet implemented)
 *
 */
s32 seqFRewButton(void)
{
   u32 tick = SEQ_BPM_TickGet();
   u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
   u32 ticks_per_measure = ticks_per_step * 16;

   int measure = tick / ticks_per_measure;
   int song_pos = 16 * (measure - 1);
   if (song_pos < 0)
      song_pos = 0;

   return seqSongPos(song_pos);
}
// -------------------------------------------------------------------------------------------------




/**
 * SD Card Available, initialize LOOPA, load session, prepare screen and menus
 *
 */
void loopaSDCardAvailable()
{
   loadSession(1); // -> XXX: load latest session with a free clip slot instead
   seqInit();
   screenShowLoopaLogo(0); // Disable logo, normal operations started

}
// -------------------------------------------------------------------------------------------------


/**
 * A buttonpress has occured
 *
 */
void loopaButtonPressed(s32 pin)
{
   if (pin == sw_startstop)
   {
      voxelClearNotes();
      MIOS32_MIDI_SendDebugMessage("Button: start/stop\n");
      seqPlayStopButton();
   }
   else if (pin == sw_armrecord)
   {
      voxelClearNotes();
      voxelClearField();
      MIOS32_MIDI_SendDebugMessage("Button: arm for record\n");
      seqRecStopButton();

   }
   else if (pin == sw_looprange)
   {
      MIOS32_DOUT_PinSet(led_looprange, 1);
      MIOS32_MIDI_SendDebugMessage("Button: loop range\n");
   }
   else if (pin == sw_editclip)
   {
      MIOS32_DOUT_PinSet(led_editclip, 1);
      MIOS32_MIDI_SendDebugMessage("Button: edit clip\n");
   }
   else if (pin == sw_clip1)
   {
      toggleMute(0);
   }
   else if (pin == sw_clip2)
   {
      toggleMute(1);
   }
   else if (pin == sw_clip3)
   {
      toggleMute(2);
   }
   else if (pin == sw_clip4)
   {
      toggleMute(3);
   }
   else if (pin == sw_clip5)
   {
      toggleMute(4);
   }
   else if (pin == sw_clip6)
   {
      toggleMute(5);
   }
   else if (pin == sw_clip7)
   {
      toggleMute(6);
   }
   else if (pin == sw_clip8)
   {
      toggleMute(7);
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * An encoder has been turned
 *
 */
void loopaEncoderTurned(s32 encoder, s32 incrementer)
{
   if (encoder == enc_clipswitch)
   {
      selectedClipNumber_ += incrementer;
      selectedClipNumber_ %= 8;
      screenSetClipSelected(selectedClipNumber_);

      screenFormattedFlashMessage("Clip %d - %d Events", selectedClipNumber_ + 1, clipEventNumber_[selectedClipNumber_]);
   }
}
// -------------------------------------------------------------------------------------------------
