// $Id$
/*
 * Header file for MIDI Input routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_IN_H
#define _SEQ_MIDI_IN_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of transport/arpeggiator busses
#define SEQ_MIDI_IN_NUM_BUSSES 4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 MODE_PLAY:1;
  };
} seq_midi_in_options_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_IN_Init(u32 mode);

extern s32 SEQ_MIDI_IN_ResetTransArpStacks(void);
extern s32 SEQ_MIDI_IN_ResetChangerStacks(void);
extern s32 SEQ_MIDI_IN_ResetAllStacks(void);

extern s32 SEQ_MIDI_IN_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 SEQ_MIDI_IN_BusReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package, u8 from_loopback_port);

extern s32 SEQ_MIDI_IN_TransposerNoteGet(u8 bus, u8 hold);
extern s32 SEQ_MIDI_IN_ArpNoteGet(u8 bus, u8 hold, u8 sorted, u8 key_num);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_midi_in_channel[SEQ_MIDI_IN_NUM_BUSSES];
extern mios32_midi_port_t seq_midi_in_port[SEQ_MIDI_IN_NUM_BUSSES];
extern u8 seq_midi_in_lower[SEQ_MIDI_IN_NUM_BUSSES];
extern u8 seq_midi_in_upper[SEQ_MIDI_IN_NUM_BUSSES];
extern seq_midi_in_options_t seq_midi_in_options[SEQ_MIDI_IN_NUM_BUSSES];

extern u8 seq_midi_in_rec_channel;
extern mios32_midi_port_t seq_midi_in_rec_port;

extern u8 seq_midi_in_sect_channel;
extern mios32_midi_port_t seq_midi_in_sect_port;
extern mios32_midi_port_t seq_midi_in_sect_fwd_port;
extern u8 seq_midi_in_sect_note[4];


#endif /* _SEQ_MIDI_IN_H */
