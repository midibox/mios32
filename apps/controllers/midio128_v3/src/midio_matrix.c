// $Id$
/*
 * Scan Matrix functions for MIDIO128 V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "midio_matrix.h"
#include "midio_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local defines
/////////////////////////////////////////////////////////////////////////////

#define NUM_ROWS 8

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_row;

static u8 button_row[MIDIO_PATCH_NUM_MATRIX][NUM_ROWS];
static u8 button_row_changed[MIDIO_PATCH_NUM_MATRIX][NUM_ROWS];
static u8 button_debounce_ctr[MIDIO_PATCH_NUM_MATRIX];

static u8 debounce_reload;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MIDIO_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
// This function initializes the matrix handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_MATRIX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  selected_row = 0;

  int matrix, row;
  for(matrix=0; matrix<MIDIO_PATCH_NUM_MATRIX; ++matrix) {
    for(row=0; row<NUM_ROWS; ++row) {
      button_row[matrix][row] = 0xff;
      button_row_changed[matrix][row] = 0x00;
    }
    button_debounce_ctr[matrix] = 0;
  }

  debounce_reload = 10; // mS

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a column.
// It should be called from the APP_SRIO_ServicePrepare()
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_MATRIX_PrepareCol(void)
{
  // increment column, wrap at 8
  if( ++selected_row >= 8 )
    selected_row = 0;

  // select next DIN column (selected cathode line = 0, all others 1)
  u8 dout_value = ~(1 << selected_row);

  // forward value to all shift registers which are assigned to the matrix function
  int matrix;
  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[0];
  for(matrix=0; matrix<MIDIO_PATCH_NUM_MATRIX; ++matrix, ++m)
    if( m->sr_dout ) {
      // MEMO: here we could apply an optional inversion if somebody requests this
      MIOS32_DOUT_SRSet(m->sr_dout-1, dout_value);
    }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected row.
// It should be called from the APP_SRIO_ServiceFinish() hook
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_MATRIX_GetRow(void)
{
  // determine the scanned row (selected_row driver has already been incremented, decrement it again)
  u8 row = (selected_row-1) & 0x7;

  int matrix;
  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[0];
  for(matrix=0; matrix<MIDIO_PATCH_NUM_MATRIX; ++matrix, ++m) {
    // decrememt debounce counter if > 0
    if( button_debounce_ctr[matrix] )
      --button_debounce_ctr[matrix];

    // get DIN value (if SR is assigned to matrix)
    if( !m->sr_din )
      continue;

    MIOS32_DIN_SRChangedGetAndClear(m->sr_din-1, 0xff); // ensure that change won't be propagated to normal DIN handler
    u8 sr_value = MIOS32_DIN_SRGet(m->sr_din-1);

    // determine pin changes
    u8 changed = sr_value ^ button_row[matrix][row];

    // ignore as long as debounce counter running
    if( button_debounce_ctr[matrix] )
      break;

    // add them to existing notifications
    button_row_changed[matrix][row] |= changed;

    // store new value
    button_row[matrix][row] = sr_value;

    // reload debounce counter if any pin has changed
    if( changed )
      button_debounce_ctr[matrix] = debounce_reload;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from a task to check for button changes
// periodically. Events (change from 0->1 or from 1->0) will be notified 
// to MIDIO_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_MATRIX_ButtonHandler(void)
{
  int matrix, row;
  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[0];
  for(matrix=0; matrix<MIDIO_PATCH_NUM_MATRIX; ++matrix, ++m) {
    for(row=0; row<NUM_ROWS; ++row) {
    
      // check if there are pin changes - must be atomic!
      MIOS32_IRQ_Disable();
      u8 changed = button_row_changed[matrix][row];
      button_row_changed[matrix][row] = 0;
      MIOS32_IRQ_Enable();

      // any pin change at this SR?
      if( !changed )
	continue;

      // check all 8 pins of the SR
      int pin;
      u8 mask = (1 << 0);
      for(pin=0; pin<8; ++pin, mask <<= 1)
	if( changed & mask )
	  MIDIO_MATRIX_NotifyToggle(matrix, row*8+pin, (button_row[matrix][row] & mask) ? 1 : 0);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function will be called by MIDIO_MATRIX_ButtonHandler whenever
// a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
static s32 MIDIO_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
{
  //MIOS32_MIDI_SendDebugMessage("matrix=%d pin=%d pin_value=%d\n", matrix, pin, pin_value);

  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[matrix];

  // here we could differ between matrix modes
  // currently only MIDIO_PATCH_MATRIX_MODE_COMMON supported (sending note events)

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = NoteOn;
  p.event = NoteOn;

  if( midio_patch_cfg.global_chn )
    p.chn = midio_patch_cfg.global_chn - 1;
  else {
    if( m->mode == MIDIO_PATCH_MATRIX_MODE_MAPPED ) {
      p.chn = m->map_chn[pin] - 1;
    } else {
      p.chn = m->chn - 1;
    }
  }

  if( m->mode == MIDIO_PATCH_MATRIX_MODE_MAPPED ) {
    p.evnt1 = m->map_evnt1[pin] & 0x7f;
  } else {
    p.evnt1 = (m->arg + pin) & 0x7f;
  }
  p.evnt2 = (pin_value ? 0x00 : 0x7f);

  // send MIDI package over enabled ports
  int i;
  u16 mask = 1;
  for(i=0; i<16; ++i, mask <<= 1) {
    if( m->enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}
