// $Id$
/*
 * MIDI Router functions for MBKB
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBKB_ROUTER_H
#define _MBKB_ROUTER_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBKB_ROUTER_NUM_NODES  16


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 src_port; // don't use mios32_midi_port_t, since data width is important for save/restore function
  u8 src_chn;  // 0 == Off, 1..16: specific source channel, 17 == All
  u8 dst_port;
  u8 dst_chn;  // 0 == Off, 1..16: specific source channel, 17 == All
} mbkb_router_node_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBKB_ROUTER_Init(u32 mode);

extern s32 MBKB_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 MBKB_ROUTER_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in);

extern s32 MBKB_ROUTER_MIDIClockInGet(mios32_midi_port_t port);
extern s32 MBKB_ROUTER_MIDIClockInSet(mios32_midi_port_t port, u8 enable);

extern s32 MBKB_ROUTER_MIDIClockOutGet(mios32_midi_port_t port);
extern s32 MBKB_ROUTER_MIDIClockOutSet(mios32_midi_port_t port, u8 enable);

extern s32 MBKB_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern mbkb_router_node_entry_t mbkb_router_node[MBKB_ROUTER_NUM_NODES];
extern u32 mbkb_router_mclk_in;
extern u32 mbkb_router_mclk_out;

#endif /* _MBKB_ROUTER_H */
