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

#ifndef _SEQ_FILE_T_H
#define _SEQ_FILE_T_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned NAME:1;
    unsigned CHN:1;
    unsigned MAPS:1;
    unsigned CFG:1;
    unsigned STEPS:1;
  };
} seq_file_t_import_flags_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_T_Init(u32 mode);

extern s32 SEQ_FILE_T_Valid(void);

extern s32 SEQ_FILE_T_Read(char *filepath, u8 track, seq_file_t_import_flags_t flags);
extern s32 SEQ_FILE_T_Write(char *filepath, u8 track);
extern s32 SEQ_FILE_T_Debug(u8 track);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_T_H */
