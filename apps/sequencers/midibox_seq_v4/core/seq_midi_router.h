// $Id$
/*
 * Header file for MIDI router functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_ROUTER_H
#define _SEQ_MIDI_ROUTER_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_MIDI_ROUTER_NUM_NODES 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:32;
  };
  struct {
    u8 src_port; // don't use mios32_midi_port_t, since data width is important for save/restore function
    u8 src_chn; // 0 == All, 1..16: specific source channel

    u8 dst_port;
    u8 dst_chn; // 0 == All, 1..16: specific destination channel
  };
} seq_midi_router_node_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_ROUTER_Init(u32 mode);
extern s32 SEQ_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_midi_router_node_t seq_midi_router_node[SEQ_MIDI_ROUTER_NUM_NODES];
extern u8 seq_midi_router_mclk_out;

#endif /* _SEQ_MIDI_ROUTER_H */
