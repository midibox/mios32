// $Id$
/*
 * MIDI Port functions for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_PORT_H
#define _MBCV_PORT_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


// keep these constants consistent with the functions in mbcv_port.c !!!
#define MBCV_PORT_NUM_IN_PORTS 8
#define MBCV_PORT_NUM_OUT_PORTS 8


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_PORT_Init(u32 mode);

extern s32 MBCV_PORT_InNumGet(void);
extern s32 MBCV_PORT_OutNumGet(void);

extern char *MBCV_PORT_InNameGet(u8 port_ix);
extern char *MBCV_PORT_OutNameGet(u8 port_ix);

extern mios32_midi_port_t MBCV_PORT_InPortGet(u8 port_ix);
extern mios32_midi_port_t MBCV_PORT_OutPortGet(u8 port_ix);

extern u8 MBCV_PORT_InIxGet(mios32_midi_port_t port);
extern u8 MBCV_PORT_OutIxGet(mios32_midi_port_t port);

extern s32 MBCV_PORT_InCheckAvailable(mios32_midi_port_t port);
extern s32 MBCV_PORT_OutCheckAvailable(mios32_midi_port_t port);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBCV_PORT_H */
