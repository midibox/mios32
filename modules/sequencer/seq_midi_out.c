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

static seq_midi_out_queue_item_t *SEQ_MIDI_OUT_SlotMalloc(void);
static void SEQ_MIDI_OUT_SlotFree(seq_midi_out_queue_item_t *item);
static void SEQ_MIDI_OUT_SlotFreeHeap(void);


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
  midi_queue = NULL;

  seq_midi_out_allocated = 0;
#if SEQ_MIDI_OUT_MALLOC_ANALYSIS
  seq_midi_out_max_allocated = 0;
  seq_midi_out_dropouts = 0;
#endif

#if SEQ_MIDI_OUT_MALLOC_METHOD >= 0 && SEQ_MIDI_OUT_MALLOC_METHOD <= 3
  alloc_pos = 0;
  alloc_heap = NULL; // memory will be allocated with first event

  int i;
  for(i=0; i<(SEQ_MIDI_OUT_MAX_EVENTS/8); ++i)
    alloc_flags[i] = 0;
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function schedules a MIDI event, which will be sent over a given
//! port at a given bpm_tick
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_package MIDI package
//! \param[in] event_type the event type
//! \param[in] timestamp the bpm_tick value at which the event should be sent
//! \return 0 if event has been scheduled successfully
//! \return -1 if out of memory
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package, seq_midi_out_event_type_t event_type, u32 timestamp)
{
  // create new item
  seq_midi_out_queue_item_t *new_item;
  if( (new_item=SEQ_MIDI_OUT_SlotMalloc()) == NULL ) {
    return -1; // allocation error
  } else {
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
//! This function empties the queue and plays all "off" events
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_OUT_FlushQueue(void)
{
  seq_midi_out_queue_item_t *item;
  while( (item=midi_queue) != NULL ) {
    if( item->event_type == SEQ_MIDI_OUT_OffEvent )
      MIOS32_MIDI_SendPackage(item->port, item->package);

    midi_queue = item->next;
    SEQ_MIDI_OUT_SlotFree(item);
  }

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
    SEQ_MIDI_OUT_SlotFree(item);
  }

  SEQ_MIDI_OUT_SlotFreeHeap();

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Local function to allocate memory
// returns NULL if no memory free
/////////////////////////////////////////////////////////////////////////////
static seq_midi_out_queue_item_t *SEQ_MIDI_OUT_SlotMalloc(void)
{
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
      return NULL;
#endif
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

    ix = ++ix % (SEQ_MIDI_OUT_MAX_EVENTS / SEQ_MIDI_OUT_MALLOC_FLAG_WIDTH);
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


/////////////////////////////////////////////////////////////////////////////
// Local function to free the allocated heap memory
/////////////////////////////////////////////////////////////////////////////
static void SEQ_MIDI_OUT_SlotFreeHeap(void)
{
#if SEQ_MIDI_OUT_MALLOC_METHOD == 4
  // not relevant
#elif SEQ_MIDI_OUT_MALLOC_METHOD == 5
  // not relevant
#else
  vPortFree(alloc_heap);
  alloc_heap = NULL;
#endif
}

//! \}
