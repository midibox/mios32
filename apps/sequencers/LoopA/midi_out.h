/*
 * Header file for MIDI output routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LOOPA_MIDI_OUT_H
#define _LOOPA_MIDI_OUT_H

#include "loopa_datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// memory alloccation method:
// 0: internal static allocation with one byte for each flag
// 1: internal static allocation with 8bit flags
// 2: internal static allocation with 16bit flags
// 3: internal static allocation with 32bit flags
// 4: FreeRTOS based pvPortMalloc
// 5: malloc provided by library
#ifndef LOOPA_MIDI_OUT_MALLOC_METHOD
#define LOOPA_MIDI_OUT_MALLOC_METHOD 3
#endif

// max number of scheduled events which will allocate memory
// each event allocates 12 bytes
// MAX_EVENTS must be a power of two! (e.g. 64, 128, 256, 512, ...)
#ifndef LOOPA_MIDI_OUT_MAX_EVENTS
#define LOOPA_MIDI_OUT_MAX_EVENTS 128
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  LOOPA_MIDI_OUT_ClkEvent,   // always sent first before any other timestamped event
  LOOPA_MIDI_OUT_TempoEvent, // dito - changes the BPM rate (located in midi_package.ALL)
  LOOPA_MIDI_OUT_CCEvent,    // sent before notes
  LOOPA_MIDI_OUT_OnEvent,    // on event
  LOOPA_MIDI_OUT_OffEvent,   // off event - sent by LoopA_MIDI_OUT_FlushQueue when queue is emptied (e.g. on Stop/Pause)
  LOOPA_MIDI_OUT_OnOffEvent  // Plays On and Off event after given length
} loopa_midi_out_event_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LoopA_MIDI_OUT_Init(u32 mode);

extern s32 LoopA_MIDI_OUT_Callback_MIDI_SendPackage_Set(void *_callback_midi_send_package);
extern s32 LoopA_MIDI_OUT_Callback_BPM_IsRunning_Set(void *_callback_bpm_is_running);
extern s32 LoopA_MIDI_OUT_Callback_BPM_TickGet_Set(void *_callback_bpm_tick_get);
extern s32 LoopA_MIDI_OUT_Callback_BPM_Set_Set(void *_callback_bpm_set);

extern s32 LoopA_MIDI_OUT_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package,
                               loopa_midi_out_event_type_t event_type, u32 ticksUntilPlayback, u32 len);
extern s32 LoopA_MIDI_OUT_FlushQueue(void);
extern s32 LoopA_MIDI_OUT_FreeHeap(void);

extern void LoopA_MIDI_OUT_Tick(); // Decrease the ticksUntilPlayback of every item, called once per tick
extern s32 LoopA_MIDI_OUT_Handler(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u32 loopa_midi_out_allocated;

#ifdef __cplusplus
}
#endif

#endif /* _LOOPA_MIDI_OUT_H */
