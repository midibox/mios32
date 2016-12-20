// $Id$
/*
 * Header file for HW configuration routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_HWCFG_H
#define _MBCV_HWCFG_H

#include <MbCvStructs.h>
#include "mbcv_lre.h"
#include "mbcv_button.h"
#include "mbcv_rgb.h"
#include <scs.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 scs_exit;
  u8 scs_shift;
  u8 scs_soft[SCS_NUM_MENU_ITEMS];

  u8 cv[CV_SE_NUM];
  u8 enc_bank[MBCV_LRE_NUM_BANKS];
  u8 cv_and_enc_bank[CV_SE_NUM];

  u8 lfo1;
  u8 lfo2;
  u8 env1;
  u8 env2;

  u8 scope[CV_SCOPE_NUM];

  u8 keyboard[MBCV_BUTTON_KEYBOARD_KEYS_NUM];
} mbcv_hwcfg_button_t;

typedef struct {
  u8 gate_sr;
  u8 clk_sr;
} mbcv_hwcfg_dout_t;

typedef struct {
  u8 enabled;
  u8 ledring_select_sr1;
  u8 ledring_select_sr2;
  u8 ledring_pattern_sr1;
  u8 ledring_pattern_sr2;
} mbcv_hwcfg_lre_t;

typedef struct {
  u8 dout_select_sr;
  u8 din_scan_sr[MBCV_BUTTON_MATRIX_ROWS/8];
} mbcv_hwcfg_bm_t;

typedef struct {
  u8              pos;
  mbcv_rgb_mode_t mode;
} mbcv_hwcfg_ws2812_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_HWCFG_Init(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mbcv_hwcfg_button_t mbcv_hwcfg_button;
extern mbcv_hwcfg_dout_t mbcv_hwcfg_dout;
extern mbcv_hwcfg_lre_t mbcv_hwcfg_lre[MBCV_LRE_NUM];
extern mbcv_hwcfg_bm_t mbcv_hwcfg_bm;
extern mbcv_hwcfg_ws2812_t mbcv_hwcfg_ws2812[MBCV_RGB_LED_NUM];

#ifdef __cplusplus
}
#endif

#endif /* _MBCV_HWCFG_H */
