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
  u8 cv[CV_SE_NUM];
  u8 enc_bank[MBCV_LRE_NUM_BANKS];
  u8 cv_and_enc_bank[CV_SE_NUM];

  u8 lfo1;
  u8 lfo2;
  u8 env1;
  u8 env2;

  u8 scope[CV_SCOPE_NUM];
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

#ifdef __cplusplus
}
#endif

#endif /* _MBCV_HWCFG_H */
