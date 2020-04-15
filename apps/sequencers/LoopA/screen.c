// LoopA 256x64px screen routines

#include <mios32.h>
#include "mios32_config.h"
#include "commonIncludes.h"

#include "app.h"
#include "hardware.h"
#include "tasks.h"
#include "screen.h"
#include "gfx_resources.h"
#include "loopa.h"
#include "ui.h"
#include "setup.h"
#include "voxelspace.h"

// -------------------------------------------------------------------------------------------
// SSD 1322 Screen routines by Hawkeye

// --- globals ---

u8 screen[64][128];             // Screen buffer [y][x]

u8 screenShowLoopaLogo_;
u8 screenShowShift_ = 0;
u8 screenShowMenu_ = 0;
u8 screenClipNumberSelected_ = 0;
u16 screenClipStepPosition_[TRACKS];
u32 screenSongStep_ = 0;
char screenFlashMessage_[40];
u8 screenFlashMessageFrameCtr_;
char sceneChangeNotification_[20] = "";
u8 screenshotRequested_ = 0;    // if set to 1, will save screenshot to sd card when the next frame is rendered

unsigned char* fontptr_ = (unsigned char*) fontsmall_b_pixdata;
u16 fontchar_bytewidth_ = 3;    // bytes to copy for a line of character pixel data
u16 fontchar_height_ = 12;      // lines to copy for a full-height character
u16 fontline_bytewidth_ = 95*3; // bytes per font pixdata line (character layout all in one line)
u8 fontInverted_ = 0;


/**
 * Set the LoopA logo font
 *
 */
void setFontLoopALogo()
{
   fontptr_ = (unsigned char*) logo_pixdata;
   fontchar_bytewidth_ = logo_width / 2;
   fontchar_height_ = logo_height;
   fontline_bytewidth_ = logo_width / 2;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the bold font
 *
 */
void setFontBold()
{
   switch (gcFontType_)
   {
      case 'a': fontptr_ = (unsigned char*) fontbold_a_pixdata; break;
      default: fontptr_ = (unsigned char*) fontbold_b_pixdata; break;
   }

   fontchar_bytewidth_ = 5;
   fontchar_height_ = 18;
   fontline_bytewidth_ = 95 * 5;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the normal font
 *
 */
void setFontNormal()
{
   switch (gcFontType_)
   {
      case 'a': fontptr_ = (unsigned char*) fontnormal_a_pixdata; break;
      default: fontptr_ = (unsigned char*) fontnormal_b_pixdata; break;
   }

   fontchar_bytewidth_ = 5;
   fontchar_height_ = 18;
   fontline_bytewidth_ = 95 * 5;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the small font
 *
 */
void setFontSmall()
{
   switch (gcFontType_)
   {
      case 'a': fontptr_ = (unsigned char*) fontsmall_a_pixdata; break;
      default: fontptr_ = (unsigned char*) fontsmall_b_pixdata; break;
   }

   fontchar_bytewidth_ = 3;
   fontchar_height_ = 12;
   fontline_bytewidth_ = 95 * 3;
}
// ----------------------------------------------------------------------------------------

/**
 * Set the small font
 *
 */
void setFontKeyIcon()
{
    fontptr_ = (unsigned char*) keyicons_pixdata;
    fontchar_bytewidth_ = 18;
    fontchar_height_ = 32;
    fontline_bytewidth_ = keyicons_width / 2;
}
// ----------------------------------------------------------------------------------------


/**
 * Set font noninverted
 * *
 */
void setFontNonInverted()
{
   fontInverted_ = 0;
}
// ----------------------------------------------------------------------------------------


/**
 * Set font inverted
 *
 */
void setFontInverted()
{
   fontInverted_ = 1;
}
// ----------------------------------------------------------------------------------------


/**
 * Invert a few lines in the screenbuffer (i.e. for highlighting an active line)
 *
 * @param startY
 * @param endY
 */
void invertDisplayLines(u8 startY, u8 endY)
{
   unsigned char* sdata = (unsigned char*) screen + startY * 128;

   u8 x, y;
   for (y = startY; y < endY; y++)
   {
      for (x = 0; x < 128; x++)
      {
         u8 first = *sdata >> 4U;
         u8 second = *sdata % 16;

         first = 15-first;
         second = 15-second;
         *sdata = (first << 4U) + second;

         sdata++;
      }
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the given string at the given pixel coordinates
 * output to screen output buffer, the next display() will render it to hardware
 * provides clipping support, coordinates can be offscreen/negative for scrolling fx
 *
 */
void printString(int xPixCoord /* even! */, int yPixCoord, const char *str)
{
   unsigned stringpos = 0;

   while (*str != '\0')
   {
      int c = *str++;
      unsigned x;

      // in-font coordinates
      unsigned f_y = 0;
      unsigned f_x = (c-32) * fontchar_bytewidth_;

      // screenbuf target coordinates
      unsigned s_y = yPixCoord;
      unsigned s_x = xPixCoord / 2 + stringpos * fontchar_bytewidth_; // 2 pixels per byte, require even xPixCoord start coordinate

      while (f_y < fontchar_height_)
      {
         if (s_y >= 0 && s_y < 64) // clip y offscreen
         {
            unsigned char* sdata = (unsigned char*) screen + s_y * 128 + s_x;
            unsigned char* fdata = (unsigned char*) fontptr_ + f_y * fontline_bytewidth_ + f_x;
            unsigned c_s_x = s_x;

            for (x = 0; x <fontchar_bytewidth_; x++)
            {
               if (c_s_x >= 0 && c_s_x < 128)
               {
                  if (!fontInverted_)
                  {
                     if (*fdata)
                        *sdata = *fdata;  // inner loop: copy 2 pixels, if onscreen
                  }
                  else
                  {
                     // "invert" font
                     u8 first = *fdata >> 4;
                     u8 second = *fdata % 16;

                     first = 15-first;
                     second = 15-second;
                     *sdata = (first << 4) + second;
                  }
               }

               c_s_x++;
               fdata++;
               sdata++;
            }
         }

         f_y++;
         s_y++;
      }

      stringpos++;
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the given formatted string at the given pixel coordinates
 * output to screen output buffer, the next display() will render it to hardware
 * provides clipping support, coordinates can be offscreen/negative for scrolling fx
 *
 */
void printFormattedString(int xPixCoord /* even! */, int yPixCoord, const char* format, ...)
{
   char buffer[64];
   va_list args;

   va_start(args, format);
   vsprintf((char *)buffer, format, args);
   return printString(xPixCoord, yPixCoord, buffer);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the current page icon (in the upper right of the screen)
 *
 */
void printPageIcon()
{
   int c = 0;
   switch(page_)
   {
      case PAGE_MUTE:
         c = KEYICON_MUTE;
         break;

      case PAGE_CLIP:
         c = KEYICON_CLIP;
         break;

      case PAGE_NOTES:
         c = KEYICON_NOTES;
         break;

      case PAGE_TRACK:
         c = KEYICON_TRACK;
         break;

      case PAGE_DISK:
         c = KEYICON_DISK;
         break;

      case PAGE_TEMPO:
         c = KEYICON_TEMPO;
         break;

      case PAGE_ARPECHO:
         c = KEYICON_ARPECHO;
         break;

      case PAGE_ROUTER:
         c = KEYICON_ROUTER;
         break;

      case PAGE_MIDIMONITOR:
         c = KEYICON_MIDIMONITOR;
         break;

      case PAGE_SETUP:
         c = KEYICON_SETUP;
         break;

      case PAGE_LIVEFX:
         c = KEYICON_LIVEFX;
         break;
   }

   unsigned x;

   // in-font coordinates
   unsigned f_y = 5;
   unsigned f_x = c * 18 /* fontchar_bytewidth */;

   // screenbuf target coordinates
   unsigned s_y = 0;
   unsigned s_x = 116;

   while (f_y < 17)
   {
      if (s_y >= 0 && s_y < 64) // clip y offscreen
      {
         unsigned char* sdata = (unsigned char*) screen + s_y * 128 + s_x;
         unsigned char* fdata = (unsigned char*) keyicons_pixdata + f_y * (keyicons_width / 2) + f_x + 3;
         unsigned c_s_x = s_x;

         for (x = 0; x < 12; x++)
         {
            if (c_s_x >= 0 && c_s_x < 128)
            {
               if (*fdata)
                  *sdata = *fdata;  // inner loop: copy 2 pixels, if onscreen
            }

            c_s_x++;
            fdata++;
            sdata++;
         }
      }

      f_y++;
      s_y++;
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the given formatted string at the given y pixel coordinates, center x
 * output to screen output buffer, the next display() will render it to hardware
 * provides clipping support, coordinates can be offscreen/negative for scrolling fx
 *
 */
void printCenterFormattedString(int yPixCoord, const char* format, ...)
{
   char buffer[64];
   va_list args;

   va_start(args, format);
   vsprintf((char *)buffer, format, args);

   int xPixCoord = 128 - (fontchar_bytewidth_ * strlen(buffer));
   return printString(xPixCoord, yPixCoord, buffer);
}
// ----------------------------------------------------------------------------------------


/**
 * Display a loopa slot time indicator
 * Format: [clipPos:clipLength]
 *         times are in steps
 *
 *         the time indicator will be rendered inverted, if this is the selected clip/active clip
 *         the display position depends on the slot number, slot #0 is upper left, slot #7 is lower right
 *
 */
void displayClipPosition(u8 clipNumber)
{
   char buffer[16];

   u16 stepLength = clipSteps_[clipNumber][activeScene_];
   u16 stepPos = screenClipStepPosition_[clipNumber];
   u8 isSelected = (clipNumber == screenClipNumberSelected_);

   u8 syncedMuteUnmuteInProgress = trackMuteToggleRequested_[clipNumber] && (tickToStep(tick_) % stepsPerMeasure_) != 0;

   if (!syncedMuteUnmuteInProgress)
   {
      if (stepLength == 0)
         sprintf((char *)buffer, "       ");
      else if (stepLength > 999)
         sprintf((char *)buffer, "%04d>1=", stepPos);
      else if (stepLength > 99)
         sprintf((char *)buffer, "%03d>%3d", stepPos, stepLength);
      else if (stepLength > 9)
         sprintf((char *)buffer, " %02d>%2d ", stepPos, stepLength);
      else
         sprintf((char *)buffer, "  %01d>%1d  ", stepPos, stepLength);
   }
   else
   {
      u8 remainSteps = 16 - (tickToStep(tick_) % stepsPerMeasure_);

      if (remainSteps > 9)
         sprintf((char *)buffer, "  %d   ", remainSteps);
      else
         sprintf((char *)buffer, "   %d   ", remainSteps);
   }

   u8 xPixCoord = clipNumber * 42;
   u8 yPixCoord = 56;
   u8 fontHeight = 7;
   u8 fontByteWidth = 3;
   u8 fontLineByteWidth = 16*3;

   unsigned char *fontptrDigitsTiny;
   switch (gcFontType_)
   {
      case 'a': fontptrDigitsTiny = (unsigned char*) digitstiny_a_pixdata; break;
      default: fontptrDigitsTiny = (unsigned char*) digitstiny_b_pixdata; break;
   }


   char *str = buffer;
   u8 stringpos = 0;
   while (*str != '\0')
   {
      int c = *str++;

      if (c == ' ')  // Map string space to font space
         c = '/';

      unsigned x;

      // in-font coordinates
      unsigned f_y = 0;
      unsigned f_x = (c-47) * fontByteWidth;

      // screenbuf target coordinates
      unsigned s_y = yPixCoord;
      unsigned s_x = xPixCoord / 2 + stringpos * fontByteWidth; // 2 pixels per byte, require even xPixCoord start coordinate

      while (f_y < fontHeight)
      {
         if (s_y >= 0 && s_y < 64) // clip y offscreen
         {
            unsigned char* sdata = (unsigned char*) screen + s_y * 128 + s_x;
            unsigned char* fdata = (unsigned char*) fontptrDigitsTiny + f_y * fontLineByteWidth + f_x;
            unsigned c_s_x = s_x;

            for (x = 0; x <fontByteWidth; x++)
            {
               if (c_s_x >= 0 && c_s_x < 128)
               {
                  if (!isSelected)
                  {
                     if (*fdata)
                        *sdata = *fdata;  // inner loop: copy 2 pixels, if onscreen
                  }
                  else
                  {
                     // "invert" font
                     u8 first = *fdata >> 4;
                     u8 second = *fdata % 16;

                     first = 15-first;
                     second = 15-second;
                     *sdata = (first << 4) + second;
                  }
               }

               c_s_x++;
               fdata++;
               sdata++;
            }
         }

         f_y++;
         s_y++;
      }

      stringpos++;
   }
}
// ----------------------------------------------------------------------------------------


/**
 * If showLogo is true, draw the LoopA Logo (usually during unit startup)
 *
 */
void screenShowLoopaLogo(u8 showLogo)
{
   screenShowLoopaLogo_ = showLogo;
}
// ----------------------------------------------------------------------------------------


/**
 * If showShift is true, draw the shift key overlay
 *
 */
void screenShowShift(u8 showShift)
{
   screenShowShift_ = showShift;
}
// ----------------------------------------------------------------------------------------


/**
 * @return true, if we are currently showing the shift overlay
 *
 */
u8 screenIsInShift()
{
   return screenShowShift_;
}
// ----------------------------------------------------------------------------------------


/**
 * If showMenu is true, draw the menu key overlay
 *
 */
void screenShowMenu(u8 showMenu)
{
    screenShowMenu_ = showMenu;
}
// ----------------------------------------------------------------------------------------


/**
 * @return if we are currently showing the menu
 *
 */
u8 screenIsInMenu()
{
  return screenShowMenu_;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the currently selected clip
 *
 */
void screenSetClipSelected(u8 clipNumber)
{
   screenClipNumberSelected_ = clipNumber;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the position info of a clip
 *
 */
void screenSetClipPosition(u8 clipNumber, u16 stepPosition)
{
   // DEBUG_MSG("[screenSetClipPosition] clip: %u stepPosition: %u", clipNumber, stepPosition);
   screenClipStepPosition_[clipNumber] = stepPosition;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the global song step position (e.g. for displaying the recording-clip step)
 *
 */
void screenSetSongStep(u32 stepPosition)
{
   screenSongStep_ = stepPosition;
}
// ----------------------------------------------------------------------------------------


/**
 * Flash a short-lived message to the center of the screen
 *
 */
void screenFormattedFlashMessage(const char* format, ...)
{
   va_list args;

   va_start(args, format);
   vsprintf((char *)screenFlashMessage_, format, args);
   screenFlashMessageFrameCtr_ = 10;
}
// ----------------------------------------------------------------------------------------


/**
 * Set scene change notification message (change in ticks)
 *
 */
void screenSetSceneChangeInTicks(u8 ticks)
{
   if (ticks)
      sprintf((char *)sceneChangeNotification_, " [...%d]", ticks);
   else
      strcpy(sceneChangeNotification_, "");
}
// ----------------------------------------------------------------------------------------


/**
 * Convert note length to pixel width
 * if ticksLength == 0 (still recording),
 *
 */
u16 noteLengthPixels(u32 ticksLength)
{
   return tickToStep(ticksLength);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the note data of a clip
 *
 */
void displayClip(u8 track)
{
   u16 x;
   u8 y;
   u16 i;
   u16 octmult = 1024/clipSteps_[track][activeScene_];  // factor-8 multiplied horizontal multiplier to make clips as wide as the screen,
                                                        // final calculations need downshifting by 3 bits (divide by 8 in the end)

   u16 curStep = (((u32)boundTickToClipSteps(tick_, track) * octmult) >> 3U) / TICKS_PER_STEP;

   // Render vertical 1/4th note indicators
   for (i=0; i<clipSteps_[track][activeScene_] / 4; i++)
   {
      x = (u16)(i * 4 * octmult) >> 3U;
      if (x < 128)
         for (y=12; y<52; y++)
               screen[y][x] = i % 4 == 0 ? 0x60 : 0x50;
   }

   // Render vertical time indicator line (if seq is playing)
   if (SEQ_BPM_IsRunning() && curStep < 128)
      for (y=0; y<64; y++)
         screen[y][curStep] = 0x88;

   s8 liveTransposeSemi = trackLiveTranspose_[track] ? liveTransposeSemitones_[liveTranspose_ + 7] : 0;

   // Render note data
   for (i=0; i < clipNotesSize_[track][activeScene_]; i++)
   {
      s32 transformedTick = (u32)((s32)quantizeTransform(track, i) * octmult) >> 3U;

      if (transformedTick >= 0) // if note starts within (potentially reconfigured) track length
      {
         u16 step = transformedTick / TICKS_PER_STEP;

         s16 note = clipNotes_[track][activeScene_][i].note + clipTranspose_[track][activeScene_] + liveTransposeSemi;
         note = note < 0 ? 0 : note;
         note = note > 127 ? 127 : note;
         u8 y = (127 - note) / 2;

         if (y < 64)
         {
            u16 len = noteLengthPixels(clipNotes_[track][activeScene_][i].length * octmult) >> 3U;

            if (clipNotes_[track][activeScene_][i].length == 0 && curStep > step)
            {
               // still recording (also check for boundary wrapping, disabled right now)
               len = curStep - step;
            }

            if (step < 128) // Note screen boundary check (notes behind clip length may be present)
            {
               for (x = step; x <= step + len; x++)
               {
                  if (clipNotes_[track][activeScene_][i].velocity > 0)
                  {
                     u8 color;
                     if (!trackMute_[track])
                        color = x == step ? 0xFF
                                          : 0x99;  // The modulo only works if we are not scrolling and screen width = track length
                     else
                        color = x == step ? 0x88
                                          : 0x66;  // The modulo only works if we are not scrolling and screen width = track length

                     screen[y][x % 128] = color;

                     if (page_ == PAGE_NOTES && i == clipActiveNote_[track][activeScene_])
                     {
                        // render cursor for selected note
                        u8 cursorX = x % 128;
                        u8 cursorY = y;

                        if (cursorY > 2 && cursorX < 126)
                           screen[cursorY - 2][cursorX + 2] = 0x0F;

                        if (cursorY > 2 && cursorX > 2)
                           screen[cursorY - 2][cursorX - 2] = 0xF0;

                        if (cursorY < 62 && cursorX < 126)
                           screen[cursorY + 2][cursorX + 2] = 0x0F;

                        if (cursorY < 62 && cursorX > 2)
                           screen[cursorY + 2][cursorX - 2] = 0xF0;
                     }
                  }
               }
            }
         }
      }
   }
}
// ----------------------------------------------------------------------------------------

/**
 * Display centered Track user instrument info, if a user instrument has been chosen (e.g. "DRM1")
 *
 */
void displayTrackInstrumentInfo()
{
   setFontSmall();
   if (isInstrument(trackMidiOutPort_[activeTrack_]))
      printCenterFormattedString(0, "%s", getPortOrInstrumentNameFromLoopAPortNumber(trackMidiOutPort_[activeTrack_]));
   else
      printCenterFormattedString(0, "%s #%d", getPortOrInstrumentNameFromLoopAPortNumber(trackMidiOutPort_[activeTrack_]), trackMidiOutChannel_[activeTrack_] + 1);
}
// ----------------------------------------------------------------------------------------


/**
 * Display upper-left scene-track info (e.g. A1 [mute])
 *
 */
void displaySceneTrackInfo(void)
{
   setFontSmall();
   printFormattedString(0, 0, "%d%c", activeTrack_ + 1, 'A' + activeScene_);

   if (trackMute_[activeTrack_])
      printFormattedString(18, 0, "[mute]%s",  sceneChangeNotification_);
   else
      printFormattedString(18, 0, "%s", sceneChangeNotification_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display context sensitive help text for given screen
 *
 */
void displayHelp(void)
{
   setFontSmall();
   switch(page_)
   {
      case PAGE_MUTE:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The MUTE screen allows for measure-synced");
         printFormattedString(0, 12, "muting and unmuting of the six tracks by ");
         printFormattedString(0, 24, "pushing GP buttons 1 - 6. You can also");
         printFormattedString(0, 36, "directly mute/unmute from everywhere");
         printFormattedString(0, 48, "when pushing/holding the SHIFT button.");
         break;
      case PAGE_CLIP:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The CLIP screen allows to set clip length,");
         printFormattedString(0, 12, "transpose (TRN), scroll steps around (SCR)");
         printFormattedString(0, 24, "and to time-stretch notes (ZOOM).");
         printFormattedString(0, 36, "FREEZEing will reset these transformations");
         printFormattedString(0, 48, "while retaining notes at their positions.");
         break;
      case PAGE_NOTES:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "On the NOTES screen, select the active");
         printFormattedString(0, 12, "note with the SELECT encoder, then you can");
         printFormattedString(0, 24, "move (POS), change the note (e.g. D-2),");
         printFormattedString(0, 36, "velocity (VEL) and length (LEN) of the");
         printFormattedString(0, 48, "active note and also DELETE unwanted notes.");
         break;
      case PAGE_LIVEFX:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The LiveFX screen allows to quantize (QU)");
         printFormattedString(0, 12, "notes, apply swing to quantized notes (SW)");
         printFormattedString(0, 24, "and randomly skip notes (PR).");
         printFormattedString(0, 36, "");
         printFormattedString(0, 48, "More FX will be added :)");
         break;
      case PAGE_TRACK:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "Leftmost options configure OUTPUT MIDI");
         printFormattedString(0, 12, "port/channel/instrument. I: and IC: define");
         printFormattedString(0, 24, "INPUT MIDI ports (or ALL). FWD will live-");
         printFormattedString(0, 36, "forward inputs to output. LTR ON enables");
         printFormattedString(0, 48, "track transposition with the LIVE encoder");
         break;
      case PAGE_TEMPO:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The leftmost option sets the current");
         printFormattedString(0, 12, "BPM of this session. FASTER/SLOWER buttons");
         printFormattedString(0, 24, "linearly increase/decrease BPM.");
         printFormattedString(0, 36, "METR. enables/disables SETTINGS-configured");
         printFormattedString(0, 48, "MIDI metronome output.");
         break;
      case PAGE_DISK:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "SELECT or the SELECT encoder choose the");
         printFormattedString(0, 12, "session number. SAVE will store the memory");
         printFormattedString(0, 24, "session to SD card. LOAD will restore the");
         printFormattedString(0, 36, "selected session number from SD card and");
         printFormattedString(0, 48, "NEW will create a new session in memory.");
         break;
      case PAGE_MIDIMONITOR:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The MIDI monitor shows current interface");
         printFormattedString(0, 12, "activity in the top two lines and displays");
         printFormattedString(0, 24, "a log of inbound and outbound MIDI packets");
         printFormattedString(0, 36, "with an abbreviated packet header hex dump");
         printFormattedString(0, 48, "and the packet timestamp since LoopA start.");
         break;
      case PAGE_ROUTER:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "The MIDI router allows to set up permanent");
         printFormattedString(0, 12, "routes from MIDI inputs (IN P and IN Ch)");
         printFormattedString(0, 24, "to MIDI outputs (OUT P and OUT Ch).");
         printFormattedString(0, 36, "You can use the SELECT encoder to scroll");
         printFormattedString(0, 48, "and select the active route.");
         break;
      case PAGE_SETUP:
         //                          "------------------------------------------"
         printFormattedString(0,  0, "In SETUP you can configure permanent");
         printFormattedString(0, 12, "runtime parameters of your LoopA, like the");
         printFormattedString(0, 24, "system font and metronome settings.");
         printFormattedString(0, 36, "You can use the SELECT encoder to scroll");
         printFormattedString(0, 48, "and select the configuration entry.");
         break;
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the normal loopa view (PAGE_TRACK)
 *
 */
void displayPageMute(void)
{
   setFontSmall();

   displaySceneTrackInfo();
   displayTrackInstrumentInfo();

   u8 clip;
   for (clip = 0; clip < TRACKS; clip++)
   {
      if (clip == activeTrack_ || clipNotesSize_[clip] > 0 || trackMuteToggleRequested_[clip])
         displayClipPosition(clip);  // only display clip position indicators, if it is the active clip or it has notes
   }

   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the edit clip page (PAGE_EDIT)
 *
 */
void displayPageClip(void)
{
   setFontSmall();

   displaySceneTrackInfo();
   displayTrackInstrumentInfo();

   command_ == COMMAND_CLIP_LEN ? setFontInverted() : setFontNonInverted();
   if (clipSteps_[activeTrack_][activeScene_] < 100)
      printFormattedString(0, 53, "Len %d", clipSteps_[activeTrack_][activeScene_]);
   else if (clipSteps_[activeTrack_][activeScene_] < 1000)
      printFormattedString(0, 53, "Le %d", clipSteps_[activeTrack_][activeScene_]);
   else
      printFormattedString(0, 53, "L %d", clipSteps_[activeTrack_][activeScene_]);

   command_ == COMMAND_CLIP_TRANSPOSE ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, "Trn %d", clipTranspose_[activeTrack_][activeScene_]);

   command_ == COMMAND_CLIP_SCROLL ? setFontInverted() : setFontNonInverted();
   printFormattedString(126, 53, "Scr %d", clipScroll_[activeTrack_][activeScene_]);

   command_ == COMMAND_CLIP_STRETCH ? setFontInverted() : setFontNonInverted();
   switch (clipStretch_[activeTrack_][activeScene_])
   {
      case 1: printFormattedString(168, 53, "Zo 1/16"); break;
      case 2: printFormattedString(168, 53, "Zo 1/8"); break;
      case 4: printFormattedString(168, 53, "Zo 1/4"); break;
      case 8: printFormattedString(168, 53, "Zo 1/2"); break;
      case 16: printFormattedString(168, 53, "Zoom 1"); break;
      case 32: printFormattedString(168, 53, "Zoom 2"); break;
      case 64: printFormattedString(168, 53, "Zoom 4"); break;
      case 128: printFormattedString(168, 53, "Zoom 8"); break;
   }

   command_ == COMMAND_CLIP_FREEZE ? setFontInverted() : setFontNonInverted();
   printFormattedString(210, 53, "Freeze");

   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Numeric note to note string helper
 *
 */
static void stringNote(char *label, u8 note)
{
   const char noteTab[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

   // print "---" if note number is 0
   if (note == 0)
      sprintf(label, "---  ");
   else
   {
      u8 octave = note / 12;
      note %= 12;

      // print semitone and octave (-2): up to 4 chars
      sprintf(label, "%s%d  ",
              noteTab[note],
              (int)octave-2);
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the note editor (PAGE_NOTES)
 *
 */
void displayPageNotes(void)
{
   setFontSmall();

   displaySceneTrackInfo();
   displayTrackInstrumentInfo();

   if (clipNotesSize_[activeTrack_][activeScene_] > 0)
   {

      u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

      if (activeNote >= clipNotesSize_[activeTrack_][activeScene_]) // necessary e.g. for clip change
         activeNote = 0;

      u16 pos = (clipNotes_[activeTrack_][activeScene_][activeNote].tick) / TICKS_PER_STEP;
      u16 length = clipNotes_[activeTrack_][activeScene_][activeNote].length;
      u8 note = clipNotes_[activeTrack_][activeScene_][activeNote].note;
      u8 velocity = clipNotes_[activeTrack_][activeScene_][activeNote].velocity;

      command_ == COMMAND_NOTE_POSITION ? setFontInverted() : setFontNonInverted();
      if (pos < 100)
         printFormattedString(0, 53, "Pos %d", pos);
      else
         printFormattedString(0, 53, "Po %d", pos);

      command_ == COMMAND_NOTE_KEY ? setFontInverted() : setFontNonInverted();

      char noteStr[8];
      stringNote(noteStr, note);
      printFormattedString(42, 53, "%s", noteStr);

      command_ == COMMAND_NOTE_VELOCITY ? setFontInverted() : setFontNonInverted();
      printFormattedString(84, 53, "Vel %d", velocity);

      command_ == COMMAND_NOTE_LENGTH ? setFontInverted() : setFontNonInverted();
      printFormattedString(126, 53, "Len %d", length);

      command_ == COMMAND_CLIP_FREEZE ? setFontInverted() : setFontNonInverted();
      printFormattedString(210, 53, "Delete");

      setFontNonInverted();
   }
   else
   {
      printCenterFormattedString(54, "No notes recorded, yet");
   }
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the track settings page (PAGE_TRACK)
 *
 */
void displayPageTrack(void)
{
   setFontSmall();
   printFormattedString(0, 0, "Track %d", activeTrack_ + 1);
   displayTrackInstrumentInfo();

   command_ == COMMAND_TRACK_OUTPORT ? setFontInverted() : setFontNonInverted();

   printFormattedString(0, 53, " %s ", getPortOrInstrumentNameFromLoopAPortNumber(trackMidiOutPort_[activeTrack_]));

   if (!isInstrument(trackMidiOutPort_[activeTrack_]))
   {
      // Also print MIDI channel, if we are not showing a user instrument
      command_ == COMMAND_TRACK_OUTCHANNEL ? setFontInverted() : setFontNonInverted();
      printFormattedString(42, 53, "Chn %d", trackMidiOutChannel_[activeTrack_] + 1);
   }

   command_ == COMMAND_TRACK_INPORT ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, "I:%s", trackMidiInPort_[activeTrack_] == 0 ? "All" : MIDI_PORT_InNameGet(MIDI_PORT_InIxGet(trackMidiInPort_[activeTrack_])));

   command_ == COMMAND_TRACK_INCHANNEL ? setFontInverted() : setFontNonInverted();
   if (trackMidiInChannel_[activeTrack_] == 16)
      printFormattedString(126, 53, "IC:All");
   else
      printFormattedString(126, 53, "IC: %d", trackMidiInChannel_[activeTrack_] + 1);

   command_ == COMMAND_TRACK_TOGGLE_FORWARD ? setFontInverted() : setFontNonInverted();
   if (trackMidiForward_[activeTrack_])
      printFormattedString(168, 53, "Fwd On");
   else
      printFormattedString(168, 53, "Fw Off");

   command_ == COMMAND_TRACK_LIVE_TRANSPOSE ? setFontInverted() : setFontNonInverted();
   if (trackLiveTranspose_[activeTrack_])
      printFormattedString(210, 53, "LTr On");
   else
      printFormattedString(210, 53, "LTr Off");


   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the disk operations page (PAGE_DISK)
 *
 */
void displayPageDisk(void)
{
   setFontSmall();

   printCenterFormattedString(0, "Disk Operations");

   command_ == COMMAND_DISK_SELECT_SESSION ? setFontInverted() : setFontNonInverted();
   printFormattedString(0, 53, "Select");

   command_ == COMMAND_DISK_SAVE ? setFontInverted() : setFontNonInverted();
   printFormattedString(42, 53, "Save");

   command_ == COMMAND_DISK_LOAD ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, "Load");

   command_ == COMMAND_DISK_NEW ? setFontInverted() : setFontNonInverted();
   printFormattedString(126, 53, "New");


   setFontNonInverted();
   setFontBold();
   printFormattedString(82, 16, "Session %d", sessionNumber_);

   setFontNormal();
   if (sessionExistsOnDisk_)
      printFormattedString(82, 32, "(on disk)", sessionNumber_);
   else
      printFormattedString(106, 32, "(new)", sessionNumber_);

   setFontSmall();
}
// ----------------------------------------------------------------------------------------


/**
 * Display the tempo settings page (PAGE_TEMPO)
 *
 */
void displayPageTempo(void)
{
   setFontSmall();

   printCenterFormattedString(0, "Tempo Settings", activeTrack_ + 1, activeScene_ + 1);

   command_ == COMMAND_TEMPO_BPM ? setFontInverted() : setFontNonInverted();
   float bpm = SEQ_BPM_IsMaster() ? bpm_ : SEQ_BPM_Get();
   printFormattedString(0, 53, "%3d.%d", (int)bpm, (int)(10*bpm)%10);

   tempoFade_ == 1 ? setFontInverted() : setFontNonInverted();
   printFormattedString(42, 53, "Faster");

   tempoFade_ == -1 ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, "Slower");

   command_ == COMMAND_TEMPO_TOGGLE_METRONOME ? setFontInverted : setFontNonInverted();
   printFormattedString(126, 53, metronomeEnabled_ ? "Metron. On" : "Metr. Off");

   if (tempoFade_ != 0)
   {
      if (tempoFade_ == 1)
         bpm_ += tempodeltaDescriptions_[gcTempodeltaType_].speed;

      if (tempoFade_ == -1)
         bpm_ -= tempodeltaDescriptions_[gcTempodeltaType_].speed;

      if (bpm_ < 30)
         bpm_ = 30;
      if (bpm_ > 300)
         bpm_ = 300;
      SEQ_BPM_Set(bpm_);
   }

   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------

/**
 * Display the MIDI router settings page (PAGE_ROUTER)
 *
 */
void displayPageRouter(void)
{
   setFontSmall();

   // print routes
   s8 i;
   s8 idx = (s8)(routerActiveRoute_ > 0 ? routerActiveRoute_ -1 : 0);
   for (i = -1; i < MIDI_ROUTER_NUM_NODES; i++)
   {
      if (i == idx)
      {
         u8 y = (u8) ((i - routerActiveRoute_) * 12 + 16);
         if (y <= 40)
         {
            midi_router_node_entry_t *n = &midi_router_node[i];
            printFormattedString(0, y, "#%d", i + 1);

            if (i == routerActiveRoute_ && command_ == COMMAND_ROUTE_IN_PORT)
               setFontInverted();
            char *port = MIDI_PORT_InNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->src_port));
            printFormattedString(42, y, "%s", port);
            setFontNonInverted();

            if (i == routerActiveRoute_ && command_ == COMMAND_ROUTE_IN_CHANNEL)
               setFontInverted();
            u8 chn = n->src_chn;
            if (chn > 0 && chn < 17)
               printFormattedString(84, y, "%d", chn);
            else
               printFormattedString(84, y, "%s", chn == 17 ? "All" : "---");
            setFontNonInverted();

            if (i == routerActiveRoute_ && command_ == COMMAND_ROUTE_OUT_PORT)
               setFontInverted();
            port = MIDI_PORT_OutNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->dst_port));
            printFormattedString(126, y, "%s", port);
            setFontNonInverted();

            if (i == routerActiveRoute_ && command_ == COMMAND_ROUTE_OUT_CHANNEL)
               setFontInverted();
            chn = n->dst_chn;
            if (chn > 0 && chn < 17)
               printFormattedString(168, y, "%d", chn);
            else
               printFormattedString(168, y, "%s", chn == 17 ? "All" : "---");
            setFontNonInverted();
         }

         idx++;
      }
   }

   invertDisplayLines(15, 29);

   command_ == COMMAND_ROUTE_SELECT ? setFontInverted() : setFontNonInverted();
   printFormattedString(0, 53, "Select");
   command_ == COMMAND_ROUTE_IN_PORT ? setFontInverted() : setFontNonInverted();
   printFormattedString(42, 53, "IN P");
   command_ == COMMAND_ROUTE_IN_CHANNEL ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, "IN Ch");
   command_ == COMMAND_ROUTE_OUT_PORT ? setFontInverted() : setFontNonInverted();
   printFormattedString(126, 53, "OUT P");
   command_ == COMMAND_ROUTE_OUT_CHANNEL ? setFontInverted() : setFontNonInverted();
   printFormattedString(168, 53, "OUT Ch");
   setFontNonInverted();

}
// ----------------------------------------------------------------------------------------


/**
 * Display the setup/settings page (PAGE_SETUP)
 *
 */
void displayPageSetup(void)
{
   setFontSmall();

   // print config settings
   s8 i;
   s8 idx = (s8)(setupActiveItem_ > 0 ? setupActiveItem_ - 1 : 0);
   mios32_midi_port_t midiPort;
   s32 status;
   char noteStr[8];

   for (i = -1; i < SETUP_NUM_ITEMS; i++)
   {
      /// DEBUG_MSG("[displayPageSetup] i %d idx %d\n", i, idx);
      if (i == idx)
      {
         u8 y = (u8) ((i - setupActiveItem_) * 12 + 16);
         if (y <= 40)
         {
            printFormattedString(0, y, "%s", setupParameters_[i].name);

            // Parameter 1 column
            if (i == setupActiveItem_ && command_ == COMMAND_SETUP_PAR1)
               setFontInverted();
            switch(i)
            {
               case SETUP_MCLK_DIN_IN:
                  midiPort = UART0;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(84, y, (status > 0) ? "IN1" : " - ");
                  break;

               case SETUP_MCLK_DIN_OUT:
                  midiPort = UART0;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(84, y, (status > 0) ? "OUT1" : " - ");
                  break;

               case SETUP_MCLK_USB_IN:
                  midiPort = USB0;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(84, y, (status > 0) ? "USB1" : " - ");
                  break;

               case SETUP_MCLK_USB_OUT:
                  midiPort = USB0;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(84, y, (status > 0) ? "USB1" : " - ");
                  break;

               case SETUP_FONT_TYPE:
                  switch (gcFontType_)
                  {
                     case 'a': printFormattedString(84, y, "a - sharp");
                        break;
                     case 'b': printFormattedString(84, y, "b - smooth");
                        break;
                  }

                  break;

               case SETUP_BEAT_LEDS_ENABLED:
                  printFormattedString(84, y, gcBeatLEDsEnabled_ ? "On" : "Off");
                  break;

               case SETUP_BEAT_DISPLAY_ENABLED:
                  printFormattedString(84, y, gcBeatDisplayEnabled_ ? "On" : "Off");
                  break;

               case SETUP_METRONOME:
                  printFormattedString(84, y, "%s", getPortOrInstrumentNameFromLoopAPortNumber(gcMetronomePort_));
                  break;

               case SETUP_TEMPODELTA:
                  printFormattedString(84, y, "%s", tempodeltaDescriptions_[gcTempodeltaType_].name);
                  break;

               case SETUP_SCREENSAVER_MINUTES:
                  printFormattedString(84, y, "%d Min", gcScreensaverAfterMinutes_);
                  break;

               case SETUP_INVERT_OLED:
                  printFormattedString(84, y, gcInvertOLED_ ? "On" : "Off");
                  break;

               case SETUP_INVERT_FOOTSWITCHES:
                  printFormattedString(84, y, (gcFootswitchesInversionmask_ & 0x01) ? "S1 Inv" : "S1 Nor");
                  break;

               case SETUP_FOOTSWITCH1_ACTION:
                  printFormattedString(84, y, "%s", footswitchActionDescriptions_[gcFootswitch1Action_].name);
                  break;

               case SETUP_FOOTSWITCH2_ACTION:
                  printFormattedString(84, y, "%s", footswitchActionDescriptions_[gcFootswitch2Action_].name);
                  break;

               case SETUP_INVERT_MUTE_LEDS:
                  printFormattedString(84, y, gcInvertMuteLEDs_ ? "On" : "Off");
                  break;

               case SETUP_TRACK_SWITCH_TYPE:
                  printFormattedString(84, y, "%s", trackswitchDescriptions_[gcTrackswitchType_].name);
                  break;

               case SETUP_FOLLOW_TRACK_TYPE:
                  printFormattedString(84, y, "%s", followtrackDescriptions_[gcFollowtrackType_].name);
                  break;

               case SETUP_LED_NOTES:
                  printFormattedString(84, y, gcLEDNotes_ ? "On (shown in MUTE screen)" : "Off");
                  break;


            }
            setFontNonInverted();

            // Parameter 2 column
            if (i == setupActiveItem_ && command_ == COMMAND_SETUP_PAR2)
               setFontInverted();
            switch(i)
            {
               case SETUP_MCLK_DIN_IN:
                  midiPort = UART1;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(126, y, (status > 0) ? "IN2" : " - ");
                  break;

               case SETUP_MCLK_DIN_OUT:
                  midiPort = UART1;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(126, y, (status > 0) ? "OUT2" : " - ");
                  break;

               case SETUP_MCLK_USB_IN:
                  midiPort = USB1;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(126, y, (status > 0) ? "USB2" : " - ");
                  break;

               case SETUP_MCLK_USB_OUT:
                  midiPort = USB1;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(126, y, (status > 0) ? "USB2" : " - ");
                  break;

               case SETUP_METRONOME:
                  if (!isInstrument(gcMetronomePort_))
                  {
                     // Print metronome MIDI channel, if we are not showing a metronome user instrument
                     printFormattedString(126, y, "Chn %d", gcMetronomeChannel_ + 1);
                  }
                  break;

               case SETUP_INVERT_FOOTSWITCHES:
                  printFormattedString(126, y, (gcFootswitchesInversionmask_ & 0x02) ? "S2 Inv" : "S2 Nor");
                  break;
            }
            setFontNonInverted();

            // Parameter 3 column
            if (i == setupActiveItem_ && command_ == COMMAND_SETUP_PAR3)
               setFontInverted();
            switch(i)
            {
               case SETUP_MCLK_DIN_IN:
                  midiPort = UART2;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(168, y, (status > 0) ? "IN3" : " - ");
                  break;

               case SETUP_MCLK_DIN_OUT:
                  midiPort = UART2;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(168, y, (status > 0) ? "OUT3" : " - ");
                  break;

               case SETUP_MCLK_USB_IN:
                  midiPort = USB2;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(168, y, (status > 0) ? "USB3" : " - ");
                  break;

               case SETUP_MCLK_USB_OUT:
                  midiPort = USB2;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(168, y, (status > 0) ? "USB3" : " - ");
                  break;

               case SETUP_METRONOME:
                  stringNote(noteStr, gcMetronomeNoteM_);
                  printFormattedString(168, y, "%s", noteStr);
                  break;
            }
            setFontNonInverted();

            // Parameter 4 column
            if (i == setupActiveItem_ && command_ == COMMAND_SETUP_PAR4)
               setFontInverted();
            switch(i)
            {
               case SETUP_MCLK_DIN_IN:
                  midiPort = UART3;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(210, y, (status > 0) ? "IN4" : " - ");
                  break;

               case SETUP_MCLK_DIN_OUT:
                  midiPort = UART3;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(210, y, (status > 0) ? "OUT4" : " - ");
                  break;

               case SETUP_MCLK_USB_IN:
                  midiPort = USB3;
                  status = MIDI_ROUTER_MIDIClockInGet(midiPort);
                  printFormattedString(210, y, (status > 0) ? "USB4" : " - ");
                  break;

               case SETUP_MCLK_USB_OUT:
                  midiPort = USB3;
                  status = MIDI_ROUTER_MIDIClockOutGet(midiPort);
                  printFormattedString(210, y, (status > 0) ? "USB4" : " - ");
                  break;

               case SETUP_METRONOME:
                  stringNote(noteStr, gcMetronomeNoteB_);
                  printFormattedString(210, y, "%s", noteStr);
            }
            setFontNonInverted();

         }

         idx++;
      }
   }

   invertDisplayLines(15, 29);

   command_ == COMMAND_SETUP_SELECT ? setFontInverted() : setFontNonInverted();
   printFormattedString(0, 53, "Select");

   command_ == COMMAND_SETUP_PAR1 ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 53, setupParameters_[setupActiveItem_].par1Name);
   command_ == COMMAND_SETUP_PAR2 ? setFontInverted() : setFontNonInverted();
   printFormattedString(126, 53, setupParameters_[setupActiveItem_].par2Name);
   command_ == COMMAND_SETUP_PAR3 ? setFontInverted() : setFontNonInverted();
   printFormattedString(168, 53, setupParameters_[setupActiveItem_].par3Name);
   command_ == COMMAND_SETUP_PAR4 ? setFontInverted() : setFontNonInverted();
   printFormattedString(210, 53, setupParameters_[setupActiveItem_].par4Name);
   setFontNonInverted();
}
// ----------------------------------------------------------------------------------------


/**
 * Print a MIDI event in a 5-character slot of the MIDI monitor page
 *
 */
void MIDIMonitorPrintEvent(u8 x, u8 y, mios32_midi_package_t package, u8 num_chars)
{
   // currently only 5 chars supported...
   if (package.type == 0xf || package.evnt0 >= 0xf8)
   {
      switch (package.evnt0)
      {
         case 0xf8:
            printFormattedString(x, y, " CLK ");
            break;
         case 0xfa:
            printFormattedString(x, y, "START");
            break;
         case 0xfb:
            printFormattedString(x, y, "CONT.");
            break;
         case 0xfc:
            printFormattedString(x, y, "STOP ");
            break;
         default:
            printFormattedString(x, y, " %02X  ", package.evnt0);
      }
   }
   else if (package.type < 8)
   {
      printFormattedString(x, y, "SysEx");
   }
   else
   {
      switch (package.event)
      {
         case NoteOff:
         case NoteOn:
         case PolyPressure:
         case CC:
         case ProgramChange:
         case Aftertouch:
         case PitchBend:
            // could be enhanced later
            printFormattedString(x, y, "%02X%02X ", package.evnt0, package.evnt1);
            break;

         default:
            // print first two bytes for unsupported events
            printFormattedString(x, y, "%02X%02X ", package.evnt0, package.evnt1);
      }
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Add a new MIDI input/output log line, but optimize: store data only,
 * if we are viewing the MIDI Monitor page
 *
 */
static u8 currentMIDIMonitorLogLine_ = 0;
static u32 MIDIMonitorTimeLine_[3];
static u8 MIDIMonitorLogInputFlagLine_[3];
static mios32_midi_port_t MIDIMonitorPortLine_[3];
static mios32_midi_package_t MIDIMonitorPackageLine_[3];
void MIDIMonitorAddLog(u8 inputFlag, mios32_midi_port_t port, mios32_midi_package_t package)
{
   if (page_ == PAGE_MIDIMONITOR)
   {
      if (package.evnt0 != 0xf8) // Filter MIDI clock events
      {
         if (currentMIDIMonitorLogLine_ == 2)
         {
            // move up log
            MIDIMonitorLogInputFlagLine_[0] = MIDIMonitorLogInputFlagLine_[1];
            MIDIMonitorLogInputFlagLine_[1] = MIDIMonitorLogInputFlagLine_[2];
            MIDIMonitorPortLine_[0] = MIDIMonitorPortLine_[1];
            MIDIMonitorPortLine_[1] = MIDIMonitorPortLine_[2];
            MIDIMonitorPackageLine_[0] = MIDIMonitorPackageLine_[1];
            MIDIMonitorPackageLine_[1] = MIDIMonitorPackageLine_[2];
            MIDIMonitorTimeLine_[0] = MIDIMonitorTimeLine_[1];
            MIDIMonitorTimeLine_[1] = MIDIMonitorTimeLine_[2];
         }

         MIDIMonitorLogInputFlagLine_[currentMIDIMonitorLogLine_] = inputFlag;
         MIDIMonitorPortLine_[currentMIDIMonitorLogLine_] = port;
         MIDIMonitorPackageLine_[currentMIDIMonitorLogLine_] = package;
         MIDIMonitorTimeLine_[currentMIDIMonitorLogLine_] = millisecondsSinceStartup_;

         if (currentMIDIMonitorLogLine_ < 2)
            currentMIDIMonitorLogLine_++;
      }
   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the MIDI monitor page
 *
 */
void displayPageMIDIMonitor()
{
   u8 i; // port index / monitor log line index
   u8 y = 0;

   setFontSmall();
   setFontNonInverted();

   // Output realtime port overview
   for (i = 1; i <= 8; ++i)
   {
      u8 port_ix_midi_in = (i >= 9) ? (i - 4) : i;

      u8 x = (i - 1) * 5 * 6;
      y = 0;

      mios32_midi_port_t port = MIDI_PORT_InPortGet(port_ix_midi_in);
      mios32_midi_package_t package = MIDI_PORT_InPackageGet(port);

      if (port == 0xff)
      {
         printFormattedString(x, y, "     ");
      }
      else if (package.type)
      {
         setFontInverted();
         MIDIMonitorPrintEvent(x, y, package, 5);
         setFontNonInverted();
      }
      else
      {
         printFormattedString(x, y, MIDI_PORT_InNameGet(port_ix_midi_in));
      }

      port = MIDI_PORT_OutPortGet(i);
      package = MIDI_PORT_OutPackageGet(port);

      y = 12;

      if (port == 0xff)
      {
         printFormattedString(x, y, "     ");
      }
      else if (package.type)
      {
         setFontInverted();
         MIDIMonitorPrintEvent(x, y, package, 5);
         setFontNonInverted();
      }
      else
      {
         printFormattedString(x, y, MIDI_PORT_OutNameGet(i));
      }
   }

   // Output high-speed last monitor log
   // like this 31345000: -> USB0 0 2 2 30 20 30
   for (i = 0; i <= currentMIDIMonitorLogLine_; i++)
   {
      y = 2 + (i + 2) * 12;
      if (MIDIMonitorLogInputFlagLine_[i])
      {
         printFormattedString(0, y, "%d -> %s %02x %02x %02x %02x %02x %02x",
                              MIDIMonitorTimeLine_[i],
                              MIDI_PORT_InNameGet(MIDI_PORT_InIxGet(MIDIMonitorPortLine_[i])),
                              MIDIMonitorPackageLine_[i].type,
                              MIDIMonitorPackageLine_[i].cable,
                              MIDIMonitorPackageLine_[i].chn,
                              MIDIMonitorPackageLine_[i].event,
                              MIDIMonitorPackageLine_[i].value1,
                              MIDIMonitorPackageLine_[i].value2);
      }
      else
      {
         printFormattedString(0, y, "%d %s -> %02x %02x %02x %02x %02x %02x",
                              MIDIMonitorTimeLine_[i],
                              MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(MIDIMonitorPortLine_[i])),
                              MIDIMonitorPackageLine_[i].type,
                              MIDIMonitorPackageLine_[i].cable,
                              MIDIMonitorPackageLine_[i].chn,
                              MIDIMonitorPackageLine_[i].event,
                              MIDIMonitorPackageLine_[i].value1,
                              MIDIMonitorPackageLine_[i].value2);
      }

   }
}
// ----------------------------------------------------------------------------------------


/**
 * Display the song page
 *
 */
void displayPageSong()
{
   // Todo
}
// ----------------------------------------------------------------------------------------


/**
 * Display the Live FX page
 *
 */
void displayPageLiveFX()
{
   displaySceneTrackInfo();
   displayTrackInstrumentInfo();

   command_ == COMMAND_LIVEFX_QUANTIZE ? setFontInverted() : setFontNonInverted();
   switch (clipFxQuantize_[activeTrack_][activeScene_])
   {
      case 3: printFormattedString(0, 53, "Q1/128"); break;
      case 6: printFormattedString(0, 53, "Qu1/64"); break;
      case 12: printFormattedString(0, 53, "Qu1/32"); break;
      case 24: printFormattedString(0, 53, "Qu1/16"); break;
      case 48: printFormattedString(0, 53, "Qu 1/8"); break;
      case 96: printFormattedString(0, 53, "Qu 1/4"); break;
      case 192: printFormattedString(0, 53, "Qu 1/2"); break;
      case 384: printFormattedString(0, 53, "Qu 1/1"); break;
      default: printFormattedString(0, 53, "Qu OFF"); break;
   }

   command_ == COMMAND_LIVEFX_SWING ? setFontInverted() : setFontNonInverted();
   if (clipFxSwing_[activeTrack_][activeScene_] != 50)
      printFormattedString(42, 53, "Sw %d%%", clipFxSwing_[activeTrack_][activeScene_]);
   else
      printFormattedString(42, 53, "Swing%%");

   command_ == COMMAND_LIVEFX_PROBABILITY ? setFontInverted() : setFontNonInverted();
   if (clipFxProbability_[activeTrack_][activeScene_])
      printFormattedString(84, 53, "Pr %d%%", clipFxProbability_[activeTrack_][activeScene_]);
   else
      printFormattedString(84, 53, "Prob %%");

   /*
   command_ == COMMAND_LIVEFX_FTS_MODE ? setFontInverted() : setFontNonInverted();
   if (clipFxFTSMode_[activeTrack_][activeScene_])
      printFormattedString(126, 53, "FTS %d", clipFxFTSMode_[activeTrack_][activeScene_]);
   else
      printFormattedString(126, 53, "FTS off");

   command_ == COMMAND_LIVEFX_FTS_NOTE ? setFontInverted() : setFontNonInverted();
   if (clipFxFTSNote_[activeTrack_][activeScene_])
      printFormattedString(168, 53, "FNte %d", clipFxFTSNote_[activeTrack_][activeScene_]);
   */

   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Return, if screensaver is active
 * @return int 1, if screensaver should be displayed
 *
 */
int isScreensaverActive()
{
   return inactivitySeconds_ > gcScreensaverAfterMinutes_ * 60;
}
// ----------------------------------------------------------------------------------------


/**
 * Display the current screen buffer (once per frame, called in app.c scheduler)
 *
 */
static u32 frameCounter_ = 0;
void display()
{
   u8 i, j;

   frameCounter_++;

   if (isScreensaverActive() && hw_enabled != HARDWARE_LOOPA_TESTMODE)
   {
      voxelFrame();
   }
   else if (screenShowLoopaLogo_ || showShiftAbout_)
   {
      // Startup/initial session loading: Render the LoopA Logo
      voxelFrame();

      if (hw_enabled == HARDWARE_STARTUP || hw_enabled == HARDWARE_LOOPA_OPERATIONAL)
      {
         setFontLoopALogo();
         printFormattedString(52, 2, " ");

         setFontBold();  // width per letter: 10px (for center calculation)
         printFormattedString(146, 2, VERSION);

         setFontSmall(); // width per letter: 6px
         printFormattedString(28, 20, MIOS32_LCD_BOOT_MSG_LINE2);
         printFormattedString(52, 32, "MIDIbox hardware platform");

         setFontBold(); // width per letter: 10px;
         printFormattedString(52, 44, "www.midiphy.com");
      }
      else
      {
         // Testmode
         setFontSmall();
         printFormattedString(0, 40, "LoopA Testmode");
         printFormattedString(0, 52, "Insert FAT32-formatted SD card to launch");
      }
   }
   else if (screenIsInMenu())
   {
      voxelFrame();
      setFontKeyIcon();
      // setFontDarkGreyAsBlack(); // TODO

      int iconId;

      /* iconId = (page_ == PAGE_SONG) ? 32 + KEYICON_SONG_INVERTED : 32 + KEYICON_SONG;
      printFormattedString(0 * 36 + 18, 0, "%c", iconId);
      */

      iconId = (page_ == PAGE_MIDIMONITOR) ? 32 + KEYICON_MIDIMONITOR_INVERTED : 32 + KEYICON_MIDIMONITOR;
      printFormattedString(1 * 36 + 18, 0, "%c", iconId);

      iconId = (page_ == PAGE_TEMPO) ? 32 + KEYICON_TEMPO_INVERTED : 32 + KEYICON_TEMPO;
      printFormattedString(2 * 36 + 18, 0, "%c", iconId);

      iconId = (page_ == PAGE_MUTE) ? 32 + KEYICON_MUTE_INVERTED : 32 + KEYICON_MUTE;
      printFormattedString(3 * 36 + 18, 0, "%c", iconId);

      iconId = (page_ == PAGE_NOTES) ? 32 + KEYICON_NOTES_INVERTED : 32 + KEYICON_NOTES;
      printFormattedString(4 * 36 + 18, 0, "%c", iconId);

      /*iconId = (page_ == PAGE_ARPECHO) ? 32 + KEYICON_ARPECHO_INVERTED : 32 + KEYICON_ARPECHO;
      printFormattedString(5 * 36 + 18, 0, "%c", iconId);
      */

      iconId = (page_ == PAGE_SETUP) ? 32 + KEYICON_SETUP_INVERTED : 32 + KEYICON_SETUP;
      printFormattedString(0 * 36, 32, "%c", iconId);

      iconId = (page_ == PAGE_ROUTER) ? 32 + KEYICON_ROUTER_INVERTED : 32 + KEYICON_ROUTER;
      printFormattedString(1 * 36, 32, "%c", iconId);

      iconId = (page_ == PAGE_DISK) ? 32 + KEYICON_DISK_INVERTED : 32 + KEYICON_DISK;
      printFormattedString(2 * 36, 32, "%c", iconId);

      iconId = 32 + KEYICON_MENU_INVERTED;
      printFormattedString(3 * 36, 32, "%c", iconId);

      iconId = (page_ == PAGE_CLIP) ? 32 + KEYICON_CLIP_INVERTED : 32 + KEYICON_CLIP;
      printFormattedString(4 * 36, 32, "%c", iconId);

      iconId = (page_ == PAGE_LIVEFX) ? 32 + KEYICON_LIVEFX_INVERTED : 32 + KEYICON_LIVEFX;
      printFormattedString(5 * 36, 32, "%c", iconId);

      iconId = (page_ == PAGE_TRACK) ? 32 + KEYICON_TRACK_INVERTED : 32 + KEYICON_TRACK;
      printFormattedString(6 * 36, 32, "%c", iconId);

      setFontBold();
   }
   else if (showShiftHelp_)
   {
      displayHelp();
   }
   else if (screenIsInShift())
   {
      voxelFrame();
      s8 flashMuteToggle = (frameCounter_ % 2) == 0; // When SEQ is running, flash active synced mute/unmute toggles until performed
      if (!SEQ_BPM_IsRunning())
         flashMuteToggle = 0; // When SEQ is not running, no synced mute/unmute toggles

      u8 iconId;
      u8 invert;
      s8 track;

      for (track = 0; track < TRACKS; track++)
      {
         // Determine inversion/blinking
         invert = (trackMuteToggleRequested_[track] && flashMuteToggle) ? (trackMute_[track] ? 1 : 0) : (trackMute_[track] ? 0 : 1);

         // Render base icon
         iconId = (trackMute_[track] ? (32 + KEYICON_TRACKUNMUTE) : (32 + KEYICON_TRACKMUTE)) -
                  invert; // in case of inversion subtracts 1 and switches to inverted icons
         setFontKeyIcon();
         setFontNonInverted();
         printFormattedString(track * 36 + 18, 0, "%c", iconId);

         // Render clip info (e.g. 1A, 2B...)
         setFontSmall();
         if (invert)
            setFontInverted();
         printFormattedString(track * 36 + 30, 7, "%d%c", track + 1, 'A' + activeScene_);
      }
      setFontNonInverted();

      setFontKeyIcon();
      iconId = 32 + KEYICON_ABOUT;
      printFormattedString(0 * 36, 32, "%c", iconId);

      iconId = 32 + KEYICON_HELP;
      printFormattedString(1 * 36, 32, "%c", iconId);

      iconId = 32 + KEYICON_SHIFT_INVERTED;
      printFormattedString(2 * 36, 32, "%c", iconId);

      setFontBold();
   }
   else
   {
      // Display page content...
      switch (page_)
      {
         case PAGE_MUTE:
            displayPageMute();
            break;

         case PAGE_CLIP:
            displayPageClip();
            break;

         case PAGE_NOTES:
            displayPageNotes();
            break;

         case PAGE_TRACK:
            displayPageTrack();
            break;

         case PAGE_DISK:
            displayPageDisk();
            break;

         case PAGE_TEMPO:
            displayPageTempo();
            break;

         case PAGE_ROUTER:
            displayPageRouter();
            break;

         case PAGE_SETUP:
            displayPageSetup();
            break;

         case PAGE_MIDIMONITOR:
            displayPageMIDIMonitor();
            break;

         case PAGE_SONG:
            displayPageSong();
            break;

         case PAGE_LIVEFX:
            displayPageLiveFX();
            break;
      }

      // Render page icon in upper right corner
      if (!screenShowLoopaLogo_ && !screenIsInMenu() && !screenIsInShift())
         printPageIcon();
   }

   // Display flash notification
   if (screenFlashMessageFrameCtr_)
   {
      setFontNormal();
      u8 len = strlen(screenFlashMessage_);
      u8 xpos = 128 - len * 5;
      u16 displacement = 10 - screenFlashMessageFrameCtr_;
      displacement = (displacement * displacement) / 3;
      printFormattedString(xpos, 26 - displacement, screenFlashMessage_);

      screenFlashMessageFrameCtr_--;
   }

   u8 flash = 0;
   if (gcBeatDisplayEnabled_ && oledBeatFlashState_ > 0)
      flash = oledBeatFlashState_ == 1 ? 0x44 : 0x66;

   // Save screenshot, if requested
   if (screenshotRequested_)
   {
      saveScreenshot();
      screenshotRequested_ = 0;
   }

   // Push screen buffer to screen
   for (j = 0; j < 64; j++)
   {
      APP_LCD_Cmd(0x15);
      APP_LCD_Data(0+0x1c);

      APP_LCD_Cmd(0x75);
      APP_LCD_Data(j);

      APP_LCD_Cmd(0x5c);

      u8 bgcol = 0;
      for (i = 0; i < 128; i++)
      {
         // two pixels at once...
         u8 out = screen[j][i];

         if (gcInvertOLED_)
         {
            // Screen inversion routine for white frontpanels :)
            u8 first = out >> 4U;
            u8 second = out % 16;

            first = 15 - first;
            second = 15 - second;
            out = (first << 4U) + second;
         }

         if (flash && out == 0)
            APP_LCD_Data(flash); // normally raise dark level slightly, but more intensively after 16 16th notes during flash
         else
            APP_LCD_Data(out);

         screen[j][i] = bgcol; // clear written pixels
      }
   }

   if (flash)
      oledBeatFlashState_ = 0;
}
// ----------------------------------------------------------------------------------------

/**
 * Save the screen as a screenshot file on the SD card
 *
 */
void saveScreenshot()
{
   MUTEX_SDCARD_TAKE;

   u8 i;
   for (i = 0; i < 100; i++)
   {
      sprintf(filename_, "screen%02d.pnm", i);
      if (FILE_FileExists(filename_) == 0)
         break;
   }

   s32 status;
   if ((status = FILE_WriteOpen(filename_, 1)) < 0)
   {
      DEBUG_MSG("saveScreenshot() error creating %s, status: %d\n", filename_, status);
   }
   else
   {
      sprintf(line_buffer_, "P5\n");
      status |= FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

      sprintf(line_buffer_, "256 64\n");
      status |= FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

      sprintf(line_buffer_, "255\n");
      status |= FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

      u8 x, y;
      for (y = 0; y < 64; y++)
      {
         for (x = 0; x < 128; x++)
         {
            u8 byte = screen[y][x];
            status |= FILE_WriteByte(byte & 0xf0U);
            status |= FILE_WriteByte((byte & 0x0fU) << 4U);
         }
      }
      status |= FILE_WriteClose();
   }

   if (status == 0)
      screenFormattedFlashMessage("screenshot saved");
   else
      screenFormattedFlashMessage("screen save failed");

   MUTEX_SDCARD_GIVE;
}
// -------------------------------------------------------------------------------------------


/**
 * Render test screen, one half is "full on" for flicker tests
 * one half contains a color gradient pattern
 *
 */
void testScreen()
{
  u16 x = 0;
  u16 y = 0;

  for (y = 0; y < 64; y++)
  {
     APP_LCD_Cmd(0x15);
     APP_LCD_Data(0x1c);

     APP_LCD_Cmd(0x75);
     APP_LCD_Data(y);

     APP_LCD_Cmd(0x5c);

     for (x = 0; x < 64; x++)
     {
        if (x < 32)
        {  // half screen pattern

           if ((x | 4) == 0 || (y | 4) == 0)
           {
              APP_LCD_Data(y & 0x0f);
              APP_LCD_Data(0);
           }
           else
           {
              APP_LCD_Data(0x00);
              APP_LCD_Data(0x00);
           }
        }
        else // half screen "white"
        {
           APP_LCD_Data(0xff);
           APP_LCD_Data(0xff);
        }
     }
  }

  while(1);
}
// -------------------------------------------------------------------------------------------
