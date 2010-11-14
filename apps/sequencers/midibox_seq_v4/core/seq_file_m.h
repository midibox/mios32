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

#ifndef _SEQ_FILE_M_H
#define _SEQ_FILE_M_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_M_Init(u32 mode);
extern s32 SEQ_FILE_M_LoadAllBanks(char *session);
extern s32 SEQ_FILE_M_UnloadAllBanks(void);

extern s32 SEQ_FILE_M_NumMaps(void);

extern s32 SEQ_FILE_M_Create(char *session);
extern s32 SEQ_FILE_M_Open(char *session);

extern s32 SEQ_FILE_M_MapRead(u8 map);
extern s32 SEQ_FILE_M_MapWrite(char *session, u8 map, u8 rename_if_empty_name);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_M_H */
