// $Id$
/*
 * Patch Layer for MIDIO128 V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO_PATCH_H
#define _MIDIO_PATCH_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MIDIO_PATCH_DOUT_SIZE 0x100
#define MIDIO_PATCH_DIN_SIZE  0x300
#define MIDIO_PATCH_CFG_SIZE  0x200

#define MIDIO_PATCH_SIZE (MIDIO_PATCH_DOUT_SIZE + MIDIO_PATCH_DIN_SIZE + MIDIO_PATCH_CFG_SIZE)


#define MIDIO_PATCH_NUM_DIN  128
#define MIDIO_PATCH_NUM_DOUT 128

#define MIDIO_PATCH_DIN_MODE_ON_OFF        0
#define MIDIO_PATCH_DIN_MODE_ON_ONLY       1
#define MIDIO_PATCH_DIN_MODE_TOGGLE        2

#define MIDIO_PATCH_MERGER_MODE_DISABLED   0
#define MIDIO_PATCH_MERGER_MODE_ENABLED    1
#define MIDIO_PATCH_MERGER_MODE_MBLINK_EP  2
#define MIDIO_PATCH_MERGER_MODE_MBLINK_FP  3



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
} midio_patch_din_entry_t;

typedef struct {
  u16 enabled_ports;
  u8 evnt0;
  u8 evnt1;
} midio_patch_dout_entry_t;

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
} midio_patch_flags_t;

typedef struct {
  midio_patch_flags_t flags;
  u8 debounce_ctr;
  u8 ts_sensitivity;
  u8 global_chn;
  u8 all_notes_off_chn;
} midio_patch_cfg_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIO_PATCH_Init(u32 mode);

extern u8  MIDIO_PATCH_ReadByte(u16 addr);
extern s32 MIDIO_PATCH_WriteByte(u16 addr, u8 byte);

extern s32 MIDIO_PATCH_Load(u8 bank, u8 patch);
extern s32 MIDIO_PATCH_Store(u8 bank, u8 patch);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern midio_patch_din_entry_t  midio_patch_din[MIDIO_PATCH_NUM_DIN];
extern midio_patch_dout_entry_t midio_patch_dout[MIDIO_PATCH_NUM_DOUT];
extern midio_patch_cfg_t        midio_patch_cfg;

#endif /* _MIDIO_PATCH_H */
