// $Id$
/*
 * CV access functions for MIDIbox NG
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
#include <aout.h>
#include <notestack.h>

#include "app.h"
#include "mbng_cv.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// each channel has an own notestack
#define NOTESTACK_SIZE 10
static notestack_t cv_notestack[MBNG_PATCH_NUM_CV_CHANNELS];
static notestack_item_t cv_notestack_items[MBNG_PATCH_NUM_CV_CHANNELS][NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_CV_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // initialize AOUT driver
  //AOUT_Init(0);
  // already done in app.c

  // reset all channels
  MBNG_CV_ResetAllChannels();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Updates all CV channels and gates
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_CV_Update(void)
{
  // update AOUTs
  AOUT_Update();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_CV_NotifyReceivedValue(mbng_event_item_t *item)
{
  int cv_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_CV_NotifyReceivedValue(%d, %d)\n", cv_subid, item->value);
  }

  // forward to CV channel
  if( cv_subid && cv_subid <= MBNG_PATCH_NUM_CV_CHANNELS ) {
    u8 cv_ix = cv_subid - 1;
    u16 value = item->value;
    u8 set_value = 1;
    u8 gate_value = 0;

    if( item->flags.general.type == MBNG_EVENT_TYPE_NOTE_ON ) {
      if( item->flags.general.use_key_or_cc ) {
	if( item->secondary_value > 0 ) {
	  // push note into note stack
	  NOTESTACK_Push(&cv_notestack[cv_ix], value, item->secondary_value);
	} else {
	  // remove note from note stack
	  NOTESTACK_Pop(&cv_notestack[cv_ix], value);
	}

	// still a note in stack?
	if( cv_notestack[cv_ix].len ) {
	  // take first note of stack
	  value = cv_notestack_items[cv_ix][0].note;
	  u8 velocity = cv_notestack_items[cv_ix][0].tag;
	  // TODO: should we provide an option to forward the notestack based velocity to the appr. EVENT_CV item?

	  gate_value = 1;

	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    DEBUG_MSG("-> Note %3d with velocity %3d\n", value, velocity);
	  }
	} else {
	  gate_value = 0;

	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    DEBUG_MSG("-> Note %3d released the gate\n", value);
	  }
	}
      } else {
	if( item->value == 0 ) {
	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    DEBUG_MSG("-> Velocity 0 won't change this channel!\n");
	  }
	  set_value = 0;
	} else {
	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    DEBUG_MSG("-> Velocity set to %3d\n", item->value);
	  }
	}
      }
    }

    if( set_value ) {
      // scale value to 16bit
      int value16;
      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
      if( map_len > 0 ) {
	value16 = MBNG_EVENT_MapIxGet(map_values, map_len, value) * (65536 / map_len);
      } else if( item->min <= item->max ) {
	int range = item->max - item->min + 1;
	value16 = (value - item->min) * (65536 / range);
      } else {
	int range = item->min - item->max + 1;
	value16 = (value - item->max)* (65536 / range);
      }

      if( value16 < 0 )
	value16 = 0;
      else if( value16 > 65535 )
	value16 = 65535;

      // change output curve
      AOUT_ConfigChannelInvertedSet(cv_ix, item->flags.CV.cv_inverted);
      AOUT_ConfigChannelHzVSet(cv_ix, item->flags.CV.cv_hz_v);

      // set CV value
      AOUT_PinSet(cv_ix, value16);

      // gate inverted?
      if( item->flags.CV.cv_gate_inverted ) {
	gate_value ^= 1;
      }

      // set gates
      AOUT_DigitalPinSet(cv_ix, gate_value);

      if( item->flags.CV.fwd_gate_to_dout_pin ) {
	if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	  DEBUG_MSG("-> Setting DOUT Pin #%d=%d\n", item->flags.CV.fwd_gate_to_dout_pin-1, gate_value);
	}	
	MIOS32_DOUT_PinSet(item->flags.CV.fwd_gate_to_dout_pin-1, gate_value);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called to reset all channels/notes (e.g. after session change)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_CV_ResetAllChannels(void)
{
  int cv;

  for(cv=0; cv<MBNG_PATCH_NUM_CV_CHANNELS; ++cv) {
    NOTESTACK_Init(&cv_notestack[cv],
                   NOTESTACK_MODE_PUSH_TOP,
                   &cv_notestack_items[cv][0],
                   NOTESTACK_SIZE);

    AOUT_PinSet(cv, 0x0000);
    AOUT_PinPitchSet(cv, 0x0000);
    AOUT_DigitalPinSet(cv, 0);
  }

  return 0; // no error
}
