// $Id$
/*
 * Header for file functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_FILE_P_H
#define _MBNG_FILE_P_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// limited by common 8.3 directory entry format
#define MBNG_FILE_P_FILENAME_LEN 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_P_Init(u32 mode);
extern s32 MBNG_FILE_P_Load(char *filename);
extern s32 MBNG_FILE_P_Unload(void);

extern s32 MBNG_FILE_P_Valid(void);

extern s32 MBNG_FILE_P_Read(char *filename);
extern s32 MBNG_FILE_P_Write(char *filename);
extern s32 MBNG_FILE_P_Debug(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern char mbng_file_p_patch_name[MBNG_FILE_P_FILENAME_LEN+1];

#endif /* _MBNG_FILE_P_H */
