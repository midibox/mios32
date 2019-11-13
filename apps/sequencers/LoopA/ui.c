// LoopA UI data structures and code

#include <math.h>
#include "commonIncludes.h"

#include "tasks.h"
#include "file.h"

#include "app.h"
#include "ui.h"
#include "hardware.h"
#include "loopa.h"
#include "setup.h"
#include "screen.h"
#include "voxelspace.h"


// ---------------------------------------------------------------------------------------------------------------------
// --- GLOBALS
// ---------------------------------------------------------------------------------------------------------------------

// --- Copy/paste buffer
u16 copiedClipSteps_;
u32 copiedClipQuantize_;
s8 copiedClipTranspose_;
s16 copiedClipScroll_;
u8 copiedClipStretch_;
NoteData copiedClipNotes_[MAXNOTES];
u16 copiedClipNotesSize_;
u8 shiftTrackMutePressed_[TRACKS] = { 0, 0, 0, 0, 0, 0};
s8 showShiftAbout_ = 0;
s8 showShiftHelp_ = 0;

u8 routerActiveRoute_ = 0;
u8 setupActiveItem_ = 0;
u8 scrubModeActive_= 0;

// ---------------------------------------------------------------------------------------------------------------------
// --- UI STATE CHANGES
// ---------------------------------------------------------------------------------------------------------------------

/**
 * Set a different active track
 */
void setActiveTrack(u8 trackNumber)
{
   activeTrack_ = trackNumber;
   screenSetClipSelected(activeTrack_);
}
// -------------------------------------------------------------------------------------------------

/**
 * Set a new active scene number
 */
void setActiveScene(u8 sceneNumber)
{
   activeScene_ = sceneNumber;

   MUTEX_DIGITALOUT_TAKE;
   MIOS32_DOUT_PinSet(HW_LED_SCENE_1, activeScene_ == 0);
   MIOS32_DOUT_PinSet(HW_LED_SCENE_2, activeScene_ == 1);
   MIOS32_DOUT_PinSet(HW_LED_SCENE_3, activeScene_ == 2);
   MIOS32_DOUT_PinSet(HW_LED_SCENE_4, activeScene_ == 3);
   MIOS32_DOUT_PinSet(HW_LED_SCENE_5, activeScene_ == 4);
   MIOS32_DOUT_PinSet(HW_LED_SCENE_6, activeScene_ == 5);
   MUTEX_DIGITALOUT_GIVE;
}
// -------------------------------------------------------------------------------------------------

/**
 * Set a new active page
 */
void setActivePage(enum LoopAPage page)
{
   page_ = page;

   // Map to a proper command, that is on this page (or no command at all)
   switch(page)
   {
      case PAGE_MUTE: command_ = COMMAND_NONE; break;
      case PAGE_ROUTER: command_ = COMMAND_ROUTE_SELECT; break;
      case PAGE_DISK: command_ = COMMAND_DISK_SELECT_SESSION; break;
      case PAGE_TEMPO: command_= COMMAND_TEMPO_BPM; break;
      case PAGE_CLIP: command_ = COMMAND_CLIP_LEN; break;
      case PAGE_NOTES: command_ = COMMAND_NOTE_POSITION; break;
      case PAGE_TRACK: command_ = COMMAND_TRACK_OUTPORT; break;
      case PAGE_SETUP: command_ = COMMAND_SETUP_SELECT; break;
      case PAGE_LIVEFX: command_ = COMMAND_LIVEFX_QUANTIZE; break;
      default:
         command_ = COMMAND_NONE; break;
   }

   if (configChangesToBeWritten_)
      writeSetup();
}
// -------------------------------------------------------------------------------------------------


/**
 * User pressed (and holds) a track mute/unmute gp key
 * @param track u8
 */
void shiftTrackMuteTogglePressed(u8 track)
{
   shiftTrackMutePressed_[track] = 1;
   toggleMute(track);
}
// -------------------------------------------------------------------------------------------------


/**
 * User released a track mute/unmute gp key
 * @param track u8
 */
void shiftTrackMuteToggleReleased(u8 track)
{
   shiftTrackMutePressed_[track] = 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Return 1, if user currently holds a track mute/unmute gp key
 * @param track u8
 */
u8 isShiftTrackMuteToggleKeyPressed(u8 track)
{
   return shiftTrackMutePressed_[track];
}
// -------------------------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------------------------------------------
// --- LED HANDLING
// ---------------------------------------------------------------------------------------------------------------------

/**
 * Update a single superflux LED (called from MUTEX_DIGITALOUT protected environment)
 *
 */
static u8 ledstate[13];
void updateSwitchLED(u8 number, u8 newState)
{
   switch (number)
   {
      case LED_GP1:
         if (newState != ledstate[LED_GP1])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP1, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP1, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP1, newState & LED_BLUE);
            ledstate[LED_GP1] = newState;
         }
         break;
      case LED_GP2:
         if (newState != ledstate[LED_GP2])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP2, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP2, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP2, newState & LED_BLUE);
            ledstate[LED_GP2] = newState;
         }
         break;
      case LED_GP3:
         if (newState != ledstate[LED_GP3])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP3, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP3, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP3, newState & LED_BLUE);
            ledstate[LED_GP3] = newState;
         }
         break;
      case LED_GP4:
         if (newState != ledstate[LED_GP4])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP4, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP4, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP4, newState & LED_BLUE);
            ledstate[LED_GP4] = newState;
         }
         break;
      case LED_GP5:
         if (newState != ledstate[LED_GP5])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP5, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP5, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP5, newState & LED_BLUE);
            ledstate[LED_GP5] = newState;
         }
         break;
      case LED_GP6:
         if (newState != ledstate[LED_GP6])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_GP6, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP6, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP6, newState & LED_BLUE);
            ledstate[LED_GP6] = newState;
         }
         break;
      case LED_RUNSTOP:
         if (newState != ledstate[LED_RUNSTOP])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_RUNSTOP, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_RUNSTOP, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_RUNSTOP, newState & LED_BLUE);
            ledstate[LED_RUNSTOP] = newState;
         }
         break;
      case LED_ARM:
         if (newState != ledstate[LED_ARM])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_ARM, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_ARM, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_ARM, newState & LED_BLUE);
            ledstate[LED_ARM] = newState;
         }
         break;
      case LED_SHIFT:
         if (newState != ledstate[LED_SHIFT])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_SHIFT, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_SHIFT, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_SHIFT, newState & LED_BLUE);
            ledstate[LED_SHIFT] = newState;
         }
         break;
      case LED_MENU:
         if (newState != ledstate[LED_MENU])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_MENU, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_MENU, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_MENU, newState & LED_BLUE);
            ledstate[LED_MENU] = newState;
         }
         break;
      case LED_COPY:
         if (newState != ledstate[LED_COPY])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_COPY, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_COPY, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_COPY, newState & LED_BLUE);
            ledstate[LED_COPY] = newState;
         }
         break;
      case LED_PASTE:
         if (newState != ledstate[LED_PASTE])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_PASTE, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_PASTE, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_PASTE, newState & LED_BLUE);
            ledstate[LED_PASTE] = newState;
         }
         break;
      case LED_DELETE:
         if (newState != ledstate[LED_DELETE])
         {
            MIOS32_DOUT_PinSet(HW_LED_RED_DELETE, newState & LED_RED);
            MIOS32_DOUT_PinSet(HW_LED_GREEN_DELETE, newState & LED_GREEN);
            MIOS32_DOUT_PinSet(HW_LED_BLUE_DELETE, newState & LED_BLUE);
            ledstate[LED_DELETE] = newState;
         }
         break;
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * Update the LED states of the general purpose switches (called every 20ms from app.c timer)
 *
 */
static u32 ledUpdateCounter_ = 0;
void updateSwitchLEDs()
{
   ledUpdateCounter_++;

   u8 led_gp1 = LED_OFF, led_gp2 = LED_OFF, led_gp3 = LED_OFF, led_gp4 = LED_OFF, led_gp5 = LED_OFF, led_gp6 = LED_OFF;
   u8 led_arm = LED_OFF, led_shift = LED_OFF, led_menu = LED_OFF;
   u8 led_copy = LED_OFF, led_paste = LED_OFF, led_delete = LED_OFF;
   u8 led_runstop = ledstate[LED_RUNSTOP];
   s8 flashMuteToggle = (ledUpdateCounter_ % 2) == 0; // When SEQ is running, flash active synced mute/unmute toggles until performed

   if (!SEQ_BPM_IsRunning())
   {
      led_runstop = LED_OFF;
      led_menu = LED_OFF;
      led_copy = LED_OFF;
      led_paste = LED_OFF;
      led_delete = LED_OFF;
      flashMuteToggle = 0; // When SEQ is not running, no synced mute/unmute toggles
   }

   // --- Menu LEDs ---
   if (screenIsInMenu())
   {
      led_menu = LED_RED;

      switch (page_)
      {
         /* case PAGE_SONG:
            led_gp1 = LED_RED;
            break; */
         case PAGE_MIDIMONITOR:
            led_gp2 = LED_RED;
            break;
         case PAGE_TEMPO:
            led_gp3 = LED_RED;
            break;
         case PAGE_MUTE:
            led_gp4 = LED_RED;
            break;
         case PAGE_NOTES:
            led_gp5 = LED_RED;
            break;
         /* case PAGE_ARPECHO:
            led_gp6 = LED_RED;
            break; */
         case PAGE_SETUP:
            led_runstop = LED_RED;
            break;
         case PAGE_ROUTER:
            led_arm = LED_RED;
            break;
         case PAGE_DISK:
            led_shift = LED_RED;
            break;
         case PAGE_CLIP:
            led_copy = LED_RED;
            break;
         case PAGE_LIVEFX:
            led_paste = LED_RED;
            break;
         case PAGE_TRACK:
            led_delete = LED_RED;
            break;
      }
   }
   else if (screenIsInShift())
   {
      led_shift = LED_RED;

      // --- Within shift, indicate unmuted tracks (green) and keys currently pressed (red)
      led_gp1 |= (trackMuteToggleRequested_[0] && flashMuteToggle) ? (trackMute_[0] ? LED_GREEN : LED_OFF) : (trackMute_[0] ? LED_OFF : LED_GREEN);
      led_gp2 |= (trackMuteToggleRequested_[1] && flashMuteToggle) ? (trackMute_[1] ? LED_GREEN : LED_OFF) : (trackMute_[1] ? LED_OFF : LED_GREEN);
      led_gp3 |= (trackMuteToggleRequested_[2] && flashMuteToggle) ? (trackMute_[2] ? LED_GREEN : LED_OFF) : (trackMute_[2] ? LED_OFF : LED_GREEN);
      led_gp4 |= (trackMuteToggleRequested_[3] && flashMuteToggle) ? (trackMute_[3] ? LED_GREEN : LED_OFF) : (trackMute_[3] ? LED_OFF : LED_GREEN);
      led_gp5 |= (trackMuteToggleRequested_[4] && flashMuteToggle) ? (trackMute_[4] ? LED_GREEN : LED_OFF) : (trackMute_[4] ? LED_OFF : LED_GREEN);
      led_gp6 |= (trackMuteToggleRequested_[5] && flashMuteToggle) ? (trackMute_[5] ? LED_GREEN : LED_OFF) : (trackMute_[5] ? LED_OFF : LED_GREEN);

      if (isShiftTrackMuteToggleKeyPressed(0))
         led_gp1 |= LED_RED;
      if (isShiftTrackMuteToggleKeyPressed(1))
         led_gp2 |= LED_RED;
      if (isShiftTrackMuteToggleKeyPressed(2))
         led_gp3 |= LED_RED;
      if (isShiftTrackMuteToggleKeyPressed(3))
         led_gp4 |= LED_RED;
      if (isShiftTrackMuteToggleKeyPressed(4))
         led_gp5 |= LED_RED;
      if (isShiftTrackMuteToggleKeyPressed(5))
         led_gp6 |= LED_RED;
   }
   else
   {
      // --- Normal pages, outside menu/shift ---

      // Always indicate active track with a blue upper LED
      led_gp1 = activeTrack_ == 0 ? LED_BLUE : LED_OFF;
      led_gp2 = activeTrack_ == 1 ? LED_BLUE : LED_OFF;
      led_gp3 = activeTrack_ == 2 ? LED_BLUE : LED_OFF;
      led_gp4 = activeTrack_ == 3 ? LED_BLUE : LED_OFF;
      led_gp5 = activeTrack_ == 4 ? LED_BLUE : LED_OFF;
      led_gp6 = activeTrack_ == 5 ? LED_BLUE : LED_OFF;

      led_arm = isRecording_ ? LED_RED : LED_OFF;

      // Page-specific additonal lighting
      switch (page_)
      {
         case PAGE_SONG:
            // TODO
            break;

         case PAGE_MIDIMONITOR:
            // nothing to be done, no buttons
            break;

         case PAGE_TEMPO:
            led_gp1 |= command_ == COMMAND_TEMPO_BPM ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_TEMPO_BPM_UP ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_TEMPO_BPM_DOWN ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_TEMPO_TOGGLE_METRONOME ? LED_RED : LED_OFF;
            break;

         case PAGE_MUTE:
            led_gp1 |= (trackMuteToggleRequested_[0] && flashMuteToggle) ? (trackMute_[0] ? LED_GREEN : LED_OFF) : (trackMute_[0] ? LED_OFF : LED_GREEN);
            led_gp2 |= (trackMuteToggleRequested_[1] && flashMuteToggle) ? (trackMute_[1] ? LED_GREEN : LED_OFF) : (trackMute_[1] ? LED_OFF : LED_GREEN);
            led_gp3 |= (trackMuteToggleRequested_[2] && flashMuteToggle) ? (trackMute_[2] ? LED_GREEN : LED_OFF) : (trackMute_[2] ? LED_OFF : LED_GREEN);
            led_gp4 |= (trackMuteToggleRequested_[3] && flashMuteToggle) ? (trackMute_[3] ? LED_GREEN : LED_OFF) : (trackMute_[3] ? LED_OFF : LED_GREEN);
            led_gp5 |= (trackMuteToggleRequested_[4] && flashMuteToggle) ? (trackMute_[4] ? LED_GREEN : LED_OFF) : (trackMute_[4] ? LED_OFF : LED_GREEN);
            led_gp6 |= (trackMuteToggleRequested_[5] && flashMuteToggle) ? (trackMute_[5] ? LED_GREEN : LED_OFF) : (trackMute_[5] ? LED_OFF : LED_GREEN);
            break;

         case PAGE_NOTES:
            led_gp1 |= command_ == COMMAND_NOTE_POSITION ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_NOTE_KEY ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_NOTE_VELOCITY ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_NOTE_LENGTH ? LED_RED : LED_OFF;
            led_gp6 |= command_ == COMMAND_NOTE_DELETE ? LED_RED : LED_OFF;
            break;

         case PAGE_LIVEFX:
            led_gp1 |= command_ == COMMAND_LIVEFX_QUANTIZE ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_LIVEFX_SWING ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_LIVEFX_PROBABILITY ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_LIVEFX_FTS_MODE ? LED_RED : LED_OFF;
            led_gp6 |= command_ == COMMAND_LIVEFX_FTS_NOTE ? LED_RED : LED_OFF;
            break;

         case PAGE_SETUP:
            led_gp1 |= command_ == COMMAND_SETUP_SELECT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_SETUP_PAR1 ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_SETUP_PAR2 ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_SETUP_PAR3 ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_SETUP_PAR4 ? LED_RED : LED_OFF;
            break;

         case PAGE_ROUTER:
            led_gp1 |= command_ == COMMAND_ROUTE_SELECT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_ROUTE_IN_PORT ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_ROUTE_IN_CHANNEL ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_ROUTE_OUT_PORT ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_ROUTE_OUT_CHANNEL ? LED_RED : LED_OFF;
            break;

         case PAGE_DISK:
            led_gp1 |= command_ == COMMAND_DISK_SELECT_SESSION ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_DISK_SAVE ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_DISK_LOAD ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_DISK_NEW ? LED_RED : LED_OFF;
            break;

         case PAGE_CLIP:
            led_gp1 |= command_ == COMMAND_CLIP_LEN ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_CLIP_TRANSPOSE ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_CLIP_SCROLL ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_CLIP_STRETCH ? LED_RED : LED_OFF;
            led_gp6 |= command_ == COMMAND_CLIP_FREEZE ? LED_RED : LED_OFF;
            break;

         case PAGE_ARPECHO:
            led_gp1 |= command_ == COMMAND_ARPECHO_MODE ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_ARPECHO_PATTERN ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_ARPECHO_DELAY ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_ARPECHO_REPEATS ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_ARPECHO_NOTELENGTH ? LED_RED : LED_OFF;
            break;

         case PAGE_TRACK:
            led_gp1 |= command_ == COMMAND_TRACK_OUTPORT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_TRACK_OUTCHANNEL ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_TRACK_INPORT ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_TRACK_INCHANNEL ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_TRACK_TOGGLE_FORWARD ? LED_RED : LED_OFF;
            break;
      }
   }

   MUTEX_DIGITALOUT_TAKE;

   updateSwitchLED(LED_GP1, led_gp1);
   updateSwitchLED(LED_GP2, led_gp2);
   updateSwitchLED(LED_GP3, led_gp3);
   updateSwitchLED(LED_GP4, led_gp4);
   updateSwitchLED(LED_GP5, led_gp5);
   updateSwitchLED(LED_GP6, led_gp6);
   updateSwitchLED(LED_RUNSTOP, led_runstop);
   updateSwitchLED(LED_ARM, led_arm);
   updateSwitchLED(LED_SHIFT, led_shift);

   if (!gcBeatLEDsEnabled_ || !SEQ_BPM_IsRunning()) // If we are using the lower right matias LED as beat flashlights, don't control them from here
   {
      updateSwitchLED(LED_MENU, led_menu);
      updateSwitchLED(LED_COPY, led_copy);
      updateSwitchLED(LED_PASTE, led_paste);
      updateSwitchLED(LED_DELETE, led_delete);
   }

   MUTEX_DIGITALOUT_GIVE;
}
// -------------------------------------------------------------------------------------------------


/**
 * Update BEAT LEDs/Clip positions
 *
 */
void updateBeatLEDsAndClipPositions(u32 bpm_tick)
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

               updateSwitchLED(LED_RUNSTOP, LED_GREEN);
               if (gcBeatLEDsEnabled_)
               {
                  updateSwitchLED(LED_MENU, LED_GREEN);
                  updateSwitchLED(LED_COPY, LED_OFF);
                  updateSwitchLED(LED_PASTE, LED_OFF);
                  updateSwitchLED(LED_DELETE, LED_OFF);
               }
               break;
            case 1:
               if (gcBeatLEDsEnabled_)
               {
                  updateSwitchLED(LED_MENU, LED_OFF);
                  updateSwitchLED(LED_COPY, LED_BLUE | LED_GREEN);
                  updateSwitchLED(LED_PASTE, LED_OFF);
                  updateSwitchLED(LED_DELETE, LED_OFF);
               }
               else
                  updateSwitchLED(LED_RUNSTOP, LED_OFF);
               break;
            case 2:
               if (gcBeatLEDsEnabled_)
               {
                  updateSwitchLED(LED_MENU, 0);
                  updateSwitchLED(LED_COPY, 0);
                  updateSwitchLED(LED_PASTE, LED_BLUE | LED_GREEN);
                  updateSwitchLED(LED_DELETE, 0);
               }
               else
                  updateSwitchLED(LED_RUNSTOP, LED_OFF);
               break;
            case 3:
               if (gcBeatLEDsEnabled_)
               {
                  updateSwitchLED(LED_MENU, 0);
                  updateSwitchLED(LED_COPY, 0);
                  updateSwitchLED(LED_PASTE, 0);
                  updateSwitchLED(LED_DELETE, LED_BLUE | LED_GREEN);
               }
               else
                  updateSwitchLED(LED_RUNSTOP, LED_OFF);
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
 * Perform live LED updates (upper right encoder section)
 *
 */
void updateLiveLEDs()
{
   u8 led_livemode_transpose = LED_OFF, led_livemode_beatloop = LED_OFF;
   u8 led_livemode_1 = LED_OFF, led_livemode_2 = LED_OFF, led_livemode_3 = LED_OFF;
   u8 led_livemode_4 = LED_OFF, led_livemode_5 = LED_OFF, led_livemode_6 = LED_OFF;

   // --- Live section (upper right) ---
   s8 liveValue;
   // Live mode LEDs
   if (liveMode_ == LIVEMODE_TRANSPOSE)
   {
      led_livemode_transpose = 1;

      liveValue = liveTransposeRequested_;

      // flash-indicate live transpose target LEDs (only during synced/delayed transposition switch)
      if (liveMode_ == LIVEMODE_TRANSPOSE && liveTransposeRequested_ != liveTranspose_)
      {
         if (tick_ % 16 >= 8)
            liveValue = liveTranspose_;
      }
   }
   else
   {
      // LIVEMODE_BEATLOOP
      led_livemode_beatloop = 1;
      liveValue = liveBeatLoop_;
   }

   switch(liveValue)
   {
      case 7:
         led_livemode_1 = 1; led_livemode_2 = 1; led_livemode_3 = 1;
         break;

      case 6:
         led_livemode_1 = 1; led_livemode_2 = 1; led_livemode_3 = 0;
         break;

      case 5:
         led_livemode_1 = 1; led_livemode_2 = 0; led_livemode_3 = 1;
         break;

      case 4:
         led_livemode_1 = 1; led_livemode_2 = 0; led_livemode_3 = 0;
         break;

      case 3:
         led_livemode_1 = 0; led_livemode_2 = 1; led_livemode_3 = 1;
         break;

      case 2:
         led_livemode_1 = 0; led_livemode_2 = 1; led_livemode_3 = 0;
         break;

      case 1:
         led_livemode_1 = 0; led_livemode_2 = 0; led_livemode_3 = 1;
         break;

      case -1:
         led_livemode_4 = 1; led_livemode_5 = 0; led_livemode_6 = 0;
         break;

      case -2:
         led_livemode_4 = 0; led_livemode_5 = 1; led_livemode_6 = 0;
         break;

      case -3:
         led_livemode_4 = 1; led_livemode_5 = 1; led_livemode_6 = 0;
         break;

      case -4:
         led_livemode_4 = 0; led_livemode_5 = 0; led_livemode_6 = 1;
         break;

      case -5:
         led_livemode_4 = 1; led_livemode_5 = 0; led_livemode_6 = 1;
         break;

      case -6:
         led_livemode_4 = 0; led_livemode_5 = 1; led_livemode_6 = 1;
         break;

      case -7:
         led_livemode_4 = 1; led_livemode_5 = 1; led_livemode_6 = 1;
         break;

      default: /* case 0, all off (by initializer) */
         break;
   }

   MUTEX_DIGITALOUT_TAKE;

   MIOS32_DOUT_PinSet(HW_LED_SCENE_SWITCH_ALL, 1);

   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_BEATLOOP, led_livemode_beatloop);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_1, led_livemode_1);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_2, led_livemode_2);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_3, led_livemode_3);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_4, led_livemode_4);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_5, led_livemode_5);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_6, led_livemode_6);
   MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_TRANSPOSE, led_livemode_transpose);

   MUTEX_DIGITALOUT_GIVE;

}
// -------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------
// --- PAGE COMMANDS -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip length
 *
 */
void editLen()
{
   command_ = command_ == COMMAND_CLIP_LEN ? COMMAND_NONE : COMMAND_CLIP_LEN;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip transpose
 *
 */
void clipTranspose()
{
   command_ = command_ == COMMAND_CLIP_TRANSPOSE ? COMMAND_NONE : COMMAND_CLIP_TRANSPOSE;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip scrolling
 *
 */
void clipScroll()
{
   command_ = command_ == COMMAND_CLIP_SCROLL ? COMMAND_NONE : COMMAND_CLIP_SCROLL;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip stretching
 *
 */
void clipZoom()
{
   command_ = command_ == COMMAND_CLIP_STRETCH ? COMMAND_NONE : COMMAND_CLIP_STRETCH;
}
// -------------------------------------------------------------------------------------------------


/**
 * Clear clip
 *
 */
void clipClear()
{
   clipNotesSize_[activeTrack_][activeScene_] = 0;

   u8 i;
   for (i=0; i<128; i++)
      notePtrsOn_[i] = -1;

   screenFormattedFlashMessage("Clip %d cleared", activeTrack_ + 1);
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: Freeze/optimize clip
 *
 */
void clipFreeze()
{
   command_ = command_ == COMMAND_CLIP_FREEZE ? COMMAND_NONE : COMMAND_CLIP_FREEZE;

   u16 optimizedNotes = 0;
   u16 optimizedAmount = 0;
   u16 i;

   s32 clipLengthInTicks = getClipLengthInTicks(activeTrack_);

   // Copy over notes that have a velocity > 0 (not deleted) and that are visible in the current clip
   // then reset all transformations, so the notes stay identical
   for (i = 0; i < clipNotesSize_[activeTrack_][activeScene_]; i++)
   {
      if (clipNotes_[activeTrack_][activeScene_][i].velocity &&
          clipNotes_[activeTrack_][activeScene_][i].length)
      {
         s32 tick = clipNotes_[activeTrack_][activeScene_][i].tick;
         tick *= clipStretch_[activeTrack_][activeScene_];
         tick = tick / 16; // divide by 16 (stretch base)

         if (tick < clipLengthInTicks)
         {
            tick += clipScroll_[activeTrack_][activeScene_] * 24;

            while (tick < 0)
               tick += clipLengthInTicks;

            clipNotes_[activeTrack_][activeScene_][optimizedNotes] = clipNotes_[activeTrack_][activeScene_][i];
            clipNotes_[activeTrack_][activeScene_][optimizedNotes].tick = tick % clipLengthInTicks;

            s16 note = clipNotes_[activeTrack_][activeScene_][optimizedNotes].note +
                       clipTranspose_[activeTrack_][activeScene_];
            note = note < 0 ? 0 : note;
            note = note > 127 ? 127 : note;
            clipNotes_[activeTrack_][activeScene_][optimizedNotes].note = note;

            optimizedNotes++;
         }
      }
   }

   clipTranspose_[activeTrack_][activeScene_] = 0;
   clipScroll_[activeTrack_][activeScene_] = 0;
   clipStretch_[activeTrack_][activeScene_] = 16;

   optimizedAmount = clipNotesSize_[activeTrack_][activeScene_] - optimizedNotes;
   clipNotesSize_[activeTrack_][activeScene_] = optimizedNotes;

   screenFormattedFlashMessage("%d notes optimized", optimizedAmount);
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: change note position
 *
 */
void notesPosition()
{
   command_ = command_ == COMMAND_NOTE_POSITION ? COMMAND_NONE : COMMAND_NOTE_POSITION;
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: change note value
 *
 */
void notesNote()
{
   command_ = command_ == COMMAND_NOTE_KEY ? COMMAND_NONE : COMMAND_NOTE_KEY;
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: change note velocity
 *
 */
void notesVelocity()
{
   command_ = command_ == COMMAND_NOTE_VELOCITY ? COMMAND_NONE : COMMAND_NOTE_VELOCITY;
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: change note length
 *
 */
void notesLength()
{
   command_ = command_ == COMMAND_NOTE_LENGTH ? COMMAND_NONE : COMMAND_NOTE_LENGTH;
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: delete note
 *
 */
void notesDeleteNote()
{
   command_ = command_ == COMMAND_NOTE_DELETE ? COMMAND_NONE : COMMAND_NOTE_DELETE;

   u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

   if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
   {
      // only perform changes, if we are in range (still on the same clip)
      clipNotes_[activeTrack_][activeScene_][activeNote].velocity = 0;
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Port: change track output MIDI port
 *
 */
void trackPortOut()
{
   command_ = command_ == COMMAND_TRACK_OUTPORT ? COMMAND_NONE : COMMAND_TRACK_OUTPORT;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Channel: change track output MIDI channel
 *
 */
void trackChannelOut()
{
   command_ = command_ == COMMAND_TRACK_OUTCHANNEL ? COMMAND_NONE : COMMAND_TRACK_OUTCHANNEL;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Port: change track input MIDI port
 *
 */
void trackPortIn()
{
   command_ = command_ == COMMAND_TRACK_INPORT ? COMMAND_NONE : COMMAND_TRACK_INPORT;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Channel: change track input MIDI channel
 *
 */
void trackChannelIn()
{
   command_ = command_ == COMMAND_TRACK_INCHANNEL ? COMMAND_NONE : COMMAND_TRACK_INCHANNEL;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Toggle Live Forwarding
 *
 */
void trackToggleForward()
{
   command_ = command_ == COMMAND_TRACK_TOGGLE_FORWARD ? COMMAND_NONE : COMMAND_TRACK_TOGGLE_FORWARD;

   if (trackMidiForward_[activeTrack_] == 0)
      trackMidiForward_[activeTrack_] = 1;
   else
      trackMidiForward_[activeTrack_] = 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Toggle Live Transposition
 *
 */
void trackToggleLiveTranspose()
{
   command_ = command_ == COMMAND_TRACK_LIVE_TRANSPOSE ? COMMAND_NONE : COMMAND_TRACK_LIVE_TRANSPOSE;

   if (trackLiveTranspose_[activeTrack_] == 0)
      trackLiveTranspose_[activeTrack_] = 1;
   else
      trackLiveTranspose_[activeTrack_] = 0;
}
// -------------------------------------------------------------------------------------------------


/**
 * Scan if selected sessionNumber_ is available on disk
 *
 */
void diskScanSessionFileAvailable()
{
   MUTEX_SDCARD_TAKE;
   sessionNumberToFilename(sessionNumber_);
   sessionExistsOnDisk_ = FILE_FileExists(filename_) == 1 ? 1 : 0;
   MUTEX_SDCARD_GIVE;
}
// -------------------------------------------------------------------------------------------------


/**
 * Disk Menu: select current session
 *
 */
void diskSelectSession()
{
   command_ = command_ == COMMAND_DISK_SELECT_SESSION ? COMMAND_NONE : COMMAND_ROUTE_SELECT;
}
// -------------------------------------------------------------------------------------------------


/**
 * Disk Menu: Save session command
 *
 */
void diskSave()
{
   command_ = COMMAND_NONE;

   saveSession(sessionNumber_);
   diskScanSessionFileAvailable();

   setActivePage(PAGE_MUTE);
}
// -------------------------------------------------------------------------------------------------


/**
 * Disk Menu: Save session command
 *
 */
void diskLoad()
{
   command_ = COMMAND_NONE;

   loadSession(sessionNumber_);

   setActivePage(PAGE_MUTE);
}
// -------------------------------------------------------------------------------------------------


/**
 * Disk Menu: Save session command
 *
 */
void diskNew()
{
   command_ = COMMAND_NONE;

   seqInit();
   setActivePage(PAGE_MUTE);

   screenFormattedFlashMessage("A fresh start... :-)");
}
// -------------------------------------------------------------------------------------------------


/**
 * Tempo: Change BPM
 *
 */
void tempoBpm()
{
   command_ = command_ == COMMAND_TEMPO_BPM ? COMMAND_NONE : COMMAND_TEMPO_BPM;
}
// -------------------------------------------------------------------------------------------------


/**
 * Tempo: Increase BPM
 *
 */
void tempoBpmUp()
{
   command_ = command_ == COMMAND_TEMPO_BPM_UP ? COMMAND_NONE : COMMAND_TEMPO_BPM_UP;
   tempoFade_ = 1;
}
// -------------------------------------------------------------------------------------------------

/**
 * Tempo: Decrease BPM
 *
 */
void tempoBpmDown()
{
   command_ = command_ == COMMAND_TEMPO_BPM_DOWN ? COMMAND_NONE : COMMAND_TEMPO_BPM_DOWN;

   tempoFade_ = -1;
}
// -------------------------------------------------------------------------------------------------


/**
 * Tempo: Toggle Metronome
 *
 */
void tempoToggleMetronome()
{
   command_ = command_ == COMMAND_TEMPO_TOGGLE_METRONOME ? COMMAND_NONE : COMMAND_TEMPO_TOGGLE_METRONOME;
   metronomeEnabled_ = metronomeEnabled_ > 0 ? 0 : 1;
}
// -------------------------------------------------------------------------------------------------


/**
 * MIDI Router: select another route
 *
 */
void routerSelectRoute()
{
   command_ = command_ == COMMAND_ROUTE_SELECT ? COMMAND_NONE : COMMAND_ROUTE_SELECT;
}
// -------------------------------------------------------------------------------------------------


/**
 * MIDI Router: select another input port
 *
 */
void routerPortIn()
{
   command_ = command_ == COMMAND_ROUTE_IN_PORT ? COMMAND_NONE : COMMAND_ROUTE_IN_PORT;
}
// -------------------------------------------------------------------------------------------------


/**
 * MIDI Router: select another input channel
 *
 */
void routerChannelIn()
{
   command_ = command_ == COMMAND_ROUTE_IN_CHANNEL ? COMMAND_NONE : COMMAND_ROUTE_IN_CHANNEL;
}
// -------------------------------------------------------------------------------------------------


/**
 * MIDI Router: select another output port
 *
 */
void routerPortOut()
{
   command_ = command_ == COMMAND_ROUTE_OUT_PORT ? COMMAND_NONE : COMMAND_ROUTE_OUT_PORT;
}
// -------------------------------------------------------------------------------------------------


/**
 * MIDI Router: select another input channel
 *
 */
void routerChannelOut()
{
   command_ = command_ == COMMAND_ROUTE_OUT_CHANNEL ? COMMAND_NONE : COMMAND_ROUTE_OUT_CHANNEL;
}
// -------------------------------------------------------------------------------------------------


/**
 * Setup: select active item
 *
 */
void setupSelectItem()
{
   command_ = command_ == COMMAND_SETUP_SELECT ? COMMAND_NONE : COMMAND_SETUP_SELECT;
}
// -------------------------------------------------------------------------------------------------


/**
 * Setup: Parameter 1 button/command depressed
 *
 */
void setupPar1()
{
   command_ = command_ == COMMAND_SETUP_PAR1 ? COMMAND_NONE : COMMAND_SETUP_PAR1;
   setupParameterDepressed(1);
}
// -------------------------------------------------------------------------------------------------


/**
 * Setup: Parameter 2 button/command depressed
 *
 */
void setupPar2()
{
   command_ = command_ == COMMAND_SETUP_PAR2 ? COMMAND_NONE : COMMAND_SETUP_PAR2;
   setupParameterDepressed(2);
}
// -------------------------------------------------------------------------------------------------


/**
 * Setup: Parameter 3 button/command depressed
 *
 */
void setupPar3()
{
   command_ = command_ == COMMAND_SETUP_PAR3 ? COMMAND_NONE : COMMAND_SETUP_PAR3;
   setupParameterDepressed(3);
}
// -------------------------------------------------------------------------------------------------


/**
 * Setup: Parameter 4 button/command depressed
 *
 */
void setupPar4()
{
   command_ = command_ == COMMAND_SETUP_PAR4 ? COMMAND_NONE : COMMAND_SETUP_PAR4;
   setupParameterDepressed(4);
}
// -------------------------------------------------------------------------------------------------


/**
 * LiveFX Page: change clip quantization
 *
 */
void liveFxQuantize()
{
   command_ = command_ == COMMAND_LIVEFX_QUANTIZE ? COMMAND_NONE : COMMAND_LIVEFX_QUANTIZE;
}
// -------------------------------------------------------------------------------------------------


/**
 * LiveFX Page: change clip swing
 *
 */
void liveFxSwing()
{
   command_ = command_ == COMMAND_LIVEFX_SWING ? COMMAND_NONE : COMMAND_LIVEFX_SWING;
}
// -------------------------------------------------------------------------------------------------


/**
 * LiveFX Page: change clip note probability
 *
 */
void liveFxProbability()
{
   command_ = command_ == COMMAND_LIVEFX_PROBABILITY ? COMMAND_NONE : COMMAND_LIVEFX_PROBABILITY;
}
// -------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------
// --- EVENT DISPATCHING ---------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

/**
 * Handle switch/button press event
 *
 */
void loopaButtonPressed(s32 pin)
{
   if (hw_enabled == HARDWARE_LOOPA_TESTMODE)
   {
      DEBUG_MSG("Button: %d pressed\n", pin);
      screenFormattedFlashMessage("Button %d pressed", pin);
      testmodeFlashAllLEDs();
   }
   else
   {
      inactivitySeconds_ = 0;

      if (pin == sw_runstop)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_SETUP);
         }
         else if (screenIsInShift())
         {
            showShiftAbout_ = 1;
         }
         else
         {
            seqPlayStopButton();
         }
      }
      else if (pin == sw_armrecord)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_ROUTER);
         }
         else if (screenIsInShift())
         {
            showShiftHelp_ = 1;
         }
         else
         {
            seqArmButton();
         }
      }
      else if (pin == sw_shift)
      {
         if (screenIsInMenu())
         {
            diskScanSessionFileAvailable();
            setActivePage(PAGE_DISK);
         }
         else
         {
            // normal mode "shift"
            screenShowShift(1);
         }
      }
      else if (pin == sw_menu)
      {
         /*if (screenIsInShift())
         {

         }
         else */
         {
            // normal mode "menu"
            calcField();
            screenShowMenu(1);
         }
      }
      else if (pin == sw_copy)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_CLIP);
         }
         else
         {
            // normal mode "copy"
            copiedClipSteps_ = clipSteps_[activeTrack_][activeScene_];
            copiedClipQuantize_ = clipFxQuantize_[activeTrack_][activeScene_];
            copiedClipTranspose_ = clipTranspose_[activeTrack_][activeScene_];
            copiedClipScroll_ = clipScroll_[activeTrack_][activeScene_];
            copiedClipStretch_ = clipStretch_[activeTrack_][activeScene_];
            memcpy(copiedClipNotes_, clipNotes_[activeTrack_][activeScene_], sizeof(copiedClipNotes_));
            copiedClipNotesSize_ = clipNotesSize_[activeTrack_][activeScene_];
            screenFormattedFlashMessage("copied clip to buffer");
         }
      }
      else if (pin == sw_paste)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_LIVEFX);
         }
         else
         {
            // paste only, if we have a clip in memory
            if (copiedClipSteps_ > 0)
            {
               clipSteps_[activeTrack_][activeScene_] = copiedClipSteps_;
               clipFxQuantize_[activeTrack_][activeScene_] = copiedClipQuantize_;
               clipTranspose_[activeTrack_][activeScene_] = copiedClipTranspose_;
               clipScroll_[activeTrack_][activeScene_] = copiedClipScroll_;
               clipStretch_[activeTrack_][activeScene_] = copiedClipStretch_;
               memcpy(clipNotes_[activeTrack_][activeScene_], copiedClipNotes_, sizeof(copiedClipNotes_));
               clipNotesSize_[activeTrack_][activeScene_] = copiedClipNotesSize_;
               screenFormattedFlashMessage("pasted clip from buffer");
            }
            else
               screenFormattedFlashMessage("no clip in buffer");
         }
      }
      else if (pin == sw_delete)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_TRACK);
         }
         else
         {
            clipClear();
            command_ = COMMAND_NONE;
         }
      }
      else if (pin == sw_gp1)
      {
         if (screenIsInMenu())
         {
            // setActivePage(PAGE_SONG); TODO
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(0); // Toggling mute/unmute in shift menu
         }
         else
         {
            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(0);
                  break;
               case PAGE_CLIP:
                  editLen();
                  break;
               case PAGE_NOTES:
                  notesPosition();
                  break;
               case PAGE_TRACK:
                  trackPortOut();
                  break;
               case PAGE_DISK:
                  diskSelectSession();
                  break;
               case PAGE_TEMPO:
                  tempoBpm();
                  break;
               case PAGE_ROUTER:
                  routerSelectRoute();
                  break;
               case PAGE_SETUP:
                  setupSelectItem();
                  break;
               case PAGE_LIVEFX:
                  liveFxQuantize();
                  break;
            }
         }
      }
      else if (pin == sw_gp2)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_MIDIMONITOR);
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(1); // Toggling mute/unmute in shift menu
         }
         else
         {
            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(1);
                  break;
               case PAGE_CLIP:
                  /* TODO: clip type (note/cc) */
                  break;
               case PAGE_NOTES:
                  notesNote();
                  break;
               case PAGE_TRACK:
                  trackChannelOut();
                  break;
               case PAGE_DISK:
                  diskSave();
                  break;
               case PAGE_TEMPO:
                  tempoBpmUp();
                  break;
               case PAGE_ROUTER:
                  routerPortIn();
                  break;
               case PAGE_LIVEFX:
                  liveFxSwing();
                  break;
            }
         }
      }
      else if (pin == sw_gp3)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_TEMPO);
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(2); // Toggling mute/unmute in shift menu
         }
         else
         {
            // Normal GP3 page handling

            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(2);
                  break;
               case PAGE_CLIP:
                  clipTranspose();
                  break;
               case PAGE_NOTES:
                  notesVelocity();
                  break;
               case PAGE_TRACK:
                  trackPortIn();
                  break;
               case PAGE_DISK:
                  diskLoad();
                  break;
               case PAGE_TEMPO:
                  tempoBpmDown();
                  break;
               case PAGE_ROUTER:
                  routerChannelIn();
                  break;
               case PAGE_SETUP:
                  setupPar1();
                  break;
               case PAGE_LIVEFX:
                  liveFxProbability();
                  break;
            }
         }
      }
      else if (pin == sw_gp4)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_MUTE);
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(3); // Toggling mute/unmute in shift menu
         }
         else
         {
            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(3);
                  break;
               case PAGE_CLIP:
                  clipScroll();
                  break;
               case PAGE_NOTES:
                  notesLength();
                  break;
               case PAGE_TRACK:
                  trackChannelIn();
                  break;
               case PAGE_DISK:
                  diskNew();
                  break;
               case PAGE_TEMPO:
                  tempoToggleMetronome();
                  break;
               case PAGE_ROUTER:
                  routerPortOut();
                  break;
               case PAGE_SETUP:
                  setupPar2();
                  break;
            }
         }
      }
      else if (pin == sw_gp5)
      {
         if (screenIsInMenu())
         {
            setActivePage(PAGE_NOTES);
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(4); // Toggling mute/unmute in shift menu
         }
         else
         {
            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(4);
                  break;
               case PAGE_CLIP:
                  clipZoom();
                  break;
               case PAGE_TRACK:
                  trackToggleForward();
                  break;
               case PAGE_ROUTER:
                  routerChannelOut();
                  break;
               case PAGE_SETUP:
                  setupPar3();
                  break;
            }
         }
      }
      else if (pin == sw_gp6)
      {
         if (screenIsInMenu())
         {
            // setActivePage(PAGE_ARPECHO); TODO
         }
         else if (screenIsInShift())
         {
            shiftTrackMuteTogglePressed(5); // Toggling mute/unmute in shift menu
         }
         else
         {
            switch (page_)
            {
               case PAGE_MUTE:
                  toggleMute(5);
                  break;
               case PAGE_CLIP:
                  clipFreeze();
                  break;
               case PAGE_TRACK:
                  trackToggleLiveTranspose();
                  break;
               case PAGE_NOTES:
                  notesDeleteNote();
                  break;
               case PAGE_SETUP:
                  setupPar4();
                  break;
            }
         }
      }
      else if (pin == sw_enc_select)
      {
         scrubModeActive_ = 1;
      }
      else if (pin == sw_enc_live)
      {
         if (!screenIsInShift())
         {
            // Outside SHIFT menu: perform live mode switch

            switch (liveMode_)
            {
               case LIVEMODE_TRANSPOSE:
                  liveMode_ = LIVEMODE_BEATLOOP;
                  break;

               default:
                  liveMode_ = LIVEMODE_TRANSPOSE;
            }
         }
         else
         {
            // Inside SHIFT menu: reset current live mode parameter
            switch (liveMode_)
            {
               case LIVEMODE_TRANSPOSE:
                  if (liveTranspose_ == 0)
                     liveTransposeRequested_ = liveAlternatingTranspose_;
                  else
                  {
                     liveAlternatingTranspose_ = liveTranspose_;
                     liveTransposeRequested_ = 0;
                  }

                  if (!SEQ_BPM_IsRunning())
                     liveTranspose_ = liveTransposeRequested_;
                  break;

               default: // LIVEMODE_BEATLOOP
                  if (liveBeatLoop_ == 0)
                     liveBeatLoop_ = liveAlternatingBeatLoop_;
                  else
                  {
                     liveAlternatingBeatLoop_ = liveBeatLoop_;
                     liveBeatLoop_ = 0;
                  }
                  break;
            }
         }
         if (!SEQ_BPM_IsRunning())
            updateLiveLEDs();
      }
      else if (pin == sw_enc_value)
      {
         valueEncoderAccel_ = 1;
      }
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * Handle switch/button release event
 *
 */
void loopaButtonReleased(s32 pin)
{
   if (hw_enabled == HARDWARE_LOOPA_TESTMODE)
   {
      DEBUG_MSG("Button: %d released\n", pin);
      screenFormattedFlashMessage("Button %d released", pin);
      testmodeFlashAllLEDs();
   }
   else
   {
      inactivitySeconds_ = 0;
      tempoFade_ = 0;

      if (pin == sw_runstop)
      {
         if (screenIsInShift())
            showShiftAbout_ = 0;
      }
      else if (pin == sw_armrecord)
      {
         if (screenIsInShift())
            showShiftHelp_ = 0;
      }
      else if (pin == sw_menu)
      {
         if (screenIsInMenu())
            screenShowMenu(0); // Left the menu by releasing the menu button
      }
      else if (pin == sw_shift)
      {
         if (screenIsInShift())
         {
            screenShowShift(0); // Left the shift overlay by releasing the shift button
            showShiftAbout_ = 0;
            showShiftHelp_ = 0;
         }
      }
      else if (pin == sw_enc_select)
      {
         scrubModeActive_ = 0;
      }
      else if (pin == sw_enc_value)
      {
         valueEncoderAccel_ = 0;

         if (scrubModeActive_)
         {
            // Screenshot feature - triggered when two lower two encoder buttons are pressed
            screenshotRequested_ = 1;
            scrubModeActive_ = 0;
         }
      }
      else if (pin == sw_gp1)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(0); // Released toggling mute/unmute in shift menu
         }
      }
      else if (pin == sw_gp2)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(1); // Released toggling mute/unmute in shift menu
         }
      }
      else if (pin == sw_gp3)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(2); // Released toggling mute/unmute in shift menu
         }
      }
      else if (pin == sw_gp4)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(3); // Released toggling mute/unmute in shift menu
         }
      }
      else if (pin == sw_gp5)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(4); // Released toggling mute/unmute in shift menu
         }
      }
      else if (pin == sw_gp6)
      {
         if (screenIsInShift())
         {
            shiftTrackMuteToggleReleased(5); // Released toggling mute/unmute in shift menu
         }
      }
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * An encoder has been turned
 *
 */
void loopaEncoderTurned(s32 encoder, s32 incrementer)
{
   inactivitySeconds_ = 0;
   incrementer = -incrementer;

   if (hw_enabled == HARDWARE_LOOPA_TESTMODE)
   {
      DEBUG_MSG("[Encoder] %d turned, direction %d\n", encoder, incrementer);
      screenFormattedFlashMessage("Enc %d, dir %d", encoder, incrementer);
      testmodeFlashAllLEDs();
   }
   else
   {
      // Value encoder input acceleration (when pressed and value encoder turned)
      if (encoder == enc_value_id)
      {
         if (valueEncoderAccel_)
         {
            switch (command_)
            {
               case COMMAND_CLIP_TRANSPOSE:
                  incrementer *= 12; // octave transposing
                  break;
               case COMMAND_NOTE_POSITION:
                  incrementer *= TICKS_PER_STEP * ACCEL_FACTOR;
               default:
                  incrementer *= ACCEL_FACTOR;
                  break;
            }
         } else
         {
            switch (command_)
            {
               case COMMAND_NOTE_POSITION:
                  incrementer *= TICKS_PER_STEP;
                  break;
            }
         }
      }

      // Upper-left Scene encoder
      if (encoder == enc_scene_id)
      {
         if (incrementer < 0)
         {
            s8 newScene = sceneChangeRequested_ - 1;
            if (newScene < 0)
               newScene = 0;

            sceneChangeRequested_ = (u8) newScene;

            if (sceneChangeRequested_ == activeScene_ ||
                !SEQ_BPM_IsRunning()) // if (after changing) now no change requested or not playing, switch at once
               setActiveScene((u8) newScene);
         } else
         {
            s8 newScene = sceneChangeRequested_ + 1;
            if (newScene >= SCENES)
               newScene = SCENES - 1;

            sceneChangeRequested_ = (u8) newScene;

            if (sceneChangeRequested_ == activeScene_ ||
                !SEQ_BPM_IsRunning()) // if (after changing) now no change requested or not playing, switch at once
               setActiveScene((u8) newScene);
         }
      }

         // Lower-left Select/Track encoder
      else if (encoder == enc_select_id)
      {
         if (!scrubModeActive_)
         {
            // Not depressed encoder, not scrubbing, normal behaviour

            if (page_ == PAGE_DISK) // Disk page - left encoder changes selected session number
            {
               s16 newSessionNumber = sessionNumber_ + incrementer;
               newSessionNumber = newSessionNumber < 0 ? 0 : newSessionNumber;
               sessionNumber_ = (u16) newSessionNumber;

               diskScanSessionFileAvailable();
            } else if (page_ == PAGE_NOTES) // Notes page - left encoder changes selected note
            {
               s16 newNote = clipActiveNote_[activeTrack_][activeScene_] += incrementer;
               if (newNote < 0)
                  newNote = clipNotesSize_[activeTrack_][activeScene_] - 1;
               if (newNote >= clipNotesSize_[activeTrack_][activeScene_])
                  newNote = 0;
               clipActiveNote_[activeTrack_][activeScene_] = (u16) newNote;
            } else if (page_ == PAGE_ROUTER) // MIDI Router page - left encoder changes active/selected route
            {
               s8 newActiveRoute = routerActiveRoute_ += incrementer;
               newActiveRoute = (s8) (newActiveRoute < 0 ? 0 : newActiveRoute);
               routerActiveRoute_ = (u8) (newActiveRoute < MIDI_ROUTER_NUM_NODES ? newActiveRoute : (
                       MIDI_ROUTER_NUM_NODES - 1));
            } else if (page_ == PAGE_SETUP) // Setup page - left encoder changes active/selected setup item
            {
               s8 newActiveItem = setupActiveItem_ += incrementer;
               newActiveItem = (s8) (newActiveItem < 0 ? 0 : newActiveItem);
               setupActiveItem_ = (u8) (newActiveItem < SETUP_NUM_ITEMS ? newActiveItem : (SETUP_NUM_ITEMS - 1));
            } else // all other pages - change active clip number
            {
               s8 newTrack = (s8) (activeTrack_ + incrementer);
               newTrack = (s8) (newTrack < 0 ? 0 : newTrack);
               setActiveTrack((u8) (newTrack >= TRACKS ? TRACKS - 1 : newTrack));
            }
         } else
         {
            // Scrubbing
            SEQ_BPM_TickSet(SEQ_BPM_TickGet() + SEQ_BPM_PPQN_Get() * incrementer);
         }
      }

         // Upper-right "Live" encoder
      else if (encoder == enc_live_id)
      {
         if (liveMode_ == LIVEMODE_TRANSPOSE)
         {
            s8 newLiveTranspose = (s8) (liveTransposeRequested_ + incrementer);
            newLiveTranspose = (s8) (newLiveTranspose < -7 ? -7 : newLiveTranspose);
            liveTransposeRequested_ = (s8) (newLiveTranspose > 7 ? 7 : newLiveTranspose);

            if (!SEQ_BPM_IsRunning())
               liveTranspose_ = liveTransposeRequested_;
         } else
         {
            // LIVEMODE_BEATLOOP
            s8 newLiveBeatloop = (s8) (liveBeatLoop_ + incrementer);
            newLiveBeatloop = (s8) (newLiveBeatloop < -7 ? -7 : newLiveBeatloop);
            liveBeatLoop_ = (s8) (newLiveBeatloop > 7 ? 7 : newLiveBeatloop);
         }

         if (!SEQ_BPM_IsRunning())
            updateLiveLEDs();
      }

         // Lower-right "Value" encoder
      else if (encoder == enc_value_id)
      {
         if (command_ == COMMAND_DISK_SELECT_SESSION) // Disk page - left encoder changes selected session number
         {
            s16 newSessionNumber = sessionNumber_ + incrementer;
            newSessionNumber = newSessionNumber < 0 ? 0 : newSessionNumber;
            sessionNumber_ = (u16) newSessionNumber;

            diskScanSessionFileAvailable();
         } else if (command_ == COMMAND_TEMPO_BPM)
         {
            bpm_ += incrementer;
            bpm_ = round(bpm_);
            if (bpm_ < 30)
               bpm_ = 30;
            if (bpm_ > 300)
               bpm_ = 300;

            SEQ_BPM_Set(bpm_);
         } else if (command_ == COMMAND_CLIP_LEN)
         {
            if (incrementer > 0)
               clipSteps_[activeTrack_][activeScene_] *= 2;
            else
               clipSteps_[activeTrack_][activeScene_] /= 2;

            if (clipSteps_[activeTrack_][activeScene_] < 4)
               clipSteps_[activeTrack_][activeScene_] = 4;

            if (clipSteps_[activeTrack_][activeScene_] > 128)
               clipSteps_[activeTrack_][activeScene_] = 128;
         } else if (command_ == COMMAND_CLIP_TRANSPOSE)
         {
            clipTranspose_[activeTrack_][activeScene_] += incrementer;

            if (clipTranspose_[activeTrack_][activeScene_] < -96)
               clipTranspose_[activeTrack_][activeScene_] = -96;

            if (clipTranspose_[activeTrack_][activeScene_] > 96)
               clipTranspose_[activeTrack_][activeScene_] = 96;
         } else if (command_ == COMMAND_CLIP_SCROLL)
         {
            clipScroll_[activeTrack_][activeScene_] += incrementer;

            if (clipScroll_[activeTrack_][activeScene_] < -1024)
               clipScroll_[activeTrack_][activeScene_] = -1024;

            if (clipScroll_[activeTrack_][activeScene_] > 1024)
               clipScroll_[activeTrack_][activeScene_] = 1024;
         } else if (command_ == COMMAND_CLIP_STRETCH)
         {
            s16 newStretch = clipStretch_[activeTrack_][activeScene_];
            if (incrementer > 0)
               newStretch *= 2;
            else
               newStretch /= 2;

            if (newStretch < 1)
               newStretch = 1;

            if (newStretch > 128)
               newStretch = 128;

            clipStretch_[activeTrack_][activeScene_] = newStretch;
         } else if (command_ == COMMAND_NOTE_POSITION)
         {
            u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

            if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
            {
               // only perform changes, if we are in range (still on the same clip)

               s16 newTick = clipNotes_[activeTrack_][activeScene_][activeNote].tick;
               newTick += incrementer;

               u32 clipLength = getClipLengthInTicks(activeTrack_);

               if (newTick < 0)
                  newTick += clipLength;

               if (newTick >= clipLength)
                  newTick -= clipLength;

               // Normalize newTick to displayed step position
               newTick = (newTick / TICKS_PER_STEP) * TICKS_PER_STEP;

               clipNotes_[activeTrack_][activeScene_][activeNote].tick = (u16) newTick;
            }
         } else if (command_ == COMMAND_NOTE_KEY)
         {
            u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

            if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
            {
               // only perform changes, if we are in range (still on the same clip)

               s16 newNote = clipNotes_[activeTrack_][activeScene_][activeNote].note;
               newNote += incrementer;

               if (newNote < 1)
                  newNote = 1;

               if (newNote >= 127)
                  newNote = 127;

               clipNotes_[activeTrack_][activeScene_][activeNote].note = newNote;
            }
         } else if (command_ == COMMAND_NOTE_LENGTH)
         {
            u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

            if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
            {
               // only perform changes, if we are in range (still on the same clip)

               s16 newLength = clipNotes_[activeTrack_][activeScene_][activeNote].length;
               newLength += incrementer * 4;

               if (newLength <= 1)
                  newLength = 1;

               if (newLength >= 1536)
                  newLength = 1536;

               clipNotes_[activeTrack_][activeScene_][activeNote].length = (u16) newLength;
            }
         } else if (command_ == COMMAND_NOTE_VELOCITY)
         {
            u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

            if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
            {
               // only perform changes, if we are in range (still on the same clip)

               s16 newVel = clipNotes_[activeTrack_][activeScene_][activeNote].velocity;
               newVel += incrementer;

               if (newVel < 1)
                  newVel = 1;

               if (newVel >= 127)
                  newVel = 127;

               clipNotes_[activeTrack_][activeScene_][activeNote].velocity = newVel;
            }
         } else if (command_ == COMMAND_NOTE_DELETE)
         {
            u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

            if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
            {
               // only perform changes, if we are in range (still on the same clip)
               clipNotes_[activeTrack_][activeScene_][activeNote].velocity = 0;
            }
         } else if (command_ == COMMAND_TRACK_OUTPORT)
         {
            trackMidiOutPort_[activeTrack_] = adjustLoopAPortNumber(trackMidiOutPort_[activeTrack_], incrementer);
         } else if (command_ == COMMAND_TRACK_OUTCHANNEL)
         {
            s8 newChannel = trackMidiOutChannel_[activeTrack_] += incrementer;
            newChannel = newChannel > 15 ? 15 : newChannel;
            newChannel = newChannel < 0 ? 0 : newChannel;

            trackMidiOutChannel_[activeTrack_] = (u8) newChannel;
         } else if (command_ == COMMAND_TRACK_INPORT)
         {
            s8 newPortIndex = (s8) (MIDI_PORT_InIxGet((mios32_midi_port_t) trackMidiInPort_[activeTrack_]) +
                                    incrementer);

            newPortIndex = (s8) (newPortIndex <= 0 ? 0 : newPortIndex);

            if (newPortIndex >= MIDI_PORT_InNumGet() - 6)
               newPortIndex = (s8) (MIDI_PORT_InNumGet() - 6);

            trackMidiInPort_[activeTrack_] = MIDI_PORT_InPortGet((u8) newPortIndex);
         } else if (command_ == COMMAND_TRACK_INCHANNEL)
         {
            s8 newChannel = trackMidiInChannel_[activeTrack_] += incrementer;
            newChannel = newChannel > 16 ? 16 : newChannel;
            newChannel = newChannel < 0 ? 0 : newChannel;

            trackMidiInChannel_[activeTrack_] = (u8) newChannel;
         } else if (command_ == COMMAND_ROUTE_SELECT)
         {
            s8 newActiveRoute = routerActiveRoute_ += incrementer;
            newActiveRoute = (s8) (newActiveRoute < 0 ? 0 : newActiveRoute);
            routerActiveRoute_ = (u8) (newActiveRoute < MIDI_ROUTER_NUM_NODES ? newActiveRoute : (
                    MIDI_ROUTER_NUM_NODES - 1));
         } else if (command_ == COMMAND_ROUTE_IN_PORT)
         {
            midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
            s8 newPortIndex = (s8) (MIDI_PORT_InIxGet((mios32_midi_port_t) n->src_port) + incrementer);

            newPortIndex = (s8) (newPortIndex < 1 ? 1 : newPortIndex);

            if (newPortIndex >= MIDI_PORT_InNumGet() - 5)
               newPortIndex = (s8) (MIDI_PORT_InNumGet() - 5);

            n->src_port = MIDI_PORT_InPortGet((u8) newPortIndex);
            configChangesToBeWritten_ = 1;
         } else if (command_ == COMMAND_ROUTE_IN_CHANNEL)
         {
            midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
            s8 newChannel = (s8) ((mios32_midi_port_t) n->src_chn + incrementer);

            newChannel = (s8) (newChannel < 0 ? 0 : newChannel);
            newChannel = (s8) (newChannel > 17 ? 17 : newChannel);

            n->src_chn = (u8) newChannel;
            configChangesToBeWritten_ = 1;
         } else if (command_ == COMMAND_ROUTE_OUT_PORT)
         {
            midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
            s8 newPortIndex = (s8) (MIDI_PORT_OutIxGet((mios32_midi_port_t) n->dst_port) + incrementer);

            newPortIndex = (s8) (newPortIndex < 1 ? 1 : newPortIndex);

            if (newPortIndex >= MIDI_PORT_OutNumGet() - 5)
               newPortIndex = (s8) (MIDI_PORT_OutNumGet() - 5);

            n->dst_port = MIDI_PORT_OutPortGet((u8) newPortIndex);
            configChangesToBeWritten_ = 1;
         } else if (command_ == COMMAND_ROUTE_OUT_CHANNEL)
         {
            midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
            s8 newChannel = (s8) ((mios32_midi_port_t) n->dst_chn + incrementer);

            newChannel = (s8) (newChannel < 0 ? 0 : newChannel);
            newChannel = (s8) (newChannel > 17 ? 17 : newChannel);

            n->dst_chn = (u8) newChannel;
            configChangesToBeWritten_ = 1;
         } else if (command_ == COMMAND_SETUP_SELECT) // Setup page - left encoder changes active/selected setup item
         {
            s8 newActiveItem = setupActiveItem_ + incrementer;
            newActiveItem = (s8) (newActiveItem < 0 ? 0 : newActiveItem);
            setupActiveItem_ = (u8) (newActiveItem < SETUP_NUM_ITEMS ? newActiveItem : (SETUP_NUM_ITEMS - 1));
         } else if (command_ == COMMAND_SETUP_PAR1)
         {
            setupParameterEncoderTurned(1, incrementer);
         } else if (command_ == COMMAND_SETUP_PAR2)
         {
            setupParameterEncoderTurned(2, incrementer);
         } else if (command_ == COMMAND_SETUP_PAR3)
         {
            setupParameterEncoderTurned(3, incrementer);
         } else if (command_ == COMMAND_SETUP_PAR4)
         {
            setupParameterEncoderTurned(4, incrementer);
         } else if (command_ == COMMAND_LIVEFX_QUANTIZE)
         {
            if (incrementer > 0)
               clipFxQuantize_[activeTrack_][activeScene_] *= 2;
            else
               clipFxQuantize_[activeTrack_][activeScene_] /= 2;

            if (clipFxQuantize_[activeTrack_][activeScene_] < 2)
               clipFxQuantize_[activeTrack_][activeScene_] = 1;  // no quantization

            if (clipFxQuantize_[activeTrack_][activeScene_] == 2)
               clipFxQuantize_[activeTrack_][activeScene_] = 3;  // 1/128th note quantization

            if (clipFxQuantize_[activeTrack_][activeScene_] > 384)
               clipFxQuantize_[activeTrack_][activeScene_] = 384;
         } else if (command_ == COMMAND_LIVEFX_SWING)
         {
            s8 newSwing = clipFxSwing_[activeTrack_][activeScene_] + incrementer;
            newSwing = (s8) (newSwing < 0 ? 0 : newSwing);
            newSwing = (s8) (newSwing > 100 ? 100 : newSwing);
            clipFxSwing_[activeTrack_][activeScene_] = newSwing;
         } else if (command_ == COMMAND_LIVEFX_PROBABILITY)
         {
            s8 newProbability = clipFxProbability_[activeTrack_][activeScene_] + incrementer;
            newProbability = (s8) (newProbability < 0 ? 0 : newProbability);
            newProbability = (s8) (newProbability > 100 ? 100 : newProbability);
            clipFxProbability_[activeTrack_][activeScene_] = newProbability;
         }
      }
   }
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

      MUTEX_DIGITALOUT_TAKE;
      updateSwitchLED(LED_RUNSTOP, LED_GREEN);
      MUTEX_DIGITALOUT_GIVE;

      screenFormattedFlashMessage("Play");
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------


/**
 * Control the arm button function
 *
 */
s32 seqArmButton(void)
{
   if (isRecording_ == 0)
   {
      screenFormattedFlashMessage("Armed");
      isRecording_ = 1;

      MUTEX_DIGITALOUT_TAKE;
      MIOS32_DOUT_PinSet(HW_LED_RED_ARM, 1);
      MUTEX_DIGITALOUT_GIVE;
   }
   else
   {
      screenFormattedFlashMessage("Disarmed");
      isRecording_ = 0;

      MUTEX_DIGITALOUT_TAKE;
      MIOS32_DOUT_PinSet(HW_LED_RED_ARM, 0);
      MUTEX_DIGITALOUT_GIVE;
   }

   return 0; // no error
}
// -------------------------------------------------------------------------------------------------

