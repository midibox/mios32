// $Id$
/*
 * Header for MIDI file access routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MID_FILE_H
#define _MID_FILE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MID_FILE_Init(u32 mode);

extern char *MID_FILE_UI_NameGet(void);
extern s32 MID_FILE_FindNext(char *filename, char *next_file);
extern s32 MID_FILE_FindPrev(char *filename, char *prev_file);

extern s32 MID_FILE_open(char *filename);
extern u32 MID_FILE_read(void *buffer, u32 len);
extern s32 MID_FILE_eof(void);
extern s32 MID_FILE_seek(u32 pos);

extern s32 MID_FILE_Receive(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 MID_FILE_SetRecordMode(u8 enable);

extern s32 MID_FILE_RecordingEnabled(void);
extern char *MID_FILE_RecordingFilename(void);
extern u32 MID_FILE_LastRecordedTick(void);
extern mios32_midi_port_t MID_FILE_LastRecordedPort(void);
extern mios32_midi_package_t MID_FILE_LastRecordedEvent(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MID_FILE_H */
