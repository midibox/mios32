// $Id$
/*
 * Demo application for SSD1322 GLCD
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
#include "app.h"

#include <glcd_font.h>
#include <app_lcd.h>


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
}



// -------------------------------------------------------------------------------------------
// Voxelspace demo by Hawkeye

u8 field[128][128];             // Heightfield
u8 screen[64][128];             // Screen buffer [y][x]


// Limits a value to 0...255
//
int limit(int x)
{
  return x < 0 ? 0 : (x > 255 ? 255 : x);
}
// -------------------------------------------------------------------------------------------


// Landscape/heightfield generator
//
void calcField(void)
{
  int p, i, j, k, k2, p2;

  field[0][0]=128;

  // "Cloud" landscape generator
  for (p = 256; p > 1; p = p2)
  {
     p2 = p >> 1;
     k = p * 8 + 20;
     k2 = k >> 1;

     for (i = 0; i < 128; i += p)
     {
        for (j = 0; j < 128; j += p)
        {
           int a = field[i][j];
           int b = field[(i + p) & 127][j];
           int c = field[i][(j + p) & 127];
           int d = field[(i + p) & 127][(j + p) & 127];

           field[i][(j + p2) & 127] = limit(((a + c)>>1)+(rand() % k - k2));
           field[(i + p2) & 127][(j + p2) & 127]= limit(((a + b + c + d)>>2)+(rand() % k - k2));
           field[(i + p2) & 127][j]= limit(((a + b) >> 1)+(rand() % k - k2));
        }
     }
  }

  // Blur
  for (k=0; k<1; k++)
     for (i=1; i<127; i++)
        for (j=1; j<127; j++)
        {
           u16 four = field[i-1][j] + field[i+1][j] + field[i][j-1] + field[i][j+1];
           four = four >> 2;
           field[i][j] = (field[i][j] + four) >> 1;
        }
}
// -------------------------------------------------------------------------------------------


// Voxel space display demo
//
// Now renders full 256x64 @ >30fps
//
void runVoxel(void)
{
   u16 y; // position
   u16 i, j, k;

   calcField();

   // multiplication/division precalculated tables
   u16 mult1dot5[64];
   for (i = 0; i < 64; i++)
      mult1dot5[i] = (u16)((float) i * 1.5);


   // initial clear screen
   for (j = 0; j < 64; j++)
      for (i = 0; i < 128; i++)
         screen[j][i] = j < 6 ? ((j << 4) + j) : 0;

   // endless animation, if it needs to be stopped, poll here
   while (1)
   {
      for (y = 30000; y >=1 ; y--)
      {
         u16 yfield;

         // Calculate screen buffer
         for (j=0; j<30; j++)  // Distance from screen plane
         {
            u16 distscaler = 38 - j;
            u16 stdheight = 400 / distscaler;
            u16 endy = mult1dot5[stdheight];
            if (endy > 63) endy = 63;

            yfield = j + y;

            // "continuous scrolling" yfield accessor adjustment
            yfield = yfield % 255;
            if (yfield > 127)
               yfield = 255-yfield;

            for (i = 0; i < 128; i++)
            {
               // --- "Even" display pixel (0, 2, 4, 6, ...) ---
               u16 xfield = (64 + (int)((((i<<1)-128)*distscaler)>>5)) & 127;
               u8 h = field[yfield][xfield];
               u8 height = h / distscaler; // Scale height to distance
               u8 starty = mult1dot5[stdheight - height];

               u8 col = h & 0xf0;
               for (k = starty; k < endy; k++)
                  screen[k][i] = (screen[k][i] & 0x0f) + col;

               // --- "Odd" display pixel (1, 3, 5, 7, ...) ---
               xfield = (64 + (int)((((i<<1)-127)*distscaler)>>5)) & 127;
               h = field[yfield][xfield];

               height = h / distscaler; // Scale height to distance
               starty = mult1dot5[stdheight - height];

               col = h >> 4;
               for (k = starty; k < endy; k++)
                  screen[k][i] = (screen[k][i] & 0xf0) + col;
            }
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
   }
}
// ----------------------------------------------------------------------------------------


// Render test screen, one half is "full on" for flicker tests
// one half contains a color gradient pattern
//
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


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  runVoxel();

  // Alternative: "Test screen for parameter tuning"
  // testScreen();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
// Very useful for tuning display initialization parameters
// Note - only tune when no other activity is going on (parallel data writes)
// good when using static "testScreen()"
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // For testing
  if( midi_package.type == CC ) {
    switch( midi_package.cc_number ) {
    case 0x10:
      APP_LCD_Cmd(0xb3); // Set_Display_Clock(0x91);
      APP_LCD_Data((u8)midi_package.value/2);
      break;
    case 0x11:
       APP_LCD_Cmd(0xca); // Set_Multiplex_Ratio(0x3F);
       APP_LCD_Data((u8)midi_package.value/2);
    case 0x12:
      APP_LCD_Cmd(0xb1); // Set_Phase_Length(0xE2);
      APP_LCD_Data((u8)midi_package.value/2);
      break;
    case 0x13:
      APP_LCD_Cmd(0xbb); // Set_Precharge_Voltage(0x1F);
      APP_LCD_Data((u8)midi_package.value*2);
      break;
    case 0x14:
      APP_LCD_Cmd(0xb6); // Set_Precharge_Period(0x08);
      APP_LCD_Data((u8)midi_package.value*2);
      break;
    case 0x15:
      APP_LCD_Cmd(0xbe); // Set_VCOMH(0x07);
      APP_LCD_Data((u8)midi_package.value*2);
      break;
    case 0x16:
       APP_LCD_Cmd(0xc1); // Constrast Current
       APP_LCD_Data(midi_package.value*2);
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}


