// $Id$
/*
 * Keyboard handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

#include "keyboard.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// scan 16 rows for up to 8*16 contacts
#define MATRIX_NUM_ROWS 16

// maximum number of supported contacts
#define KEYBOARD_NUM_PINS (KEYBOARD_NUM*8*16)

// for FantomXR's Yamaha keyboard - currently only a hardcoded option
#define FANTOM_XR_VARIANT 0

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

keyboard_config_t keyboard_config[KEYBOARD_NUM];


/////////////////////////////////////////////////////////////////////////////
// Local structures
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 connected_keyboards_num;

static u16 din_value[KEYBOARD_NUM][MATRIX_NUM_ROWS];
static u16 din_value_changed[KEYBOARD_NUM][MATRIX_NUM_ROWS];

// for velocity
static u16 timestamp;
static u16 din_activated_timestamp[KEYBOARD_NUM][KEYBOARD_NUM_PINS];

// for debouncing each pin has a flag which notifies if the Note On/Off value has already been sent
#if (KEYBOARD_NUM_PINS % 8)
# error "KEYBOARD_NUM_PINS must be dividable by 8!"
#endif
static u8 key_note_on_sent[KEYBOARD_NUM][KEYBOARD_NUM_PINS / 8];
static u8 key_note_off_sent[KEYBOARD_NUM][KEYBOARD_NUM_PINS / 8];

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 KEYBOARD_MIDI_SendNote(u8 kb, u8 note_number, u8 velocity);
static s32 KEYBOARD_MIDI_SendCtrl(u8 kb, u8 ctrl_number, u32 ain_value);
static char *KEYBOARD_GetNoteName(u8 note, char str[4]);


/////////////////////////////////////////////////////////////////////////////
// Initialize the keyboard handler
// mode == 0: init configuration + runtime variables
// mode > 0: init only runtime variables
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_Init(u32 mode)
{
  u8 init_configuration = mode == 0;

  connected_keyboards_num = 0;

  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
    if( init_configuration ) {
      kc->midi_ports = 0x1011; // OSC1, OUT1 and USB1
      kc->midi_chn = kb+1;
      kc->note_offset = 36;

      kc->delay_fastest = 100;
      kc->delay_slowest = 2000;

      // set low verbose level by default (only warnings)
      kc->verbose_level = 1;

      kc->scan_velocity = 1;
      kc->scan_optimized = 0;

      if( kb == 0 ) {
	kc->num_rows = 8;
	kc->dout_sr1 = 1;
	kc->dout_sr2 = 2; // with num_rows<=8 the SR2 will mirror SR1
	kc->din_sr1 = 1;
	kc->din_sr2 = 2;
	kc->din_key_offset = 32;
	kc->din_inverted = 0;
	kc->ain_pitchwheel = 0;
	kc->ctrl_pitchwheel = 0x80;
	kc->ain_modwheel = 0;
	kc->ctrl_modwheel = 1;
      } else {
	kc->num_rows = 0;
	kc->dout_sr1 = 0;
	kc->dout_sr2 = 0;
	kc->din_sr1 = 0;
	kc->din_sr2 = 0;
	kc->din_key_offset = 32;
	kc->din_inverted = 0;
	kc->ain_pitchwheel = 0;
	kc->ctrl_pitchwheel = 0x80;
	kc->ain_modwheel = 0;
	kc->ctrl_modwheel = 1;
      }
    }

    // runtime variables:
    kc->selected_row = 0;
    kc->prev_row = 0xff;

    if( kc->num_rows )
      connected_keyboards_num = kb + 1;

    // initialize DIN arrays
    int row;
    u16 inversion = kc->din_inverted ? 0xffff : 0x0000;
    for(row=0; row<MATRIX_NUM_ROWS; ++row) {
      din_value[kb][row] = 0xffff ^ inversion; // default state: buttons depressed
      din_value_changed[kb][row] = 0x0000;
    }

    // initialize timestamps
    int i;
    timestamp = 0;
    for(i=0; i<KEYBOARD_NUM_PINS; ++i) {
      din_activated_timestamp[kb][i] = 0;
    }

    for(i=0; i<KEYBOARD_NUM_PINS/8; ++i) {
      key_note_on_sent[kb][i] = 0x00;
      key_note_off_sent[kb][i] = 0x00;
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Sets/Gets number of connected keyboards
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_ConnectedNumSet(u8 num)
{
  if( num > KEYBOARD_NUM )
     num = KEYBOARD_NUM;

  connected_keyboards_num = num;

  return 0; // no error
}

u8 KEYBOARD_ConnectedNumGet(void)
{
  return connected_keyboards_num;
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SRIO_ServicePrepare(void)
{
  // increment timestamp for velocity delay measurements
  ++timestamp;

  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<connected_keyboards_num; ++kb, ++kc) {

    // optional scan optimization for break/make: if break not active, we don't need to scan make
    if( kc->scan_velocity && !kc->break_inverted && kc->scan_optimized ) {
      u8 skip_make = 0;

      if( kc->prev_row & 1 ) { // last scan was break contact
	if( (!kc->din_inverted && din_value[kb][kc->prev_row] == 0xffff) ||
	    ( kc->din_inverted && din_value[kb][kc->prev_row] == 0x0000) ) {
	  // skip scan of make contacts
	  skip_make = 1;
	}
      }

      // if scan optimization active: scan break before make
      if( (kc->selected_row & 1) ) { // break
	if( skip_make ) {
	  if( (kc->selected_row += 2) >= kc->num_rows ) // switch to next break
	    kc->selected_row = 1; // restart at first break
	} else {
	  kc->selected_row -= 1; // switch to make
	}
      } else {
	if( (kc->selected_row+=3) >= kc->num_rows ) {
	  kc->selected_row = 1; // restart at first break
	}
      }
    } else {
      // select next column, wrap at num_rows
      if( ++kc->selected_row >= kc->num_rows ) {
	kc->selected_row = 0;
      }
    }

    // selection pattern (active selection is 0, all other outputs 1)
    u16 selection_mask = ~(1 << (u16)kc->selected_row);
    if( kc->din_inverted )
      selection_mask ^= 0xffff;

    if( kc->dout_sr1 )
      MIOS32_DOUT_SRSet(kc->dout_sr1-1, selection_mask >> 0);

    // mirror selection if <= 8 rows for second SR, otherwise output remaining 8 selection lines
    if( kc->dout_sr2 )
      MIOS32_DOUT_SRSet(kc->dout_sr2-1, selection_mask >> ((kc->num_rows <= 8) ? 0 : 8));
  }
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_SRIO_ServiceFinish(void)
{
  // check DINs
  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<connected_keyboards_num; ++kb, ++kc) {
    u16 sr_value = 0;

    // the DIN scan was done with previous row selection, not the current one:
    int prev_row = kc->prev_row;

    // store new previous row for next scan
    kc->prev_row = kc->selected_row;

    if( prev_row == 0xff ) // not scanned yet
      continue;

    if( kc->din_sr1 ) {
      MIOS32_DIN_SRChangedGetAndClear(kc->din_sr1-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= MIOS32_DIN_SRGet(kc->din_sr1-1);
    } else {
      sr_value |= 0x00ff;
    }

    if( kc->din_sr2 ) {
      MIOS32_DIN_SRChangedGetAndClear(kc->din_sr2-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value |= (u16)MIOS32_DIN_SRGet(kc->din_sr2-1) << 8;
    } else {
      sr_value |= 0xff00;
    }

    if( kc->din_inverted )
      sr_value ^= 0xffff;

    // determine pin changes
    u16 changed = sr_value ^ din_value[kb][prev_row];

    if( changed ) {
      // add them to existing notifications
      din_value_changed[kb][prev_row] |= changed;

      // store new value
      din_value[kb][prev_row] = sr_value;

      // number of pins per row depends on assigned DINs:
      int pins_per_row = kc->din_sr2 ? 16 : 8;

      // store timestamp for changed pin on 1->0 transition
      u8 sr_pin;
      u16 mask = 0x01;
      for(sr_pin=0; sr_pin<pins_per_row; ++sr_pin, mask <<= 1) {
	if( (changed & mask) && !(sr_value & mask) ) {
	  din_activated_timestamp[kb][prev_row*MATRIX_NUM_ROWS + sr_pin] = timestamp;
	}
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// will be called on pin changes
/////////////////////////////////////////////////////////////////////////////
static void KEYBOARD_NotifyToggle(u8 kb, u8 row, u8 column, u8 depressed)
{
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];
  char note_str[4];

  // Display status of all rows
  if( kc->verbose_level >= 2 ) {
    DEBUG_MSG("---\n");
    int i;    
    for(i=0; i<MATRIX_NUM_ROWS; ++i) {
      int v = ~din_value[kb][i];
      DEBUG_MSG("DOUT SR%d.%d:  %c%c%c%c%c%c%c%c  %c%c%c%c%c%c%c%c\n",
		(i / 8)+1,
		7 - (i % 8),
		(v & 0x0001) ? '1' : '0',
		(v & 0x0002) ? '1' : '0',
		(v & 0x0004) ? '1' : '0',
		(v & 0x0008) ? '1' : '0',
		(v & 0x0010) ? '1' : '0',
		(v & 0x0020) ? '1' : '0',
		(v & 0x0040) ? '1' : '0',
		(v & 0x0080) ? '1' : '0',
		(v & 0x0100) ? '1' : '0',
		(v & 0x0200) ? '1' : '0',
		(v & 0x0400) ? '1' : '0',
		(v & 0x0800) ? '1' : '0',
		(v & 0x1000) ? '1' : '0',
		(v & 0x2000) ? '1' : '0',
		(v & 0x4000) ? '1' : '0',
		(v & 0x8000) ? '1' : '0'
		);
    }
  }

#if FANTOM_XR_VARIANT
  // determine key number based on row/column
  int key = row*6 + (column % 8); // (6 keys are connected to each DIN)

  // ensure valid range
  if( key > 127 )
    key = 127;

  // determine note number (here we could insert an octave shift)
  int note_number = key + kc->note_offset;

  u8 break_contact = column < 8;
  int pin_break = row*MATRIX_NUM_ROWS + ((column % 8) + 0);
  int pin_make  = row*MATRIX_NUM_ROWS + ((column % 8) + 8);

  //DEBUG_MSG("key:%d %c note:%d break:%d pin_make:%d pin_break:%d\n", key, depressed ? 'o' : '*', note_number, break_contact, pin_make, pin_break);
#else
  // determine key number based on row/column
  int key;
  if( kc->scan_velocity ) {
    key = ((column >= 8) ? kc->din_key_offset : 0) + 8*(row/2) + (column % 8);
  } else {
    key = ((column >= 8) ? kc->din_key_offset : 0) + 8*(row) + (column % 8);
  }

  // ensure valid range
  if( key > 127 )
    key = 127;

  // determine note number (here we could insert an octave shift)
  int note_number = key + kc->note_offset;

  // check if key is assigned to an "break contact"
  // and determine reference to break and make pin (valid when make pin active)
  u8 break_contact = (row & 1); // odd numbers
  int pin_make, pin_break;
  if( kc->scan_velocity ) {
    break_contact = (row & 1); // odd numbers
    pin_make  = (row)*MATRIX_NUM_ROWS + column;
    pin_break = (row+1)*MATRIX_NUM_ROWS + column;
  } else {
    break_contact = 0;
    pin_make  = (row)*MATRIX_NUM_ROWS + column;
    pin_break = pin_make; // just use the same pin...
  }
#endif

  // break pin inverted?
  if( break_contact && kc->scan_velocity && kc->break_inverted )
    depressed = depressed ? 0 : 1;

  // ensure valid note range
  if( note_number > 127 )
    note_number = 127;
  else if( note_number < 0 )
    note_number = 0;

  if( kc->verbose_level >= 2 )
    DEBUG_MSG("KB%d: DOUT#%d.D%d / DIN#%d.D%d: %s  -->  key=%2d, %s, note=%s (%d)\n",
	      kb+1,
	      (row < 8) ? kc->dout_sr1 : kc->dout_sr2, 7 - (row % 8),
	      (column < 8) ? kc->din_sr1 : kc->din_sr2, column % 8,
	      depressed ? "depressed" : "pressed  ",
	      key,
	      break_contact ? "break contact" : "make contact",
	      KEYBOARD_GetNoteName(note_number, note_str), note_number);

  // determine key mask and pointers for access to combined arrays
  u8 key_mask = (1 << (key % 8));
  u8 *note_on_sent = (u8 *)&key_note_on_sent[kb][key / 8];
  u8 *note_off_sent = (u8 *)&key_note_off_sent[kb][key / 8];


  if( kc->scan_velocity ) {
    // break contacts don't send MIDI notes, but they are used for delay measurements,
    // and they release the Note On/Off debouncing mechanism
    if( break_contact ) {
      if( depressed ) {
#if FANTOM_XR_VARIANT
	// for this variant we have to play Note Off when Break is released, because make is bouncing
	*note_off_sent |= key_mask;

	if( kc->verbose_level >= 2 )
	  DEBUG_MSG("DEPRESSED note=%s\n", KEYBOARD_GetNoteName(note_number, note_str));

	KEYBOARD_MIDI_SendNote(kb, note_number, 0x00);
#endif
	*note_on_sent &= ~key_mask;
	*note_off_sent &= ~key_mask;
      }
      return;
    }
  } else {
    // no velocity: no debouncing yet, we could use the timestamp...
  }

  // branch depending on pressed or released key
  if( depressed ) {
    if( !kc->scan_velocity || !(*note_off_sent & key_mask) ) {
#if !FANTOM_XR_VARIANT
      // (in FANTOM_XR variant the note off is played with depressed break)
      *note_off_sent |= key_mask;

      if( kc->verbose_level >= 2 )
	DEBUG_MSG("DEPRESSED note=%s\n", KEYBOARD_GetNoteName(note_number, note_str));

      KEYBOARD_MIDI_SendNote(kb, note_number, 0x00);
#endif      
    }

  } else {

    if( !kc->scan_velocity || !(*note_on_sent & key_mask) ) {
      *note_on_sent |= key_mask;

      int velocity = 127;

      if( !kc->scan_velocity ) {
	if( kc->verbose_level >= 2 )
	  DEBUG_MSG("PRESSED note=%s, velocity=%d\n", KEYBOARD_GetNoteName(note_number, note_str), velocity);
      } else {
	// determine timestamps between break and make contact
	u16 timestamp_break = din_activated_timestamp[kb][pin_break];
	u16 timestamp_make  = din_activated_timestamp[kb][pin_make];

	// and the delta delay (IMPORTANT: delay variable needs same resolution like timestamps to handle overrun correctly!)
	s16 delay = timestamp_make - timestamp_break;

	if( delay > kc->delay_fastest ) {
	  // determine velocity depending on delay
	  // lineary scaling - here we could also apply a curve table
	  velocity = 127 - (((delay-kc->delay_fastest) * 127) / (kc->delay_slowest-kc->delay_fastest));
	  // saturate to ensure that range 1..127 won't be exceeded
	  if( velocity < 1 )
	    velocity = 1;
	  if( velocity > 127 )
	    velocity = 127;
	}

	if( kc->verbose_level >= 2 )
	  DEBUG_MSG("PRESSED note=%s, delay=%d, velocity=%d\n", KEYBOARD_GetNoteName(note_number, note_str), delay, velocity);
      }

      KEYBOARD_MIDI_SendNote(kb, note_number, velocity);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called periodically (each mS) to check for pin changes
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_Periodic_1mS(void)
{
  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<connected_keyboards_num; ++kb, ++kc) {
    // number of pins per row depends on assigned DINs:
    int pins_per_row = kc->din_sr2 ? 16 : 8;

    int row;
    for(row=0; row<kc->num_rows; ++row) {
      // check if there are pin changes - must be atomic!
      MIOS32_IRQ_Disable();
      u16 changed = din_value_changed[kb][row];
      din_value_changed[kb][row] = 0;
      MIOS32_IRQ_Enable();

      // any pin change at this SR?
      if( !changed )
	continue;

      // check the 16 captured pins of the two SRs
      int sr_pin;
      u16 mask = 0x01;
      for(sr_pin=0; sr_pin<pins_per_row; ++sr_pin, mask <<= 1)
	if( changed & mask )
	  KEYBOARD_NotifyToggle(kb, row, sr_pin, (din_value[kb][row] & mask) ? 1 : 0);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from AIN_NotifyChange in app.c
/////////////////////////////////////////////////////////////////////////////
void KEYBOARD_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<connected_keyboards_num; ++kb, ++kc) {
    if( pin == ((int)kc->ain_pitchwheel - 1) )
      KEYBOARD_MIDI_SendCtrl(kb, kc->ctrl_pitchwheel, pin_value);

    if( pin == ((int)kc->ain_modwheel - 1) )
      KEYBOARD_MIDI_SendCtrl(kb, kc->ctrl_modwheel, pin_value);
  }
}


/////////////////////////////////////////////////////////////////////////////
// Help function to send a MIDI note over given ports
/////////////////////////////////////////////////////////////////////////////
static s32 KEYBOARD_MIDI_SendNote(u8 kb, u8 note_number, u8 velocity)
{
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];

  if( kc->midi_chn ) {
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( kc->midi_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
	MIOS32_MIDI_SendNoteOn(port, kc->midi_chn-1, note_number, velocity);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to send a MIDI controller over given ports
/////////////////////////////////////////////////////////////////////////////
static s32 KEYBOARD_MIDI_SendCtrl(u8 kb, u8 ctrl_number, u32 ain_value)
{
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];

  int value7bit = ain_value >> 5;

  if( kc->midi_chn ) {
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( kc->midi_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);

	if( ctrl_number < 128 )
	  MIOS32_MIDI_SendCC(port, kc->midi_chn-1, ctrl_number, value7bit);
	else if( ctrl_number == 128 ) {
	  u16 pb_value = (value7bit == 0x40) ? 0x2000 : ((value7bit << 7) | value7bit);
	  MIOS32_MIDI_SendPitchBend(port, kc->midi_chn-1, pb_value);
	} else if( ctrl_number == 129 ) {
	  MIOS32_MIDI_SendAftertouch(port, kc->midi_chn-1, value7bit);
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to put the note name into a string (buffer has 3 chars + terminator)
/////////////////////////////////////////////////////////////////////////////
static char *KEYBOARD_GetNoteName(u8 note, char str[4])
{
  const char note_tab[12][3] = { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };

  // determine octave, note contains semitone number thereafter
  int octave = note / 12;
  note %= 12;
        
  str[0] = octave >= 2 ? (note_tab[note][0] + 'A'-'a') : note_tab[note][0];
  str[1] = note_tab[note][1];

  switch( octave ) {
  case 0:  str[2] = '2'; break; // -2
  case 1:  str[2] = '1'; break; // -1
  default: str[2] = '0' + (octave-2); // 0..7
  }

  str[3] = 0;

  return str;
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 || strcmp(word, "1") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 || strcmp(word, "0") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  keyboard <1|2> (or kb <1|2>):     print current configuration of given keyboard number");
  out("  set kb <1|2> debug <on|off>:      enables/disables debug mode (not stored in EEPROM)");
  out("  set kb <1|2> midi_ports <ports>   selects the MIDI ports (values: see documentation)");
  out("  set kb <1|2> midi_chn <0-16>      selects the MIDI channel (0=off)");
  out("  set kb <1|2> note_offset <0-127>  selects the note offset (transpose)");
  out("  set kb <1|2> rows <0-%d>:         how many rows should be scanned? (0=off)", MATRIX_NUM_ROWS);
  out("  set kb <1|2> velocity <on|off>:   keyboard supports break and make contacts?");
  out("  set kb <1|2> optimized <on|off>:  make contacts only scanned if break contacts activated");
  out("  set kb <1|2> dout_sr1 <0-%d>:     selects first DOUT shift register (0=off)", MIOS32_SRIO_ScanNumGet());
  out("  set kb <1|2> dout_sr2 <0-%d>:     selects second DOUT shift register (0=off)", MIOS32_SRIO_ScanNumGet());
  out("  set kb <1|2> din_sr1 <0-%d>:      selects first DIN shift register (0=off)", MIOS32_SRIO_ScanNumGet());
  out("  set kb <1|2> din_sr2 <0-%d>:      selects second DIN shift register (0=off)", MIOS32_SRIO_ScanNumGet());
  out("  set kb <1|2> din_key_offset <0-127> selects the key offset between DIN1 and DIN2");
  out("  set kb <1|2> din_inverted <on|off>: DINs inverted?");
  out("  set kb <1|2> break_inverted <on|off>: Only break contacts inverted?");
  out("  set kb <1|2> delay_fastest <0-65535>: fastest delay for velocity calculation");
  out("  set kb <1|2> delay_slowest <0-65535>: slowest delay for velocity calculation");
  out("  set kb <1|2> ain_pitchwheel <0..5> or off: assigns pitchwheel to given J5.A<0..5> pin");
  out("  set kb <1|2> ctrl_pitchwheel <0-129>: assigns CC/PB(=128)/AT(=129) to PitchWheel");
  out("  set kb <1|2> ain_modwheel <0..5> or off: assigns modwheel to given J5.A<0..5> pin");
  out("  set kb <1|2> ctrl_modwheel <0-129>:   assigns CC/PB(=128)/AT(=129) to ModWheel");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  u8 input_line_parsed = 1;
  int input_len = strlen(input);

  if( !(parameter = strtok_r(input, separators, &brkt)) ) {
    input_line_parsed = 0; // input line has to be restored
  } else {
    if( strcmp(parameter, "kb") == 0 || strcmp(parameter, "keyboard") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Missing keyboard number (1..%d)!", KEYBOARD_NUM);
	return 1; // command taken
      }

      int kb = get_dec(parameter);
      if( kb < 1 || kb > KEYBOARD_NUM) {
	out("Invalid Keyboard number specified as first parameter (expecting kb 1..%d)!", KEYBOARD_NUM);
	return 1; // command taken
      }
      KEYBOARD_TerminalPrintConfig(kb-1, _output_function);
    } else if( strcmp(parameter, "set") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Missing parameter after 'set'!");
	return 1; // command taken
      }

      if( strcmp(parameter, "kb") != 0 && strcmp(parameter, "keyboard") != 0 ) {
	input_line_parsed = 0; // input line has to be restored
      } else {
	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing keyboard number (1..%d)!", KEYBOARD_NUM);
	  return 1; // command taken
	}

	int kb = get_dec(parameter);
	if( kb < 1 || kb > KEYBOARD_NUM) {
	  out("Invalid Keyboard number specified as first parameter (expecting kb 1..%d)!", KEYBOARD_NUM);
	  return 1; // command taken
	}

	kb-=1; // the user counts from 1
	keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];

	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing parameter name and value after 'set kb %d'!", kb+1);
	  return 1; // command taken
	}

	/////////////////////////////////////////////////////////////////////
	if( strcmp(parameter, "midi_ports") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the MIDI port selection mask!");
	    return 1; // command taken
	  }

	  int mask = get_dec(parameter);

	  if( mask < 0 || mask > 0xffff ) {
	    out("Mask should be in the range between 0 and 0xffff (see documenation)!");
	    return 1; // command taken
	  } else {
	    kc->midi_ports = mask;
	    out("Keyboard #%d: MIDI ports selection 0x%04x", kb+1, kc->midi_ports);
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "midi_chn") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the MIDI channel!");
	    return 1; // command taken
	  }

	  int chn = get_dec(parameter);

	  if( chn < 0 || chn > 16 ) {
	    out("Channel should be in the range between 0 and 16 (0=off)!");
	    return 1; // command taken
	  } else {
	    kc->midi_chn = chn;
	    out("Keyboard #%d: MIDI channel %d", kb+1, kc->midi_chn);
	  }
	/////////////////////////////////////////////////////////////////////
        } else if( strcmp(parameter, "note_offset") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the Note offset!");
	    return 1; // command taken
	  }

	  int offset = get_dec(parameter);

	  if( offset < 0 || offset > 127 ) {
	    out("Note Offset should be in the range between 0 and 127!");
	    return 1; // command taken
	  } else {
	    kc->note_offset = offset;
	    out("Keyboard #%d: Note Offset %d", kb+1, kc->note_offset);
	  }
	/////////////////////////////////////////////////////////////////////
        } else if( strcmp(parameter, "din_key_offset") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the key offset!");
	    return 1; // command taken
	  }

	  int offset = get_dec(parameter);

	  if( offset < 0 || offset > 127 ) {
	    out("Key Offset should be in the range between 0 and 127!");
	    return 1; // command taken
	  } else {
	    kc->din_key_offset = offset;
	    out("Keyboard #%d: DIN Key Offset %d", kb+1, kc->din_key_offset);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "debug") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify on or off (alternatively 1 or 0)");
	    return 1; // command taken
	  }

	  int on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	  } else {
	    kc->verbose_level = on_off ? 2 : 1;

	    out("Keyboard #%d: debug mode %s", kb+1, on_off ? "enabled" : "disabled");
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "rows") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the number of rows (0..%d)", MATRIX_NUM_ROWS);
	    return 1; // command taken
	  }

	  int rows = get_dec(parameter);

	  if( rows < 0 || rows > MATRIX_NUM_ROWS ) {
	    out("Rows should be in the range between 0 (off) and %d", MATRIX_NUM_ROWS);
	    return 1; // command taken
	  } else {
	    kc->num_rows = rows;
	    out("Keyboard #%d: %d rows will be scanned!", kb+1, kc->num_rows);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "dout_sr1") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the DOUT number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  }

	  int sr = get_dec(parameter);

	  if( sr < 0 || sr > MATRIX_NUM_ROWS ) {
	    out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  } else {
	    kc->dout_sr1 = sr;
	    out("Keyboard #%d: dout_sr1 assigned to %d!", kb+1, kc->dout_sr1);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "dout_sr2") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the DOUT number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  }

	  int sr = get_dec(parameter);

	  if( sr < 0 || sr > MATRIX_NUM_ROWS ) {
	    out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  } else {
	    kc->dout_sr2 = sr;
	    out("Keyboard #%d: dout_sr2 assigned to %d!", kb+1, kc->dout_sr2);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "din_sr1") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the DIN number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  }

	  int sr = get_dec(parameter);

	  if( sr < 0 || sr > MATRIX_NUM_ROWS ) {
	    out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  } else {
	    kc->din_sr1 = sr;
	    out("Keyboard #%d: din_sr1 assigned to %d!", kb+1, kc->din_sr1);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "din_sr2") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the DIN number to which the first column is assigned (0..%d)", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  }

	  int sr = get_dec(parameter);

	  if( sr < 0 || sr > MATRIX_NUM_ROWS ) {
	    out("Shift register should be in the range between 0 (off) and %d", MIOS32_SRIO_ScanNumGet());
	    return 1; // command taken
	  } else {
	    kc->din_sr2 = sr;
	    out("Keyboard #%d: din_sr2 assigned to %d!", kb+1, kc->din_sr2);
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "velocity") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify on or off (alternatively 1 or 0)");
	    return 1; // command taken
	  }

	  int on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	  } else {
	    kc->scan_velocity = on_off;

	    out("Keyboard #%d: velocity mode %s", kb+1, on_off ? "enabled" : "disabled");
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "optimized") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify on or off (alternatively 1 or 0)");
	    return 1; // command taken
	  }

	  int on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	  } else {
	    kc->scan_optimized = on_off;

	    out("Keyboard #%d: optimized scan %s", kb+1, on_off ? "enabled" : "disabled");
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "din_inverted") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify on or off (alternatively 1 or 0)");
	    return 1; // command taken
	  }

	  int on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	  } else {
	    kc->din_inverted = on_off;

	    out("Keyboard #%d: DIN values are %sinverted", kb+1, kc->din_inverted ? "" : "not ");
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "break_inverted") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify on or off (alternatively 1 or 0)");
	    return 1; // command taken
	  }

	  int on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	  } else {
	    kc->break_inverted = on_off;

	    out("Keyboard #%d: Break contacts are %sinverted", kb+1, kc->break_inverted ? "" : "not ");
	    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "delay_fastest") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the fastest delay for velocity calculation!");
	    return 1; // command taken
	  }

	  int delay = get_dec(parameter);

	  if( delay < 0 || delay > 65535 ) {
	    out("Delay should be in the range between 0 and 65535");
	    return 1; // command taken
	  } else {
	    kc->delay_fastest = delay;
	    out("Keyboard #%d: delay_fastest set to %d!", kb+1, kc->delay_fastest);
	  }
	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "delay_slowest") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the slowest delay for velocity calculation!");
	    return 1; // command taken
	  }

	  int delay = get_dec(parameter);

	  if( delay < 0 || delay > 65535 ) {
	    out("Delay should be in the range between 0 and 65535");
	    return 1; // command taken
	  } else {
	    kc->delay_slowest = delay;
	    out("Keyboard #%d: delay_slowest set to %d!", kb+1, kc->delay_slowest);
	  }

	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "ain_pitchwheel") == 0 || strcmp(parameter, "ain_modwheel") == 0 ) {
	  u8 pitchwheel = strcmp(parameter, "ain_pitchwheel") == 0;

	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify J5.Ax number (0..5) or off!");
	    return 1; // command taken
	  }

	  int ain=0;
	  if( strcmp(parameter, "off") != 0 ) {
	    ain = get_dec(parameter);

	    if( ain < 0 || ain >= 6 ) {
	      out("AIN pin should be in the range of 0..5");
	      return 1; // command taken
	    }
	    ain += 1;
	  }

	  char wheel_name[20];
	  if( pitchwheel ) {
	    strcpy(wheel_name, "PitchWheel");
	    kc->ain_pitchwheel = ain;
	  } else {
	    strcpy(wheel_name, "ModWheel");
	    kc->ain_modwheel = ain;
	  }

	  if( ain ) {
	    out("Keyboard #%d: %s assigned to J5.A%d!", kb+1, wheel_name, ain-1);
	  } else {
	    out("Keyboard #%d: %s disabled!", kb+1, wheel_name);
	  }

	/////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "ctrl_pitchwheel") == 0 || strcmp(parameter, "ctrl_modwheel") == 0 ) {
	  u8 pitchwheel = strcmp(parameter, "ctrl_pitchwheel") == 0;

	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the CC number (or 128 for PitchBend or 129 for Aftertouch)!");
	    return 1; // command taken
	  }

	  int ctrl = get_dec(parameter);

	  if( ctrl < 0 || ctrl > 129 ) {
	    out("Controller Number should be in the range between 0 and 129!");
	    return 1; // command taken
	  } else {
	    char wheel_name[20];
	    if( pitchwheel ) {
	      strcpy(wheel_name, "PitchWheel");
	      kc->ctrl_pitchwheel = ctrl;
	    } else {
	      strcpy(wheel_name, "ModWheel");
	      kc->ctrl_modwheel = ctrl;
	    }
	    if( ctrl < 128 )
	      out("Keyboard #%d: %s sends CC#%d", kb+1, wheel_name, ctrl);
	    else if( ctrl == 128 )
	      out("Keyboard #%d: %s sends PitchBend", kb+1, wheel_name);
	    else
	      out("Keyboard #%d: %s sends Aftertouch", kb+1, wheel_name);
	  }
	/////////////////////////////////////////////////////////////////////
	} else {
	  out("Unknown parameter for keyboard configuration - type 'help' to list available parameters!");
	  return 1; // command taken
	}
      }
    } else {
      // out("Unknown command - type 'help' to list available commands!");
      input_line_parsed = 0; // input line has to be restored
    }
  }

  if( !input_line_parsed ) {
    // restore input line (replace NUL characters by spaces)
    int i;
    char *input_ptr = input;
    for(i=0; i<input_len; ++i, ++input_ptr)
      if( !*input_ptr )
	*input_ptr = ' ';

    return 0; // command not taken
  }

  return 1; // command taken
}


/////////////////////////////////////////////////////////////////////////////
// Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_TerminalPrintConfig(int kb, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];

  out("kb %d debug %s", kb+1, (kc->verbose_level >= 2) ? "on" : "off");
  out("kb %d midi_ports 0x%04x", kb+1, kc->midi_ports);
  out("kb %d midi_chn %d", kb+1, kc->midi_chn);
  out("kb %d note_offset %d", kb+1, kc->note_offset);
  out("kb %d rows %d", kb+1, kc->num_rows);
  out("kb %d velocity %s", kb+1, kc->scan_velocity ? "on" : "off");
  out("kb %d optimized %s", kb+1, kc->scan_optimized ? "on" : "off");
  out("kb %d dout_sr1 %d", kb+1, kc->dout_sr1);
  out("kb %d dout_sr2 %d", kb+1, kc->dout_sr2);
  out("kb %d din_sr1 %d", kb+1, kc->din_sr1);
  out("kb %d din_sr2 %d", kb+1, kc->din_sr2);
  out("kb %d din_key_offset %d", kb+1, kc->din_key_offset);
  out("kb %d din_inverted %s", kb+1, kc->din_inverted ? "on" : "off");
  out("kb %d break_inverted %s", kb+1, kc->break_inverted ? "on" : "off");
  out("kb %d delay_fastest %d", kb+1, kc->delay_fastest);
  out("kb %d delay_slowest %d", kb+1, kc->delay_slowest);

  if( kc->ain_pitchwheel )
    out("kb %d ain_pitchwheel %d", kb+1, kc->ain_pitchwheel-1);
  else
    out("kb %d ain_pitchwheel off", kb+1);
  out("kb %d ctrl_pitchwheel %d (%s)", kb+1, kc->ctrl_pitchwheel,
      (kc->ctrl_pitchwheel < 128) ? "CC" : (kc->ctrl_pitchwheel == 128 ? "PitchBend" : "Aftertouch"));

  if( kc->ain_modwheel )
    out("kb %d ain_modwheel %d", kb+1, kc->ain_modwheel-1);
  else
    out("kb %d ain_modwheel off", kb+1);
  out("kb %d ctrl_modwheel %d (%s)", kb+1, kc->ctrl_modwheel,
      (kc->ctrl_modwheel < 128) ? "CC" : (kc->ctrl_modwheel == 128 ? "PitchBend" : "Aftertouch"));

  return 0; // no error
}
