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

#ifndef _MBNG_FILE_L_H
#define _MBNG_FILE_L_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// limited by common 8.3 directory entry format
#define MBNG_FILE_L_FILENAME_LEN 8

// maximum number of labels (conditional entries count as one label)
#define MBNG_FILE_L_NUM_LABELS 512

// item types (stored in info->label_pos)
#define MBNG_FILE_L_ITEM_UNCONDITIONAL   0
#define MBNG_FILE_L_ITEM_CONDITIONAL     1

// condition labels (not stored in info->label_pos)
#define MBNG_FILE_L_ITEM_COND_EQ        10
#define MBNG_FILE_L_ITEM_COND_LT        11
#define MBNG_FILE_L_ITEM_COND_LTEQ      12
#define MBNG_FILE_L_ITEM_COND_GT        13
#define MBNG_FILE_L_ITEM_COND_GTEQ      14
#define MBNG_FILE_L_ITEM_COND_ELSE      15


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_FILE_L_Init(u32 mode);
extern s32 MBNG_FILE_L_Load(char *filename);
extern s32 MBNG_FILE_L_Unload(void);

extern s32 MBNG_FILE_L_Valid(void);

extern s32 MBNG_FILE_L_Read(char *filename);
extern s32 MBNG_FILE_L_Write(char *filename);
extern s32 MBNG_FILE_L_Debug(void);

extern const char *MBNG_FILE_L_GetLabel(char *label, u16 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern char mbng_file_l_patch_name[MBNG_FILE_L_FILENAME_LEN+1];

#endif /* _MBNG_FILE_L_H */
