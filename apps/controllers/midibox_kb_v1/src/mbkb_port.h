// $Id$
/*
 * MIDI Port functions for MBKB
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBKB_PORT_H
#define _MBKB_PORT_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


// keep these constants consistent with the functions in midio_port.c !!!
#define MBKB_PORT_NUM_IN_PORTS 13
#define MBKB_PORT_NUM_OUT_PORTS 13
#define MBKB_PORT_NUM_CLK_PORTS 12


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 MIDI_CLOCK:1;
    u8 ACTIVE_SENSE:1;
  };
} mbkb_port_mon_filter_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBKB_PORT_Init(u32 mode);

extern s32 MBKB_PORT_InNumGet(void);
extern s32 MBKB_PORT_OutNumGet(void);
extern s32 MBKB_PORT_ClkNumGet(void);

extern char *MBKB_PORT_InNameGet(u8 port_ix);
extern char *MBKB_PORT_OutNameGet(u8 port_ix);
extern char *MBKB_PORT_ClkNameGet(u8 port_ix);

extern mios32_midi_port_t MBKB_PORT_InPortGet(u8 port_ix);
extern mios32_midi_port_t MBKB_PORT_OutPortGet(u8 port_ix);
extern mios32_midi_port_t MBKB_PORT_ClkPortGet(u8 port_ix);

extern u8 MBKB_PORT_InIxGet(mios32_midi_port_t port);
extern u8 MBKB_PORT_OutIxGet(mios32_midi_port_t port);
extern u8 MBKB_PORT_ClkIxGet(mios32_midi_port_t port);

extern s32 MBKB_PORT_InCheckAvailable(mios32_midi_port_t port);
extern s32 MBKB_PORT_OutCheckAvailable(mios32_midi_port_t port);
extern s32 MBKB_PORT_ClkCheckAvailable(mios32_midi_port_t port);

extern mios32_midi_package_t MBKB_PORT_OutPackageGet(mios32_midi_port_t port);
extern mios32_midi_package_t MBKB_PORT_InPackageGet(mios32_midi_port_t port);

extern s32 MBKB_PORT_MonFilterSet(mbkb_port_mon_filter_t filter);
extern mbkb_port_mon_filter_t MBKB_PORT_MonFilterGet(void);

extern s32 MBKB_PORT_Period1mS(void);

extern s32 MBKB_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package);
extern s32 MBKB_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 MBKB_PORT_EventNameGet(mios32_midi_package_t package, char *label, u8 num_chars);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBKB_PORT_H */
