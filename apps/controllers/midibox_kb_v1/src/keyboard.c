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

#include "keyboard.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// scan 16 columns for up to 8*16 (16x16 pins) keys (each key connected to two rows)
#define MATRIX_NUM_ROWS 16

// maximum number of supported keys
#define KEYBOARD_NUM_PINS (KEYBOARD_NUM*16*16)

// used MIDI port and channel (DEFAULT, USB0, UART0 or UART1)
#define KEYBOARD_MIDI_PORT DEFAULT
#define KEYBOARD_MIDI_CHN  Chn1

// initial minimum/maximum delay to calculate velocity
// (will be copied into variables, so that values could be changed during runtime)
#define INITIAL_KEYBOARD_DELAY_FASTEST 4
#define INITIAL_KEYBOARD_DELAY_SLOWEST 200

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

static u8 verbose_level;

static u8 connected_keyboards_num;

static u8 selected_row;
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


/////////////////////////////////////////////////////////////////////////////
// Initialize the keyboard handler
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_Init(u32 mode)
{
  // set low verbose level by default (only warnings)
  verbose_level = 1;

  // start with first column
  selected_row = 0;

  // number of connected keyboards
  connected_keyboards_num = 1;

  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
    kc->enabled_ports = 0x1011; // OSC1, OUT1 and USB1
    kc->midi_chn = kb+1;
    kc->note_offset = 36;

    kc->delay_fastest = 4;
    kc->delay_slowest = 200;

    kc->scan_velocity = 1;

    if( kb == 0 ) {
      kc->dout_sr1 = 1;
      kc->dout_sr2 = 2;
      kc->din_sr1 = 1;
      kc->din_sr2 = 0;
    } else {
      kc->dout_sr1 = 0;
      kc->dout_sr2 = 0;
      kc->din_sr1 = 2;
      kc->din_sr2 = 0;
    }

    // initialize DIN arrays
    int row;
    for(row=0; row<MATRIX_NUM_ROWS; ++row) {
      din_value[kb][row] = 0xffff; // default state: buttons depressed
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
// Sets/Gets the verbose level
/////////////////////////////////////////////////////////////////////////////
s32 KEYBOARD_VerboseLevelSet(u8 level)
{
  verbose_level = level;
  return 0; // no error
}

u8 KEYBOARD_VerboseLevelGet(void)
{
  return verbose_level;
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
  // select next column, wrap at 16
  if( ++selected_row >= MATRIX_NUM_ROWS ) {
    selected_row = 0;

    // increment timestamp for velocity delay measurements
    ++timestamp;
  }

  // selection pattern (active selection is 0, all other outputs 1)
  u16 selection_mask = ~(1 << (u16)selected_row);

  int kb;
  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
  for(kb=0; kb<connected_keyboards_num; ++kb, ++kc) {
    if( kc->dout_sr1 )
      MIOS32_DOUT_SRSet(kc->dout_sr1-1, (selection_mask >> 0) & 0xff);
    if( kc->dout_sr2 )
      MIOS32_DOUT_SRSet(kc->dout_sr2-1, (selection_mask >> 8) & 0xff);
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

    // determine pin changes
    u16 changed = sr_value ^ din_value[kb][selected_row];

    if( changed ) {
      // add them to existing notifications
      din_value_changed[kb][selected_row] |= changed;

      // store new value
      din_value[kb][selected_row] = sr_value;

      // number of pins per row depends on assigned DINs:
      int pins_per_row = kc->din_sr2 ? 16 : 8;

      // store timestamp for changed pin on 1->0 transition
      u8 sr_pin;
      u16 mask = 0x01;
      for(sr_pin=0; sr_pin<pins_per_row; ++sr_pin, mask <<= 1) {
	if( (changed & mask) && !(sr_value & mask) ) {
	  din_activated_timestamp[kb][selected_row*pins_per_row + sr_pin] = timestamp;
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

  // determine pin number based on row/column

  // each key has two contacts, I call them "early contact" and "final contact"
  // the assignments can be determined by setting verbose_level to 2
  // the early contacts are at row 0, 2, 4, 6, 8, 10, 12, 14
  // the final contacts are at row 1, 3, 5, 7, 9, 11, 13, 15

  // number of pins per row depends on assigned DINs:
  int pins_per_row = kc->din_sr2 ? 16 : 8;

  // determine key number:
  int key = pins_per_row*(row / 2) + column;

  // check if key is assigned to an "early contact"
  u8 early_contact = !(row & 1); // even numbers

  // determine note number (here we could insert an octave shift)
  int note_number = key + kc->note_offset;

  // ensure valid note range
  if( note_number > 127 )
    note_number = 127;
  else if( note_number < 0 )
    note_number = 0;

  if( verbose_level >= 2 )
    DEBUG_MSG("KB%d: row=%d, column=%d, depressed=%d  -->  key=%d, early_contact:%d, note_number=%d\n",
	kb+1, row, column, depressed, key, early_contact, note_number);

  // determine key mask and pointers for access to combined arrays
  u8 key_mask = (1 << (key % 8));
  u8 *note_on_sent = (u8 *)&key_note_on_sent[kb][key / 8];
  u8 *note_off_sent = (u8 *)&key_note_off_sent[kb][key / 8];


  // early contacts don't send MIDI notes, but they are used for delay measurements,
  // and they release the Note On/Off debouncing mechanism
  if( early_contact ) {
    if( depressed ) {
      *note_on_sent &= ~key_mask;
      *note_off_sent &= ~key_mask;
    }
    return;
  }
  // branch depending on pressed or released key
  if( depressed ) {
    if( !(*note_off_sent & key_mask) ) {
      *note_off_sent |= key_mask;

      if( verbose_level >= 2 )
	DEBUG_MSG("DEPRESSED key=%d\n", key);

      KEYBOARD_MIDI_SendNote(kb, note_number, 0x00);
    }

  } else {

    if( !(*note_on_sent & key_mask) ) {
      *note_on_sent |= key_mask;

      // determine timestamps between early and final contact
      u16 timestamp_early = din_activated_timestamp[kb][(row-1)*pins_per_row + column];
      u16 timestamp_final = din_activated_timestamp[kb][row*pins_per_row + column];

      // and the delta delay (IMPORTANT: delay variable needs same resolution like timestamps to handle overrun correctly!)
      s16 delay = timestamp_final - timestamp_early;

      int velocity = 127;
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
      if( verbose_level >= 2 )
	DEBUG_MSG("PRESSED key=%d, delay=%d, velocity=%d\n", key, delay, velocity);

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
  for(kb=0; kb<connected_keyboards_num; ++kb) {
    int row;
    for(row=0; row<MATRIX_NUM_ROWS; ++row) {
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
      for(sr_pin=0; sr_pin<16; ++sr_pin, mask <<= 1)
	if( changed & mask )
	  KEYBOARD_NotifyToggle(kb, row, sr_pin, (din_value[kb][row] & mask) ? 1 : 0);
    }
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
      if( kc->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
	MIOS32_MIDI_SendNoteOn(port, kc->midi_chn-1, note_number, velocity);
      }
    }
  }

  return 0; // no error
}
