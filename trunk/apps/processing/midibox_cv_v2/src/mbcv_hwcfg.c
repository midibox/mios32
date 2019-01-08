// $Id$
/*
 * Hardware Soft-Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "mbcv_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// following predefenitions are matching with the standard layout
// They can be overwritten in the MBCV_HW.CV2 file, which is loaded from SD Card during startup

#define SR_CFG(sr, pin)  (((sr-1)<<3)+pin)

mbcv_hwcfg_button_t mbcv_hwcfg_button = {
  .scs_exit = SR_CFG(0, 0),
  .scs_shift = SR_CFG(0, 0),
  .scs_soft = {
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
   },

  .cv = {
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0)
  },

  .enc_bank = {
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0)
  },

  .cv_and_enc_bank = {
    SR_CFG(1, 0),
    SR_CFG(1, 1),
    SR_CFG(1, 2),
    SR_CFG(1, 3),
    SR_CFG(1, 4),
    SR_CFG(1, 5),
    SR_CFG(1, 6),
    SR_CFG(1, 7)
  },

  .lfo1 = SR_CFG(2, 0),
  .lfo2 = SR_CFG(2, 1),
  .env1 = SR_CFG(2, 2),
  .env2 = SR_CFG(2, 3),

  .scope = {
    SR_CFG(2, 4),
    SR_CFG(2, 5),
    SR_CFG(2, 6),
    SR_CFG(2, 7),
   },

  .keyboard = {
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
    SR_CFG(0, 0),
   },
};


mbcv_hwcfg_dout_t mbcv_hwcfg_dout = {
  .gate_sr = 1,
  .clk_sr = 2,
};


mbcv_hwcfg_lre_t mbcv_hwcfg_lre[MBCV_LRE_NUM] = {
  {
    .enabled = 0,
    .ledring_select_sr1 = 3,
    .ledring_select_sr2 = 4,
    .ledring_pattern_sr1 = 5,
    .ledring_pattern_sr2 = 6,
  },

  {
    .enabled = 0,
    .ledring_select_sr1 = 7,
    .ledring_select_sr2 = 8,
    .ledring_pattern_sr1 = 9,
    .ledring_pattern_sr2 = 10,
  },
};

mbcv_hwcfg_bm_t mbcv_hwcfg_bm = {
  .dout_select_sr = 0,
  .din_scan_sr = {0, 0}
};

// will be initialized in MBCV_HWCFG_Init()
mbcv_hwcfg_ws2812_t mbcv_hwcfg_ws2812[MBCV_RGB_LED_NUM];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_HWCFG_Init(u32 mode)
{
  int i;

  // clear all encoder entries
  // they will only be preloaded via the MBCV_HW.CV2 file
  for(i=1; i<MIOS32_ENC_NUM_MAX; ++i) { // start at 1 since the first encoder is allocated by SCS
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(i);
    enc_config.cfg.type = NON_DETENTED;
    enc_config.cfg.sr = 0;
    enc_config.cfg.pos = 0;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(i, enc_config);
  }

  // clear all LED entries
  {
    mbcv_hwcfg_ws2812_t *ws2812 = &mbcv_hwcfg_ws2812[0];
    for(i=0; i<MBCV_RGB_LED_NUM; ++i, ++ws2812) {
      ws2812->pos = i;
      ws2812->mode = MBCV_RGB_MODE_DISABLED;
    }
  }

  return 0; // no error
}
