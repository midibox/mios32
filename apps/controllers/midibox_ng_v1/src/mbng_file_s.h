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

#ifndef _MBNG_FILE_S_H
#define _MBNG_FILE_S_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// limited by common 8.3 directory entry format
#define MBNG_FILE_S_FILENAME_LEN 8

// number of supported snapshots
#define MBNG_FILE_S_NUM_SNAPSHOTS 128

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_S_Init(u32 mode);
extern s32 MBNG_FILE_S_Load(char *filename, int snapshot);
extern s32 MBNG_FILE_S_Unload(void);

extern s32 MBNG_FILE_S_Valid(void);

extern s32 MBNG_FILE_S_SnapshotGet(void);
extern s32 MBNG_FILE_S_SnapshotSet(u8 snapshot);

extern s32 MBNG_FILE_S_Read(char *filename, int snapshot);
extern s32 MBNG_FILE_S_Write(char *filename, int snapshot);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern char mbng_file_s_patch_name[MBNG_FILE_S_FILENAME_LEN+1];

#endif /* _MBNG_FILE_S_H */
