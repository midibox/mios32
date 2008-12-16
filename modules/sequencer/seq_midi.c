// $Id$
/*
 * MIDI Output Routines
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

#include "seq_midi_out.h"
#include "seq_bpm.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// an item of the MIDI output queue
typedef struct seq_midi_out_queue_item_t {
  u8                    port;
  u8                    event_type;
  mios32_midi_package_t package;
  u32                   timestamp;
  struct seq_midi_out_queue_item_t *next;
} seq_midi_out_queue_item_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u32 seq_midi_out_queue_size; // for analysis purposes


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_midi_out_queue_item_t *midi_queue;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Init(u32 mode)
{
  midi_queue = NULL;
  seq_midi_out_queue_size = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a MIDI event over given port at a given time
// returns -1 if out of memory
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package, seq_midi_out_event_type_t event_type, u32 timestamp)
{
  // create new item
  seq_midi_out_queue_item_t *new_item;
  if( (new_item=(seq_midi_out_queue_item_t *)pvPortMalloc(sizeof(seq_midi_out_queue_item_t))) == NULL ) {
    return -1; // allocation error
  } else {
    ++seq_midi_out_queue_size;

    new_item->port = port;
    new_item->package = midi_package;
    new_item->event_type = event_type;
    new_item->timestamp = timestamp;
    new_item->next = NULL;
  }
 
#if 0
  printf("[SEQ_MIDI_OUT_Send:%d] %02x %02x %02x\n\r", timestamp, midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
#endif

  // search in queue for last item which has the same (or earlier) timestamp
  seq_midi_out_queue_item_t *item;
  if( (item=midi_queue) == NULL ) {
    // no item in queue -- first element
    midi_queue = new_item;
  } else {
    u8 insert_before_item = 0;
    seq_midi_out_queue_item_t *last_item = midi_queue;
    seq_midi_out_queue_item_t *next_item;
    do {
      // Clock and Tempo events are sorted before CC and Note events at a given timestamp
      if( (event_type == SEQ_MIDI_OUT_ClkEvent || event_type == SEQ_MIDI_OUT_TempoEvent ) && 
	  item->timestamp == timestamp &&
	  (item->event_type == SEQ_MIDI_OUT_OnEvent || item->event_type == SEQ_MIDI_OUT_CCEvent) ) {
	// found any event with same timestamp, insert clock before these events
	// note that the Clock event order doesn't get lost if clock events 
	// are queued at the same timestamp (e.g. MIDI start -> MIDI clock)
	insert_before_item = 1;
	break;
      }

      // CCs are sorted before notes at a given timestamp
      // (new CC before On events at the same timestamp)
      // CCs are still played after Off or Clock events
      if( event_type == SEQ_MIDI_OUT_CCEvent && 
	  item->timestamp == timestamp &&
	  item->event_type == SEQ_MIDI_OUT_OnEvent ) {
	// found On event with same timestamp, play CC before On event
	insert_before_item = 1;
	break;
      }

      if( (next_item=item->next) == NULL ) {
	// end of queue reached, insert new item at the end
	break;
      }
	
      if( next_item->timestamp > timestamp ) {
	// found entry with later timestamp
	break;
      }

      // switch to next item
      last_item = item;
      item = next_item;
    } while( 1 );

    // insert/add item into/to list
    if( insert_before_item ) {
      if( last_item == midi_queue )
	midi_queue = new_item;
      else
	last_item->next = new_item;
      new_item->next = item;
    } else {
      item->next = new_item;
      new_item->next = next_item;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function empties the queue and plays all off events
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_FlushQueue(void)
{
  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL ) {
    if( item->event_type == SEQ_MIDI_OUT_OffEvent )
      MIOS32_MIDI_SendPackage(item->port, item->package);

    midi_queue = item->next;
    vPortFree(item);
    --seq_midi_out_queue_size;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called periodically (1 mS) to check timestamped
// MIDI events which have to be sent
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Handler(void)
{
  // search in queue for items which have to be played now (or have been missed earlier)
  // note that we are going through a sorted list, therefore we can exit once a timestamp
  // has been found which has to be played later than now

  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL && item->timestamp <= SEQ_BPM_TickGet() ) {
#if 0
    printf("[SEQ_MIDI_OUT_Handler:%d] %02x %02x %02x\n\r", item->timestamp, item->package.evnt0, item->package.evnt1, item->package.evnt2);
#endif

    // if tempo event: change BPM stored in midi_package.ALL
    if( item->event_type == SEQ_MIDI_OUT_TempoEvent ) {
      SEQ_BPM_Set(item->package.ALL);
    } else {
      MIOS32_MIDI_SendPackage(item->port, item->package);
    }

    midi_queue = item->next;
    vPortFree(item);
    --seq_midi_out_queue_size;
  }

  return 0; // no error
}
