// $Id$
/*
 * Header file for UART MIDI functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_UART_MIDI_H
#define _MIOS32_UART_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_UART_MIDI_Init(u32 mode);

extern s32 MIOS32_UART_MIDI_CheckAvailable(u8 uart_port);

extern s32 MIOS32_UART_MIDI_RS_OptimisationSet(u8 uart_port, u8 enable);
extern s32 MIOS32_UART_MIDI_RS_OptimisationGet(u8 uart_port);
extern s32 MIOS32_UART_MIDI_RS_Reset(u8 uart_port);

extern s32 MIOS32_UART_MIDI_Periodic_mS(void);

extern s32 MIOS32_UART_MIDI_PackageSend_NonBlocking(u8 uart_port, mios32_midi_package_t package);
extern s32 MIOS32_UART_MIDI_PackageSend(u8 uart_port, mios32_midi_package_t package);
extern s32 MIOS32_UART_MIDI_PackageReceive(u8 uart_port, mios32_midi_package_t *package);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_UART_MIDI_H */
