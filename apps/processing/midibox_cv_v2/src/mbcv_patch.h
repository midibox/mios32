// $Id$
/*
 * Patch Layer for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_PATCH_H
#define _MBCV_PATCH_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBCV_PATCH_SIZE 256

#define MBCV_PATCH_NUM_CV      8  // should be at least 8, and dividable by 8!
#define MBCV_PATCH_NUM_ROUTER  16

#define MBCV_PATCH_CV_CURVE_V_OCTAVE    0
#define MBCV_PATCH_CV_CURVE_HZ_V        1
#define MBCV_PATCH_CV_CURVE_INVERTED    2
#define MBCV_PATCH_CV_CURVE_EXPONENTIAL 3

#define MBCV_PATCH_MERGER_MODE_DISABLED   0
#define MBCV_PATCH_MERGER_MODE_ENABLED    1
#define MBCV_PATCH_MERGER_MODE_MBLINK_EP  2
#define MBCV_PATCH_MERGER_MODE_MBLINK_FP  3



/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 src_port; // don't use mios32_midi_port_t, since data width is important for save/restore function
  u8 src_chn;  // 0 == Off, 1..16: specific source channel, 17 == All
  u8 dst_port;
  u8 dst_chn;  // 0 == Off, 1..16: specific source channel, 17 == All
} mbcv_patch_router_entry_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    u8 MERGER_MODE:2;
  };
} mbcv_patch_flags_t;

typedef struct {
  mbcv_patch_flags_t flags;
  u8 ext_clk_divider;
  u8 ext_clk_pulsewidth;
} mbcv_patch_cfg_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

extern s32 MBCV_PATCH_Init(u32 mode);

extern u8  MBCV_PATCH_ReadByte(u16 addr);
extern s32 MBCV_PATCH_WriteByte(u16 addr, u8 byte);

extern s32 MBCV_PATCH_Load(char *filename);
extern s32 MBCV_PATCH_Store(char *filename);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern mbcv_patch_router_entry_t mbcv_patch_router[MBCV_PATCH_NUM_ROUTER];
extern u32 mbcv_patch_router_mclk_in;
extern u32 mbcv_patch_router_mclk_out;

extern mbcv_patch_cfg_t mbcv_patch_cfg;

extern u8 mbcv_patch_gateclr_cycles;

#ifdef __cplusplus
}
#endif


#endif /* _MBCV_PATCH_H */
