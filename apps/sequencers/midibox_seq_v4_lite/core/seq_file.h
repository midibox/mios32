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

// for compatibility with DOSFS
// TODO: change
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are session directories located?
#define SEQ_FILE_SESSION_PATH "/SESSIONS"


// additional error codes
// see also basic error codes which are documented in file.h

// used by seq_file_b.c
#define SEQ_FILE_B_ERR_INVALID_BANK    -128 // invalid bank number
#define SEQ_FILE_B_ERR_INVALID_GROUP   -129 // invalid group number
#define SEQ_FILE_B_ERR_INVALID_PATTERN -130 // invalid pattern number
#define SEQ_FILE_B_ERR_FORMAT          -131 // invalid bank file format
#define SEQ_FILE_B_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_B_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_B_ERR_NO_FILE         -134 // no or invalid bank file
#define SEQ_FILE_B_ERR_P_TOO_LARGE     -135 // during pattern write: pattern too large for slot in bank

// used by seq_file_g.c
#define SEQ_FILE_G_ERR_FORMAT          -192 // invalid config file format
#define SEQ_FILE_G_ERR_READ            -193 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_G_ERR_WRITE           -194 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_G_ERR_NO_FILE         -195 // no or invalid config file

// used by seq_file_hw.c
#define SEQ_FILE_HW_ERR_FORMAT         -208 // invalid config file format
#define SEQ_FILE_HW_ERR_READ           -209 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_HW_ERR_WRITE          -210 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_HW_ERR_NO_FILE        -211 // no or invalid config file

// used by seq_file_t.c
#define SEQ_FILE_T_ERR_FORMAT          -224 // invalid file format
#define SEQ_FILE_T_ERR_READ            -225 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_T_ERR_WRITE           -226 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_T_ERR_NO_FILE         -227 // no or invalid file
#define SEQ_FILE_T_ERR_TRACK           -228 // invalid track number

// used by seq_file_gc.c
#define SEQ_FILE_GC_ERR_FORMAT         -140 // invalid config file format
#define SEQ_FILE_GC_ERR_READ           -141 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_WRITE          -142 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_NO_FILE        -143 // no or invalid config file


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_Init(u32 mode);

extern s32 SEQ_FILE_LoadAllFiles(u8 including_hw);
extern s32 SEQ_FILE_UnloadAllFiles(void);

extern s32 SEQ_FILE_StoreSessionName(void);
extern s32 SEQ_FILE_LoadSessionName(void);

extern s32 SEQ_FILE_FormattingRequired(void);
extern s32 SEQ_FILE_Format(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// name/directory of session
extern char seq_file_session_name[13];
extern char seq_file_new_session_name[13];

extern char *seq_file_backup_notification;

extern u8 seq_file_copy_percentage;
extern u8 seq_file_backup_percentage;

#endif /* _SEQ_FILE_H */
