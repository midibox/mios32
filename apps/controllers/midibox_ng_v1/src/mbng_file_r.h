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

#ifndef _MBNG_FILE_R_H
#define _MBNG_FILE_R_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// limited by common 8.3 directory entry format
#define MBNG_FILE_R_FILENAME_LEN 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u16 ALL;

  struct {
    u16 load:1;
    u16 section:8;
  };
} mbng_file_r_req_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_R_Init(u32 mode);
extern s32 MBNG_FILE_R_Load(char *filename);
extern s32 MBNG_FILE_R_Unload(void);

extern s32 MBNG_FILE_R_Valid(void);

extern s32 MBNG_FILE_R_Read(char *filename, u8 section);
extern s32 MBNG_FILE_R_ReadRequest(char *filename, u8 section);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern char mbng_file_r_script_name[MBNG_FILE_R_FILENAME_LEN+1];
extern mbng_file_r_req_t mbng_file_r_req;

#endif /* _MBNG_FILE_R_H */
