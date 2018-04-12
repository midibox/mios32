// $Id: screen.c 1223 2011-06-23 21:26:52Z hawkeye $

#include <mios32.h>

#include <stdarg.h>
#include <string.h>

#include <app_lcd.h>
#include <seq_bpm.h>

#include "tasks.h"
#include "gfx_resources.h"
#include "loopa.h"


// -------------------------------------------------------------------------------------------
// SSD 1322 Screen routines by Hawkeye

// --- globals ---

u8 screen[64][128];             // Screen buffer [y][x]

u8 screenShowLoopaLogo_;
u8 screenClipNumberSelected_ = 0;
u16 screenClipStepPosition_[TRACKS];
u32 screenSongStep_ = 0;
char screenFlashMessage_[40];
u8 screenFlashMessageFrameCtr_;
char sceneChangeNotification_[20] = "";
u8 screenNewPagePanelFrameCtr_ = 0;

unsigned char* fontptr_ = (unsigned char*) fontsmall_pixdata;
u16 fontchar_bytewidth_ = 3;    // bytes to copy for a line of character pixel data
u16 fontchar_height_ = 12;      // lines to copy for a full-height character
u16 fontline_bytewidth_ = 95*3; // bytes per font pixdata line (character layout all in one line)
u8 fontInverted_ = 0;


/**
 * Set the bold font
 *
 */
void setFontBold()
{
   fontptr_ = (unsigned char*) fontbold_pixdata;
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
   fontptr_ = (unsigned char*) fontnormal_pixdata;
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
   fontptr_ = (unsigned char*) fontsmall_pixdata;
   fontchar_bytewidth_ = 3;
   fontchar_height_ = 12;
   fontline_bytewidth_ = 95 * 3;
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

   u8 syncedMuteUnmuteInProgress = trackMuteToggleRequested_[clipNumber] && (tickToStep(tick_) % beatLoopSteps_) != 0;

   if (!syncedMuteUnmuteInProgress)
   {
      if (stepLength == 0)
         sprintf((char *)buffer, "       ");
      else if (stepLength > 99)
         sprintf((char *)buffer, "%03d>%3d", stepPos, stepLength);
      else if (stepLength > 9)
         sprintf((char *)buffer, " %02d>%2d ", stepPos, stepLength);
      else
         sprintf((char *)buffer, "  %01d>%1d  ", stepPos, stepLength);
   }
   else
   {
      u8 remainSteps = 16 - (tickToStep(tick_) % beatLoopSteps_);

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
            unsigned char* fdata = (unsigned char*) digitstiny_pixdata + f_y * fontLineByteWidth + f_x;
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
 * If showLogo is true, draw the MBLoopa Logo (usually during unit startup)
 *
 */
void screenShowLoopaLogo(u8 showLogo)
{
   screenShowLoopaLogo_ = showLogo;
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
 * Notify, that a screen page change has occured (flash a page descriptor for a while)
 *
 */
void screenNotifyPageChanged()
{
   screenNewPagePanelFrameCtr_ = 20;
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
void displayClip(u8 clip)
{
   u16 x;
   u8 y;
   u16 i;
   u16 mult = 128/clipSteps_[clip][activeScene_];  // horizontal multiplier to make clips as wide as the screen

   u16 curStep = ((u32)boundTickToClipSteps(tick_, clip) * mult) / 24;

   // Render vertical 1/4th note indicators
   for (i=0; i<clipSteps_[clip][activeScene_] / 4; i++)
   {
      x = i * 4 * mult;
      if (x < 128)
         for (y=12; y<52; y++)
               screen[y][x] = i % 4 == 0 ? 0x60 : 0x50;
   }

   // Render vertical time indicator line (if seq is playing)
   if (SEQ_BPM_IsRunning() && curStep < 128)
      for (y=0; y<64; y++)
         screen[y][curStep] = 0x88;


   // Render note data
   for (i=0; i < clipNotesSize_[clip][activeScene_]; i++)
   {
      s32 transformedStep = (s32)quantizeTransform(clip, i) * mult;

      if (transformedStep >= 0) // if note starts within (potentially reconfigured) clip length
      {
         u16 step = transformedStep / 24;

         s16 note = clipNotes_[clip][activeScene_][i].note + clipTranspose_[clip][activeScene_];
         note = note < 0 ? 0 : note;
         note = note > 127 ? 127 : note;
         u8 y = (127 - note) / 2;

         if (y < 64)
         {
            u16 len = noteLengthPixels(clipNotes_[clip][activeScene_][i].length * mult);

            if (clipNotes_[clip][activeScene_][i].length == 0 && curStep > step)
            {
               // still recording (also check for boundary wrapping, disabled right now)
               len = curStep - step;
            }

            for (x = step; x <= step + len; x++)
            {
               if (clipNotes_[clip][activeScene_][i].velocity > 0)
               {
                  u8 color;
                  if (!trackMute_[clip])
                     color = x == step ? 0xFF
                                       : 0x99;  // The modulo only works if we are not scrolling and screen width = clip length
                  else
                     color = x == step ? 0x99
                                       : 0x88;  // The modulo only works if we are not scrolling and screen width = clip length

                  screen[y][x % 128] = color;

                  if (page_ == PAGE_NOTES && i == clipActiveNote_[clip][activeScene_])
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
// ----------------------------------------------------------------------------------------


/**
 * Display the normal loopa view (PAGE_TRACK)
 *
 */
void displayPageTrack(void)
{
   setFontSmall();

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 8, "M");
      printString(250, 20, "U");
      printString(250, 32, "T");
      printString(250, 44, "E");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   if (trackMute_[activeTrack_])
      printCenterFormattedString(0, "[Clip %d Scene %d [muted]%s]", activeTrack_ + 1, activeScene_ + 1, sceneChangeNotification_);
   else
      printCenterFormattedString(0, "[Clip %d Scene %d%s]", activeTrack_ + 1, activeScene_ + 1, sceneChangeNotification_);

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
void displayPageEdit(void)
{
   setFontSmall();

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 8, "E");
      printString(250, 20, "D");
      printString(250, 32, "I");
      printString(250, 44, "T");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   printCenterFormattedString(0, "Edit Settings [Clip %d Scene %d%s]", activeTrack_ + 1, activeScene_ + 1, sceneChangeNotification_);

   command_ == COMMAND_CLIPLEN ? setFontInverted() : setFontNonInverted();
   if (clipSteps_[activeTrack_][activeScene_] < 100)
      printFormattedString(0, 54, "Len %d", clipSteps_[activeTrack_][activeScene_]);
   else
      printFormattedString(0, 54, "Le %d", clipSteps_[activeTrack_][activeScene_]);

   command_ == COMMAND_QUANTIZE ? setFontInverted() : setFontNonInverted();
   switch (clipQuantize_[activeTrack_][activeScene_])
   {
      case 3: printFormattedString(42, 54, "Q1/128"); break;
      case 6: printFormattedString(42, 54, "Qu1/64"); break;
      case 12: printFormattedString(42, 54, "Qu1/32"); break;
      case 24: printFormattedString(42, 54, "Qu1/16"); break;
      case 48: printFormattedString(42, 54, "Qu 1/8"); break;
      case 96: printFormattedString(42, 54, "Qu 1/4"); break;
      case 192: printFormattedString(42, 54, "Qu 1/2"); break;
      case 384: printFormattedString(42, 54, "Qu 1/1"); break;
      default: printFormattedString(42, 54, "Qu OFF"); break;
   }

   command_ == COMMAND_TRANSPOSE ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 54, "Trn %d", clipTranspose_[activeTrack_][activeScene_]);

   command_ == COMMAND_SCROLL ? setFontInverted() : setFontNonInverted();
   printFormattedString(126, 54, "Scr %d", clipScroll_[activeTrack_][activeScene_]);

   command_ == COMMAND_STRETCH ? setFontInverted() : setFontNonInverted();
   switch (clipStretch_[activeTrack_][activeScene_])
   {
      case 1: printFormattedString(168, 54, "Zo 1/16"); break;
      case 2: printFormattedString(168, 54, "Zo 1/8"); break;
      case 4: printFormattedString(168, 54, "Zo 1/4"); break;
      case 8: printFormattedString(168, 54, "Zo 1/2"); break;
      case 16: printFormattedString(168, 54, "Zoom 1"); break;
      case 32: printFormattedString(168, 54, "Zoom 2"); break;
      case 64: printFormattedString(168, 54, "Zoom 4"); break;
      case 128: printFormattedString(168, 54, "Zoom 8"); break;
   }

   command_ == COMMAND_CLEAR ? setFontInverted() : setFontNonInverted();
   printFormattedString(210, 54, "Clear");

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

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 8, "N");
      printString(250, 20, "O");
      printString(250, 32, "T");
      printString(250, 44, "E");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   printCenterFormattedString(0, "Note Editor [Clip %d Scene %d%s]", activeTrack_ + 1, activeScene_ + 1, sceneChangeNotification_);

   if (clipNotesSize_[activeTrack_][activeScene_] > 0)
   {

      u16 activeNote = clipActiveNote_[activeTrack_][activeScene_];

      if (activeNote >= clipNotesSize_[activeTrack_][activeScene_]) // necessary e.g. for clip change
         activeNote = 0;

      u16 pos = (clipNotes_[activeTrack_][activeScene_][activeNote].tick) / 24;
      u16 length = clipNotes_[activeTrack_][activeScene_][activeNote].length;
      u8 note = clipNotes_[activeTrack_][activeScene_][activeNote].note;
      u8 velocity = clipNotes_[activeTrack_][activeScene_][activeNote].velocity;

      command_ == COMMAND_POSITION ? setFontInverted() : setFontNonInverted();
      if (pos < 100)
         printFormattedString(0, 54, "Pos %d", pos);
      else
         printFormattedString(0, 54, "Po %d", pos);

      command_ == COMMAND_NOTE ? setFontInverted() : setFontNonInverted();

      char noteStr[8];
      stringNote(noteStr, note);
      printFormattedString(42, 54, "%s", noteStr);

      command_ == COMMAND_VELOCITY ? setFontInverted() : setFontNonInverted();
      printFormattedString(84, 54, "Vel %d", velocity);

      command_ == COMMAND_LENGTH ? setFontInverted() : setFontNonInverted();
      printFormattedString(126, 54, "Len %d", length);

      command_ == COMMAND_CLEAR ? setFontInverted() : setFontNonInverted();
      printFormattedString(210, 54, "Delete");

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
 * Display the track midi settings page (PAGE_MIDI)
 *
 */
void displayPageMidi(void)
{
   setFontSmall();

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 8, "M");
      printString(250, 20, "I");
      printString(250, 32, "D");
      printString(250, 44, "I");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   printCenterFormattedString(0, "MIDI Parameters [Track %d]", activeTrack_ + 1);

   command_ == COMMAND_PORT ? setFontInverted() : setFontNonInverted();
   switch (trackMidiPort_[activeTrack_])
   {
      case 0x20: printFormattedString(0, 54, " OUT1 "); break;
      case 0x21: printFormattedString(0, 54, " OUT2 "); break;
      case 0x22: printFormattedString(0, 54, " OUT3 "); break;
      case 0x23: printFormattedString(0, 54, " OUT4 "); break;
   }

   command_ == COMMAND_CHANNEL ? setFontInverted() : setFontNonInverted();
   printFormattedString(42, 54, " Chn %d ", trackMidiChannel_[activeTrack_] + 1);

   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the main menu (PAGE_DISK)
 *
 */
void displayPageDisk(void)
{
   setFontSmall();

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 8, "D");
      printString(250, 20, "I");
      printString(250, 32, "S");
      printString(250, 44, "K");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   printCenterFormattedString(0, "Disk Operations", activeTrack_ + 1, activeScene_ + 1);

   command_ == COMMAND_SAVE ? setFontInverted() : setFontNonInverted();
   printFormattedString(0, 54, "Save");

   command_ == COMMAND_LOAD ? setFontInverted() : setFontNonInverted();
   printFormattedString(42, 54, "Load");

   command_ == COMMAND_NEW ? setFontInverted() : setFontNonInverted();
   printFormattedString(84, 54, "New");


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
 * Display the bpm settings page (PAGE_BPM)
 *
 */
void displayPageBpm(void)
{
   setFontSmall();

   if (screenNewPagePanelFrameCtr_ > 0)
   {
      setFontInverted();
      printString(250, 14, "B");
      printString(250, 26, "P");
      printString(250, 38, "M");
      setFontNonInverted();
      screenNewPagePanelFrameCtr_--;
   }

   printCenterFormattedString(0, "Tempo Settings", activeTrack_ + 1, activeScene_ + 1);

   command_ == COMMAND_BPM ? setFontInverted() : setFontNonInverted();
   u16 bpm = SEQ_BPM_IsMaster() ? bpm_ : SEQ_BPM_Get();
   printFormattedString(0, 54, "%d BPM", bpm);

   setFontNonInverted();
   displayClip(activeTrack_);
}
// ----------------------------------------------------------------------------------------


/**
 * Display the current screen buffer (once per frame, called in app.c scheduler)
 *
 */
void display(void)
{
   u8 i, j;

   if (screenShowLoopaLogo_)
   {
      // Startup/initial session loading: Render the MBLoopa Logo

      setFontBold();  // width per letter: 10px (for center calculation)
      printFormattedString(78, 2, "LoopA V2.01");

      setFontSmall(); // width per letter: 6px
      printFormattedString(28, 20, "(C) Hawkeye, latigid on, TK. 2018");
      printFormattedString(52, 32, "MIDIbox hardware platform");

      setFontBold(); // width per letter: 10px;
      printFormattedString(52, 44, "www.midiphy.com");
   }
   else
   {
      // Display page content...
      switch (page_)
      {
         case PAGE_TRACK:
            displayPageTrack();
            break;

         case PAGE_EDIT:
            displayPageEdit();
            break;

         case PAGE_NOTES:
            displayPageNotes();
            break;

         case PAGE_MIDI:
            displayPageMidi();
            break;

         case PAGE_DISK:
            displayPageDisk();
            break;

         case PAGE_BPM:
            displayPageBpm();
            break;
      }
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
   // no flashing for now :)
   //if (oledBeatFlashState_ > 0)
   //   flash = oledBeatFlashState_ == 1 ? 0x44 : 0x66;

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
         // first two pixels...
         u8 out = screen[j][i];
         if (flash && out == 0)
            APP_LCD_Data(flash); // normally raise dark level slightly, but more intensively after 16 16th notes during flash
         else
            APP_LCD_Data(out);

         screen[j][i] = bgcol; // clear written pixels

         // next two pixels
         i++;
         out = screen[j][i];

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

           if (x || 4 == 0 || y || 4 == 0)
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
