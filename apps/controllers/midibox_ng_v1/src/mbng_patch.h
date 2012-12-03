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

#define MBNG_PATCH_DOUT_SIZE 0x100
#define MBNG_PATCH_DIN_SIZE  0x300
#define MBNG_PATCH_CFG_SIZE  0x200

#define MBNG_PATCH_SIZE (MBNG_PATCH_DOUT_SIZE + MBNG_PATCH_DIN_SIZE + MBNG_PATCH_CFG_SIZE)


// temporary - will be changed with second (or enhanced) SRIO chain
#define MBNG_PATCH_NUM_DIN    128
#define MBNG_PATCH_NUM_ENC    128
#define MBNG_PATCH_NUM_DOUT   128
#define MBNG_PATCH_NUM_AIN      6
#define MBNG_PATCH_NUM_AINSER 128
#define MBNG_PATCH_NUM_MATRIX  16

#define MBNG_PATCH_DIN_MODE_ON_OFF        0
#define MBNG_PATCH_DIN_MODE_ON_ONLY       1
#define MBNG_PATCH_DIN_MODE_TOGGLE        2

#define MBNG_PATCH_MATRIX_MODE_COMMON     0
#define MBNG_PATCH_MATRIX_MODE_MAPPED     1

#define MBNG_PATCH_MERGER_MODE_DISABLED   0
#define MBNG_PATCH_MERGER_MODE_ENABLED    1
#define MBNG_PATCH_MERGER_MODE_MBLINK_EP  2
#define MBNG_PATCH_MERGER_MODE_MBLINK_FP  3



/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 enabled_ports;
  u8 mode;
  u8 evnt0_on;
  u8 evnt1_on;
  u8 evnt2_on;
  u8 evnt0_off;
  u8 evnt1_off;
  u8 evnt2_off;
} mbng_patch_din_entry_t;

typedef struct {
  u16 enabled_ports;
  u16 min;
  u16 max;
  u8 mode;
  u8 evnt0;
  u8 evnt1;
} mbng_patch_enc_entry_t;

typedef struct {
  u16 enabled_ports;
  u8 evnt0;
  u8 evnt1;
} mbng_patch_dout_entry_t;

typedef struct {
  u16 enabled_ports;
  u8 evnt0;
  u8 evnt1;
} mbng_patch_ain_entry_t;

typedef struct {
  u16 enabled_ports;
  u8 mode;
  u8 chn;
  u8 arg;
  u8 sr_din;
  u8 sr_dout;
  u8 map_chn[64];
  u8 map_evnt1[64];
} mbng_patch_matrix_entry_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    u8 MERGER_MODE:2;
    u8 ALT_PROGCHNG:1;
    u8 FORWARD_IO:1;
    u8 INVERSE_DIN:1;
    u8 INVERSE_DOUT:1;
  };
} mbng_patch_flags_t;

typedef struct {
  mbng_patch_flags_t flags;
  u8 debounce_ctr;
  u8 global_chn;
  u8 all_notes_off_chn;
  u8 enc_group_size;
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

extern mbng_patch_din_entry_t  mbng_patch_din[MBNG_PATCH_NUM_DIN];
extern mbng_patch_dout_entry_t mbng_patch_dout[MBNG_PATCH_NUM_DOUT];
extern mbng_patch_enc_entry_t  mbng_patch_enc[MBNG_PATCH_NUM_ENC];
extern mbng_patch_ain_entry_t  mbng_patch_ain[MBNG_PATCH_NUM_AIN];
extern mbng_patch_ain_entry_t  mbng_patch_ainser[MBNG_PATCH_NUM_AINSER];
extern mbng_patch_matrix_entry_t mbng_patch_matrix[MBNG_PATCH_NUM_MATRIX];

extern mbng_patch_cfg_t        mbng_patch_cfg;

#endif /* _MBNG_PATCH_H */
