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

// used by seq_file_m.c
#define SEQ_FILE_M_ERR_INVALID_BANK    -144 // invalid bank number
#define SEQ_FILE_M_ERR_INVALID_MAP     -146 // invalid map number
#define SEQ_FILE_M_ERR_FORMAT          -147 // invalid bank file format
#define SEQ_FILE_M_ERR_READ            -148 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_M_ERR_WRITE           -149 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_M_ERR_NO_FILE         -150 // no or invalid bank file
#define SEQ_FILE_M_ERR_M_TOO_LARGE     -151 // during map write: map too large for slot in bank

// used by seq_file_s.c
#define SEQ_FILE_S_ERR_INVALID_BANK    -160 // invalid bank number
#define SEQ_FILE_S_ERR_INVALID_SONG    -161 // invalid song number
#define SEQ_FILE_S_ERR_FORMAT          -162 // invalid bank file format
#define SEQ_FILE_S_ERR_READ            -163 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_S_ERR_WRITE           -164 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_S_ERR_NO_FILE         -165 // no or invalid bank file
#define SEQ_FILE_S_ERR_S_TOO_LARGE     -166 // during song write: song too large for slot in bank

// used by seq_file_c.c
#define SEQ_FILE_C_ERR_FORMAT          -176 // invalid config file format
#define SEQ_FILE_C_ERR_READ            -177 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_C_ERR_WRITE           -178 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_C_ERR_NO_FILE         -179 // no or invalid config file

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
#define SEQ_FILE_GC_ERR_FORMAT         -230 // invalid config file format
#define SEQ_FILE_GC_ERR_READ           -231 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_WRITE          -232 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_NO_FILE        -233 // no or invalid config file

// used by seq_file_bm.c
#define SEQ_FILE_BM_ERR_FORMAT         -246 // invalid config file format
#define SEQ_FILE_BM_ERR_READ           -247 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_BM_ERR_WRITE          -248 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_BM_ERR_NO_FILE        -249 // no or invalid config file

// used by seq_file_presets.c
#define SEQ_FILE_PRESETS_ERR_FORMAT    -256 // invalid config file format
#define SEQ_FILE_PRESETS_ERR_READ      -257 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_PRESETS_ERR_WRITE     -258 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_PRESETS_ERR_NO_FILE   -259 // no or invalid config file


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_Init(u32 mode);

extern s32 SEQ_FILE_LoadAllFiles(u8 including_hw);
extern s32 SEQ_FILE_UnloadAllFiles(void);
extern s32 SEQ_FILE_SaveAllFiles(void);

extern s32 SEQ_FILE_StoreSessionName(void);
extern s32 SEQ_FILE_LoadSessionName(void);

extern s32 SEQ_FILE_FormattingRequired(void);
extern s32 SEQ_FILE_Format(void);

extern s32 SEQ_FILE_CreateBackup(void);

extern s32 SEQ_FILE_IsValidSessionName(char *name);
extern s32 SEQ_FILE_CreateSession(char *name, u8 new_session);
extern s32 SEQ_FILE_DeleteSession(char *name);


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
