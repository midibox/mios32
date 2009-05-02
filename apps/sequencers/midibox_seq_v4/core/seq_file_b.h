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

#ifndef _SEQ_FILE_B_H
#define _SEQ_FILE_B_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_FILE_B_NUM_BANKS 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_B_Init(u32 mode);
extern s32 SEQ_FILE_B_LoadAllBanks(void);
extern s32 SEQ_FILE_B_UnloadAllBanks(void);

extern s32 SEQ_FILE_B_NumPatterns(u8 bank);

extern s32 SEQ_FILE_B_Create(u8 bank);
extern s32 SEQ_FILE_B_Open(u8 bank);

extern s32 SEQ_FILE_B_PatternRead(u8 bank, u8 pattern, u8 target_group);
extern s32 SEQ_FILE_B_PatternWrite(u8 bank, u8 pattern, u8 source_group);

extern s32 SEQ_FILE_B_PatternPeekName(u8 bank, u8 pattern, u8 non_cached, char *pattern_name);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_B_H */
