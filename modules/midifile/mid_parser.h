// $Id$
/*
 * Header file for MIDI file parser
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MID_PARSER_H
#define _MID_PARSER_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// can be overruled in mios32_config.h
#ifndef MID_PARSER_MAX_TRACKS
#define MID_PARSER_MAX_TRACKS 32
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MID_PARSER_Init(u32 mode);

extern s32 MID_PARSER_InstallFileCallbacks(void *mid_parser_getc, void *mid_parser_eof, void *mid_parser_seek);
extern s32 MID_PARSER_InstallEventCallbacks(void *mid_parser_playevent, void *mid_parser_playmeta);

extern s32 MID_PARSER_Read(void);
extern s32 MID_PARSER_FetchEvents(u32 tick_offset, u32 num_ticks);
extern s32 MID_PARSER_RestartSong(void);

extern s32 MIDI_PARSER_FormatGet(void);
extern s32 MIDI_PARSER_PPQN_Get(void);
extern s32 MIDI_PARSER_TrackNumGet(void);




/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MID_PARSER_H */
