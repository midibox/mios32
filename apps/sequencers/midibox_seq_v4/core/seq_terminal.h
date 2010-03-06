// $Id$
/*
 * Header file of MBSEQ Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_TERMINAL_H
#define _SEQ_TERMINAL_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_TERMINAL_Init(u32 mode);
extern s32 SEQ_TERMINAL_Parse(mios32_midi_port_t port, u8 byte);

extern s32 SEQ_TERMINAL_PrintSystem(void);
extern s32 SEQ_TERMINAL_PrintGlobals(void);
extern s32 SEQ_TERMINAL_PrintTracks(void);
extern s32 SEQ_TERMINAL_PrintTrack(u8 track);
extern s32 SEQ_TERMINAL_PrintCurrentMixerMap(void);
extern s32 SEQ_TERMINAL_PrintCurrentSong(void);
extern s32 SEQ_TERMINAL_PrintGrooveTemplates(void);
extern s32 SEQ_TERMINAL_PrintSdCardInfo(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_TERMINAL_H */
