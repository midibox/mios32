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

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// temporary - will be changed with second (or enhanced) SRIO chain
#define MBNG_PATCH_NUM_DIN         128
#define MBNG_PATCH_NUM_DOUT        128
#define MBNG_PATCH_NUM_ENC         128
#define MBNG_PATCH_NUM_AIN           6
#define MBNG_PATCH_NUM_AINSER_MODULES 2
#define MBNG_PATCH_NUM_MF_MODULES    4
#define MBNG_PATCH_NUM_MATRIX_DIN    8
#define MBNG_PATCH_NUM_MATRIX_DOUT   8
#define MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS 4

#define MBNG_PATCH_NUM_BANKS        16

#define MBNG_PATCH_NUM_MATRIX_ROWS_MAX   16
#define MBNG_PATCH_NUM_MATRIX_COLORS_MAX  3

/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


typedef struct {
  u16 button_emu_id_offset;
  u8 num_rows;
  u8 inverted;
  u8 sr_dout_sel1;
  u8 sr_dout_sel2;
  u8 sr_din1;
  u8 sr_din2;
} mbng_patch_matrix_din_entry_t;

typedef struct {
  u16 led_emu_id_offset;
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

typedef union {
  u8 ALL;
  
  struct {
    u8 chn:4;
    u8 enabled:1;
  };
} mbng_patch_mf_flags_t;

typedef struct {
  u16 ts_first_button_id;
  u8 midi_in_port;
  u8 midi_out_port;
  u8 config_port;
  mbng_patch_mf_flags_t flags;
} mbng_patch_mf_entry_t;

typedef struct {
  u8 enable_mask;
} mbng_patch_ain_entry_t;

typedef union {
  u8 ALL;
  
  struct {
    u8 enabled:1;
    u8 cs:1;
  };
} mbng_patch_ainser_flags_t;

typedef struct {
  mbng_patch_ainser_flags_t flags;
} mbng_patch_ainser_entry_t;

typedef struct {
  u16 first_n;
  u16 num;
  u16 first_id;
} mbng_patch_bank_ctrl_t;

typedef struct {
  mbng_patch_bank_ctrl_t button;
  mbng_patch_bank_ctrl_t led;
  mbng_patch_bank_ctrl_t enc;
  mbng_patch_bank_ctrl_t ain;
  mbng_patch_bank_ctrl_t ainser;
  mbng_patch_bank_ctrl_t mf;
  u8 valid;
} mbng_patch_bank_entry_t;

typedef struct {
  u8 debounce_ctr;
  u8 global_chn;
  u8 all_notes_off_chn;
  u8 convert_note_off_to_on0;
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

extern s32 MBNG_PATCH_BankEntryInit(mbng_patch_bank_entry_t *b, u8 valid);
extern s32 MBNG_PATCH_BankSet(u8 new_bank);
extern s32 MBNG_PATCH_BankGet(void);
extern s32 MBNG_PATCH_NumBanksGet(void);
extern s32 MBNG_PATCH_BankCtrlIdGet(u32 ix, mbng_event_item_id_t* id);
extern s32 MBNG_PATCH_BankCtrlInBank(mbng_event_item_t *item);
extern s32 MBNG_PATCH_BankCtrlIsActive(mbng_event_item_t *item);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern mbng_patch_matrix_din_entry_t mbng_patch_matrix_din[MBNG_PATCH_NUM_MATRIX_DIN];
extern mbng_patch_matrix_dout_entry_t mbng_patch_matrix_dout[MBNG_PATCH_NUM_MATRIX_DOUT];

extern mbng_patch_ain_entry_t mbng_patch_ain;

extern mbng_patch_ainser_entry_t mbng_patch_ainser[MBNG_PATCH_NUM_AINSER_MODULES];

extern mbng_patch_mf_entry_t mbng_patch_mf[MBNG_PATCH_NUM_MF_MODULES];

extern mbng_patch_bank_entry_t mbng_patch_bank[MBNG_PATCH_NUM_BANKS];

extern mbng_patch_cfg_t mbng_patch_cfg;

#endif /* _MBNG_PATCH_H */
