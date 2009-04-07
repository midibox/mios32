// $Id: mios32_uart_midi.h 144 2008-12-02 00:07:05Z tk $
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

extern s32 MIOS32_UART_MIDI_PackageSend_NonBlocking(u8 uart_port, mios32_midi_package_t package);
extern s32 MIOS32_UART_MIDI_PackageSend(u8 uart_port, mios32_midi_package_t package);
extern s32 MIOS32_UART_MIDI_PackageReceive(u8 uart_port, mios32_midi_package_t *package);



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_UART_MIDI_H */
