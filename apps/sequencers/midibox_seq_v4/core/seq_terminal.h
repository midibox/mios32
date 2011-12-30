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
extern s32 SEQ_TERMINAL_Parse(mios32_midi_port_t port, char byte);
extern s32 SEQ_TERMINAL_ParseLine(char *input, void *_output_function);

extern s32 SEQ_TERMINAL_PrintHelp(void *_output_function);
extern s32 SEQ_TERMINAL_PrintSystem(void *_output_function);
extern s32 SEQ_TERMINAL_PrintGlobalConfig(void *_output_function);
extern s32 SEQ_TERMINAL_PrintBookmarks(void *_output_function);
extern s32 SEQ_TERMINAL_PrintSessionConfig(void *_output_function);
extern s32 SEQ_TERMINAL_PrintTracks(void *_output_function);
extern s32 SEQ_TERMINAL_PrintTrack(void *_output_function, u8 track);
extern s32 SEQ_TERMINAL_PrintCurrentMixerMap(void *_output_function);
extern s32 SEQ_TERMINAL_PrintCurrentSong(void *_output_function);
extern s32 SEQ_TERMINAL_PrintGrooveTemplates(void *_output_function);
extern s32 SEQ_TERMINAL_PrintMemoryInfo(void *_output_function);
extern s32 SEQ_TERMINAL_PrintSdCardInfo(void *_output_function);
extern s32 SEQ_TERMINAL_PrintRouterInfo(void *_output_function);
extern s32 SEQ_TERMINAL_TestAoutPin(void *_output_function, u8 pin_number, u8 level);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_TERMINAL_H */
