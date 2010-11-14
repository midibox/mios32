// $Id$
/*
 * Header for file functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_FILE_S_H
#define _SEQ_FILE_S_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_S_Init(u32 mode);
extern s32 SEQ_FILE_S_LoadAllBanks(char *session);
extern s32 SEQ_FILE_S_UnloadAllBanks(void);

extern s32 SEQ_FILE_S_NumSongs(void);

extern s32 SEQ_FILE_S_Create(char *session);
extern s32 SEQ_FILE_S_Open(char *session);

extern s32 SEQ_FILE_S_SongRead(u8 song);
extern s32 SEQ_FILE_S_SongWrite(char *session, u8 song, u8 rename_if_empty_name);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_S_H */
