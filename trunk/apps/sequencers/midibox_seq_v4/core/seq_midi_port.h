// $Id$
/*
 * Header file for MIDI port functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_PORT_H
#define _SEQ_MIDI_PORT_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 MIDI_CLOCK:1;
    u8 ACTIVE_SENSE:1;
  };
} seq_midi_port_mon_filter_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_PORT_Init(u32 mode);

extern s32 SEQ_MIDI_PORT_InNumGet(void);
extern s32 SEQ_MIDI_PORT_OutNumGet(void);
extern s32 SEQ_MIDI_PORT_ClkNumGet(void);

extern char *SEQ_MIDI_PORT_InNameGet(u8 port_ix);
extern char *SEQ_MIDI_PORT_OutNameGet(u8 port_ix);
extern char *SEQ_MIDI_PORT_ClkNameGet(u8 port_ix);

extern mios32_midi_port_t SEQ_MIDI_PORT_InPortGet(u8 port_ix);
extern mios32_midi_port_t SEQ_MIDI_PORT_OutPortGet(u8 port_ix);
extern mios32_midi_port_t SEQ_MIDI_PORT_ClkPortGet(u8 port_ix);

extern u8 SEQ_MIDI_PORT_InIxGet(mios32_midi_port_t port);
extern u8 SEQ_MIDI_PORT_OutIxGet(mios32_midi_port_t port);
extern u8 SEQ_MIDI_PORT_ClkIxGet(mios32_midi_port_t port);

extern s32 SEQ_MIDI_PORT_InCheckAvailable(mios32_midi_port_t port);
extern s32 SEQ_MIDI_PORT_OutCheckAvailable(mios32_midi_port_t port);
extern s32 SEQ_MIDI_PORT_ClkCheckAvailable(mios32_midi_port_t port);

extern s32 SEQ_MIDI_PORT_InPortFromNameGet(const char* name);
extern s32 SEQ_MIDI_PORT_OutPortFromNameGet(const char* name);
extern s32 SEQ_MIDI_PORT_ClkPortFromNameGet(const char* name);

extern char *SEQ_MIDI_PORT_InPortToName(mios32_midi_port_t port, char str_buffer[5]);
extern char *SEQ_MIDI_PORT_OutPortToName(mios32_midi_port_t port, char str_buffer[5]);
extern char *SEQ_MIDI_PORT_ClkPortToName(mios32_midi_port_t port, char str_buffer[5]);

extern s32 SEQ_MIDI_PORT_OutMuteGet(mios32_midi_port_t port);
extern s32 SEQ_MIDI_PORT_OutMuteSet(mios32_midi_port_t port, u8 mute);

extern s8  SEQ_MIDI_PORT_ClkDelayGet(mios32_midi_port_t port);
extern s32 SEQ_MIDI_PORT_ClkDelaySet(mios32_midi_port_t port, s8 delay);
extern s8  SEQ_MIDI_PORT_ClkIxDelayGet(u8 port_ix);
extern s32 SEQ_MIDI_PORT_ClkIxDelaySet(u8 port_ix, s8 delay);

extern s32 SEQ_MIDI_PORT_ClkDelayUpdate(mios32_midi_port_t port);
extern s32 SEQ_MIDI_PORT_ClkDelayUpdateAll(void);

extern s8  SEQ_MIDI_PORT_TickDelayMaxNegativeOffset(void);

extern mios32_midi_package_t SEQ_MIDI_PORT_OutPackageGet(mios32_midi_port_t port);
extern mios32_midi_package_t SEQ_MIDI_PORT_InPackageGet(mios32_midi_port_t port);

extern s32 SEQ_MIDI_PORT_MonFilterSet(seq_midi_port_mon_filter_t filter);
extern seq_midi_port_mon_filter_t SEQ_MIDI_PORT_MonFilterGet(void);

extern s32 SEQ_MIDI_PORT_Period1mS(void);

extern s32 SEQ_MIDI_PORT_FilterOscPacketsSet(u8 filter);
extern s32 SEQ_MIDI_PORT_FilterOscPacketsGet(void);

extern s32 SEQ_MIDI_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package);
extern s32 SEQ_MIDI_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u32 seq_midi_port_multi_enable_flags;

extern u8 seq_midi_port_out_combined_ctr;
extern u8 seq_midi_port_in_combined_ctr;

#endif /* _SEQ_MIDI_PORT_H */
