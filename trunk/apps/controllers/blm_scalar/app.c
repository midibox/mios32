// $Id$
/*
 * BLM_SCALAR for MIOS32
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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

#include <blm_scalar.h>
#include "sysex.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_BLM_CHECK		( tskIDLE_PRIORITY + 2 )

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_BLM_Check(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// send layout informations or ping each 5 seconds
static u32 sysexRequestTimer;
static u8 midiDataReceived;

static u8 testmode;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  sysexRequestTimer = 0;
  midiDataReceived = 0;
  testmode = 1;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize BLM_SCALAR driver
  BLM_SCALAR_Init(0);

  // initialize SysEx parser
  SYSEX_Init(0);

  // start BLM check task
  xTaskCreate(TASK_BLM_Check, (signed portCHAR *)"BLM_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_BLM_CHECK, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This function notifies received BLM data and exits testmode
/////////////////////////////////////////////////////////////////////////////
static void notifyDataReceived(void)
{
  midiDataReceived = 1;
  testmode = 0;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // control the Duo-LEDs via Note On/Off Events
  // The colour is controlled with velocity value:
  // 0x00:       both LEDs off
  // 0x01..0x3f: green LED on
  // 0x40..0x5f: red LED on
  // 0x60..0x7f: both LEDs on

  // MIDI event assignments: see README.txt

  if( midi_package.event == NoteOff || midi_package.event == NoteOn ) {
    u8 chn = midi_package.chn;
    u8 note = midi_package.note;
    u8 velocity = midi_package.velocity;

    u8 led_mod_ix = 0;
    u8 led_row_ix = 0;
    u8 led_column_ix = 0;
    u8 modify_led = 0;

    if( note <= 0x0f ) {
      // BLM16x16 LEDs
      led_mod_ix = chn >> 2;
      led_row_ix = ((chn&3) << 1) + ((note >> 3) & 1);
      led_column_ix = note & 0x7;
      modify_led = 1;
    } else if( note == 0x40 ) {
      // extra column LEDs
      led_mod_ix = 4;
      led_row_ix = (chn&3) << 1;
      led_column_ix = chn >> 2;
      modify_led = 1;
    } else if( chn == 0 && note >= 0x60 && note <= 0x6f ) {
      // extra row LEDs
      led_mod_ix = 4;
      led_row_ix = 1 + ((note >> 1) & 6);
      led_column_ix = note & 3;
      modify_led = 1;
    } else if( chn == 0xf && note >= 0x60 && note <= 0x6f ) {
      // additional extra LEDs
      led_mod_ix = 4;
      led_row_ix = 1 + ((note >> 1) & 6);
      led_column_ix = 4 + (note & 3);
      modify_led = 1;
    }


    if( modify_led ) {
      u8 led_mask = 1 << led_column_ix;

      // 90 xx 00 is the same like a note off event!
      // (-> http://www.borg.com/~jglatt/tech/midispec.htm)
      if( midi_package.event == NoteOff || velocity == 0x00 ) {
	// Note Off or velocity == 0x00: clear both LEDs
	blm_scalar_led[led_mod_ix][led_row_ix][0] &= ~led_mask;
#if BLM_SCALAR_NUM_COLOURS >= 2
	blm_scalar_led[led_mod_ix][led_row_ix][1] &= ~led_mask;
#endif
      } else if( velocity < 0x40 ) {
	// Velocity < 0x40: set green LED, clear red LED
	blm_scalar_led[led_mod_ix][led_row_ix][0] |= led_mask;
#if BLM_SCALAR_NUM_COLOURS >= 2
	blm_scalar_led[led_mod_ix][led_row_ix][1] &= ~led_mask;
#endif
      } else if( velocity < 0x60 ) {
	// Velocity < 0x60: clear green LED, set red LED
	blm_scalar_led[led_mod_ix][led_row_ix][0] &= ~led_mask;
#if BLM_SCALAR_NUM_COLOURS >= 2
	blm_scalar_led[led_mod_ix][led_row_ix][1] |= led_mask;
#endif
      } else {
	// Velocity >= 0x60: set both LEDs
	blm_scalar_led[led_mod_ix][led_row_ix][0] |= led_mask;
#if BLM_SCALAR_NUM_COLOURS >= 2
	blm_scalar_led[led_mod_ix][led_row_ix][1] |= led_mask;
#endif
      }

      notifyDataReceived();
    }
  }

  // "check for packed format" which is transfered via CCs
  else if( midi_package.event == CC ) {
    u8 chn = midi_package.chn;
    u8 cc_number = midi_package.cc_number;
    u8 pattern = midi_package.value;

    if( cc_number & 0x01 )
      pattern |= (1 << 7);

    switch( cc_number & 0xfe ) {
      case 0x10: blm_scalar_led[chn>>2][((chn&3)<<1) + 0][0] = pattern; break;
      case 0x12: blm_scalar_led[chn>>2][((chn&3)<<1) + 1][0] = pattern; break;

      case 0x18:
      case 0x1a: {
	u8 mod = (cc_number & 0x02) ? 2 : 0;
	u8 offset = chn >> 3;
	u8 mask = 1 << (chn&7);
	if( pattern & 0x01 ) { blm_scalar_led[mod+0][offset+0][0] |= mask; } else { blm_scalar_led[mod+0][offset+0][0] &= ~mask; }
	if( pattern & 0x02 ) { blm_scalar_led[mod+0][offset+2][0] |= mask; } else { blm_scalar_led[mod+0][offset+2][0] &= ~mask; }
	if( pattern & 0x04 ) { blm_scalar_led[mod+0][offset+4][0] |= mask; } else { blm_scalar_led[mod+0][offset+4][0] &= ~mask; }
	if( pattern & 0x08 ) { blm_scalar_led[mod+0][offset+6][0] |= mask; } else { blm_scalar_led[mod+0][offset+6][0] &= ~mask; }
	if( pattern & 0x10 ) { blm_scalar_led[mod+1][offset+0][0] |= mask; } else { blm_scalar_led[mod+1][offset+0][0] &= ~mask; }
	if( pattern & 0x20 ) { blm_scalar_led[mod+1][offset+2][0] |= mask; } else { blm_scalar_led[mod+1][offset+2][0] &= ~mask; }
	if( pattern & 0x40 ) { blm_scalar_led[mod+1][offset+4][0] |= mask; } else { blm_scalar_led[mod+1][offset+4][0] &= ~mask; }
	if( pattern & 0x80 ) { blm_scalar_led[mod+1][offset+6][0] |= mask; } else { blm_scalar_led[mod+1][offset+6][0] &= ~mask; }
      } break;

      case 0x20: blm_scalar_led[chn>>2][((chn&3)<<1) + 0][1] = pattern; break;
      case 0x22: blm_scalar_led[chn>>2][((chn&3)<<1) + 1][1] = pattern; break;

      case 0x28:
      case 0x2a: {
#if BLM_SCALAR_NUM_COLOURS >= 2
	u8 mod = (cc_number & 0x02) ? 2 : 0;
	u8 offset = chn >> 3;
	u8 mask = 1 << (chn&7);
	if( pattern & 0x01 ) { blm_scalar_led[mod+0][offset+0][1] |= mask; } else { blm_scalar_led[mod+0][offset+0][1] &= ~mask; }
	if( pattern & 0x02 ) { blm_scalar_led[mod+0][offset+2][1] |= mask; } else { blm_scalar_led[mod+0][offset+2][1] &= ~mask; }
	if( pattern & 0x04 ) { blm_scalar_led[mod+0][offset+4][1] |= mask; } else { blm_scalar_led[mod+0][offset+4][1] &= ~mask; }
	if( pattern & 0x08 ) { blm_scalar_led[mod+0][offset+6][1] |= mask; } else { blm_scalar_led[mod+0][offset+6][1] &= ~mask; }
	if( pattern & 0x10 ) { blm_scalar_led[mod+1][offset+0][1] |= mask; } else { blm_scalar_led[mod+1][offset+0][1] &= ~mask; }
	if( pattern & 0x20 ) { blm_scalar_led[mod+1][offset+2][1] |= mask; } else { blm_scalar_led[mod+1][offset+2][1] &= ~mask; }
	if( pattern & 0x40 ) { blm_scalar_led[mod+1][offset+4][1] |= mask; } else { blm_scalar_led[mod+1][offset+4][1] &= ~mask; }
	if( pattern & 0x80 ) { blm_scalar_led[mod+1][offset+6][1] |= mask; } else { blm_scalar_led[mod+1][offset+6][1] &= ~mask; }
#endif
      } break;

      case 0x40: 
      case 0x42: {
	if( chn == 0 ) {
	  int mod = 4;
	  // Bit  0 -> led_row_ix 0x0, Bit 0
	  // Bit  1 -> led_row_ix 0x2, Bit 0
	  // Bit  2 -> led_row_ix 0x4, Bit 0
	  // Bit  3 -> led_row_ix 0x6, Bit 0
	  // Bit  4 -> led_row_ix 0x0, Bit 1
	  // Bit  5 -> led_row_ix 0x2, Bit 1
	  // Bit  6 -> led_row_ix 0x4, Bit 1
	  // Bit  7 -> led_row_ix 0x6, Bit 1
	  // Bit  8 -> led_row_ix 0x0, Bit 2
	  // Bit  9 -> led_row_ix 0x2, Bit 2
	  // Bit 10 -> led_row_ix 0x4, Bit 2
	  // Bit 11 -> led_row_ix 0x6, Bit 2
	  // Bit 12 -> led_row_ix 0x0, Bit 3
	  // Bit 13 -> led_row_ix 0x2, Bit 3
	  // Bit 14 -> led_row_ix 0x4, Bit 3
	  // Bit 15 -> led_row_ix 0x6, Bit 3
	  if( cc_number >= 0x42 ) {
	    blm_scalar_led[mod][0][0] = (blm_scalar_led[mod][0][0] & 0xf3) | ((pattern << 2) & 0x04) | ((pattern >> 1) & 0x08);
	    blm_scalar_led[mod][2][0] = (blm_scalar_led[mod][2][0] & 0xf3) | ((pattern << 1) & 0x04) | ((pattern >> 2) & 0x08);
	    blm_scalar_led[mod][4][0] = (blm_scalar_led[mod][4][0] & 0xf3) | ((pattern << 0) & 0x04) | ((pattern >> 3) & 0x08);
	    blm_scalar_led[mod][6][0] = (blm_scalar_led[mod][6][0] & 0xf3) | ((pattern >> 1) & 0x04) | ((pattern >> 4) & 0x08);
	  } else {
	    blm_scalar_led[mod][0][0] = (blm_scalar_led[mod][0][0] & 0xfc) | ((pattern >> 0) & 0x01) | ((pattern >> 3) & 0x02);
	    blm_scalar_led[mod][2][0] = (blm_scalar_led[mod][2][0] & 0xfc) | ((pattern >> 1) & 0x01) | ((pattern >> 4) & 0x02);
	    blm_scalar_led[mod][4][0] = (blm_scalar_led[mod][4][0] & 0xfc) | ((pattern >> 2) & 0x01) | ((pattern >> 5) & 0x02);
	    blm_scalar_led[mod][6][0] = (blm_scalar_led[mod][6][0] & 0xfc) | ((pattern >> 3) & 0x01) | ((pattern >> 6) & 0x02);
	  }
	}
      } break;

      case 0x48: 
      case 0x4a: {
#if BLM_SCALAR_NUM_COLOURS >= 2
	if( chn == 0 ) {
	  int mod = 4;
	  if( cc_number >= 0x4a ) {
	    blm_scalar_led[mod][0][1] = (blm_scalar_led[mod][0][1] & 0xf3) | ((pattern << 2) & 0x04) | ((pattern >> 1) & 0x08);
	    blm_scalar_led[mod][2][1] = (blm_scalar_led[mod][2][1] & 0xf3) | ((pattern << 1) & 0x04) | ((pattern >> 2) & 0x08);
	    blm_scalar_led[mod][4][1] = (blm_scalar_led[mod][4][1] & 0xf3) | ((pattern << 0) & 0x04) | ((pattern >> 3) & 0x08);
	    blm_scalar_led[mod][6][1] = (blm_scalar_led[mod][6][1] & 0xf3) | ((pattern >> 1) & 0x04) | ((pattern >> 4) & 0x08);
	  } else {
	    blm_scalar_led[mod][0][1] = (blm_scalar_led[mod][0][1] & 0xfc) | ((pattern >> 0) & 0x01) | ((pattern >> 3) & 0x02);
	    blm_scalar_led[mod][2][1] = (blm_scalar_led[mod][2][1] & 0xfc) | ((pattern >> 1) & 0x01) | ((pattern >> 4) & 0x02);
	    blm_scalar_led[mod][4][1] = (blm_scalar_led[mod][4][1] & 0xfc) | ((pattern >> 2) & 0x01) | ((pattern >> 5) & 0x02);
	    blm_scalar_led[mod][6][1] = (blm_scalar_led[mod][6][1] & 0xfc) | ((pattern >> 3) & 0x01) | ((pattern >> 6) & 0x02);
	  }
	}
#endif
      } break;


      case 0x60: 
      case 0x62: {
	u8 mod = 4;
	u8 led_row_ix = (cc_number >= 0x62) ? 5 : 1;
	if( chn == 0 ) {
	  blm_scalar_led[mod][led_row_ix+0][0] = (blm_scalar_led[mod][led_row_ix+0][0] & 0xf0) | (pattern & 0x0f);
	  blm_scalar_led[mod][led_row_ix+2][0] = (blm_scalar_led[mod][led_row_ix+2][0] & 0xf0) | (pattern >> 4);
	} else if( chn == 15 ) {
	  blm_scalar_led[mod][led_row_ix+0][0] = (blm_scalar_led[mod][led_row_ix+0][0] & 0x0f) | (pattern << 4);
	  blm_scalar_led[mod][led_row_ix+2][0] = (blm_scalar_led[mod][led_row_ix+2][0] & 0x0f) | (pattern & 0xf0);
	}
      } break;

      case 0x68: 
      case 0x6a: {
#if BLM_SCALAR_NUM_COLOURS >= 2
	u8 mod = 4;
	u8 led_row_ix = (cc_number >= 0x6a) ? 5 : 1;
	if( chn == 0 ) {
	  blm_scalar_led[mod][led_row_ix+0][1] = (blm_scalar_led[mod][led_row_ix+0][1] & 0xf0) | (pattern & 0x0f);
	  blm_scalar_led[mod][led_row_ix+2][1] = (blm_scalar_led[mod][led_row_ix+2][1] & 0xf0) | (pattern >> 4);
	} else if( chn == 15 ) {
	  blm_scalar_led[mod][led_row_ix+0][1] = (blm_scalar_led[mod][led_row_ix+0][1] & 0x0f) | (pattern << 4);
	  blm_scalar_led[mod][led_row_ix+2][1] = (blm_scalar_led[mod][led_row_ix+2][1] & 0x0f) | (pattern & 0xf0);
	}
#endif
      } break;
    }

    notifyDataReceived();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // prepare DOUT registers to drive the column
  BLM_SCALAR_PrepareCol();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  // call the BLM_GetRow function after scan is finished to capture the read DIN values
  BLM_SCALAR_GetRow();
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
  // a pot has been moved, send modulation CC#1
  if( pin < 16 ) {
    // convert 12bit value to 7bit value
    u8 value_7bit = pin_value >> 5;

#if 0
    MIOS32_MIDI_SendCC(DEFAULT, pin, 0x01, value_7bit);
#else
    MIOS32_MIDI_SendCC(USB0, pin, 0x01, value_7bit);
    MIOS32_MIDI_SendCC(UART0, pin, 0x01, value_7bit);
#endif
  }
}



/////////////////////////////////////////////////////////////////////////////
// This task is called each mS to check the BLM button states
/////////////////////////////////////////////////////////////////////////////

// will be called on BLM pin changes (see TASK_BLM_Check)
void DIN_BLM_NotifyToggle(u32 pin, u32 pin_value)
{
  // determine row and column (depends on the way how the matrix has been connected)
  u8 blm_row = pin >> 3;
  u8 blm_column = pin & 7;

  u8 row = (blm_row >> 1);
  u8 column = blm_column | ((blm_row&1) << 3);
  u8 velocity = pin_value ? 0x00 : 0x7f;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("DIN_BLM_NotifyToggle(%d, %d) -> blm_row=%d, blm_column=%d -> row=%d, column=%d\n", pin, pin_value,
	    blm_row, blm_column, row, column);
#endif

  // send pin number and value as Note On Event
  u8 chn = 0;
  u8 note = 0;

  if( row < 16 ) {
    // BLM 16x16
    chn = row;
    note = column;
  } else if( (blm_row & 0xf9) == 0x20 && blm_column < 4 ) {
    // Extra column
    u8 extra_row = ((blm_row >> 1) & 3) | (blm_column << 2);
    chn = extra_row;
    note = 0x40;
  } else if( (blm_row & 0xf9) == 0x21 ) {
    u8 extra_column = ((blm_row << 1) & 0x0c) | (blm_column & 0x3);
    if( blm_column < 4 ) {
      // Extra row
      chn = 0x0;
      note = 0x60 + extra_column;
    } else {
      // Additional extra buttons
      chn = 0xf;
      note = 0x60 + extra_column;
    }
  }

#if 0
  MIOS32_MIDI_SendNoteOn(DEFAULT, chn, note, velocity);
#else
  MIOS32_MIDI_SendNoteOn(USB0, chn, note, velocity);
  MIOS32_MIDI_SendNoteOn(UART0, chn, note, velocity);
#endif


  // testmode enabled after startup as long as nothing has been received
  if( testmode ) {
    // cycle colour whenever button has been pressed (velocity > 0 )
    if( velocity ) {
#if BLM_SCALAR_NUM_COLOURS >= 2
      if( BLM_SCALAR_DOUT_PinGet(0, pin) )
	BLM_SCALAR_DOUT_PinSet(1, pin, BLM_SCALAR_DOUT_PinGet(1, pin) ? 0 : 1);
#endif
      BLM_SCALAR_DOUT_PinSet(0, pin, BLM_SCALAR_DOUT_PinGet(0, pin) ? 0 : 1);
    }
  }
}

static void TASK_BLM_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for BLM pin changes, call DIN_BLM_NotifyToggle on each toggled pin
    BLM_SCALAR_ButtonHandler(DIN_BLM_NotifyToggle);

    // send layout informations each 5 seconds if BLM hasn't been updated
    if( ++sysexRequestTimer > 5000 ) {
      sysexRequestTimer = 0;

      if( midiDataReceived ) {
#if 0
	SYSEX_SendAck(DEFAULT, SYSEX_ACK, 0x00);
#else
	SYSEX_SendAck(USB0, SYSEX_ACK, 0x00);
	SYSEX_SendAck(UART0, SYSEX_ACK, 0x00);
#endif
      } else {
#if 0
	SYSEX_SendLayoutInfo(DEFAULT);
#else
	SYSEX_SendLayoutInfo(USB0);
	SYSEX_SendLayoutInfo(UART0);
#endif
      }

      midiDataReceived = 0;
    }
  }
}
