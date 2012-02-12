// $Id$
/*
 * LCD utility functions
 *
 * The 2x80 screen is buffered and can be output over multiple LCDs 
 * (e.g. 2 * 2x40, but also 4 * 2x20)
 *
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_midi_port.h"
#include "seq_midi_sysex.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_layer.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// can be overruled in mios32_config.h
#ifndef LCD_NUM_DEVICES
# define LCD_NUM_DEVICES          2
#endif

#ifndef LCD_COLUMNS_PER_DEVICE
# define LCD_COLUMNS_PER_DEVICE  40
#endif


// shouldn't be overruled
#define LCD_MAX_LINES    2
#define LCD_MAX_COLUMNS  (LCD_NUM_DEVICES*LCD_COLUMNS_PER_DEVICE)



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 charset_menu[64] = {
  0x01, 0x03, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, // left-arrow
  0x00, 0x00, 0x00, 0x10, 0x18, 0x1c, 0x18, 0x10, // right-arrow
  0x01, 0x03, 0x07, 0x13, 0x19, 0x1c, 0x18, 0x10, // left/right arrow
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
};

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
// Display Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_Init(u32 mode)
{
#if 0
  // obsolete
  // now done in main.c

  u8 dev;

  // first LCD already initialized
  for(dev=1; dev<LCD_NUM_DEVICES; ++dev) {
    MIOS32_LCD_DeviceSet(dev);
    MIOS32_LCD_Init(0);
  }

  // switch back to first LCD
  MIOS32_LCD_DeviceSet(0);
#endif

  return 0; // no error
}


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
  int remote_first_x[LCD_MAX_LINES];
  int remote_last_x[LCD_MAX_LINES];
  int x, y;

  for(y=0; y<2; ++y) {
    remote_first_x[y] = -1;
    remote_last_x[y] = -1;
  }

  MUTEX_LCD_TAKE;

  u8 *ptr = (u8 *)lcd_buffer;
  for(y=0; y<LCD_MAX_LINES; ++y)
    for(x=0; x<LCD_MAX_COLUMNS; ++x) {

      if( force || !(*ptr & 0x80) ) {
	if( remote_first_x[y] == -1 )
	  remote_first_x[y] = x;
	remote_last_x[y] = x;

	if( x != next_x || y != next_y ) {
	  MIOS32_LCD_DeviceSet(x / LCD_COLUMNS_PER_DEVICE);
	  MIOS32_LCD_CursorSet(x % LCD_COLUMNS_PER_DEVICE, y);
	}
	MIOS32_LCD_PrintChar(*ptr & 0x7f);

	MIOS32_IRQ_Disable(); // must be atomic
	*ptr |= 0x80;
	MIOS32_IRQ_Enable();

	next_y = y;
	next_x = x+1;

	// for multiple LCDs: ensure that cursor is set when we reach the next partition
	if( (next_x % LCD_COLUMNS_PER_DEVICE) == 0 )
	  next_x = -1;
      }
      ++ptr;
    }

  MUTEX_LCD_GIVE;

  // forward display changes to remote client
  if( seq_midi_sysex_remote_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER || seq_midi_sysex_remote_active_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER ) {
    for(y=0; y<LCD_MAX_LINES; ++y)
      if( remote_first_x[y] >= 0 )
	SEQ_MIDI_SYSEX_REMOTE_Server_SendLCD(remote_first_x[y],
					     y,
					     (u8 *)&lcd_buffer[y][remote_first_x[y]],
					     remote_last_x[y]-remote_first_x[y]+1);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// initialise character set (if not already active)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_InitSpecialChars(seq_lcd_charset_t charset)
{
  static seq_lcd_charset_t current_charset = SEQ_LCD_CHARSET_None;
  s32 status = 0;

  if( charset != current_charset ) {
    current_charset = charset;

    MUTEX_LCD_TAKE;
    int dev;
    for(dev=0; dev<LCD_NUM_DEVICES; ++dev) {
      MIOS32_LCD_DeviceSet(dev);
      switch( charset ) {
        case SEQ_LCD_CHARSET_Menu:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_menu);
	  break;
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
	  status = -1; // charset doesn't exist
      }
    }

    MUTEX_LCD_GIVE;

    // forward charset change to remote client
    if( seq_midi_sysex_remote_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER )
      SEQ_MIDI_SYSEX_REMOTE_Server_SendCharset(charset);
  }

  return status; // no error
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
s32 SEQ_LCD_PrintSpaces(int num)
{
  while( num > 0 ) {
    SEQ_LCD_PrintChar(' ');
    --num;
  }

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
// prints a long horizontal bar for a 7bit value
// (32 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintLongHBar(u8 value)
{
  int i;

  const u8 hbar_table[4] = { 1, 2, 3, 3 };

  for(i=0; i<32; ++i) {
    if( (value/4) < i )
      SEQ_LCD_PrintChar(' ');
    else if( (value/4) > i )
      SEQ_LCD_PrintChar(3);
    else
      SEQ_LCD_PrintChar(hbar_table[value%4]);
  }

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
    // determine octave, note contains semitone number thereafter
    u8 octave = note / 12;
    note %= 12;

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
  if( len < 96 ) {
    int len_percent = (len*100)/96;
    SEQ_LCD_PrintFormattedString("%3d%%", len_percent);
  } else { // gilde
    SEQ_LCD_PrintString("Gld.");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints the probability value (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintProbability(u8 probability)
{
  return SEQ_LCD_PrintFormattedString("%3d%%", probability);
}


/////////////////////////////////////////////////////////////////////////////
// prints the step delay value (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintStepDelay(s32 delay)
{
  return SEQ_LCD_PrintFormattedString("%3d ", delay);
}


/////////////////////////////////////////////////////////////////////////////
// prints the roll mode (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintRollMode(u8 roll_mode)
{
  if( roll_mode == 0 )
    return SEQ_LCD_PrintString("----");

  return SEQ_LCD_PrintFormattedString("%d%c%02d", 
				      ((roll_mode & 0x30)>>4) + 2,
				      (roll_mode & 0x40) ? 'U' : 'D',
				      roll_mode&0xf);
}


/////////////////////////////////////////////////////////////////////////////
// prints the roll2 mode (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintRoll2Mode(u8 roll2_mode)
{
  if( roll2_mode == 0 )
    return SEQ_LCD_PrintString("----");

  int gatelength = (4 - (roll2_mode >> 5)) * ((roll2_mode&0x1f)+1);

  if( gatelength > 96 ) {
    return SEQ_LCD_PrintFormattedString("%dx++", (roll2_mode>>5) + 2);
  }

  return SEQ_LCD_PrintFormattedString("%dx%02d", (roll2_mode>>5) + 2, gatelength);
}


/////////////////////////////////////////////////////////////////////////////
// prints event type of MIDI package with given number of chars
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintEvent(mios32_midi_package_t package, u8 num_chars)
{
  // currently only 5 chars supported...
  if( package.type == 0xf || package.evnt0 >= 0xf8 ) {
    switch( package.evnt0 ) {
      case 0xf8: SEQ_LCD_PrintString(" CLK "); break;
      case 0xfa: SEQ_LCD_PrintString("START"); break;
      case 0xfb: SEQ_LCD_PrintString("CONT."); break;
      case 0xfc: SEQ_LCD_PrintString("STOP "); break;
      default:
	SEQ_LCD_PrintFormattedString(" %02X  ", package.evnt0);
    }
  } else if( package.type < 8 ) {
    SEQ_LCD_PrintString("SysEx");
  } else {
    switch( package.event ) {
      case NoteOff:
      case NoteOn:
      case PolyPressure:
      case CC:
      case ProgramChange:
      case Aftertouch:
      case PitchBend:
	// could be enhanced later
        SEQ_LCD_PrintFormattedString("%02X%02X ", package.evnt0, package.evnt1);
	break;

      default:
	// print first two bytes for unsupported events
        SEQ_LCD_PrintFormattedString("%02X%02X ", package.evnt0, package.evnt1);
    }
  }

  // TODO: enhanced messages
  if( num_chars > 5 )
    SEQ_LCD_PrintSpaces(num_chars-5);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints layer event
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintLayerEvent(u8 track, u8 step, u8 par_layer, u8 instrument, u8 step_view, int print_edit_value)
{
  seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(track, par_layer);
  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  seq_layer_evnt_t layer_event;
  SEQ_LAYER_GetEvntOfLayer(track, step, par_layer, instrument, &layer_event);

  // TODO: tmp. solution to print chord velocity correctly
  if( layer_type == SEQ_PAR_Type_Velocity && (seq_cc_trk[track].link_par_layer_chord == 0) )
    layer_type = SEQ_PAR_Type_Chord;

  switch( layer_type ) {
  case SEQ_PAR_Type_None:
    SEQ_LCD_PrintString("None");
    break;

  case SEQ_PAR_Type_Note:
  case SEQ_PAR_Type_Velocity: {
    u8 note = (print_edit_value >= 0) ? print_edit_value : layer_event.midi_package.note;
    if( step_view ) {
      if( layer_event.midi_package.note &&
	  (print_edit_value >= 0 || (layer_event.midi_package.velocity && SEQ_TRG_GateGet(track, step, instrument))) ) {
	if( SEQ_CC_Get(track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator )
	  SEQ_LCD_PrintArp(note);
	else
	  SEQ_LCD_PrintNote(note);
	SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
      } else {
	SEQ_LCD_PrintString("----");
      }
    } else {
      if( layer_type == SEQ_PAR_Type_Note ) {
	if( note ) {
	  if( SEQ_CC_Get(track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator )
	    SEQ_LCD_PrintArp(note);
	  else
	    SEQ_LCD_PrintNote(note);
	  SEQ_LCD_PrintChar(' ');
	} else {
	  SEQ_LCD_PrintString("----");
	}
      } else {
	SEQ_LCD_PrintFormattedString("%3d", layer_event.midi_package.velocity);
	SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
      }
    }
  } break;

  case SEQ_PAR_Type_Chord: {
    u8 par_value;
    // more or less dirty - a velocity layer can force SEQ_PAR_Type_Chord
    if( SEQ_PAR_AssignmentGet(track, par_layer) == SEQ_PAR_Type_Velocity )
      par_value = SEQ_PAR_ChordGet(track, step, instrument, 0x0000);
    else
      par_value = (print_edit_value >= 0) ? print_edit_value : SEQ_PAR_Get(track, step, par_layer, instrument);

    if( par_value && (print_edit_value >= 0 || SEQ_TRG_GateGet(track, step, instrument)) ) {
      u8 chord_ix = par_value & 0x1f;
      u8 chord_char = ((chord_ix >= 0x10) ? 'a' : 'A') + (chord_ix & 0xf);
      u8 chord_oct = par_value >> 5;
      SEQ_LCD_PrintFormattedString("%c/%d", chord_char, chord_oct);
      SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
    } else {
      SEQ_LCD_PrintString("----");
    }
  } break;
	  
  case SEQ_PAR_Type_Length:
    SEQ_LCD_PrintGatelength(layer_event.len);
    break;

  case SEQ_PAR_Type_CC:
  case SEQ_PAR_Type_ProgramChange:
  case SEQ_PAR_Type_PitchBend: {
    if( event_mode == SEQ_EVENT_MODE_CC && !SEQ_TRG_GateGet(track, step, instrument) ) {
      SEQ_LCD_PrintString("----");
    } else {
      u8 value = (layer_type == SEQ_PAR_Type_ProgramChange) ? layer_event.midi_package.evnt1 : layer_event.midi_package.value;
      SEQ_LCD_PrintFormattedString("%3d", value);
      SEQ_LCD_PrintVBar(value >> 4);
    }
  } break;

  case SEQ_PAR_Type_Probability:
    SEQ_LCD_PrintProbability(SEQ_PAR_ProbabilityGet(track, step, instrument, 0x0000));
    break;

  case SEQ_PAR_Type_Delay:
    SEQ_LCD_PrintStepDelay(SEQ_PAR_StepDelayGet(track, step, instrument, 0x0000));
    break;

  case SEQ_PAR_Type_Roll:
    SEQ_LCD_PrintRollMode(SEQ_PAR_RollModeGet(track, step, instrument, 0x0000));
    break;

  case SEQ_PAR_Type_Roll2:
    SEQ_LCD_PrintRoll2Mode(SEQ_PAR_Roll2ModeGet(track, step, instrument, 0x0000));
    break;

  default:
    SEQ_LCD_PrintString("????");
    break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// prints selected group/track (4 characters)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintGxTy(u8 group, u16 selected_tracks)
{
  const char selected_tracks_tab[16] = { '-', '1', '2', 'M', '3', 'M', 'M', 'M', '4', 'M', 'M', 'M', 'M', 'M', 'M', 'A' };
  u8 track4 = (selected_tracks >> (group*4)) & 0x0f;

  SEQ_LCD_PrintChar('G');
  SEQ_LCD_PrintChar('1' + group);
  SEQ_LCD_PrintChar('T');
  SEQ_LCD_PrintChar(selected_tracks_tab[track4 & 0xf]);

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


/////////////////////////////////////////////////////////////////////////////
// Help function to print a list (e.g. directory)
// Each item has 9 characters maximum (no termination required if 9 chars)
// Prints item_width * num_items + (num_items-1) characters
// (last space is left out so that arrows can be displayed at the end of list)
// *list: pointer to list array
// item_width: maximum width of item (defines also the string length!)
// max_items_on_screen: how many items are displayed on screen?
//
// A space is added at the end of each item. The last item gets
// a page scroll marker (< or > or <>)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LCD_PrintList(char *list, u8 item_width, u8 num_items, u8 max_items_on_screen, u8 selected_item_on_screen, u8 view_offset)
{
  int item;

  for(item=0; item<max_items_on_screen; ++item) {
    char *list_item = (char *)&list[item*item_width];
    int len, pos;

    for(len=0; len<item_width; ++len)
      if( list_item[len] == 0 )
	break;

    if( item == selected_item_on_screen && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(item_width);
    else {
      int centered_offset = ((item_width+1)-len)/2;

      if( centered_offset )
	SEQ_LCD_PrintSpaces(centered_offset);

      for(pos=0; pos<len; ++pos)
	SEQ_LCD_PrintChar(list_item[pos]);

      centered_offset = item_width-centered_offset-len;
      if( centered_offset > 0 )
	SEQ_LCD_PrintSpaces(centered_offset);
    }

    if( item < (max_items_on_screen-1) )
      SEQ_LCD_PrintChar(' ');
    else {
      if( view_offset == 0 && selected_item_on_screen == 0 )
	SEQ_LCD_PrintChar(0x01); // right arrow
      else if( (view_offset+selected_item_on_screen+1) >= num_items )
	SEQ_LCD_PrintChar(0x00); // left arrow
      else
	SEQ_LCD_PrintChar(0x02); // left/right arrow
    }
  }

  return 0; // no error
}
