// $Id: seq_file_b.h 1454 2012-04-03 22:54:57Z midilab $
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

#define SEQ_FILE_B_NUM_BANKS 4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_B_Init(u32 mode);
extern s32 SEQ_FILE_B_LoadAllBanks(char *session);
extern s32 SEQ_FILE_B_UnloadAllBanks(void);
extern s32 SEQ_FILE_B_SaveAllBanks(char *session);

extern s32 SEQ_FILE_B_NumPatterns(u8 bank);

extern s32 SEQ_FILE_B_Create(char *session, u8 bank);
extern s32 SEQ_FILE_B_Open(char *session, u8 bank);

extern s32 SEQ_FILE_B_PatternRead(u8 bank, u8 pattern, u8 target_group,  u16 remix_map);
extern s32 SEQ_FILE_B_PatternWrite(char *session, u8 bank, u8 pattern, u8 source_group, u8 rename_if_empty_name);

extern s32 SEQ_FILE_B_PatternPeekName(u8 bank, u8 pattern, u8 non_cached, char *pattern_name);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_B_H */
