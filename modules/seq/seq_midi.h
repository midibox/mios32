// $Id$
/*
 * Header file for MIDI routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_H
#define _SEQ_MIDI_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_MIDI_ClkEvent,   // always sent first before any other timestamped event
  SEQ_MIDI_TempoEvent, // dito - changes the BPM rate (located in midi_package.ALL)
  SEQ_MIDI_CCEvent,    // sent before notes
  SEQ_MIDI_OnEvent,    // note on event
  SEQ_MIDI_OffEvent    // note off event - sent by SEQ_MIDI_FlushQueue when queue is emptied (e.g. on Stop/Pause)
} seq_midi_event_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_Init(u32 mode);

extern s32 SEQ_MIDI_Send(mios32_midi_port_t port, mios32_midi_package_t midi_package, seq_midi_event_type_t event_type, u32 timestamp);
extern s32 SEQ_MIDI_FlushQueue(void);
extern s32 SEQ_MIDI_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u32 seq_midi_queue_size;

#endif /* _SEQ_MIDI_H */
