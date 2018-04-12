// $Id: screen.c 1223 2011-06-23 21:26:52Z hawkeye $

#include <mios32.h>

#include <stdarg.h>
#include <string.h>

#include <glcd_font.h>
#include <app_lcd.h>

#include "gfx_resources.h"
#include "voxelspace.h"


// -------------------------------------------------------------------------------------------
// SSD 1322 Screen routines by Hawkeye

// --- globals ---

u8 screen[64][128];             // Screen buffer [y][x]

u8 screenShowLoopaLogo_;
u8 screenClipNumberSelected_ = 0;
s8 screenRecordingClipNumber_ = -1;
u16 screenClipStepPosition_[8];
u16 screenClipStepLength_[8];
u32 screenSongStep_ = 0;
char screenFlashMessage_[40];
u8 screenFlashMessageFrameCtr_;

unsigned char* fontptr_ = (unsigned char*) fontsmall_pixdata;
u16 fontchar_bytewidth_ = 3;    // bytes to copy for a line of character pixel data
u16 fontchar_height_ = 12;      // lines to copy for a full-height character
u16 fontline_bytewidth_ = 95*3; // bytes per font pixdata line (character layout all in one line)


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
                  if (*fdata)
                     *sdata = *fdata;  // inner loop: copy 2 pixels, if onscreen

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
   char buffer[64]; // TODO: tmp!!! Provide a streamed COM method later!
   va_list args;

   va_start(args, format);
   vsprintf((char *)buffer, format, args);
   return printString(xPixCoord, yPixCoord, buffer);
}
// ----------------------------------------------------------------------------------------



/**
 * Display a loopa slot time indicator
 * Format: [clipPos:clipLength]
 *         times are in steps/quarternotes
 *
 *         the time indicator will be rendered inverted, if this is the selected clip/active clip
 *         the display position depends on the slot number, slot #0 is upper left, slot #7 is lower right
 *
 */
void displayClipPosition(u8 clipNumber)
{
   char buffer[16];

   u8 loopStartChar = '<';
   u8 loopEndChar = '=';

   u16 stepLength = screenClipStepLength_[clipNumber];
   u16 stepPos = screenClipStepPosition_[clipNumber];
   u8 isSelected = (clipNumber == screenClipNumberSelected_);
   u8 isRecording = (clipNumber == screenRecordingClipNumber_);

   if (!isRecording)
   {
      if (stepLength == 0)
         sprintf((char *)buffer, "           ");
      else if (stepLength > 999)
         sprintf((char *)buffer, "%c%04d>%4d%c", loopStartChar, stepPos, stepLength, loopEndChar);
      else if (stepLength > 99)
         sprintf((char *)buffer, " %c%03d>%3d%c ", loopStartChar, stepPos, stepLength, loopEndChar);
      else if (stepLength > 9)
         sprintf((char *)buffer, "  %c%02d>%2d%c  ", loopStartChar, stepPos, stepLength, loopEndChar);
   }
   else
   {
      sprintf((char *)buffer, ":: %05d ;;", screenSongStep_);
   }

   u8 xPixCoord = (clipNumber % 4) * 64;
   u8 yPixCoord = clipNumber < 4 ? 0 : 56;
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
 * Set the currently recorded-to clip
 *
 */
void screenSetClipRecording(u8 clipNumber, u8 recordingActiveFlag)
{
   if (recordingActiveFlag)
      screenRecordingClipNumber_ = clipNumber;
   else
      screenRecordingClipNumber_ = -1;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the length of a clip
 *
 */
void screenSetClipLength(u8 clipNumber, u16 stepLength)
{
   DEBUG_MSG("[screenSetClipLength] clip: %d steplength: %d", clipNumber, stepLength);
   screenClipStepLength_[clipNumber] = stepLength;
}
// ----------------------------------------------------------------------------------------


/**
 * Set the position info of a clip
 *
 */
void screenSetClipPosition(u8 clipNumber, u16 stepPosition)
{
   DEBUG_MSG("[screenSetClipPosition] clip: %u stepPosition: %u", clipNumber, stepPosition);
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
 * Display the current screen buffer
 *
 */
void display(void)
{
   u8 i, j;

   if (screenShowLoopaLogo_)
   {
      // Startup/initial session loading: Render the MBLoopa Logo
      setFontBold();
      printFormattedString(82, 10, "MBLoopA V1");
      setFontSmall();
      printFormattedString(64, 32, "(C) Hawkeye, TK. 2015");
      printFormattedString(52, 44, "MIDIbox hardware platform");
   }
   else
   {
      // Render normal operations fonts/menus

      // printFormattedString(0, 51, "%s %s %u:%u", screenMode, screenFile, screenPosBar, screenPosStep % 16);

      u8 clip;
      for (clip = 0; clip < 8; clip++)
         displayClipPosition(clip);
   }

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

   // Push screen buffer
   for (j = 0; j < 64; j++)
   {
      APP_LCD_Cmd(0x15);
      APP_LCD_Data(0+0x1c);

      APP_LCD_Cmd(0x75);
      APP_LCD_Data(j);

      APP_LCD_Cmd(0x5c);

      u8 bgcol = j < 6 ? ((j << 4) + j) : 0;
      for (i = 0; i < 128; i++)
      {
         APP_LCD_Data(screen[j][i]);
         screen[j][i] = bgcol;
         i++;
         APP_LCD_Data(screen[j][i]);
         screen[j][i] = bgcol;
      }
   }

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
