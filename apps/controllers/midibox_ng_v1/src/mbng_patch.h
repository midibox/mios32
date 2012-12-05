// $Id$
/*
 * Patch Layer for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_PATCH_H
#define _MBNG_PATCH_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// temporary - will be changed with second (or enhanced) SRIO chain
#define MBNG_PATCH_NUM_DIN    128
#define MBNG_PATCH_NUM_ENC    128
#define MBNG_PATCH_NUM_DOUT   128
#define MBNG_PATCH_NUM_AIN      6
#define MBNG_PATCH_NUM_AINSER 128
#define MBNG_PATCH_NUM_MATRIX  16


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


typedef struct {
  u8 sr_din;
  u8 sr_dout;
} mbng_patch_matrix_entry_t;

typedef struct {
  u8 debounce_ctr;
  u8 global_chn;
  u8 all_notes_off_chn;
  u8 button_group_size;
  u8 enc_group_size;
  u8 ain_group_size;
  u8 ainser_group_size;
} mbng_patch_cfg_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_PATCH_Init(u32 mode);

extern s32 MBNG_PATCH_Load(char *filename);
extern s32 MBNG_PATCH_Store(char *filename);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern mbng_patch_matrix_entry_t mbng_patch_matrix[MBNG_PATCH_NUM_MATRIX];

extern mbng_patch_cfg_t        mbng_patch_cfg;

#endif /* _MBNG_PATCH_H */
