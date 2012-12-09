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

// used by mbng_file_c.c
#define MBNG_FILE_C_ERR_READ            -130 // error while reading file (exact error status cannot be determined anymore)
#define MBNG_FILE_C_ERR_WRITE           -131 // error while writing file (exact error status cannot be determined anymore)
#define MBNG_FILE_C_ERR_NO_FILE         -132 // no or invalid bank file
#define MBNG_FILE_C_ERR_P_TOO_LARGE     -133 // during patch write: patch too large for slot in bank

// used by mbng_file_l.c
#define MBNG_FILE_L_ERR_READ            -140 // error while reading file (exact error status cannot be determined anymore)
#define MBNG_FILE_L_ERR_WRITE           -141 // error while writing file (exact error status cannot be determined anymore)
#define MBNG_FILE_L_ERR_NO_FILE         -142 // no or invalid bank file
#define MBNG_FILE_L_ERR_P_TOO_LARGE     -143 // during label write: label too large for slot in bank


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
