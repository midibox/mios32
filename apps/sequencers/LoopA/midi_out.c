// $Id$
//! \defgroup LOOPA_MIDI_OUT
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

#include "midi_out.h"
#include "seq_bpm.h"

#if LOOPA_MIDI_OUT_MALLOC_METHOD != 5
// FreeRTOS based malloc required
#include <FreeRTOS.h>

#endif


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#ifndef DEBUG_MSG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// an item of the MIDI output queue
typedef struct loopa_midi_out_queue_item_t
{
   u8 port;
   u8 event_type;
   u16 len;
   mios32_midi_package_t package;
   s16 ticksUntilPlayback;
   struct loopa_midi_out_queue_item_t *next;
} loopa_midi_out_queue_item_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static loopa_midi_out_queue_item_t *LOOPA_MIDI_OUT_SlotMalloc(void);

static void LOOPA_MIDI_OUT_SlotFree(loopa_midi_out_queue_item_t *item);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

//! contains number of events which have allocated memory
u32 loopa_midi_out_allocated;

//! only for analysis purposes - has to be enabled with LOOPA_MIDI_OUT_MALLOC_ANALYSIS
#if LOOPA_MIDI_OUT_MALLOC_ANALYSIS
u32 loopa_midi_out_max_allocated;
u32 loopa_midi_out_dropouts;
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 (*callback_midi_send_package)(mios32_midi_port_t port, mios32_midi_package_t midi_package);

static s32 (*callback_bpm_is_running)(void);

static u32 (*callback_bpm_tick_get)(void);

static s32 (*callback_bpm_set)(float bpm);

static loopa_midi_out_queue_item_t *midi_queue;


#if LOOPA_MIDI_OUT_MALLOC_METHOD >= 0 && LOOPA_MIDI_OUT_MALLOC_METHOD <= 3

// determine flag array width and mask
#if LOOPA_MIDI_OUT_MALLOC_METHOD == 0
# define LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH 1
# define LOOPA_MIDI_OUT_MALLOC_FLAG_MASK  1
static u8 alloc_flags[LOOPA_MIDI_OUT_MAX_EVENTS];
#elif LOOPA_MIDI_OUT_MALLOC_METHOD == 1
# define LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH 8
# define LOOPA_MIDI_OUT_MALLOC_FLAG_MASK  0xff
static u8 alloc_flags[LOOPA_MIDI_OUT_MAX_EVENTS/8];
#elif LOOPA_MIDI_OUT_MALLOC_METHOD == 2
# define LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH 16
# define LOOPA_MIDI_OUT_MALLOC_FLAG_MASK  0xffff
static u16 alloc_flags[LOOPA_MIDI_OUT_MAX_EVENTS/16];
#else
# define LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH 32
# define LOOPA_MIDI_OUT_MALLOC_FLAG_MASK  0xffffffff
static u32 alloc_flags[LOOPA_MIDI_OUT_MAX_EVENTS / 32];
#endif

// Note: we could easily provide an option for static heap allocation as well
static loopa_midi_out_queue_item_t *alloc_heap;
static u32 alloc_pos;
#endif

#if LOOPA_MIDI_OUT_SUPPORT_DELAY
#define PPQN_DELAY_NUM 256
static s8 ppqn_delay[PPQN_DELAY_NUM];
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initialisation of MIDI output scheduler
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Init(u32 mode)
{
   // install default callback functions (selected with NULL)
   LoopA_MIDI_OUT_Callback_MIDI_SendPackage_Set(NULL);
   LoopA_MIDI_OUT_Callback_BPM_IsRunning_Set(NULL);
   LoopA_MIDI_OUT_Callback_BPM_TickGet_Set(NULL);
   LoopA_MIDI_OUT_Callback_BPM_Set_Set(NULL);

   // don't re-initialize queue to ensure that memory can be delocated properly
   // when this function is called multiple times
   // we assume, that gcc will always fill the memory range with zero on application start
   //  midi_queue = NULL;

   loopa_midi_out_allocated = 0;
#if LOOPA_MIDI_OUT_MALLOC_ANALYSIS
   loopa_midi_out_max_allocated = 0;
   loopa_midi_out_dropouts = 0;
#endif

   // memory will be allocated with first event
   LoopA_MIDI_OUT_FreeHeap();

#if LOOPA_MIDI_OUT_SUPPORT_DELAY
   {
     int i;
     for(i=0; i<PPQN_DELAY_NUM; ++i) {
       ppqn_delay[i] = 0;
     }
   }
#endif

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Allows to change the function which is called whenever a MIDI package
//! should be sent.
//!
//! This becomes useful if the MIDI output should be filtered, converted
//! or rendered into a MIDI file.
//!
//! \param[in] *_callback_midi_send_package pointer to callback function:<BR>
//! \code
//!   s32 callback_midi_send_package(mios32_midi_port_t port, mios32_midi_package_t midi_package)
//!   {
//!     // ...
//!     // do something with port and midi_package
//!     // ...
//!
//!     return 0; // no error
//!   }
//! \endcode
//! If set to NULL, the default function MIOS32_MIDI_SendPackage function will
//! be used. This allows you to restore the default setup properly.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Callback_MIDI_SendPackage_Set(void *_callback_midi_send_package)
{
   callback_midi_send_package = (_callback_midi_send_package == NULL)
                                ? MIOS32_MIDI_SendPackage
                                : _callback_midi_send_package;

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Allows to change the function which is called to check if the BPM generator
//! is running.
//!
//! Together with the other callback functions, this becomes useful if the MIDI
//! output should be rendered into a MIDI file (in this case, the function
//! should return 1, so that LOOPA_MIDI_OUT_Handler handler will generate MIDI output)
//!
//! \param[in] *_callback_bpm_is_running pointer to callback function:<BR>
//! \code
//!   s32 callback_bpm_is_running(void)
//!   {
//!     return 1; // always running
//!   }
//! \endcode
//! If set to NULL, the default function SEQ_BPM_IsRunning function will
//! be used. This allows you to restore the default setup properly.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Callback_BPM_IsRunning_Set(void *_callback_bpm_is_running)
{
   callback_bpm_is_running = (_callback_bpm_is_running == NULL)
                             ? SEQ_BPM_IsRunning
                             : _callback_bpm_is_running;

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Allows to change the function which is called to retrieve the current
//! BPM tick.
//!
//! Together with the other callback functions, this becomes useful if the MIDI
//! output should be rendered into a MIDI file (in this case, the function
//! should return a number which is incremented after each rendering step).
//!
//! \param[in] *_callback_bpm_tick_get pointer to callback function:<BR>
//! \code
//!   u32 callback_bpm_tick_get(void)
//!   {
//!     return my_bpm_tick; // this variable will be incremented after each rendering step
//!   }
//! \endcode
//! If set to NULL, the default function SEQ_BPM_TickGet function will
//! be used. This allows you to restore the default setup properly.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Callback_BPM_TickGet_Set(void *_callback_bpm_tick_get)
{
   callback_bpm_tick_get = (_callback_bpm_tick_get == NULL)
                           ? SEQ_BPM_TickGet
                           : _callback_bpm_tick_get;

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Allows to change the function which is called to change the song tempo.
//!
//! Together with the other callback functions, this becomes useful if the MIDI
//! output should be rendered into a MIDI file (in this case, the function
//! should add the appr. meta event into the MIDI file).
//!
//! \param[in] *_callback_bpm_set pointer to callback function:<BR>
//! \code
//!   u32 callback_bpm_set(float bpm)
//!   {
//!     // ...
//!     // do something with bpm value
//!     // ...
//!
//!     return 0; // no error
//!   }
//! \endcode
//! If set to NULL, the default function SEQ_BPM_Set function will
//! be used. This allows you to restore the default setup properly.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Callback_BPM_Set_Set(void *_callback_bpm_set)
{
   callback_bpm_set = (_callback_bpm_set == NULL)
                      ? SEQ_BPM_Set
                      : _callback_bpm_set;

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function schedules a MIDI event, which will be sent over a given
//! port at a given bpm_tick
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_package MIDI package
//! If the re-schedule feature LOOPA_MIDI_OUT_ReSchedule() should be used, the
//! mios32_midi_package_t.cable field should be initialized with a tag (supported
//! range: 0-15)
//! \param[in] event_type the event type
//! \param[in] ticksUntilPlayback the bpm_tick value at which the event should be sent
//! \return 0 if event has been scheduled successfully
//! \return -1 if out of memory
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package,
                        loopa_midi_out_event_type_t event_type,
                        u32 ticksUntilPlayback, u32 len)
{
   // failsave measure:
   // don't take On or OnOff item if heap is almost completely allocated
   if (loopa_midi_out_allocated >= (LOOPA_MIDI_OUT_MAX_EVENTS - 2) && // should be enough for a On *and* Off event
       (event_type == LOOPA_MIDI_OUT_OnEvent || event_type == LOOPA_MIDI_OUT_OnOffEvent))
   {
      return -1; // allocation error
   };

   // create new item
   loopa_midi_out_queue_item_t *new_item;
   if ((new_item = LOOPA_MIDI_OUT_SlotMalloc()) == NULL)
   {
      return -1; // allocation error
   }
   else
   {
      new_item->port = port;
      new_item->package = midi_package;
      new_item->event_type = event_type;
      new_item->ticksUntilPlayback = ticksUntilPlayback;
      new_item->len = len;
      new_item->next = NULL;
   }

#if DEBUG_VERBOSE_LEVEL >= 2
#if DEBUG_VERBOSE_LEVEL == 2
   if( event_type != LOOPA_MIDI_OUT_ClkEvent )
#endif
   DEBUG_MSG("[LoopA_MIDI_OUT_Send:%u] (tag %d) %02x %02x %02x len:%u @%u\n", ticksUntilPlayback, midi_package.cable, midi_package.evnt0, midi_package.evnt1, midi_package.evnt2, len, SEQ_BPM_TickGet());
#endif

   // search in queue for last item which has the same (or earlier) ticksUntilPlayback
   loopa_midi_out_queue_item_t *item;
   if ((item = midi_queue) == NULL)
   {
      // no item in queue -- first element
      midi_queue = new_item;
   }
   else
   {
      u8 insert_before_item = 0;
      loopa_midi_out_queue_item_t *last_item = NULL;
      loopa_midi_out_queue_item_t *next_item;
      do
      {
         // Clock and Tempo events are sorted before CC and Note events at a given ticksUntilPlayback
         if ((event_type == LOOPA_MIDI_OUT_ClkEvent || event_type == LOOPA_MIDI_OUT_TempoEvent) &&
             item->ticksUntilPlayback >= ticksUntilPlayback &&
             (item->event_type == LOOPA_MIDI_OUT_OnEvent ||
              item->event_type == LOOPA_MIDI_OUT_OffEvent ||
              item->event_type == LOOPA_MIDI_OUT_OnOffEvent ||
              item->event_type == LOOPA_MIDI_OUT_CCEvent))
         {
            // found any event with same ticksUntilPlayback, insert clock before these events
            // note that the Clock event order doesn't get lost if clock events 
            // are queued at the same ticksUntilPlayback (e.g. MIDI start -> MIDI clock)
            insert_before_item = 1;
            break;
         }

         // CCs are sorted before notes at a given ticksUntilPlayback
         // (new CC before On events at the same ticksUntilPlayback)
         // CCs are still played after Off or Clock events
         if (event_type == LOOPA_MIDI_OUT_CCEvent &&
             item->ticksUntilPlayback == ticksUntilPlayback &&
             (item->event_type == LOOPA_MIDI_OUT_OnEvent || item->event_type == LOOPA_MIDI_OUT_OnOffEvent))
         {
            // found On event with same ticksUntilPlayback, play CC before On event
            insert_before_item = 1;
            break;
         }

         if (item->ticksUntilPlayback > ticksUntilPlayback)
         {
            // found entry with later ticksUntilPlayback
            insert_before_item = 1;
            break;
         }

         if ((next_item = item->next) == NULL)
         {
            // end of queue reached, insert new item at the end
            break;
         }

         if (next_item->ticksUntilPlayback > ticksUntilPlayback)
         {
            // found entry with later ticksUntilPlayback
            break;
         }

         // switch to next item
         last_item = item;
         item = next_item;
      } while (1);

      // insert/add item into/to list
      if (insert_before_item)
      {
         if (last_item == NULL)
            midi_queue = new_item;
         else
            last_item->next = new_item;
         new_item->next = item;
      } else
      {
         item->next = new_item;
         new_item->next = next_item;
      }
   }

   // schedule off event now if length > 16bit (since it cannot be stored in event record)
   if (event_type == LOOPA_MIDI_OUT_OnOffEvent && len > 0xffff)
   {
      return LoopA_MIDI_OUT_Send(port, midi_package, event_type, ticksUntilPlayback + len, 0);
   }

   // display queue
#if DEBUG_VERBOSE_LEVEL >= 4
   DEBUG_MSG("--- vvv ---\n");
   item=midi_queue;
   while( item != NULL ) {
     DEBUG_MSG("[%u] (tag %d) %02x %02x %02x len:%u @%u\n", item->ticksUntilPlayback, item->package.cable, item->package.evnt0, item->package.evnt1, item->package.evnt2, item->len, SEQ_BPM_TickGet());
     item = item->next;
   }
   DEBUG_MSG("--- ^^^ ---\n");

#endif


   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function empties the queue and plays all "off" events
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_FlushQueue(void)
{
   loopa_midi_out_queue_item_t *item;
   while ((item = midi_queue) != NULL)
   {
      if (item->event_type == LOOPA_MIDI_OUT_OffEvent || item->event_type == LOOPA_MIDI_OUT_OnOffEvent)
      {
         item->package.velocity = 0; // ensure that velocity is 0
         callback_midi_send_package(item->port, item->package);
      }

      midi_queue = item->next;
      LOOPA_MIDI_OUT_SlotFree(item);
   }

   return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function frees the complete allocated memory.<BR>
//! It should only be called after LOOPA_MIDI_OUT_FlushQueue to prevent stucking
//! Note events
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_FreeHeap(void)
{
   // ensure that all items are delocated
   loopa_midi_out_queue_item_t *item;
   while ((item = midi_queue) != NULL)
   {
      midi_queue = item->next;
      LOOPA_MIDI_OUT_SlotFree(item);
   }

   // free memory
   if (alloc_heap != NULL)
   {
      vPortFree(alloc_heap);
      alloc_heap = NULL;
   }

   alloc_pos = 0;
   loopa_midi_out_allocated = 0;

   int i;
   for (i = 0; i < (LOOPA_MIDI_OUT_MAX_EVENTS / LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH); ++i)
      alloc_flags[i] = 0;

   return 0; // no error
}

/**
 *  Decrease the ticksUntilPlayback of every item in queue, called once per tick and play items that are scheduled
 *
 */
void LoopA_MIDI_OUT_Tick()
{
   loopa_midi_out_queue_item_t *item = midi_queue;
   while (item != NULL)
   {
      item->ticksUntilPlayback--;
            item = item->next;
   }

   LoopA_MIDI_OUT_Handler();
}
// ---------------------------------------------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically (1 mS) to check for timestamped
//! MIDI events which have to be sent.
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 LoopA_MIDI_OUT_Handler(void)
{
   // exit if BPM generator not running
   if (!callback_bpm_is_running())
      return 0;

   // search in queue for items which have to be played now (or have been missed earlier)
   // note that we are going through a sorted list, therefore we can exit once a ticksUntilPlayback
   // has been found which has to be played later than now

   loopa_midi_out_queue_item_t *item;
   while ((item = midi_queue) != NULL && item->ticksUntilPlayback <= 0)
   {
#if DEBUG_VERBOSE_LEVEL >= 2
#if DEBUG_VERBOSE_LEVEL == 2
      if( item->event_type != LOOPA_MIDI_OUT_ClkEvent )
#endif
      DEBUG_MSG("[LoopA_MIDI_OUT_Handler:%u] (tag %d) %02x %02x %02x @%u\n", item->ticksUntilPlayback, item->package.cable, item->package.evnt0, item->package.evnt1, item->package.evnt2, SEQ_BPM_TickGet());
#endif

      // if tempo event: change BPM stored in midi_package.ALL
      if (item->event_type == LOOPA_MIDI_OUT_TempoEvent)
         callback_bpm_set(item->package.ALL);
      else
         callback_midi_send_package(item->port, item->package);

      // schedule Off event if requested
      if (item->event_type == LOOPA_MIDI_OUT_OnOffEvent && item->len)
      {
         // ensure that we get a free memory slot by releasing the current item before queuing the off item
         loopa_midi_out_queue_item_t copy;
         copy.port = item->port;
         copy.event_type = item->event_type;
         copy.len = item->len;
         copy.package.ALL = item->package.ALL;
         copy.ticksUntilPlayback = item->ticksUntilPlayback;
         copy.next = item->next;
         copy.package.velocity = 0; // ensure that velocity is 0

         // remove item from queue
         midi_queue = item->next;
         LOOPA_MIDI_OUT_SlotFree(item);

         u32 delayed_timestamp = copy.len;

         LoopA_MIDI_OUT_Send(copy.port, copy.package, LOOPA_MIDI_OUT_OffEvent, delayed_timestamp, 0);

      }
      else
      {
         // remove item from queue
         midi_queue = item->next;
         LOOPA_MIDI_OUT_SlotFree(item);
      }
   }

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local function to allocate memory
// returns NULL if no memory free
/////////////////////////////////////////////////////////////////////////////
static loopa_midi_out_queue_item_t *LOOPA_MIDI_OUT_SlotMalloc(void)
{
   // limit max number of allocted items for all methods
   if (loopa_midi_out_allocated >= LOOPA_MIDI_OUT_MAX_EVENTS)
   {
      return NULL;
   }

   // allocate memory if this hasn't been done yet
   if (alloc_heap == NULL)
   {
      alloc_heap = (loopa_midi_out_queue_item_t *) pvPortMalloc(
              sizeof(loopa_midi_out_queue_item_t) * LOOPA_MIDI_OUT_MAX_EVENTS);
      if (alloc_heap == NULL)
      {
         return NULL;
      }
   }

   // is there still a free slot?
   if (loopa_midi_out_allocated >= LOOPA_MIDI_OUT_MAX_EVENTS)
   {
      return NULL;
   }

   // search for next free slot
   s32 new_pos = -1;

   s32 i;
#if LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH == 1
   // start with +1, since the chance is higher that this block is free
   s32 ix = (alloc_pos + 1) % LOOPA_MIDI_OUT_MAX_EVENTS;
   for(i=0; i<LOOPA_MIDI_OUT_MAX_EVENTS; ++i) {
     if( !alloc_flags[ix] ) {
       alloc_flags[ix] = 1;
       new_pos = ix;
       break;
     }
 
     ix = ++ix % LOOPA_MIDI_OUT_MAX_EVENTS;
   }
#else
   s32 ix = ((alloc_pos / LOOPA_MIDI_OUT_MAX_EVENTS) + 1) % (LOOPA_MIDI_OUT_MAX_EVENTS / LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH);
   u32 mask;
   for (i = 0; i < LOOPA_MIDI_OUT_MAX_EVENTS; ++i)
   {
      u32 flags;
      if ((flags = alloc_flags[ix]) != LOOPA_MIDI_OUT_MALLOC_FLAG_MASK)
      {
         mask = (1 << 0);
         u8 j;
         for (j = 0; j < LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH; ++j)
         {
            if ((flags & mask) == 0)
            {
               new_pos = LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH * ix + j;
               alloc_flags[ix] |= mask;
               break;
            }
            mask <<= 1;
         }
         if (j < LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH)
         {
            break;
         }

            // we should never reach this point! (can be checked by setting a breakpoint or printf to this location)
         return NULL;
      }

      ++ix;
      ix %= (LOOPA_MIDI_OUT_MAX_EVENTS / LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH);
   }
#endif

   if (new_pos == -1)
   {
      // should never happen! (can be checked by setting a breakpoint or printf to this location)
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[LOOPA_MIDI_OUT_SlotMalloc] Malfunction case #2\n");
#endif
      return NULL;
   }

   alloc_pos = new_pos;
   ++loopa_midi_out_allocated;

   return &alloc_heap[alloc_pos];
}


/////////////////////////////////////////////////////////////////////////////
// Local function to free memory
/////////////////////////////////////////////////////////////////////////////
static void LOOPA_MIDI_OUT_SlotFree(loopa_midi_out_queue_item_t *item)
{
#if LOOPA_MIDI_OUT_MALLOC_METHOD == 4
   vPortFree(item);
   --loopa_midi_out_allocated;
#elif LOOPA_MIDI_OUT_MALLOC_METHOD == 5
   free(item);
   --loopa_midi_out_allocated;
#else

   ///////////////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////////

   if (item >= alloc_heap)
   {
      u32 pos = item - alloc_heap;
      if (pos < LOOPA_MIDI_OUT_MAX_EVENTS)
      {
#if LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH == 1
         alloc_flags[pos] = 0;
#else
         alloc_flags[pos / LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH] &= ~(1 << (pos % LOOPA_MIDI_OUT_MALLOC_FLAG_WIDTH));
#endif
         if (loopa_midi_out_allocated) // TODO: check why it can happen that out_allocated == 0
            --loopa_midi_out_allocated;
      } else
      {
         // should never happen! (can be checked by setting a breakpoint or ptintf to this location)
#if DEBUG_VERBOSE_LEVEL >= 1
         DEBUG_MSG("[LOOPA_MIDI_OUT_SlotFree] Malfunction case #2\n");
#endif
         return;
      }
   } else
   {
      // should never happen! (can be checked by setting a breakpoint or printf to this location)
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[LOOPA_MIDI_OUT_SlotFree] Malfunction case #1\n");
#endif
      return;
   }
#endif
}


//! \}
