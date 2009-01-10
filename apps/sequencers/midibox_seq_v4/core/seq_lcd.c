// $Id$
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

const u8 charset_hbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // empty bar
  0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, // "|  "
  0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // "|| "
  0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00, // "|||"
  0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, // " o "
  0x00, 0x10, 0x14, 0x15, 0x15, 0x14, 0x10, 0x00, // " > "
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // not used
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // not used
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
s32 SEQ_LCD_InitSpecialChars(seq_lcd_charset_t charset)
{
  static seq_lcd_charset_t current_charset = SEQ_LCD_CHARSET_None;

  if( charset != current_charset ) {
    current_charset = charset;

    int dev;
    for(dev=0; dev<2; ++dev) {
      MIOS32_LCD_DeviceSet(dev);
      switch( charset ) {
        case SEQ_LCD_CHARSET_VBars:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_vbars);
	  break;
        case SEQ_LCD_CHARSET_HBars:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_hbars);
	  break;
        default:
	  return -1; // charset doesn't exist
      }
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
// prints padded string
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintStringPadded(char *str, u32 width)
{
  // replacement for not supported "%:-40s" of simple sprintf function

  u32 pos;
  u8 fill = 0;
  for(pos=0; pos<width; ++pos) {
    char c = str[pos];
    if( c == 0 )
      fill = 1;
    if( fill )
      MIOS32_LCD_PrintChar(' ');
    else
      MIOS32_LCD_PrintChar(c);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a vertical bar for a 3bit value
// (1 character)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintVBar(u8 value)
{
  return MIOS32_LCD_PrintChar(value);
}


/////////////////////////////////////////////////////////////////////////////
// prints a horizontal bar for a 4bit value
// (5 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintHBar(u8 value)
{
  // special chars which should be print depending on meter value (16 entries, only 14 used)
  const u8 hbar_table[16][5] = {
    { 4, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0 },
    { 2, 0, 0, 0, 0 },
    { 3, 0, 0, 0, 0 },
    { 3, 1, 0, 0, 0 },
    { 3, 2, 0, 0, 0 },
    { 3, 3, 0, 0, 0 },
    { 3, 3, 1, 0, 0 },
    { 3, 3, 2, 0, 0 },
    { 3, 3, 3, 0, 0 },
    { 3, 3, 3, 1, 0 },
    { 3, 3, 3, 2, 0 },
    { 3, 3, 3, 3, 1 },
    { 3, 3, 3, 3, 2 },
    { 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3 }
  };

  int i;
  for(i=0; i<5; ++i)
    MIOS32_LCD_PrintChar(hbar_table[value][i]);

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
// prints an arp event (3 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintArp(u8 arp)
{
  if( arp < 4 )
    MIOS32_LCD_PrintString("---");
  else {
    int key_num = (arp >> 2) & 0x3;
    int arp_oct = (arp >> 4) & 0x7;

    if( arp_oct < 2 ) { // Multi Arp
      MIOS32_LCD_PrintChar('*');
      arp_oct = ((arp >> 2) & 7) - 4;
    } else {
      MIOS32_LCD_PrintChar('1' + key_num);
      arp_oct -= 4;
    }

    if( arp_oct >= 0 )
      MIOS32_LCD_PrintFormattedString("+%d", arp_oct);
    else
      MIOS32_LCD_PrintFormattedString("-%d", -arp_oct);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the gatelength (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGatelength(u8 len)
{
  const char len_tab[24][5] = {
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
    "100%"  // 23
  };

  if( len < 24 ) { // gatelength
    MIOS32_LCD_PrintString((char *)len_tab[len]);
  } else if( len < 32 ) { // gilde
    MIOS32_LCD_PrintString("Gld.");
  } else { // multi trigger
    MIOS32_LCD_PrintFormattedString("%1dx%2d", (len>>5)+1, (len&0x1f)+1);
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
// prints the pattern number (2 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintPattern(seq_pattern_t pattern)
{
  if( pattern.DISABLED ) {
    MIOS32_LCD_PrintChar('-');
    MIOS32_LCD_PrintChar('-');
  } else {
    MIOS32_LCD_PrintChar('A' + pattern.group + (pattern.lower ? 32 : 0));
    MIOS32_LCD_PrintChar('1' + pattern.num);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the pattern name as a 20 character string
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintPatternName(seq_pattern_t pattern, char *pattern_name)
{
  // if string only contains spaces, print pattern number instead
  int i;
  u8 found_char = 0;
  for(i=0; i<20; ++i)
    if( pattern_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    MIOS32_LCD_PrintFormattedString("%-20s", pattern_name);
  else {
    MIOS32_LCD_PrintString("<Pattern ");
    SEQ_LCD_PrintPattern(pattern);
    MIOS32_LCD_PrintChar('>');
    SEQ_LCD_PrintSpaces(8);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the track name as a 20 character string
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintTrackName(u8 track, char *track_name)
{
  // if string only contains spaces, print track number instead
  int i;
  u8 found_char = 0;
  for(i=0; i<20; ++i)
    if( track_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    MIOS32_LCD_PrintFormattedString("%-20s", track_name);
  else {
    MIOS32_LCD_PrintFormattedString("<Track #%d", track+1);
    MIOS32_LCD_PrintChar('>');
    SEQ_LCD_PrintSpaces((track > 9) ? 9 : 10);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints MIDI port (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintMIDIPort(mios32_midi_port_t port)
{
  switch( port >> 4 ) {
    case 0:  MIOS32_LCD_PrintString("Def."); break;
    case 1:  MIOS32_LCD_PrintFormattedString("USB%x", port%16); break;
    case 2:  MIOS32_LCD_PrintFormattedString("UAR%x", port%16); break;
    case 3:  MIOS32_LCD_PrintFormattedString("IIC%x", port%16); break;
    default: MIOS32_LCD_PrintFormattedString("0x%02X", port); break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints step view (6 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintStepView(u8 step_view)
{

  MIOS32_LCD_PrintFormattedString("S%2d-%2d", (step_view*16)+1, (step_view+1)*16);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected step (6 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintSelectedStep(u8 step_sel, u8 step_max)
{
  MIOS32_LCD_PrintFormattedString("%3d/%2d", step_sel+1, step_max+1);

  return 0; // no error
}


