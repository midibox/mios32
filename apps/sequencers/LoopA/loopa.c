// LoopA Core Logic
// (c) Hawkeye 2015-2019
//
// This file provides the LoopA core data structures and logic
// =================================================================================================

#include "commonIncludes.h"

#include "loopa.h"
#include "hardware.h"
#include "ui.h"
#include "screen.h"
#include "app_lcd.h"
#include "voxelspace.h"
#include "setup.h"

// --  Local types ---

// --- Global vars ---

static s32 (*loopaTrackPlayEventCallback)(s8 loopaTrack, mios32_midi_package_t midi_package, u32 tick) = 0;
static s32 (*loopaTrackMetaEventCallback)(s8 loopaTrack, u8 meta, u32 len, u8 *buffer, u32 tick) = 0;

u32 millisecondsSinceStartup_ = 0;    // global "uptime" timer
u16 inactivitySeconds_ = 0;           // screensaver timer
u32 tick_ = 0;                        // global seq tick
u16 sessionNumber_ = 0;               // currently active session number (directory e.g. /SESSIONS/0001)
u8 sessionExistsOnDisk_ = 0;          // is the currently selected session number already saved on disk/sd card
enum LoopAPage page_ = PAGE_MUTE;     // currently active page/view
enum Command command_ = COMMAND_NONE; // currently active command
s8 tempoFade_ = 0;                    // 0: no tempo change ongoing | +1: increase tempo button pressed | -1: decrease tempo button pressed (ongoing event)

u8 activeTrack_ = 0;                  // currently active or last active clip number (0..5)
u8 activeScene_ = 0;                  // currently active scene number (0-15)
u16 beatLoopSteps_ = 16;              // number of steps for one beatloop (adjustable)
u8 isRecording_ = 0;                  // set, if currently recording to the selected clip
u8 oledBeatFlashState_ = 0;           // 0: don't flash, 1: flash slightly (normal 1/4th note), 2: flash intensively (after four 1/4th notes or 16 steps)

u16 seqRecEnabledPorts_;
s16 notePtrsOn_[128];                 // during recording - pointers to notes that are currently "on" (and waiting for an "off", therefore length yet undetermined) (-1: note not depressed)

u8 ffwdSilentMode_;                   // for FFWD function
char filename_[20];                   // global, for filename operations

// --- Track data (saved to session on disk) ---
u8 trackMute_[TRACKS];                // mute state of each clip
s8 trackMidiOutPort_[TRACKS];         // if negative: map to user defined instrument, if positive: standard mios port number
u8 trackMidiOutChannel_[TRACKS];

// --- Clip data (saved to session on disk) ---
u16 clipSteps_[TRACKS][SCENES];       // number of steps for each clip
u32 clipQuantize_[TRACKS][SCENES];    // brings all clip notes close to the specified timing, e.g. quantize = 4 - close to 4th notes, 16th = quantize to step, ...
s8 clipTranspose_[TRACKS][SCENES];
s16 clipScroll_[TRACKS][SCENES];
u8 clipStretch_[TRACKS][SCENES];      // 1: compress to 1/16th, 2: compress to 1/8th ... 16: no stretch, 32: expand 2x, 64: expand 4x, 128: expand 8x
NoteData clipNotes_[TRACKS][SCENES][MAXNOTES];  // clip note data storage (chained list, note start time and length/velocity)
u16 clipNotesSize_[TRACKS][SCENES];   // active number of notes currently in use for that clip

// TODO: save to session
float bpm_ = 120.0;                   // bpm
u8 stepsPerMeasure_ = 16;             // 16 measures default for a 4/4 beat (4 quarternotes = 16 16th notes per measure)
u8 metronomeEnabled_ = 0;             // Set to 1, if metronome is turned on in bpm screen
s8 trackMidiInPort_[TRACKS];          // If set to 0: enable recording from all midi ports (default)
u8 trackMidiInChannel_[TRACKS];       // If set to 16: enable recording from all midi channels (default)
u8 trackMidiForward_[TRACKS];         // If set to 1: forward midi notes to out port/channel (live play)

// --- Secondary data (not on disk) ---
u8 trackMuteToggleRequested_[TRACKS]; // 1: perform a mute/unmute toggle of the clip at the next beatstep (synced mute/unmute)
u8 sceneChangeRequested_ = 0;         // If != activeScene_, this will be the scene we are changing to
u16 clipActiveNote_[TRACKS][SCENES];  // currently active edited note number, when in noteroll editor


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
 * Help function: convert step number to tick number
 * @return step
 *
 */
u32 stepToTick(u16 step)
{
   return step * (SEQ_BPM_PPQN_Get() / 4);
}
// -------------------------------------------------------------------------------------------------


/**
 * Hard bound tick to the configured length (number of steps) of a clip
 *
 */
u32 boundTickToClipSteps(u32 tick, u8 clip)
{
   return tick % stepToTick(clipSteps_[clip][activeScene_]);
}
// -------------------------------------------------------------------------------------------------


/**
 * Quantize a "tick" time event to a tick quantization measure (e.g. 384), wrap around
 * ticks going over the clip length (clipLengthInTicks)
 *
 */
u32 quantize(u32 tick, u32 quantizeMeasure, u32 clipLengthInTicks)
{
   if (quantizeMeasure < 3)
      return tick; // no quantization

   u32 mod = tick % quantizeMeasure;
   u32 tickBase = tick - (tick % quantizeMeasure); // default: snap to previous measure

   // note: i did not like this improvement, as notes were cut off as they were moved newly ahead of the current play position and
   // thus were retriggered while still being held on the keyboard, but i fixed it, now not playing notes with length 0 (still being held) ;-)
   if (mod > (quantizeMeasure >> 1))
      tickBase = (tick - (tick % quantizeMeasure) + quantizeMeasure) % clipLengthInTicks; // snap to next measure as we are closer to this one

   return tickBase;
}
// -------------------------------------------------------------------------------------------------


/**
 * Transform (stretch and scroll) and then quantize a note in a clip
 *
 */
s32 quantizeTransform(u8 clip, u16 noteNumber)
{

   // Idea: scroll first, and modulo-map to trackstart/end boundaries
   //       scale afterwards
   //       apply track len afterwards
   //       drop notes with ticks < 0 and ticks > tracklen

   s32 tick = clipNotes_[clip][activeScene_][noteNumber].tick;
   s32 quantizeMeasure = clipQuantize_[clip][activeScene_];
   s32 clipLengthInTicks = getClipLengthInTicks(clip);

   // stretch
   tick *= clipStretch_[clip][activeScene_];
   tick = tick >> 4; // divide by 16 (stretch base)

   // only consider notes, that are within the clip length after stretching
   if (tick >= clipLengthInTicks)
      return -1;

   // scroll
   tick += clipScroll_[clip][activeScene_] * 24;

   while (tick < 0)
      tick += clipLengthInTicks;

   tick %= clipLengthInTicks;

   return quantize(tick, quantizeMeasure, clipLengthInTicks);
}
// -------------------------------------------------------------------------------------------------


/**
 * Get the clip length in ticks
 */
u32 getClipLengthInTicks(u8 clip)
{
   return stepToTick(clipSteps_[clip][activeScene_]);
}
// -------------------------------------------------------------------------------------------------


/**
 * Request (or cancel) a synced mute/unmute toggle
 *
 */
void toggleMute(u8 clipNumber)
{
   if (trackMute_[clipNumber])
      setActiveTrack(clipNumber); // if track was unmuted, set it as active track

   if (SEQ_BPM_IsRunning())
      trackMuteToggleRequested_[clipNumber] = !trackMuteToggleRequested_[clipNumber];
   else
      trackMute_[clipNumber] = !trackMute_[clipNumber];
}
// -------------------------------------------------------------------------------------------------


/**
 * Synchronized (to beatstep) mutes and unmutes
 *
 */
void performSyncedMutesAndUnmutes()
{
   u8 clip;

   for (clip = 0; clip < TRACKS; clip++)
   {
      if (trackMuteToggleRequested_[clip])
      {
         if (tickToStep(tick_) % beatLoopSteps_ == 0)
         {
            u8 state = trackMute_[clip];
            trackMute_[clip] = !state;

            trackMuteToggleRequested_[clip] = 0;
         }
      }
   }
};
// -------------------------------------------------------------------------------------------------


/**
 * Synchronized (to beatstep) scene changes
 *
 */
void performSyncedSceneChanges()
{
   if (sceneChangeRequested_ != activeScene_)
   {
      u8 sceneChangeInTicks = beatLoopSteps_ - (tickToStep(tick_) % beatLoopSteps_);

      // flash indicate target scene (after synced scene switch)
      if (tick_ % 16 < 8)
      {
         MUTEX_DIGITALOUT_TAKE;
         MIOS32_DOUT_PinSet(led_scene1, sceneChangeRequested_ == 0);
         MIOS32_DOUT_PinSet(led_scene2, sceneChangeRequested_ == 1);
         MIOS32_DOUT_PinSet(led_scene3, sceneChangeRequested_ == 2);
         MIOS32_DOUT_PinSet(led_scene4, sceneChangeRequested_ == 3);
         MIOS32_DOUT_PinSet(led_scene5, sceneChangeRequested_ == 4);
         MIOS32_DOUT_PinSet(led_scene6, sceneChangeRequested_ == 5);
         MUTEX_DIGITALOUT_GIVE;
      }
      else
      {
         MUTEX_DIGITALOUT_TAKE;
         MIOS32_DOUT_PinSet(led_scene1, 0);
         MIOS32_DOUT_PinSet(led_scene2, 0);
         MIOS32_DOUT_PinSet(led_scene3, 0);
         MIOS32_DOUT_PinSet(led_scene4, 0);
         MIOS32_DOUT_PinSet(led_scene5, 0);
         MIOS32_DOUT_PinSet(led_scene6, 0);
         MUTEX_DIGITALOUT_GIVE;
      }

      if (tickToStep(tick_) % beatLoopSteps_ == 0)
      {
         setActiveScene(sceneChangeRequested_);
         sceneChangeRequested_ = activeScene_;
         sceneChangeInTicks = 0;
      }

      screenSetSceneChangeInTicks(sceneChangeInTicks);
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * convert sessionNumber to global filename_
 *
 */
void sessionNumberToFilename(u16 sessionNumber)
{
   sprintf(filename_, "/SESSIONS/%04d.LPA", sessionNumber);
}
// -------------------------------------------------------------------------------------------------


/**
 * Save session to defined session number file
 *
 */
void saveSession(u16 sessionNumber)
{
   sessionNumberToFilename(sessionNumber);

   MUTEX_SDCARD_TAKE;

   s32 status;
   if ((status = FILE_WriteOpen(filename_, 1)) < 0)
   {
      DEBUG_MSG("[FILE] Failed to open/create %s, status: %d\n", filename_, status);
   }
   else
   {
      FILE_WriteBuffer((u8*)"LPAV2200", 8);

      status |= FILE_WriteBuffer((u8*)trackMute_, sizeof(trackMute_));
      status |= FILE_WriteBuffer((u8*)trackMidiOutPort_, sizeof(trackMidiOutPort_));
      status |= FILE_WriteBuffer((u8*)trackMidiOutChannel_, sizeof(trackMidiOutChannel_));
      
      status |= FILE_WriteBuffer((u8*)clipSteps_, sizeof(clipSteps_));
      status |= FILE_WriteBuffer((u8*)clipQuantize_, sizeof(clipQuantize_));
      status |= FILE_WriteBuffer((u8*)clipTranspose_, sizeof(clipTranspose_));
      status |= FILE_WriteBuffer((u8*)clipScroll_, sizeof(clipScroll_));
      status |= FILE_WriteBuffer((u8*)clipStretch_, sizeof(clipStretch_));
      
      status |= FILE_WriteBuffer((u8*)clipNotes_, sizeof(clipNotes_));
      status |= FILE_WriteBuffer((u8*)clipNotesSize_, sizeof(clipNotesSize_));
      status |= FILE_WriteBuffer((u8*)notePtrsOn_, sizeof(notePtrsOn_));

      status |= FILE_WriteClose();
   }

   if (status == 0)
      screenFormattedFlashMessage("Saved");
   else
      screenFormattedFlashMessage("Save failed");

   MUTEX_SDCARD_GIVE;
}
// -------------------------------------------------------------------------------------------------



/**
 * Load session from defined session number file
 *
 */
void loadSession(u16 sessionNumber)
{
   sessionNumberToFilename(sessionNumber);

   MUTEX_SDCARD_TAKE;

   s32 status;
   file_t file;
   if ((status = FILE_ReadOpen(&file, filename_)) < 0)
   {
      DEBUG_MSG("[FILE] Failed to open %s, status: %d\n", filename_, status);
   }
   else
   {
      char version[8];
      FILE_ReadBuffer((u8*)version, 8);

      status |= FILE_ReadBuffer((u8*)trackMute_, sizeof(trackMute_));
      status |= FILE_ReadBuffer((u8*)trackMidiOutPort_, sizeof(trackMidiOutPort_));
      status |= FILE_ReadBuffer((u8*)trackMidiOutChannel_, sizeof(trackMidiOutChannel_));

      status |= FILE_ReadBuffer((u8*)clipSteps_, sizeof(clipSteps_));
      status |= FILE_ReadBuffer((u8*)clipQuantize_, sizeof(clipQuantize_));
      status |= FILE_ReadBuffer((u8*)clipTranspose_, sizeof(clipTranspose_));
      status |= FILE_ReadBuffer((u8*)clipScroll_, sizeof(clipScroll_));
      status |= FILE_ReadBuffer((u8*)clipStretch_, sizeof(clipStretch_));

      status |= FILE_ReadBuffer((u8*)clipNotes_, sizeof(clipNotes_));
      status |= FILE_ReadBuffer((u8*)clipNotesSize_, sizeof(clipNotesSize_));
      status |= FILE_ReadBuffer((u8*)notePtrsOn_, sizeof(notePtrsOn_));

      status |= FILE_ReadClose(&file);
   }

   if (status == 0)
      screenFormattedFlashMessage("Loaded");
   else
      screenFormattedFlashMessage("Load failed");

   MUTEX_SDCARD_GIVE;
}
// -------------------------------------------------------------------------------------------------


/**
 * First callback from app - render Loopa Startup logo on screen
 *
 */
void loopaStartup()
{
   calcField();
   screenShowLoopaLogo(1);
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
      MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
   }

   /* Hawkeye new - no continuation if (SEQ_BPM_ChkReqCont())
   {
      MIDI_ROUTER_SendMIDIClockEvent(0xfb, 0);
   }
   */

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

      // set initial BPM
      if (bpm_tick == 0)
         SEQ_BPM_Set(bpm_);

      if (bpm_tick == 0) // send start (again) to synchronize with new MIDI songs
         MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);

      seqTick(bpm_tick);
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * This function plays all "off" events
 * Should be called on sequencer reset/restart/pause to avoid hanging notes
 *
 */
s32 seqPlayOffEvents(void)
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
   loopaTrackPlayEventCallback = seqPlayEvent;

   // since timebase has been changed, ensure that Off-Events are played
   // (otherwise they will be played much later...)
   if (play_off_events)
      seqPlayOffEvents();

   // release  FFWD mode
   ffwdSilentMode_ = 0;

   // set pulses per quarter note (internal resolution, 96 is standard)
   SEQ_BPM_PPQN_Set(96); // 384

   // reset BPM tick
   SEQ_BPM_TickSet(0);

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Sets new song position (new_song_pos resolution: 16th notes)
 *
 */
s32 seqSongPos(u16 new_song_pos)
{
   u32 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

   portENTER_CRITICAL();

   // set new tick value
   SEQ_BPM_TickSet(new_tick);

   // DEBUG_MSG("[SEQ] Setting new song position %u (-> %u ticks)\n", new_song_pos, new_tick);

   // since timebase has been changed, ensure that Off-Events are played
   // (otherwise they will be played much later...)
   seqPlayOffEvents();

   portEXIT_CRITICAL();

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Update BEAT LEDs / Clip positions
 *
 */
void seqUpdateBeatLEDs(u32 bpm_tick)
{
   static u8 lastLEDstate = 255;

   u16 ticksPerStep = SEQ_BPM_PPQN_Get() / 4;
   u8 beatled = (bpm_tick / ticksPerStep) % 4;

   if (beatled != lastLEDstate)
   {
      lastLEDstate = beatled;

      if (!screenIsInMenu() && !screenIsInShift())
      {
         MUTEX_DIGITALOUT_TAKE;
         switch (beatled)
         {
            case 0:
               oledBeatFlashState_ = (bpm_tick / (ticksPerStep * 4) % 4 == 0) ? 2 : 1; // flash background (strong/normal)

               updateLED(LED_RUNSTOP, LED_GREEN);
               if (gcBeatLEDsEnabled_)
               {
                  updateLED(LED_MENU, LED_GREEN);
                  updateLED(LED_COPY, LED_OFF);
                  updateLED(LED_PASTE, LED_OFF);
                  updateLED(LED_DELETE, LED_OFF);
               }
               break;
            case 1:
               if (gcBeatLEDsEnabled_)
               {
                  updateLED(LED_MENU, LED_OFF);
                  updateLED(LED_COPY, LED_BLUE | LED_GREEN);
                  updateLED(LED_PASTE, LED_OFF);
                  updateLED(LED_DELETE, LED_OFF);
               }
               else
                  updateLED(LED_RUNSTOP, LED_OFF);
               break;
            case 2:
               if (gcBeatLEDsEnabled_)
               {
                  updateLED(LED_MENU, 0);
                  updateLED(LED_COPY, 0);
                  updateLED(LED_PASTE, LED_BLUE | LED_GREEN);
                  updateLED(LED_DELETE, 0);
               }
               else
                  updateLED(LED_RUNSTOP, LED_OFF);
               break;
            case 3:
               if (gcBeatLEDsEnabled_)
               {
                  updateLED(LED_MENU, 0);
                  updateLED(LED_COPY, 0);
                  updateLED(LED_PASTE, 0);
                  updateLED(LED_DELETE, LED_BLUE | LED_GREEN);
               }
               else
                  updateLED(LED_RUNSTOP, LED_OFF);
               break;
         }
         MUTEX_DIGITALOUT_GIVE;
      }

      // New step, Update clip positions
      u8 i;
      for (i = 0; i < TRACKS; i++)
      {
         screenSetClipPosition(i, ((u32) (bpm_tick / ticksPerStep) % clipSteps_[i][activeScene_]));
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
s32 seqTick(u32 bpm_tick)
{
   tick_ = bpm_tick;

   u16 step = tickToStep(bpm_tick);

   // send MIDI clock depending on ppqn
   if ((bpm_tick % (SEQ_BPM_PPQN_Get()/24)) == 0)
   {
      // DEBUG_MSG("Tick %d, SEQ BPM PPQN/24 %d", bpm_tick, SEQ_BPM_PPQN_Get()/24);
      MIDI_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);
   }

   // perform synced track mutes/unmutes (only at full steps)
   if ((bpm_tick % (SEQ_BPM_PPQN_Get()/4)) == 0)
   {
      performSyncedMutesAndUnmutes();
      performSyncedSceneChanges();
   }

   u8 track;
   for (track = 0; track < TRACKS; track++)
   {
      if (!trackMute_[track])
      {
         u32 clipNoteTime = boundTickToClipSteps(bpm_tick, track);
         u16 i;

         for (i=0; i < clipNotesSize_[track][activeScene_]; i++)
         {
            if (clipNotes_[track][activeScene_][i].length > 0) // not still being held/recorded!
            {
               if (quantizeTransform(track, i) == clipNoteTime)
               {
                  s16 note = clipNotes_[track][activeScene_][i].note + clipTranspose_[track][activeScene_];
                  note = note < 0 ? 0 : note;
                  note = note > 127 ? 127 : note;

                  mios32_midi_package_t package;
                  package.type = NoteOn;
                  package.event = NoteOn;
                  package.chn = isInstrument(trackMidiOutPort_[track]) ? getInstrumentChannelNumberFromLoopAPortNumber(trackMidiOutPort_[track]) : trackMidiOutChannel_[track];
                  package.note = note;
                  package.velocity = clipNotes_[track][activeScene_][i].velocity;
                  hookMIDISendPackage(track, package); // play NOW

                  package.type = NoteOff;
                  package.event = NoteOff;
                  package.note = note;
                  package.velocity = 0;
                  seqPlayEvent(track, package, bpm_tick + clipNotes_[track][activeScene_][i].length); // always play off event (schedule later)
               }
            }
         }
      }
   }

   // Send metronome tick on each beat, if enabled

   if (metronomeEnabled_ && (bpm_tick % 96) == 0)
   {
      mios32_midi_package_t p;

      p.type     = NoteOn;
      p.cable    = 15; // use tag of track #16 - unfortunately more than 16 tags are not supported
      p.event    = NoteOn;
      p.chn      = isInstrument(gcMetronomePort_) ? getInstrumentChannelNumberFromLoopAPortNumber(gcMetronomePort_) : gcMetronomeChannel_;
      p.note     = gcMetronomeNoteB_;
      p.velocity = 96;
      u16 len =  5;

      if (step % stepsPerMeasure_ == 0)
      {
         p.note = gcMetronomeNoteM_;
         p.velocity = 127;
      }

      /// screenFormattedFlashMessage("metr note %d on %x#%d", p.note, getMIOSPortNumberFromLoopAPortNumber(gcMetronomePort_), p.chn);

      if (p.note)
         SEQ_MIDI_OUT_Send(METRONOME_PSEUDO_PORT, p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, len);
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Schedule a MIDI event to be played at a given tick
 *
 */
s32 seqPlayEvent(s8 loopaTrack, mios32_midi_package_t midi_package, u32 tick)
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
      hookMIDISendPackage(loopaTrack, midi_package);
      return 0;
   }

   seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnEvent;
   if (midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0))
      event_type = SEQ_MIDI_OUT_OffEvent;

   // output events on virtual "loopaTrack" port (which are then redirected just in time by the SEQ back to hookMIDISendPackage)
   u32 status = 0;
   status |= SEQ_MIDI_OUT_Send(loopaTrack, midi_package, event_type, tick, 0);

   return status;
}
// -------------------------------------------------------------------------------------------------


/**
 * Ignore track meta events
 *
 */
s32 seqIgnoreMetaEvent(s8 clipNumber, u8 meta, u32 len, u8 *buffer, u32 tick)
{
   return 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Realtime output hook: called exactly, when the MIDI scheduler has a package to send
 * @param loopaTrack s8 if zero or positive = loopa track number, else virtual metronome channel
 */
s32 hookMIDISendPackage(s8 loopaTrack, mios32_midi_package_t package)
{
   // realtime events are already scheduled by MIDI_ROUTER_SendMIDIClockEvent()
   if (package.evnt0 >= 0xf8)
   {
      MIOS32_MIDI_SendPackage(UART0, package);
   }
   else
   {
      mios32_midi_port_t port =
              loopaTrack != METRONOME_PSEUDO_PORT ? getMIOSPortNumberFromLoopAPortNumber(trackMidiOutPort_[loopaTrack]) :
              getMIOSPortNumberFromLoopAPortNumber(gcMetronomePort_);

      /// screenFormattedFlashMessage("HMSP - intr %d outp %d", loopaTrack, port);

      MIOS32_MIDI_SendPackage(port, package);

      /// screenFormattedFlashMessage("trk %d note %d on %x#%d", loopaTrack, package.note, port, package.chn);

      // DEBUG only, can additionally also send to USB0 for testing
      // port = USB0;
      // MIOS32_MIDI_SendPackage(port, package);
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Handle a stop request
 *
 */
void handleStop()
{
   screenFormattedFlashMessage("Stopped");

   MUTEX_DIGITALOUT_TAKE;
   updateLED(LED_RUNSTOP, LED_OFF);
   updateLED(LED_ARM, LED_OFF);
   if (gcBeatLEDsEnabled_)
   {
      updateLED(LED_MENU, LED_OFF);
      updateLED(LED_COPY, LED_OFF);
      updateLED(LED_PASTE, LED_OFF);
      updateLED(LED_DELETE, LED_OFF);
   }
   MUTEX_DIGITALOUT_GIVE;

   SEQ_BPM_Stop(); // stop sequencer

   // Clear "stuck" notes, that may still be depressed while stop has been hit
   u8 i;
   for (i=0; i<128; i++)
      notePtrsOn_[i] = -1;

   isRecording_ = 0;
}
// -------------------------------------------------------------------------------------------------

/**
 * Initialize LoopA SEQ
 *
 */
s32 seqInit()
{
   // record over USB0 and UART0/1
   seqRecEnabledPorts_ = 0x01 | (0x03 << 4);

   // reset sequencer
   seqReset(0);

   // init BPM generator
   SEQ_BPM_Init(0);
   SEQ_BPM_ModeSet(SEQ_BPM_MODE_Auto);
   SEQ_BPM_Set(120.0);
   bpm_ = 120;

   // scheduler should send packages to private hook
   SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(hookMIDISendPackage);

   // install seqPlayEvent callback for clipFetchEvents()
   loopaTrackPlayEventCallback = seqPlayEvent;
   loopaTrackMetaEventCallback = seqIgnoreMetaEvent;

   u8 i, j;
   for (i = 0; i < TRACKS; i++)
   {
      trackMute_[i] = 0;
      trackMidiOutPort_[i] = 0x20; // UART0 aka OUT1
      trackMidiOutChannel_[i] = 0; // Channel 1
      trackMidiInPort_[i] = 0;     // Enable recording from all ports
      trackMidiInChannel_[i] = 16; // Enable recording from all channels
      trackMidiForward_[i] = 0;    // Disable note forwarding/live play on this track by default
      trackMuteToggleRequested_[i] = 0;

      for (j = 0; j < SCENES; j++)
      {
         clipQuantize_[i][j] = 1;
         clipTranspose_[i][j] = 0;
         clipScroll_[i][j] = 0;
         clipStretch_[i][j] = 16;
         clipSteps_[i][j] = 64;
         clipNotesSize_[i][j] = 0;
         clipActiveNote_[i][j] = 0;
      }
   }

   for (i=0; i<128; i++)
      notePtrsOn_[i] = -1;

   // turn on LEDs for active track, scene and page
   setActiveTrack(activeTrack_);
   setActiveScene(activeScene_);
   setActivePage(page_);

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * SD Card available, initialize LOOPA, load session, prepare screen and menus
 *
 */
void loopaSDCardAvailable()
{
   readSetup();

   loadSession(1); // Todo: load last session (stored in setup)
   seqInit();
   trackMute_[0] = 0;
   seqArmButton();
   screenShowLoopaLogo(0); // Disable logo, normal operations started
}
// -------------------------------------------------------------------------------------------------


/**
 * Record midi event
 *
 */
void loopaRecord(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
   // Check, if the active track should record this event (matching input port)
   if (trackMidiInPort_[activeTrack_] == 0 || trackMidiInPort_[activeTrack_] == port)
   {
      // Check, if the active track should record this event (matching input channel)
      if (trackMidiInChannel_[activeTrack_] == 16 || trackMidiInChannel_[activeTrack_] == midi_package.chn)
      {
         // Record event, if armed, sequencer running and we have enough space left
         if (isRecording_ && SEQ_BPM_IsRunning())
         {
            /// screenFormattedFlashMessage("port trk: %d in: %d", trackMidiInPort_[activeTrack_], port);

            u16 clipNoteNumber = clipNotesSize_[activeTrack_][activeScene_];
            u32 clipNoteTime = boundTickToClipSteps(tick_, activeTrack_);

            if (clipNoteNumber < MAXNOTES && midi_package.type == NoteOn && midi_package.velocity > 0)
            {
               clipNotes_[activeTrack_][activeScene_][clipNoteNumber].tick = clipNoteTime;
               clipNotes_[activeTrack_][activeScene_][clipNoteNumber].length = 0; // not yet determined
               clipNotes_[activeTrack_][activeScene_][clipNoteNumber].note = midi_package.note;
               clipNotes_[activeTrack_][activeScene_][clipNoteNumber].velocity = midi_package.velocity;

               if (notePtrsOn_[midi_package.note] >= 0)
                  screenFormattedFlashMessage("dbg: note alrdy on");

               notePtrsOn_[midi_package.note] = clipNoteNumber;

               // screenFormattedFlashMessage("Note %d on - ptr %d", midi_package.note, clipNoteNumber);
               clipNotesSize_[activeTrack_][activeScene_]++;
            }
            else if (midi_package.type == NoteOff || (midi_package.type == NoteOn && midi_package.velocity == 0))
            {
               s16 notePtr = notePtrsOn_[midi_package.note];

               if (notePtr >= 0)
               {
                  s32 len = clipNoteTime - clipNotes_[activeTrack_][activeScene_][notePtr].tick;

                  if (len == 0)
                     len = 1;

                  if (len < 0)
                     len += getClipLengthInTicks(activeTrack_);

                  // screenFormattedFlashMessage("o %d - p %d - l %d", midi_package.note, notePtr, len);
                  clipNotes_[activeTrack_][activeScene_][notePtr].length = len;
               }
               notePtrsOn_[midi_package.note] = -1;
            }
         }

         // Live Forward
         if (trackMidiForward_[activeTrack_])
         {
            // Todo later: may process midi_package to incorporate live fx like transposition
            midi_package.chn = isInstrument(trackMidiOutPort_[activeTrack_]) ?
                               getInstrumentChannelNumberFromLoopAPortNumber(trackMidiOutPort_[activeTrack_]) :
                               trackMidiOutChannel_[activeTrack_];

            hookMIDISendPackage(activeTrack_, midi_package);
         }
      }
   }
}
// -------------------------------------------------------------------------------------------------
