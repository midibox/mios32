// $Id$
//! \defgroup SEQ_MIDI_OUT
//!
//! Functions for schedules MIDI output
//!
//! \{
/* ==========================================================================
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

#if SEQ_MIDI_OUT_MALLOC_METHOD != 5
// FreeRTOS based malloc required
#include <FreeRTOS.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// an item of the MIDI output queue
typedef struct seq_midi_out_queue_item_t {
  u8                    port;
  u8                    event_type;
  u16                   len;
  mios32_midi_package_t package;
  u32                   timestamp;
  struct seq_midi_out_queue_item_t *next;
} seq_midi_out_queue_item_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static seq_midi_out_queue_item_t *SEQ_MIDI_OUT_SlotMalloc(void);
static void SEQ_MIDI_OUT_SlotFree(seq_midi_out_queue_item_t *item);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

//! contains number of events which have allocated memory
u32 seq_midi_out_allocated;

//! only for analysis purposes - has to be enabled with SEQ_MIDI_OUT_MALLOC_ANALYSIS
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
u32 seq_midi_out_max_allocated;
u32 seq_midi_out_dropouts;
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_midi_out_queue_item_t *midi_queue;


#if SEQ_MIDI_OUT_MALLOC_METHOD >= 0 && SEQ_MIDI_OUT_MALLOC_METHOD <= 3

// determine flag array width and mask
#if SEQ_MIDI_OUT_MALLOC_METHOD == 0
# define SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH 1
# define SEQ_MIDI_OUT_MALLOC_FLAG_MASK  1
  static u8 alloc_flags[SEQ_MIDI_OUT_MAX_EVENTS];
#elif SEQ_MIDI_OUT_MALLOC_METHOD == 1
# define SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH 8
# define SEQ_MIDI_OUT_MALLOC_FLAG_MASK  0xff
  static u8 alloc_flags[SEQ_MIDI_OUT_MAX_EVENTS/8];
#elif SEQ_MIDI_OUT_MALLOC_METHOD == 2
# define SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH 16
# define SEQ_MIDI_OUT_MALLOC_FLAG_MASK  0xffff
  static u16 alloc_flags[SEQ_MIDI_OUT_MAX_EVENTS/16];
#else
# define SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH 32
# define SEQ_MIDI_OUT_MALLOC_FLAG_MASK  0xffffffff
  static u32 alloc_flags[SEQ_MIDI_OUT_MAX_EVENTS/32];
#endif

// Note: we could easily provide an option for static heap allocation as well
static seq_midi_out_queue_item_t *alloc_heap;
static u32 alloc_pos;
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initialisation of MIDI output scheduler
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Init(u32 mode)
{
  // don't re-initialize queue to ensure that memory can be delocated properly
  // when this function is called multiple times
  // we assume, that gcc will always fill the memory range with zero on application start
  //  midi_queue = NULL;

  seq_midi_out_allocated = 0;
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
  seq_midi_out_max_allocated = 0;
  seq_midi_out_dropouts = 0;
#endif

  // memory will be allocated with first event
  SEQ_MIDI_OUT_FreeHeap();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function schedules a MIDI event, which will be sent over a given
//! port at a given bpm_tick
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_package MIDI package
//! If the re-schedule feature SEQ_MIDI_OUT_ReSchedule() should be used, the
//! mios32_midi_package_t.cable field should be initialized with a tag (supported
//! range: 0-15)
//! \param[in] event_type the event type
//! \param[in] timestamp the bpm_tick value at which the event should be sent
//! \return 0 if event has been scheduled successfully
//! \return -1 if out of memory
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package, seq_midi_out_event_type_t event_type, u32 timestamp, u32 len)
{
  // failsave measure:
  // don't take On or OnOff item if heap is almost completely allocated
  if( seq_midi_out_allocated >= (SEQ_MIDI_OUT_MAX_EVENTS-2) && // should be enough for a On *and* Off event
      (event_type == SEQ_MIDI_OUT_OnEvent || event_type == SEQ_MIDI_OUT_OnOffEvent) ) {
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
    return -1; // allocation error
  };

  // create new item
  seq_midi_out_queue_item_t *new_item;
  if( (new_item=SEQ_MIDI_OUT_SlotMalloc()) == NULL ) {
    return -1; // allocation error
  } else {
    new_item->port = port;
    new_item->package = midi_package;
    new_item->event_type = event_type;
    new_item->timestamp = timestamp;
    new_item->len = len;
    new_item->next = NULL;
  }

#if DEBUG_VERBOSE_LEVEL >= 1
#if DEBUG_VERBOSE_LEVEL == 1
  if( event_type != SEQ_MIDI_OUT_ClkEvent )
#endif
  DEBUG_MSG("[SEQ_MIDI_OUT_Send:%u] (tag %d) %02x %02x %02x len:%u\n", timestamp, midi_package.cable, midi_package.evnt0, midi_package.evnt1, midi_package.evnt2, len);
#endif

  // search in queue for last item which has the same (or earlier) timestamp
  seq_midi_out_queue_item_t *item;
  if( (item=midi_queue) == NULL ) {
    // no item in queue -- first element
    midi_queue = new_item;
  } else {
    u8 insert_before_item = 0;
    seq_midi_out_queue_item_t *last_item = NULL;
    seq_midi_out_queue_item_t *next_item;
    do {
      // Clock and Tempo events are sorted before CC and Note events at a given timestamp
      if( (event_type == SEQ_MIDI_OUT_ClkEvent || event_type == SEQ_MIDI_OUT_TempoEvent ) && 
	  item->timestamp >= timestamp &&
	  (item->event_type == SEQ_MIDI_OUT_OnEvent || 
	   item->event_type == SEQ_MIDI_OUT_OffEvent || 
	   item->event_type == SEQ_MIDI_OUT_OnOffEvent || 
	   item->event_type == SEQ_MIDI_OUT_CCEvent) ) {
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
	  (item->event_type == SEQ_MIDI_OUT_OnEvent || item->event_type == SEQ_MIDI_OUT_OnOffEvent) ) {
	// found On event with same timestamp, play CC before On event
	insert_before_item = 1;
	break;
      }

      if( item->timestamp > timestamp ) {
	// found entry with later timestamp
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
      if( last_item == NULL )
	midi_queue = new_item;
      else
	last_item->next = new_item;
      new_item->next = item;
    } else {
      item->next = new_item;
      new_item->next = next_item;
    }
  }

  // schedule off event now if length > 16bit (since it cannot be stored in event record)
  if( event_type == SEQ_MIDI_OUT_OnOffEvent && len > 0xffff ) {
    return SEQ_MIDI_OUT_Send(port, midi_package, event_type, timestamp+len, 0);
  }

  // display queue
#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("--- vvv ---\n");
  item=midi_queue;
  while( item != NULL ) {
    DEBUG_MSG("[%u] (tag %d) %02x %02x %02x len:%u\n", item->timestamp, item->package.cable, item->package.evnt0, item->package.evnt1, item->package.evnt2, item->len);
    item = item->next;
  }
  DEBUG_MSG("--- ^^^ ---\n");
  
#endif


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function re-schedules MIDI Off/OnOff events assigned to a given "tag"
//! (0..15, stored in mios32_midi_package_t.cable of events which already have been
//! sent.
//!
//! Usually only SEQ_MIDI_OUT_Off events will be re-scheduled, all events
//! which don't match the event_type will be ignored so that no special tag is required
//! for such events.
//!
//! Usecase: sustained notes can be realized this way: schedule a Note Off
//! event at timestamp 0xffffffff, reschedule it to timestamp == bpm_tick
//! once the sequencer determined, that the off event should be played.
//!
//! \param[in] tag (0..15) the mios32_midi_package.t.cable number of events which should be re-scheduled
//! \param[in] event_type the event type which should be rescheduled
//! \param[in] timestamp the bpm_tick value at which the event should be sent
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_ReSchedule(u8 tag, seq_midi_out_event_type_t event_type, u32 timestamp)
{
  // search in queue for items with the given tag

  seq_midi_out_queue_item_t *prev_item = NULL;
  seq_midi_out_queue_item_t *item = midi_queue;
  while( item != NULL ) {
    // filter event_type and tag
    // and ignore events, which will be played with next invocation of the Out Handler to avoid,
    // that a re-scheduled event will be checked again
    if( (item->event_type == event_type) && (item->package.cable == tag) && (item->timestamp > timestamp) ) {

      // ensure that we get a free memory slot by releasing the current item before queuing the off item
      seq_midi_out_queue_item_t copy = *item;

      // remove item from queue
      seq_midi_out_queue_item_t *next_item = item->next;
      SEQ_MIDI_OUT_SlotFree(item);

      // fix link to next item
      if( prev_item == NULL ) {
	if( next_item == NULL ) {
	  midi_queue = NULL;
	  item = NULL;
	} else {
	  item = midi_queue;
	  item->next = next_item;
	}
      } else {
	item = next_item;
	prev_item->next = item;
      }

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDI_OUT_ReSchedule:%u] (tag %d) %02x %02x %02x\n", timestamp, copy.package.cable, copy.package.evnt0, copy.package.evnt1, copy.package.evnt2);
#endif

      // re-schedule copied item at new timestamp
      SEQ_MIDI_OUT_Send(copy.port, copy.package, copy.event_type, timestamp, copy.len);
    } else {
      // switch to next item
      prev_item = item;
      item = item->next;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function empties the queue and plays all "off" events
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_FlushQueue(void)
{
  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL ) {
    if( item->event_type == SEQ_MIDI_OUT_OffEvent || item->event_type == SEQ_MIDI_OUT_OnOffEvent ) {
      item->package.velocity = 0; // ensure that velocity is 0
      MIOS32_MIDI_SendPackage(item->port, item->package);
    }

    midi_queue = item->next;
    SEQ_MIDI_OUT_SlotFree(item);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function frees the complete allocated memory.<BR>
//! It should only be called after SEQ_MIDI_OUT_FlushQueue to prevent stucking
//! Note events
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_FreeHeap(void)
{
  // ensure that all items are delocated
  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL ) {
    midi_queue = item->next;
    SEQ_MIDI_OUT_SlotFree(item);
  }

  // free memory
#if SEQ_MIDI_OUT_MALLOC_METHOD == 4
  // not relevant
#elif SEQ_MIDI_OUT_MALLOC_METHOD == 5
  // not relevant
#else
  if( alloc_heap != NULL ) {
    vPortFree(alloc_heap);
    alloc_heap = NULL;
  }

  alloc_pos = 0;
  seq_midi_out_allocated = 0;

  int i;
  for(i=0; i<(SEQ_MIDI_OUT_MAX_EVENTS/SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH); ++i)
    alloc_flags[i] = 0;
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically (1 mS) to check for timestamped
//! MIDI events which have to be sent.
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Handler(void)
{
  // exit if BPM generator not running
  if( !SEQ_BPM_IsRunning() )
    return 0;

  // search in queue for items which have to be played now (or have been missed earlier)
  // note that we are going through a sorted list, therefore we can exit once a timestamp
  // has been found which has to be played later than now

  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL && item->timestamp <= SEQ_BPM_TickGet() ) {
#if DEBUG_VERBOSE_LEVEL >= 1
#if DEBUG_VERBOSE_LEVEL == 1
    if( item->event_type != SEQ_MIDI_OUT_ClkEvent )
#endif
    DEBUG_MSG("[SEQ_MIDI_OUT_Handler:%u] (tag %d) %02x %02x %02x\n", item->timestamp, item->package.cable, item->package.evnt0, item->package.evnt1, item->package.evnt2);
#endif

    // if tempo event: change BPM stored in midi_package.ALL
    if( item->event_type == SEQ_MIDI_OUT_TempoEvent ) {
      SEQ_BPM_Set(item->package.ALL);
    } else {
      MIOS32_MIDI_SendPackage(item->port, item->package);
    }

    // schedule Off event if requested
    if( item->event_type == SEQ_MIDI_OUT_OnOffEvent && item->len ) {
      // ensure that we get a free memory slot by releasing the current item before queuing the off item
      seq_midi_out_queue_item_t copy = *item;
      copy.package.velocity = 0; // ensure that velocity is 0

      // remove item from queue
      midi_queue = item->next;
      SEQ_MIDI_OUT_SlotFree(item);

      SEQ_MIDI_OUT_Send(copy.port, copy.package, SEQ_MIDI_OUT_OffEvent, copy.len + copy.timestamp, 0);
    } else {
      // remove item from queue
      midi_queue = item->next;
      SEQ_MIDI_OUT_SlotFree(item);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local function to allocate memory
// returns NULL if no memory free
/////////////////////////////////////////////////////////////////////////////
static seq_midi_out_queue_item_t *SEQ_MIDI_OUT_SlotMalloc(void)
{
  // limit max number of allocted items for all methods
  if( seq_midi_out_allocated >= SEQ_MIDI_OUT_MAX_EVENTS ) {
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
    return NULL;
  }

#if SEQ_MIDI_OUT_MALLOC_METHOD == 4 || SEQ_MIDI_OUT_MALLOC_METHOD == 5
  seq_midi_out_queue_item_t *item;
#if SEQ_MIDI_OUT_MALLOC_METHOD == 4
  if( (item=(seq_midi_out_queue_item_t *)pvPortMalloc(sizeof(seq_midi_out_queue_item_t))) == NULL ) {
#else
  if( (item=(seq_midi_out_queue_item_t *)malloc(sizeof(seq_midi_out_queue_item_t))) == NULL ) {
#endif

#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
    return NULL;
  }

  ++seq_midi_out_allocated;
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
  if( seq_midi_out_allocated > seq_midi_out_max_allocated )
    seq_midi_out_max_allocated = seq_midi_out_allocated;
#endif

  return item;
#else

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  // allocate memory if this hasn't been done yet
  if( alloc_heap == NULL ) {
    alloc_heap = (seq_midi_out_queue_item_t *)pvPortMalloc(
      sizeof(seq_midi_out_queue_item_t)*SEQ_MIDI_OUT_MAX_EVENTS);
    if( alloc_heap == NULL ) {
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
      ++seq_midi_out_dropouts;
#endif
      return NULL;
    }
  }

  // is there still a free slot?
  if( seq_midi_out_allocated >= SEQ_MIDI_OUT_MAX_EVENTS ) {
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
    return NULL;
  }

  // search for next free slot
  s32 new_pos = -1;

  s32 i;
#if SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH == 1
  // start with +1, since the chance is higher that this block is free
  s32 ix = (alloc_pos + 1) % SEQ_MIDI_OUT_MAX_EVENTS;
  for(i=0; i<SEQ_MIDI_OUT_MAX_EVENTS; ++i) {
    if( !alloc_flags[ix] ) {
      alloc_flags[ix] = 1;
      new_pos = ix;
      break;
    }

    ix = ++ix % SEQ_MIDI_OUT_MAX_EVENTS;
  }
#else
  s32 ix = ((alloc_pos/SEQ_MIDI_OUT_MAX_EVENTS) + 1) % (SEQ_MIDI_OUT_MAX_EVENTS / SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH);
  u32 mask;
  for(i=0; i<SEQ_MIDI_OUT_MAX_EVENTS; ++i) {
    u32 flags;
    if( (flags=alloc_flags[ix]) != SEQ_MIDI_OUT_MALLOC_FLAG_MASK ) {
      mask = (1 << 0);
      u8 j;
      for(j=0; j<SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH; ++j) {
	if( (flags & mask) == 0 ) {
	  new_pos = SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH*ix + j;
	  alloc_flags[ix] |= mask;
	  break;
	}
	mask <<= 1;
      }
      if( j < SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH ) {
	break;
      }

      // we should never reach this point! (can be checked by setting a breakpoint or printf to this location)
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
      return NULL;
    }

    ++ix;
    ix %= (SEQ_MIDI_OUT_MAX_EVENTS / SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH);
  }
#endif

  if( new_pos == -1 ) {
    // should never happen! (can be checked by setting a breakpoint or printf to this location)
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
    ++seq_midi_out_dropouts;
#endif
    return NULL;
  }

  alloc_pos = new_pos;
  ++seq_midi_out_allocated;

#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
  if( seq_midi_out_allocated > seq_midi_out_max_allocated )
    seq_midi_out_max_allocated = seq_midi_out_allocated;
#endif

  return &alloc_heap[alloc_pos];
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Local function to free memory
/////////////////////////////////////////////////////////////////////////////
static void SEQ_MIDI_OUT_SlotFree(seq_midi_out_queue_item_t *item)
{
#if SEQ_MIDI_OUT_MALLOC_METHOD == 4
  vPortFree(item);
  --seq_midi_out_allocated;
#elif SEQ_MIDI_OUT_MALLOC_METHOD == 5
  free(item);
  --seq_midi_out_allocated;
#else

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  if( item >= alloc_heap ) {
    u32 pos = item - alloc_heap;
    if( pos < SEQ_MIDI_OUT_MAX_EVENTS ) {
#if SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH == 1
      alloc_flags[pos] = 0;
#else
      alloc_flags[pos/SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH] &= ~(1 << (pos%SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH));
#endif
      if( seq_midi_out_allocated ) // TODO: check why it can happen that out_allocated == 0
	--seq_midi_out_allocated;
    } else {
      // should never happen! (can be checked by setting a breakpoint or ptintf to this location)
      return;
    }
  } else {
    // should never happen! (can be checked by setting a breakpoint or printf to this location)
    return;
  }
#endif
}


//! \}
