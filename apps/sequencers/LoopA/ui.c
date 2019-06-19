// LoopA UI data structures and code

#include <math.h>
#include "commonIncludes.h"

#include "tasks.h"
#include "file.h"

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

u8 routerActiveRoute_ = 0;
u8 setupActiveItem_ = 0;

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
   MIOS32_DOUT_PinSet(led_scene1, activeScene_ == 0);
   MIOS32_DOUT_PinSet(led_scene2, activeScene_ == 1);
   MIOS32_DOUT_PinSet(led_scene3, activeScene_ == 2);
   MIOS32_DOUT_PinSet(led_scene4, activeScene_ == 3);
   MIOS32_DOUT_PinSet(led_scene5, activeScene_ == 4);
   MIOS32_DOUT_PinSet(led_scene6, activeScene_ == 5);
   MUTEX_DIGITALOUT_GIVE;
}
// -------------------------------------------------------------------------------------------------

/**
 * Set a new active page
 */
void setActivePage(enum LoopAPage page)
{
   page_ = page;

   MUTEX_DIGITALOUT_TAKE;
   MIOS32_DOUT_PinSet(led_page_1, page == PAGE_MUTE);
   MIOS32_DOUT_PinSet(led_page_2, page == PAGE_CLIP);
   MIOS32_DOUT_PinSet(led_page_3, page == PAGE_NOTES);
   MIOS32_DOUT_PinSet(led_page_4, page == PAGE_TRACK);
   MIOS32_DOUT_PinSet(led_page_5, page == PAGE_DISK);
   MIOS32_DOUT_PinSet(led_page_6, page == PAGE_TEMPO);
   MUTEX_DIGITALOUT_GIVE;

   // Map to a proper command, that is on this page (or no command at all)
   switch(page)
   {
      case PAGE_MUTE: command_ = COMMAND_NONE; break;
      case PAGE_ROUTER: command_ = COMMAND_ROUTE_SELECT; break;
      case PAGE_DISK: command_ = COMMAND_DISK_SELECT_SESSION; break;
      case PAGE_TEMPO: command_= COMMAND_TEMPO_BPM; break;
      case PAGE_CLIP: command_ = COMMAND_CLIPLEN; break;
      case PAGE_NOTES: command_ = COMMAND_POSITION; break;
      case PAGE_TRACK: command_ = COMMAND_TRACK_OUTPORT; break;
      case PAGE_SETUP: command_ = COMMAND_SETUP_SELECT; break;
      default:
         command_ = COMMAND_NONE; break;
   }

   if (configChangesToBeWritten_)
      writeSetup();
}
// -------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
// --- LED HANDLING
// ---------------------------------------------------------------------------------------------------------------------

/**
 * Update a single LED (called from MUTEX_DIGITALOUT protected environment)
 *
 */
static u8 ledstate[13];
void updateLED(u8 number, u8 newState)
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
 * Update the LED states of the Matias switches (called every 20ms from app.c timer)
 *
 */
void updateLEDs()
{
   u8 led_gp1 = LED_OFF, led_gp2 = LED_OFF, led_gp3 = LED_OFF, led_gp4 = LED_OFF, led_gp5 = LED_OFF, led_gp6 = LED_OFF;
   u8 led_arm = LED_OFF, led_shift = LED_OFF, led_menu = LED_OFF;
   u8 led_copy = LED_OFF, led_paste = LED_OFF, led_delete = LED_OFF;

   u8 led_runstop = ledstate[LED_RUNSTOP];

   if (!SEQ_BPM_IsRunning())
   {
      led_runstop = LED_OFF;
      led_menu = LED_OFF;
      led_copy = LED_OFF;
      led_paste = LED_OFF;
      led_delete = LED_OFF;
   }

   if (screenIsInMenu())
   {
      led_menu = LED_RED;

      switch (page_)
      {
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
         case PAGE_FX:
            led_paste = LED_RED;
            break;
         case PAGE_TRACK:
            led_delete = LED_RED;
            break;
      }
   }
   else
   {
      // Normal pages, outside menu/shift

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
         case PAGE_MUTE:
            led_gp1 |= trackMute_[0] ? LED_OFF : LED_GREEN;
            led_gp2 |= trackMute_[1] ? LED_OFF : LED_GREEN;
            led_gp3 |= trackMute_[2] ? LED_OFF : LED_GREEN;
            led_gp4 |= trackMute_[3] ? LED_OFF : LED_GREEN;
            led_gp5 |= trackMute_[4] ? LED_OFF : LED_GREEN;
            led_gp6 |= trackMute_[5] ? LED_OFF : LED_GREEN;
            break;

         case PAGE_CLIP:
            led_gp1 |= command_ == COMMAND_CLIPLEN ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_QUANTIZE ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_TRANSPOSE ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_SCROLL ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_STRETCH ? LED_RED : LED_OFF;
            led_gp6 |= command_ == COMMAND_FREEZE ? LED_RED : LED_OFF;
            break;

         case PAGE_FX:
            break;

         case PAGE_NOTES:
            led_gp1 |= command_ == COMMAND_POSITION ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_NOTE_KEY ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_NOTE_VELOCITY ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_NOTE_LENGTH ? LED_RED : LED_OFF;
            led_gp6 |= command_ == COMMAND_NOTE_DELETE ? LED_RED : LED_OFF;
            break;

         case PAGE_TRACK:
            led_gp1 |= command_ == COMMAND_TRACK_OUTPORT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_TRACK_OUTCHANNEL ? LED_RED : LED_OFF;
            break;

         case PAGE_DISK:
            led_gp1 |= command_ == COMMAND_DISK_SELECT_SESSION ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_DISK_SAVE ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_DISK_LOAD ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_DISK_NEW ? LED_RED : LED_OFF;
            break;

         case PAGE_TEMPO:
            led_gp1 |= command_ == COMMAND_TEMPO_BPM ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_TEMPO_BPM_UP ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_TEMPO_BPM_DOWN ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_TEMPO_TOGGLE_METRONOME ? LED_RED : LED_OFF;
            break;

         case PAGE_ROUTER:
            led_gp1 |= command_ == COMMAND_ROUTE_SELECT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_ROUTE_IN_PORT ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_ROUTE_IN_CHANNEL ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_ROUTE_OUT_PORT ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_ROUTE_OUT_CHANNEL ? LED_RED : LED_OFF;
            break;

         case PAGE_SETUP:
            led_gp1 |= command_ == COMMAND_SETUP_SELECT ? LED_RED : LED_OFF;
            led_gp2 |= command_ == COMMAND_SETUP_PAR1 ? LED_RED : LED_OFF;
            led_gp3 |= command_ == COMMAND_SETUP_PAR2 ? LED_RED : LED_OFF;
            led_gp4 |= command_ == COMMAND_SETUP_PAR3 ? LED_RED : LED_OFF;
            led_gp5 |= command_ == COMMAND_SETUP_PAR4 ? LED_RED : LED_OFF;
            break;
      }
   }

   MUTEX_DIGITALOUT_TAKE;

   updateLED(LED_GP1, led_gp1);
   updateLED(LED_GP2, led_gp2);
   updateLED(LED_GP3, led_gp3);
   updateLED(LED_GP4, led_gp4);
   updateLED(LED_GP5, led_gp5);
   updateLED(LED_GP6, led_gp6);
   updateLED(LED_RUNSTOP, led_runstop);
   updateLED(LED_ARM, led_arm);
   updateLED(LED_SHIFT, led_shift);


   if (!gcBeatLEDsEnabled_ || !SEQ_BPM_IsRunning()) // If we are using the lower right matias LED as beat flashlights, don't control them from here
   {
      updateLED(LED_MENU, led_menu);
      updateLED(LED_COPY, led_copy);
      updateLED(LED_PASTE, led_paste);
      updateLED(LED_DELETE, led_delete);
   }

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
   command_ = command_ == COMMAND_CLIPLEN ? COMMAND_NONE : COMMAND_CLIPLEN;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip quantization
 *
 */
void clipQuantize()
{
   command_ = command_ == COMMAND_QUANTIZE ? COMMAND_NONE : COMMAND_QUANTIZE;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip transpose
 *
 */
void clipTranspose()
{
   command_ = command_ == COMMAND_TRANSPOSE ? COMMAND_NONE : COMMAND_TRANSPOSE;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip scrolling
 *
 */
void clipScroll()
{
   command_ = command_ == COMMAND_SCROLL ? COMMAND_NONE : COMMAND_SCROLL;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: change clip stretching
 *
 */
void clipZoom()
{
   command_ = command_ == COMMAND_STRETCH ? COMMAND_NONE : COMMAND_STRETCH;
}
// -------------------------------------------------------------------------------------------------


/**
 * Edit Clip Page: Clear clip
 *
 */
void clipClear()
{
   command_ = command_ == COMMAND_FREEZE ? COMMAND_NONE : COMMAND_FREEZE;

   clipNotesSize_[activeTrack_][activeScene_] = 0;

   u8 i;
   for (i=0; i<128; i++)
      notePtrsOn_[i] = -1;

   screenFormattedFlashMessage("Clip %d cleared", activeTrack_ + 1);
}
// -------------------------------------------------------------------------------------------------


/**
 * Notes: change note position
 *
 */
void notesPosition()
{
   command_ = command_ == COMMAND_POSITION ? COMMAND_NONE : COMMAND_POSITION;
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
 * Midi Track Port: change clip output MIDI port
 *
 */
void trackPortOut()
{
   command_ = command_ == COMMAND_TRACK_OUTPORT ? COMMAND_NONE : COMMAND_TRACK_OUTPORT;
}
// -------------------------------------------------------------------------------------------------


/**
 * Midi Track Channel: change clip output MIDI channel
 *
 */
void trackChannelOut()
{
   command_ = command_ == COMMAND_TRACK_OUTCHANNEL ? COMMAND_NONE : COMMAND_TRACK_OUTCHANNEL;
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
   screenNotifyPageChanged();
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
   screenNotifyPageChanged();
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
   screenNotifyPageChanged();

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


// -------------------------------------------------------------------------------------------------
// --- EVENT DISPATCHING ---------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

/**
 * Handle switch/button press event
 *
 */
void loopaButtonPressed(s32 pin)
{
   DEBUG_MSG("Button: %d pressed\n", pin);

   inactivitySeconds_ = 0;

   if (pin == sw_runstop)
   {
      if (screenIsInMenu())
      {
         setActivePage(PAGE_SETUP);
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
      else
      {
         seqArmButton();
      }
   }
   else if (pin == sw_shift)
   {
      if (screenIsInMenu())
      {
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
         copiedClipQuantize_ = clipQuantize_[activeTrack_][activeScene_];
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
         setActivePage(PAGE_FX);
      }
      else
      {
         // paste only, if we have a clip in memory
         if (copiedClipSteps_ > 0)
         {
            clipSteps_[activeTrack_][activeScene_] = copiedClipSteps_;
            clipQuantize_[activeTrack_][activeScene_] = copiedClipQuantize_;
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
         clipClear(); // shortcut: clear track
         command_ = COMMAND_NONE;
      }
   }
   else if (pin == sw_gp1)
   {
      if (screenIsInMenu())
      {
         // setActivePage(PAGE_SYSEX); TODO
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
         }
      }
   }
   else if (pin == sw_gp2)
   {
      if (screenIsInMenu())
      {
         setActivePage(PAGE_MIDIMONITOR);
      }
      else
      {
         switch (page_)
         {
            case PAGE_MUTE:
               toggleMute(1);
               break;
            case PAGE_CLIP:
               clipQuantize();
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
         }
      }
   }
   else if (pin == sw_gp3)
   {
      if (screenIsInMenu())
      {
         setActivePage(PAGE_TEMPO);
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
         }
      }
   }
   else if (pin == sw_gp4)
   {
      if (screenIsInMenu())
      {
         setActivePage(PAGE_MUTE);
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
         // setActivePage(PAGE_SONG); TODO
      }
      else
      {
         switch (page_)
         {
            case PAGE_MUTE:
               toggleMute(5);
               break;
            case PAGE_CLIP:
               clipClear();
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
   else if (pin == sw_encoder2)
   {
      setActivePage(PAGE_MUTE); // shortcut back to track display
      command_ = COMMAND_NONE;
      screenNotifyPageChanged();
   }
}
// -------------------------------------------------------------------------------------------------


/**
 * * Handle switch/button release event
 *
 */
void loopaButtonReleased(s32 pin)
{
   inactivitySeconds_ = 0;
   tempoFade_ = 0;

   if (screenIsInMenu() && pin == sw_menu)
   {
      DEBUG_MSG("leave menu");
      screenShowMenu(0); // Left the menu by releasing the menu button
   }

   if (screenIsInShift() && pin == sw_shift)
   {
      screenShowShift(0); // Left the shift overlay by releasing the shift button
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
   DEBUG_MSG("[Encoder] %d turned, direction %d\n", encoder, incrementer);

   // Upper-left Scene encoder
   if (encoder == enc_scene_id)
   {
      if (incrementer < 0)
      {
         s8 newScene = sceneChangeRequested_ - 1;
         if (newScene < 0)
            newScene = 0;

         sceneChangeRequested_ = (u8)newScene;

         if (sceneChangeRequested_ == activeScene_ || !SEQ_BPM_IsRunning()) // if (after changing) now no change requested or not playing, switch at once
            setActiveScene((u8)newScene);
      }
      else
      {
         s8 newScene = sceneChangeRequested_ + 1;
         if (newScene >= SCENES)
            newScene = SCENES - 1;

         sceneChangeRequested_ = (u8)newScene;

         if (sceneChangeRequested_ == activeScene_ || !SEQ_BPM_IsRunning()) // if (after changing) now no change requested or not playing, switch at once
            setActiveScene((u8)newScene);
      }
   }

   // Lower-left Select/Track encoder
   else if (encoder == enc_select_id)
   {
      if (page_ == PAGE_DISK) // Disk page - left encoder changes selected session number
      {
         s16 newSessionNumber = sessionNumber_ + incrementer;
         newSessionNumber = newSessionNumber < 0 ? 0 : newSessionNumber;
         sessionNumber_ = (u16)newSessionNumber;

         diskScanSessionFileAvailable();
      }
      else if (page_ == PAGE_NOTES) // Notes page -left encoder changes selected note
      {
         s16 newNote = clipActiveNote_[activeTrack_][activeScene_] += incrementer;
         if (newNote < 0)
            newNote = clipNotesSize_[activeTrack_][activeScene_] - 1;
         if (newNote >= clipNotesSize_[activeTrack_][activeScene_])
            newNote = 0;
         clipActiveNote_[activeTrack_][activeScene_] = (u16)newNote;
      }
      else if (page_ == PAGE_ROUTER) // MIDI Router page - left encoder changes active/selected route
      {
         s8 newActiveRoute = routerActiveRoute_ += incrementer;
         newActiveRoute = (s8)(newActiveRoute < 0 ? 0 : newActiveRoute);
         routerActiveRoute_ = (u8)(newActiveRoute < MIDI_ROUTER_NUM_NODES ? newActiveRoute : (MIDI_ROUTER_NUM_NODES - 1));
      }
      else if (page_ == PAGE_SETUP) // Setup page - left encoder changes active/selected setup item
      {
         s8 newActiveItem = setupActiveItem_ += incrementer;
         newActiveItem = (s8)(newActiveItem < 0 ? 0 : newActiveItem);
         setupActiveItem_ = (u8)(newActiveItem < SETUP_NUM_ITEMS ? newActiveItem : (SETUP_NUM_ITEMS - 1));
      }
      else // all other pages - change active clip number
      {
         s8 newTrack = (s8)(activeTrack_ + incrementer);
         newTrack = (s8)(newTrack < 0 ? 0 : newTrack);
         setActiveTrack((u8)(newTrack >= TRACKS ? TRACKS-1 : newTrack));
      }
   }

   // Upper-right "Live" encoder
   else if (encoder == enc_live_id)
   {
      // switch through pages

      enum LoopAPage page = page_;

      if (page == PAGE_MUTE && incrementer < 0)
         page = PAGE_MUTE;
      else if (page >= PAGE_TEMPO && incrementer > 0)
         page = PAGE_TEMPO;
      else
         page += incrementer;

      if (page == PAGE_DISK)
         diskScanSessionFileAvailable();

      setActivePage(page);

      screenNotifyPageChanged();
   }

   // Lower-right "Value" encoder
   else if (encoder == enc_value_id)
   {
      if (command_ == COMMAND_DISK_SELECT_SESSION) // Disk page - left encoder changes selected session number
      {
         s16 newSessionNumber = sessionNumber_ + incrementer;
         newSessionNumber = newSessionNumber < 0 ? 0 : newSessionNumber;
         sessionNumber_ = (u16)newSessionNumber;

         diskScanSessionFileAvailable();
      }
      else if (command_ == COMMAND_TEMPO_BPM)
      {
         bpm_ += incrementer;
         bpm_ = round(bpm_);
         if (bpm_ < 30)
            bpm_ = 30;
         if (bpm_ > 300)
            bpm_ = 300;

         SEQ_BPM_Set(bpm_);
      }
      else if (command_ == COMMAND_CLIPLEN)
      {
         if (incrementer > 0)
            clipSteps_[activeTrack_][activeScene_] *= 2;
         else
            clipSteps_[activeTrack_][activeScene_] /= 2;

         if (clipSteps_[activeTrack_][activeScene_] < 4)
            clipSteps_[activeTrack_][activeScene_] = 4;

         if (clipSteps_[activeTrack_][activeScene_] > 128)
            clipSteps_[activeTrack_][activeScene_] = 128;
      }
      else if (command_ == COMMAND_QUANTIZE)
      {
         if (incrementer > 0)
            clipQuantize_[activeTrack_][activeScene_] *= 2;
         else
            clipQuantize_[activeTrack_][activeScene_] /= 2;

         if (clipQuantize_[activeTrack_][activeScene_] < 2)
            clipQuantize_[activeTrack_][activeScene_] = 1;  // no quantization

         if (clipQuantize_[activeTrack_][activeScene_] == 2)
            clipQuantize_[activeTrack_][activeScene_] = 3;  // 1/128th note quantization

         if (clipQuantize_[activeTrack_][activeScene_] > 384)
            clipQuantize_[activeTrack_][activeScene_] = 384;
      }
      else if (command_ == COMMAND_TRANSPOSE)
      {
         if (incrementer > 0)
            clipTranspose_[activeTrack_][activeScene_]++;
         else
            clipTranspose_[activeTrack_][activeScene_]--;

         if (clipTranspose_[activeTrack_][activeScene_] < -96)
            clipTranspose_[activeTrack_][activeScene_] = -96;

         if (clipTranspose_[activeTrack_][activeScene_] > 96)
            clipTranspose_[activeTrack_][activeScene_] = 96;
      }
      else if (command_ == COMMAND_SCROLL)
      {
         clipScroll_[activeTrack_][activeScene_] += incrementer;

         if (clipScroll_[activeTrack_][activeScene_] < -1024)
            clipScroll_[activeTrack_][activeScene_] = -1024;

         if (clipScroll_[activeTrack_][activeScene_] > 1024)
            clipScroll_[activeTrack_][activeScene_] = 1024;
      }
      else if (command_ == COMMAND_STRETCH)
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
      }
      else if (command_ == COMMAND_POSITION)
      {
         u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

         if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
         {
            // only perform changes, if we are in range (still on the same clip)

            s16 newTick = clipNotes_[activeTrack_][activeScene_][activeNote].tick;
            newTick += incrementer > 0 ? 24 : -24;
            u32 clipLength = getClipLengthInTicks(activeTrack_);

            if (newTick < 0)
               newTick += clipLength;

            if (newTick >= clipLength)
               newTick -= clipLength;

            clipNotes_[activeTrack_][activeScene_][activeNote].tick = (u16)newTick;
         }
      }
      else if (command_ == COMMAND_NOTE_KEY)
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
      }
      else if (command_ == COMMAND_NOTE_LENGTH)
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

            clipNotes_[activeTrack_][activeScene_][activeNote].length = (u16)newLength;
         }
      }
      else if (command_ == COMMAND_NOTE_VELOCITY)
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
      }
      else if (command_ == COMMAND_NOTE_DELETE)
      {
         u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

         if (activeNote < clipNotesSize_[activeTrack_][activeScene_])
         {
            // only perform changes, if we are in range (still on the same clip)
            clipNotes_[activeTrack_][activeScene_][activeNote].velocity = 0;
         }
      }
      else if (command_ == COMMAND_TRACK_OUTPORT)
      {
         trackMidiPort_[activeTrack_] = adjustLoopAPortNumber(trackMidiPort_[activeTrack_], incrementer);
      }
      else if (command_ == COMMAND_TRACK_OUTCHANNEL)
      {
         s8 newChannel = trackMidiChannel_[activeTrack_] += incrementer;
         newChannel = newChannel > 15 ? 15 : newChannel;
         newChannel = newChannel < 0 ? 0 : newChannel;

         trackMidiChannel_[activeTrack_] = (u8)newChannel;
      }
      else if (command_ == COMMAND_ROUTE_SELECT)
      {
         s8 newActiveRoute = routerActiveRoute_ += incrementer;
         newActiveRoute = (s8)(newActiveRoute < 0 ? 0 : newActiveRoute);
         routerActiveRoute_ = (u8)(newActiveRoute < MIDI_ROUTER_NUM_NODES ? newActiveRoute : (MIDI_ROUTER_NUM_NODES - 1));
      }
      else if (command_ == COMMAND_ROUTE_IN_PORT)
      {
         midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
         s8 newPortIndex = (s8)(MIDI_PORT_InIxGet((mios32_midi_port_t)n->src_port) + incrementer);

         newPortIndex = (s8)(newPortIndex < 1 ? 1 : newPortIndex);

         if (newPortIndex >= MIDI_PORT_InNumGet()-6)
            newPortIndex = (s8)(MIDI_PORT_InNumGet()-6);

         n->src_port = MIDI_PORT_InPortGet((u8)newPortIndex);
         configChangesToBeWritten_ = 1;
      }
      else if (command_ == COMMAND_ROUTE_IN_CHANNEL)
      {
         midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
         s8 newChannel = (s8)((mios32_midi_port_t)n->src_chn + incrementer);

         newChannel = (s8)(newChannel < 0 ? 0 : newChannel);
         newChannel = (s8)(newChannel > 17 ? 17 : newChannel);

         n->src_chn = (u8)newChannel;
         configChangesToBeWritten_ = 1;
      }
      else if (command_ == COMMAND_ROUTE_OUT_PORT)
      {
         midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
         s8 newPortIndex = (s8)(MIDI_PORT_OutIxGet((mios32_midi_port_t)n->dst_port) + incrementer);

         newPortIndex = (s8)(newPortIndex < 1 ? 1 : newPortIndex);

         if (newPortIndex >= MIDI_PORT_OutNumGet()-6)
            newPortIndex = (s8)(MIDI_PORT_OutNumGet()-6);

         n->dst_port = MIDI_PORT_OutPortGet((u8)newPortIndex);
         configChangesToBeWritten_ = 1;
      }
      else if (command_ == COMMAND_ROUTE_OUT_CHANNEL)
      {
         midi_router_node_entry_t *n = &midi_router_node[routerActiveRoute_];
         s8 newChannel = (s8)((mios32_midi_port_t)n->dst_chn + incrementer);

         newChannel = (s8)(newChannel < 0 ? 0 : newChannel);
         newChannel = (s8)(newChannel > 17 ? 17 : newChannel);

         n->dst_chn = (u8)newChannel;
         configChangesToBeWritten_ = 1;
      }
      else if (command_ == COMMAND_SETUP_SELECT) // Setup page - left encoder changes active/selected setup item
      {
         s8 newActiveItem = setupActiveItem_ += incrementer;
         newActiveItem = (s8)(newActiveItem < 0 ? 0 : newActiveItem);
         setupActiveItem_ = (u8)(newActiveItem < SETUP_NUM_ITEMS ? newActiveItem : (SETUP_NUM_ITEMS - 1));
      }
      else if (command_ == COMMAND_SETUP_PAR1)
      {
         setupParameterEncoderTurned(1, incrementer);
      }
      else if (command_ == COMMAND_SETUP_PAR2)
      {
         setupParameterEncoderTurned(2, incrementer);
      }
      else if (command_ == COMMAND_SETUP_PAR3)
      {
         setupParameterEncoderTurned(3, incrementer);
      }
      else if (command_ == COMMAND_SETUP_PAR4)
      {
         setupParameterEncoderTurned(4, incrementer);
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
      updateLED(LED_RUNSTOP, LED_GREEN);
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


/**
 * Fast forward is put on depressed LL encoder as "scrub"
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
 * Rewind is put on depressed LL encoder as "scrub"
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

