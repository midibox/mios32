// $Id: mbng_din.c 1684 2013-02-06 20:40:38Z tk $
//! \defgroup MBNG_KB
//! Keyboard access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <keyboard.h>
#include "app.h"
#include "tasks.h"
#include "mbng_kb.h"
#include "mbng_event.h"
#include "mbng_patch.h"


#if defined(MIOS32_FAMILY_LPC17xx)
// not enough memory
# define SUPPORT_SECURE_NOTE_OFF_HANDLING 0
#else
# define SUPPORT_SECURE_NOTE_OFF_HANDLING 1
#endif

/////////////////////////////////////////////////////////////////////////////
//! local types
/////////////////////////////////////////////////////////////////////////////

#if SUPPORT_SECURE_NOTE_OFF_HANDLING
typedef struct {
  u16 midi_ports;
  u8 chn;
  u8 note;
} kb_played_note_t;
#endif


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

#if SUPPORT_SECURE_NOTE_OFF_HANDLING
static kb_played_note_t kb_played_note[KEYBOARD_NUM][KEYBOARD_MAX_KEYS];
#endif


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the KB handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // NOTE: kb_played_note isn't initialized by intention!
  // to ensure that NoteOff will be played on patch changes

  u8 any_key_was_played = 0;
#if SUPPORT_SECURE_NOTE_OFF_HANDLING
  {
    int kb, key;
    for(kb=0; kb<KEYBOARD_NUM; ++kb) {
      kb_played_note_t *kbn = (kb_played_note_t *)&kb_played_note[kb][0];
      for(key=0; key<KEYBOARD_MAX_KEYS; ++key, ++kbn) {
	if( kbn->midi_ports ) {
	  any_key_was_played = 1;
	  break;
	}
      }
    }
  }
#endif

  if( !any_key_was_played ) {
    KEYBOARD_Init(0);

    // disable keyboard SR assignments by default
    int kb;
    keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
    for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
      kc->num_rows = 0;
      kc->dout_sr1 = 0;
      kc->dout_sr2 = 0;
      kc->din_sr1 = 0;
      kc->din_sr2 = 0;

      // due to slower scan rate:
      kc->delay_fastest = 5;
      kc->delay_fastest_black_keys = 0; // if 0, we take delay_fastest, otherwise we take the dedicated value for the black keys
      kc->delay_slowest = 100;
    }

    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by keyboard.c when a key has been toggled
//! It's activated from mios32_config.h:
//! #define KEYBOARD_NOTIFY_TOGGLE_HOOK MBNG_KB_NotifyToggle
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_NotifyToggle(u8 kb, u8 note_number, u8 velocity)
{
  u16 hw_id = MBNG_EVENT_CONTROLLER_KB + kb + 1;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_KB_NotifyToggle(%d, %d, %d)\n", hw_id & 0xfff, note_number, velocity);
  }

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, 0, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to BUTTON hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    // for Note Off handling
    u8 new_note = 1;
#if SUPPORT_SECURE_NOTE_OFF_HANDLING
    // play original key over original port/chn
    if( !velocity && kb < KEYBOARD_NUM && note_number < KEYBOARD_MAX_KEYS ) {
      kb_played_note_t *kbn = (kb_played_note_t *)&kb_played_note[kb][note_number];

      if( kbn->midi_ports ) {
	new_note = 0;

	item.enabled_ports = kbn->midi_ports;
	item.stream[0] = (item.stream[0] & 0xf0) | (kbn->chn & 0x0f);

	if( item.flags.use_key_or_cc ) {
	  // for keyboard zones
	  if( note_number < item.min || note_number > item.max )
	    continue;

	  item.value = kbn->note;
	  item.secondary_value = 0;
	} else {
	  item.stream[1] = kbn->note;
	  item.value = 0;
	}
      }
    }
#endif
    
    if( new_note ) {
      // transpose
      s8 kb_transpose = (s8)item.custom_flags.KB.kb_transpose;
      int note = note_number + kb_transpose;
      if( item.stream_size >= 2 ) {
	if( note < 0 )
	  note = 0;
	else if( note > 127 )
	  note = 127;
      }

      // velocity map
      if( item.custom_flags.KB.kb_velocity_map ) {
	s32 mapped_velocity;
	if( (mapped_velocity=MBNG_EVENT_MapValue(item.custom_flags.KB.kb_velocity_map, velocity, 0, 0)) >= 0 ) {
	  velocity = mapped_velocity;
	}
      }

      if( item.flags.use_key_or_cc ) {
	// for keyboard zones
	if( note_number < item.min || note_number > item.max )
	  continue;
	
	item.value = note;
	item.secondary_value = velocity;
      } else {
	item.stream[1] = note;

	if( velocity == 0 ) { // we should always send Note-Off on velocity==0
	  item.value = 0;
	} else {
	  // scale 7bit value between min/max with fixed point artithmetic
	  int value = velocity;

	  s32 mapped_value;
	  if( (mapped_value=MBNG_EVENT_MapValue(item.map, value, 0, 0)) >= 0 ) {
	    velocity = mapped_value;
	  } else {
	    s16 min = item.min;
	    s16 max = item.max;

	    if( min <= max ) {
	      velocity = min + (((256*velocity)/128) * (max-min+1) / 256);
	    } else {
	      velocity = min - (((256*velocity)/128) * (min-max+1) / 256);
	    }
	  }

	  item.value = velocity;
	}
      }

#if SUPPORT_SECURE_NOTE_OFF_HANDLING
      // store original key/port/chn
      if( velocity && kb < KEYBOARD_NUM && note_number < KEYBOARD_MAX_KEYS ) {
	kb_played_note_t *kbn = (kb_played_note_t *)&kb_played_note[kb][note_number];

	kbn->midi_ports = item.enabled_ports;
	kbn->chn = item.stream[0] & 0x0f;
	kbn->note = note;
      }
#endif
    }

    if( MBNG_EVENT_NotifySendValue(&item) == 2 )
      break; // stop has been requested
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_KB_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the "break_is_make" switch
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_BreakIsMakeSet(u8 kb, u8 value)
{
  if( kb >= KEYBOARD_NUM )
    return -1; // invalid keyboard

  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];
  kc->break_is_make = value ? 1 : 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Play Off Events
//! Returns number of played keys
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_AllNotesOff(u8 kb)
{
  s32 num_played_keys = 0;

  if( kb >= KEYBOARD_NUM )
    return -1; // invalid keyboard

#if SUPPORT_SECURE_NOTE_OFF_HANDLING
  int key;
  kb_played_note_t *kbn = (kb_played_note_t *)&kb_played_note[kb][0];
  for(key=0; key<KEYBOARD_MAX_KEYS; ++key, ++kbn) {
    if( kbn->midi_ports ) {
      ++num_played_keys;

      int i;
      u32 mask = 1;
      for(i=0; i<16; ++i, mask <<= 1) {
	if( kbn->midi_ports & mask ) {
	  // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	  mios32_midi_port_t port = USB0 + ((i&0x1c) << 2) + (i&3);
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendNoteOn(port, kbn->chn, kbn->note, 0);
	  MUTEX_MIDIOUT_GIVE;
	}
      }

      kbn->midi_ports = 0; // don't play NoteOff again
    }
  }
#endif

  return num_played_keys;
}

//! \}
