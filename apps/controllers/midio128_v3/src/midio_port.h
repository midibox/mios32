// $Id$
/*
 * MIDI Port functions for MIDIO128 V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO_PORT_H
#define _MIDIO_PORT_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


// keep these constants consistent with the functions in midio_port.c !!!
#define MIDIO_PORT_NUM_IN_PORTS 8
#define MIDIO_PORT_NUM_OUT_PORTS 8


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIO_PORT_Init(u32 mode);

extern s32 MIDIO_PORT_InNumGet(void);
extern s32 MIDIO_PORT_OutNumGet(void);

extern char *MIDIO_PORT_InNameGet(u8 port_ix);
extern char *MIDIO_PORT_OutNameGet(u8 port_ix);

extern mios32_midi_port_t MIDIO_PORT_InPortGet(u8 port_ix);
extern mios32_midi_port_t MIDIO_PORT_OutPortGet(u8 port_ix);

extern u8 MIDIO_PORT_InIxGet(mios32_midi_port_t port);
extern u8 MIDIO_PORT_OutIxGet(mios32_midi_port_t port);

extern s32 MIDIO_PORT_InCheckAvailable(mios32_midi_port_t port);
extern s32 MIDIO_PORT_OutCheckAvailable(mios32_midi_port_t port);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIDIO_PORT_H */
