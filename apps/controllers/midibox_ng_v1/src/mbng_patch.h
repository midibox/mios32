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
#define MBNG_PATCH_NUM_DIN         128
#define MBNG_PATCH_NUM_ENC         128
#define MBNG_PATCH_NUM_DOUT        128
#define MBNG_PATCH_NUM_AIN           6
#define MBNG_PATCH_NUM_AINSER      128
#define MBNG_PATCH_NUM_MATRIX_DIN    8
#define MBNG_PATCH_NUM_MATRIX_DOUT   8

#define MBNG_PATCH_NUM_MATRIX_ROWS_MAX   16
#define MBNG_PATCH_NUM_MATRIX_COLORS_MAX  3

/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


typedef struct {
  u8 num_rows;
  u8 inverted;
  u8 sr_dout_sel1;
  u8 sr_dout_sel2;
  u8 sr_din1;
  u8 sr_din2;
} mbng_patch_matrix_din_entry_t;

typedef struct {
  u8 num_rows;
  u8 inverted;
  u8 sr_dout_sel1;
  u8 sr_dout_sel2;
  u8 sr_dout_r1;
  u8 sr_dout_r2;
  u8 sr_dout_g1;
  u8 sr_dout_g2;
  u8 sr_dout_b1;
  u8 sr_dout_b2;
} mbng_patch_matrix_dout_entry_t;

typedef struct {
  u8 debounce_ctr;
  u8 global_chn;
  u8 all_notes_off_chn;
  u8 button_group_size;
  u8 led_group_size;
  u8 enc_group_size;
  u8 matrix_din_group_size;
  u8 matrix_dout_group_size;
  u8 ain_group_size;
  u8 ainser_group_size;
  u8 mf_group_size;
  u8 sysex_dev;
  u8 sysex_pat;
  u8 sysex_bnk;
  u8 sysex_ins;
  u8 sysex_chn;
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

extern mbng_patch_matrix_din_entry_t mbng_patch_matrix_din[MBNG_PATCH_NUM_MATRIX_DIN];
extern mbng_patch_matrix_dout_entry_t mbng_patch_matrix_dout[MBNG_PATCH_NUM_MATRIX_DOUT];

extern mbng_patch_cfg_t mbng_patch_cfg;

#endif /* _MBNG_PATCH_H */
