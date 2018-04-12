// $Id: voxelspace.c 1223 2011-06-23 21:26:52Z hawkeye $

#include <mios32.h>

#include <app_lcd.h>

#include <seq_bpm.h>
#include "screen.h"
#include "voxelspace.h"


// -------------------------------------------------------------------------------------------
// Voxelspace demo by Hawkeye


u8 field[128][128];             // Heightfield [y][x]

u8 screen[64][128];             // Screen buffer [y][x]
u16 mult1dot5[64];              // multiplication/division precalculated tables
u8 notedraw[128];               // Current note on/note off note draw line (vertical, in time)


// Limits a value to 0...255
//
int limit(int x)
{
   return x < 0 ? 0 : (x > 255 ? 255 : x);
}
// -------------------------------------------------------------------------------------------


// Wrap single value index out-of-bound accesses to array limits (128 -> 0, -1 -> 127)
int wrap(int c)
{
   if (c == 128)
      return 0;

   if (c == -1)
      return 127;

   return c;
}
// -------------------------------------------------------------------------------------------


// Smoothen the current landscape
//
void blur(void)
{

   int i, j;

   // Blur
   for (i=0; i<128; i++)
      for (j=0; j<128; j++)
      {
         u16 four = field[wrap(i-1)][j] + field[wrap(i+1)][j] + field[i][wrap(j-1)] + field[i][wrap(j+1)];
         four = four >> 2;
         field[i][j] = (field[i][j] + four) >> 1;
      }

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
   blur();

   // Calc multiplication table
   for (i = 0; i < 64; i++)
      mult1dot5[i] = (u16)((float) i * 1.5);

   // initially clear screen with "horizon"
   for (j = 0; j < 64; j++)
      for (i = 0; i < 128; i++)
         screen[j][i] = j < 6 ? ((j << 4) + j) : 0;

   // initially clear notedraw line
   voxelClearNotes();

}
// -------------------------------------------------------------------------------------------


// Render a single voxel frame at the current position in voxel space
//
static int tempEncoderAccel = 0;
static u16 yVoxel = 30000;  // voxel space position
static u8 tickLine = 0;

void voxelFrame(void)
{
   u16 i, j, k;
   u8 seqRunning = SEQ_BPM_IsRunning();

   yVoxel++;
   u16 yfield;
   yVoxel += tempEncoderAccel;


   // Output random terrain noise, that will be blurred down
   yfield = (40 + yVoxel) % 128;
   for (i = 0; i < 128; i++)
   {
      field[yfield][i] = rand() % (seqRunning ? 255 : 208);
   }

   // Output current notedraw heightline
   yfield = (29 + yVoxel) % 128;
   for (i = 0; i < 128; i++)
   {
      if (notedraw[i] > 0)
         field[yfield][i] = notedraw[i];
      else if (tickLine)
         field[yfield][i] = 160;
   }

   tickLine = 0;
   blur();

   // Calculate screen buffer
   for (j=0; j<30; j++)  // Distance from screen plane
   {
      u16 distscaler = 38 - j;
      u16 stdheight = 400 / distscaler;
      u16 endy = mult1dot5[stdheight];
      if (endy > 63) endy = 63;

      yfield = (j + yVoxel) % 128;

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
   display();
}
// ----------------------------------------------------------------------------------------


// Voxel frame output during demo/screensaver mode
//
void voxelFrameDemo(void)
{
   u16 i, j, k;

   yVoxel--;
   u16 yfield;
   yVoxel += tempEncoderAccel;

   // Calculate screen buffer
   for (j=0; j<30; j++)  // Distance from screen plane
   {
      u16 distscaler = 38 - j;
      u16 stdheight = 400 / distscaler;
      u16 endy = mult1dot5[stdheight];
      if (endy > 63) endy = 63;

      yfield = j + yVoxel;

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
   display();
}
// ----------------------------------------------------------------------------------------


/**
 * Add a note in the voxel space
 *
 */
void voxelNoteOn(u8 note, u8 velocity)
{

   // MIOS32_MIDI_SendDebugMessage("voxelNoteOn %d %d", note, velocity);

   u8 vel = limit(128 + velocity * 2);

   notedraw[note] = limit(vel);

   if (note > 0)
      notedraw[note-1] = limit((vel*80)/100);

   if (note > 1)
      notedraw[note-2] = limit((vel*60)/100);

   if (note < 127)
      notedraw[note+1] = limit((vel*80)/100);

   if (note < 126)
   notedraw[note+2] = limit((vel*60)/100);

}
// ----------------------------------------------------------------------------------------


/**
 * Add a note in the voxel space
 *
 */
void voxelNoteOff(u8 note)
{
   // MIOS32_MIDI_SendDebugMessage("voxelNoteOff %d", note);

   notedraw[note] = 0;

   if (note > 0)
      notedraw[note-1] = 0;

   if (note > 1)
      notedraw[note-2] = 0;

   if (note < 127)
      notedraw[note+1] = 0;

   if (note < 126)
      notedraw[note+2] = 0;

}
// ----------------------------------------------------------------------------------------


/**
 * Add a tick line in the voxel space
 *
 */
void voxelTickLine()
{
   tickLine = 1;
}
// ----------------------------------------------------------------------------------------


/**
 * Clear current notes
 *
 */
void voxelClearNotes()
{
   u8 i;

   for (i = 0; i < 128; i++)
      notedraw[i] = 0;
}
// ----------------------------------------------------------------------------------------


/**
 * Clear current voxel field
 *
 */
void voxelClearField()
{
   u8 i, j;

   for (i = 0; i < 128; i++)
      for (j = 0; j < 128; j++)
         field[i][j] = 0;
}
// ----------------------------------------------------------------------------------------



