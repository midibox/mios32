// $Id: seq_file_bm.h 1205 2011-05-13 22:06:38Z tk $
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

#ifndef _SEQ_FILE_BM_H
#define _SEQ_FILE_BM_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_BM_Init(u32 mode);
extern s32 SEQ_FILE_BM_Load(char *session, u8 global);
extern s32 SEQ_FILE_BM_Unload(u8 global);

extern s32 SEQ_FILE_BM_Valid(u8 global);

extern s32 SEQ_FILE_BM_Read(char *session, u8 global);
extern s32 SEQ_FILE_BM_Write(char *session, u8 global);
extern s32 SEQ_FILE_BM_Debug(u8 global);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_BM_H */
