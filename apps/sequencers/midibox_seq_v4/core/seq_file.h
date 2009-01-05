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

#ifndef _SEQ_FILE_H
#define _SEQ_FILE_H

#include <dosfs.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_Init(u32 mode);

extern s32 SEQ_FILE_ReadOpen(PFILEINFO fileinfo, char *filepath);
extern s32 SEQ_FILE_ReadBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len);
extern s32 SEQ_FILE_ReadByte(PFILEINFO fileinfo, u8 *byte);
extern s32 SEQ_FILE_ReadHWord(PFILEINFO fileinfo, u16 *hword);
extern s32 SEQ_FILE_ReadWord(PFILEINFO fileinfo, u32 *word);

extern s32 SEQ_FILE_WriteOpen(PFILEINFO fileinfo, char *filepath, u8 create);
extern s32 SEQ_FILE_WriteBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len);
extern s32 SEQ_FILE_WriteByte(PFILEINFO fileinfo, u8 byte);
extern s32 SEQ_FILE_WriteHWord(PFILEINFO fileinfo, u16 hword);
extern s32 SEQ_FILE_WriteWord(PFILEINFO fileinfo, u32 word);
extern s32 SEQ_FILE_WriteClose(PFILEINFO fileinfo);

extern s32 SEQ_FILE_Seek(PFILEINFO fileinfo, u32 offset);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_H */
