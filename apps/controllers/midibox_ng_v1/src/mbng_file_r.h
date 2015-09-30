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


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_R_Init(u32 mode);
extern s32 MBNG_FILE_R_Load(char *filename);
extern s32 MBNG_FILE_R_Unload(void);

extern s32 MBNG_FILE_R_Valid(void);

extern s32 MBNG_FILE_R_TokenizedNgrSet(u8 enable);
extern s32 MBNG_FILE_R_TokenizedNgrGet(void);
extern s32 MBNG_FILE_R_TokenMemPrint(void);

extern s32 MBNG_FILE_R_VarSectionSet(u8 section);
extern s32 MBNG_FILE_R_VarSectionGet();
extern s32 MBNG_FILE_R_VarValueSet(u8 value);
extern s32 MBNG_FILE_R_VarValueGet();

extern s32 MBNG_FILE_R_Parser(u32 line, char *line_buffer, u8 *if_state, u8 *nesting_level, char *load_filename, u8 tokenize_req);

extern s32 MBNG_FILE_R_Read(char *filename, u8 cont_script, u8 tokenize_req);
extern s32 MBNG_FILE_R_ReadRequest(char *filename, u8 section, s16 value, u8 notify_done);
extern s32 MBNG_FILE_R_CheckRequest(void);
extern s32 MBNG_FILE_R_RunStop(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern char mbng_file_r_script_name[MBNG_FILE_R_FILENAME_LEN+1];

#endif /* _MBNG_FILE_R_H */
