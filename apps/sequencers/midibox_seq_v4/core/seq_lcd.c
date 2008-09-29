// $Id: mios32_dout.c 24 2008-09-16 17:50:55Z tk $
/*
 * LCD utility functions
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

#include "seq_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u8 current_charset = SEQ_LCD_CHARSET_NONE;

const u8 charset_vbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e
};


/////////////////////////////////////////////////////////////////////////////
// Clear both LCDs
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_Clear(void)
{
  int i;

  // clear both LCDs
  for(i=0; i<2; ++i) {
    MIOS32_LCD_DeviceSet(i);
    MIOS32_LCD_Clear();
  }

  // select first LCD again
  MIOS32_LCD_DeviceSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// initialise character set (if not already active)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_InitSpecialChars(u8 charset)
{
  if( charset != current_charset ) {
    current_charset = charset;

    switch( charset ) {
      case SEQ_LCD_CHARSET_VBARS:
	MIOS32_LCD_SpecialCharsInit((u8 *)charset_vbars);
	break;
      default:
	return -1; // charset doesn't exist
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints <num> spaces
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintSpaces(u8 num)
{
  do {
    MIOS32_LCD_PrintChar(' ');
  } while( --num );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a note string (3 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintNote(u8 note)
{
  const char note_tab[12][3] = { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };

  // print "---" if note number is 0
  if( note == 0 )
    MIOS32_LCD_PrintString("---");
  else {
    u8 octave = 0;

    // determine octave, note contains semitone number thereafter
    while( note >= 12 ) {
      ++octave;
      note -= 12;
    }

    // print semitone (capital letter if octave >= 2)
    MIOS32_LCD_PrintChar(octave >= 2 ? (note_tab[note][0] + 'A'-'a') : note_tab[note][0]);
    MIOS32_LCD_PrintChar(note_tab[note][1]);

    // print octave
    switch( octave ) {
      case 0:  MIOS32_LCD_PrintChar('2'); break; // -2
      case 1:  MIOS32_LCD_PrintChar('1'); break; // -1
      default: MIOS32_LCD_PrintChar('0' + (octave-2)); // 0..7
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the gatelength (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGatelength(u8 len)
{
  const char len_tab[23][5] = {
    "  4%", // 0
    "  8%", // 1
    " 13%", // 2
    " 17%", // 3
    " 21%", // 4
    " 25%", // 5
    " 29%", // 6
    " 33%", // 7
    " 38%", // 8
    " 42%", // 9
    " 46%", // 10
    " 50%", // 11
    " 54%", // 12
    " 58%", // 13
    " 52%", // 14
    " 67%", // 15
    " 71%", // 16
    " 75%", // 17
    " 79%", // 18
    " 83%", // 19
    " 88%", // 20
    " 92%", // 21
    " 96%", // 22
    "100%" // 23
  };

  if( len < 24 ) { // gatelength
    MIOS32_LCD_PrintString(len_tab[len]);
  } else if( len < 32 ) { // gilde
    MIOS32_LCD_PrintString("Gld.");
  } else { // multi trigger
    char tmp[5];
    sprintf(tmp, "%1dx%2d", (len>>5)+1, (len&0x1f)+1);
    MIOS32_LCD_PrintString(tmp);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected group/track (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGxTy(u8 group, u8 selected_tracks)
{
  const char selected_tracks_tab[16] = { '-', '1', '2', 'M', '3', 'M', 'M', 'M', '4', 'M', 'M', 'M', 'M', 'M', 'M', 'A' };

  MIOS32_LCD_PrintChar('G');
  MIOS32_LCD_PrintChar('1' + group);
  MIOS32_LCD_PrintChar('T');
  MIOS32_LCD_PrintChar(selected_tracks_tab[selected_tracks & 0xf]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected parameter layer (8 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintParLayer(u8 layer)
{
  const char selected_par_layer[3][7] = { "Note  ", "Vel.  ", "Len.  " };

  MIOS32_LCD_PrintChar('A' + layer);
  MIOS32_LCD_PrintChar(':');
  MIOS32_LCD_PrintString(selected_par_layer[layer]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected trigger layer (8 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintTrgLayer(u8 layer)
{
  const char selected_trg_layer[3][7] = { "Gate  ", "Acc.  ", "Roll  " };

  MIOS32_LCD_PrintChar('A' + layer);
  MIOS32_LCD_PrintChar(':');
  MIOS32_LCD_PrintString(selected_trg_layer[layer]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints MIDI port (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintMIDIPort(u8 port)
{
  MIOS32_LCD_PrintString("Def.");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints step view (6 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintStepView(u8 step_view)
{
  char tmp[7];

  sprintf(tmp, "S%2d-%2d", (step_view*16)+1, (step_view+1)*16);
  MIOS32_LCD_PrintString(tmp);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected step (6 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintSelectedStep(u8 step_sel, u8 step_max)
{
  char tmp[7];

  sprintf(tmp, "%3d/%2d", step_sel+1, step_max+1);
  MIOS32_LCD_PrintString(tmp);

  return 0; // no error
}


