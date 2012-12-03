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

#ifndef _MBNG_FILE_H
#define _MBNG_FILE_H

// for compatibility with DOSFS
// TODO: change
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// additional error codes
// see also basic error codes which are documented in file.h

// used by mbng_file_p.c
#define MBNG_FILE_P_ERR_INVALID_BANK    -128 // invalid bank number
#define MBNG_FILE_P_ERR_INVALID_GROUP   -129 // invalid group number
#define MBNG_FILE_P_ERR_INVALID_PATCH   -130 // invalid patch number
#define MBNG_FILE_P_ERR_FORMAT          -131 // invalid bank file format
#define MBNG_FILE_P_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define MBNG_FILE_P_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define MBNG_FILE_P_ERR_NO_FILE         -134 // no or invalid bank file
#define MBNG_FILE_P_ERR_P_TOO_LARGE     -135 // during patch write: patch too large for slot in bank


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_Init(u32 mode);

extern s32 MBNG_FILE_LoadAllFiles(u8 including_hw);
extern s32 MBNG_FILE_UnloadAllFiles(void);

extern s32 MBNG_FILE_StatusMsgSet(char *msg);
extern char *MBNG_FILE_StatusMsgGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_FILE_H */
