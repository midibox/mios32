// $Id$
/*
 * LCD utility functions
 *
 * Both 2x40 LCD screens are buffered.
 * The application should only access the displays via SEQ_LCD_* commands.
 *
 * The buffer method has the advantage, that multiple tasks can write to the
 * LCD without accessing the IO pins or the requirement for semaphores (to save time)
 *
 * Only changed characters (marked with flag 7 of each buffer byte) will be 
 * transfered to the LCD. This greatly improves performance as well, especially
 * if a graphical display should ever be supported by MBSEQ, but also in the
 * emulation.
 *
 * Another advantage: LCD access works independent from the physical dimension
 * of the LCDs. They are combined to one large 2x80 display, and SEQ_LCD_Update()
 * will take care for switching between the devices and setting the cursor.
 * If different LCDs should be used, only SEQ_LCD_Update() needs to be changed.
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
#include <stdarg.h>

#include "seq_lcd.h"
#include "seq_midi_port.h"
#include "seq_cc.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define LCD_MAX_LINES    2
#define LCD_MAX_COLUMNS  80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 charset_vbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e
};

static const u8 charset_hbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // empty bar
  0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, // "|  "
  0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // "|| "
  0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00, // "|||"
  0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, // " o "
  0x00, 0x10, 0x14, 0x15, 0x15, 0x14, 0x10, 0x00, // " > "
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // not used
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // not used
};

static const u8 charset_drum_symbols_big[64] = {
  0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, // dot
  0x00, 0x04, 0x0e, 0x1f, 0x1f, 0x0e, 0x04, 0x00, // full
  0x00, 0x0e, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, // outlined
  0x00, 0x0e, 0x11, 0x15, 0x15, 0x11, 0x0e, 0x00, // outlined with dot
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const u8 charset_drum_symbols_medium[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // no dot
  0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, // left dot
  0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, // right dot
  0x00, 0x00, 0x00, 0x0a, 0x0a, 0x00, 0x00, 0x00, // both dots
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const u8 charset_drum_symbols_small[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0
  0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, // 1
  0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, // 2
  0x00, 0x00, 0x00, 0x14, 0x14, 0x00, 0x00, 0x00, // 3
  0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, // 4
  0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00, // 5
  0x00, 0x00, 0x00, 0x14, 0x14, 0x00, 0x00, 0x00, // 6
  0x00, 0x00, 0x00, 0x15, 0x15, 0x00, 0x00, 0x00, // 7
};

static u8 lcd_buffer[LCD_MAX_LINES][LCD_MAX_COLUMNS];

static u16 lcd_cursor_x;
static u16 lcd_cursor_y;


/////////////////////////////////////////////////////////////////////////////
// Buffer handling functions
/////////////////////////////////////////////////////////////////////////////

// clears the buffer
s32 SEQ_LCD_Clear(void)
{
  int i;
  
  u8 *ptr = (u8 *)lcd_buffer;
  for(i=0; i<LCD_MAX_LINES*LCD_MAX_COLUMNS; ++i)
    *ptr++ = ' ';

  lcd_cursor_x = 0;
  lcd_cursor_y = 0;

  return 0; // no error
}

// prints char into buffer and increments cursor
s32 SEQ_LCD_PrintChar(char c)
{
  if( lcd_cursor_y >= LCD_MAX_LINES || lcd_cursor_x >= LCD_MAX_COLUMNS )
    return -1; // invalid cursor range

  u8 *ptr = &lcd_buffer[lcd_cursor_y][lcd_cursor_x++];
  if( (*ptr & 0x7f) != c )
      *ptr = c;

  return 0; // no error
}

// allows to change the buffer from other tasks w/o the need for semaphore handling
// it doesn't change the cursor
s32 SEQ_LCD_BufferSet(u16 x, u16 y, char *str)
{
  // we assume, that the CPU allows atomic accesses to bytes, 
  // therefore no thread locking is required

  if( lcd_cursor_y >= LCD_MAX_LINES )
    return -1; // invalid cursor range

  u8 *ptr = &lcd_buffer[y][x];
  while( *str != '\0' ) {
    if( x++ >= LCD_MAX_COLUMNS )
      break;
    if( (*ptr & 0x7f) != *str )
      *ptr = *str;
    ++ptr;
    ++str;
  }

  return 0; // no error
}

// sets the cursor to a new buffer location
s32 SEQ_LCD_CursorSet(u16 column, u16 line)
{
  // set character position
  lcd_cursor_x = column;
  lcd_cursor_y = line;

  return 0;
}

// transfers the buffer to LCDs
// if force != 0, it is ensured that the whole screen will be refreshed, regardless
// if characters have changed or not
s32 SEQ_LCD_Update(u8 force)
{
  int next_x = -1;
  int next_y = -1;

  int x, y;
  u8 *ptr = (u8 *)lcd_buffer;
  for(y=0; y<LCD_MAX_LINES; ++y)
    for(x=0; x<LCD_MAX_COLUMNS; ++x) {

      if( force || !(*ptr & 0x80) ) {
	if( x != next_x || y != next_y ) {
	  // for 2 * 2x40 LCDs
	  MIOS32_LCD_DeviceSet((x >= 40) ? 1 : 0);
	  MIOS32_LCD_CursorSet(x%40, y);
	}
	MIOS32_LCD_PrintChar(*ptr & 0x7f);
	*ptr |= 0x80;
	next_y = y;
	next_x = x+1;

	// for 2 * 2x40 LCDs: ensure that cursor is set when we reach the second half
	if( next_x == 40 )
	  next_x = -1;
      }
      ++ptr;
    }

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
        case SEQ_LCD_CHARSET_DrumSymbolsBig:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_drum_symbols_big);
	  break;
        case SEQ_LCD_CHARSET_DrumSymbolsMedium:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_drum_symbols_medium);
	  break;
        case SEQ_LCD_CHARSET_DrumSymbolsSmall:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_drum_symbols_small);
	  break;
        default:
	  return -1; // charset doesn't exist
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a string
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintString(char *str)
{
  while( *str != '\0' ) {
    if( lcd_cursor_x >= LCD_MAX_COLUMNS )
      break;
    SEQ_LCD_PrintChar(*str);
    ++str;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a formatted string
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintFormattedString(char *format, ...)
{
  char buffer[LCD_MAX_COLUMNS]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return SEQ_LCD_PrintString(buffer);
}


/////////////////////////////////////////////////////////////////////////////
// prints <num> spaces
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintSpaces(u8 num)
{
  do {
    SEQ_LCD_PrintChar(' ');
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
    SEQ_LCD_PrintChar(fill ? ' ' : c);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a vertical bar for a 3bit value
// (1 character)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintVBar(u8 value)
{
  return SEQ_LCD_PrintChar(value);
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
    SEQ_LCD_PrintChar(hbar_table[value][i]);

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
    SEQ_LCD_PrintString("---");
  else {
    u8 octave = 0;

    // determine octave, note contains semitone number thereafter
    while( note >= 12 ) {
      ++octave;
      note -= 12;
    }

    // print semitone (capital letter if octave >= 2)
    SEQ_LCD_PrintChar(octave >= 2 ? (note_tab[note][0] + 'A'-'a') : note_tab[note][0]);
    SEQ_LCD_PrintChar(note_tab[note][1]);

    // print octave
    switch( octave ) {
      case 0:  SEQ_LCD_PrintChar('2'); break; // -2
      case 1:  SEQ_LCD_PrintChar('1'); break; // -1
      default: SEQ_LCD_PrintChar('0' + (octave-2)); // 0..7
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
    SEQ_LCD_PrintString("---");
  else {
    int key_num = (arp >> 2) & 0x3;
    int arp_oct = (arp >> 4) & 0x7;

    if( arp_oct < 2 ) { // Multi Arp
      SEQ_LCD_PrintChar('*');
      arp_oct = ((arp >> 2) & 7) - 4;
    } else {
      SEQ_LCD_PrintChar('1' + key_num);
      arp_oct -= 4;
    }

    if( arp_oct >= 0 )
      SEQ_LCD_PrintFormattedString("+%d", arp_oct);
    else
      SEQ_LCD_PrintFormattedString("-%d", -arp_oct);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the gatelength (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGatelength(u8 len)
{
  if( len <= 96 ) {
    int len_percent = (len*100)/96;
    SEQ_LCD_PrintFormattedString("%3d%%", len_percent);
  } else { // gilde
    SEQ_LCD_PrintString("Gld.");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints selected group/track (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGxTy(u8 group, u8 selected_tracks)
{
  const char selected_tracks_tab[16] = { '-', '1', '2', 'M', '3', 'M', 'M', 'M', '4', 'M', 'M', 'M', 'M', 'M', 'M', 'A' };

  SEQ_LCD_PrintChar('G');
  SEQ_LCD_PrintChar('1' + group);
  SEQ_LCD_PrintChar('T');
  SEQ_LCD_PrintChar(selected_tracks_tab[selected_tracks & 0xf]);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// prints the pattern number (2 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintPattern(seq_pattern_t pattern)
{
  if( pattern.DISABLED ) {
    SEQ_LCD_PrintChar('-');
    SEQ_LCD_PrintChar('-');
  } else {
    SEQ_LCD_PrintChar('A' + pattern.group + (pattern.lower ? 32 : 0));
    SEQ_LCD_PrintChar('1' + pattern.num);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the pattern label as a 15 character string
// The label is located at character position 5..19 of a pattern name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintPatternLabel(seq_pattern_t pattern, char *pattern_name)
{
  // if string only contains spaces, print pattern number instead
  int i;
  u8 found_char = 0;
  for(i=5; i<20; ++i)
    if( pattern_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    SEQ_LCD_PrintStringPadded((char *)&pattern_name[5], 15);
  else {
    SEQ_LCD_PrintString("<Pattern ");
    SEQ_LCD_PrintPattern(pattern);
    SEQ_LCD_PrintChar('>');
    SEQ_LCD_PrintSpaces(3);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the pattern category as a 5 character string
// The category is located at character position 0..4 of a pattern name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintPatternCategory(seq_pattern_t pattern, char *pattern_name)
{
  // if string only contains spaces, print "NoCat" instead
  int i;
  u8 found_char = 0;
  for(i=0; i<5; ++i)
    if( pattern_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    SEQ_LCD_PrintStringPadded((char *)&pattern_name[0], 5);
  else
    SEQ_LCD_PrintString("NoCat");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the track label as a 15 character string
// The label is located at character position 5..19 of a track name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintTrackLabel(u8 track, char *track_name)
{
  // if string only contains spaces, print track number instead
  int i;
  u8 found_char = 0;
  for(i=5; i<20; ++i)
    if( track_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    SEQ_LCD_PrintStringPadded((char *)&track_name[5], 15);
  else {
    // "  USB1 Chn.xx  "
    SEQ_LCD_PrintChar(' ');
    SEQ_LCD_PrintChar(' ');
    SEQ_LCD_PrintMIDIOutPort(SEQ_CC_Get(track, SEQ_CC_MIDI_PORT));
    SEQ_LCD_PrintChar(' ');
    SEQ_LCD_PrintFormattedString("Chn.%2d  ", SEQ_CC_Get(track, SEQ_CC_MIDI_CHANNEL)+1);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the track category as a 5 character string
// The category is located at character position 0..4 of a track name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintTrackCategory(u8 track, char *track_name)
{
  // if string only contains spaces, print "NoCat" instead
  int i;
  u8 found_char = 0;
  for(i=0; i<5; ++i)
    if( track_name[i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    SEQ_LCD_PrintStringPadded((char *)&track_name[0], 5);
  else
    SEQ_LCD_PrintString("NoCat");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the drum instrument as a 5 character string
// The drum name is located at character position drum * (0..4) of a track name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintTrackDrum(u8 track, u8 drum, char *track_name)
{
  // if string only contains spaces, print "NoCat" instead
  int i;
  u8 found_char = 0;
  for(i=0; i<5; ++i)
    if( track_name[5*drum+i] != ' ' ) {
      found_char = 1;
      break;
    }

  if( found_char )
    SEQ_LCD_PrintStringPadded((char *)&track_name[5*drum], 5);
  else {
    SEQ_LCD_PrintFormattedString("Drm%c ", 'A'+drum);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints MIDI In port (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintMIDIInPort(mios32_midi_port_t port)
{
  return SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(port)));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints MIDI Out port (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintMIDIOutPort(mios32_midi_port_t port)
{
  return SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(port)));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints step view (6 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintStepView(u8 step_view)
{

  SEQ_LCD_PrintFormattedString("S%2d-%2d", (step_view*16)+1, (step_view+1)*16);

  return 0; // no error
}
