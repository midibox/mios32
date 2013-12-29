// $Id: mbcv_map.h 1895 2013-12-20 21:45:42Z tk $
/*
 * Header file for MIDIbox CV V2 LEDRing/Encoder functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_LRE_H
#define _MBCV_LRE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// maximum number of encoders (and LED rings)
#define MBCV_LRE_NUM_ENC 32

// maximum number of banks
#define MBCV_LRE_NUM_BANKS 2

// maximum number of LEDring patterns
#define MBCV_LRE_NUM_DOUT_PATTERNS 2

// maximum number of LEDring positions
#define MBCV_LRE_NUM_DOUT_PATTERN_POS 17

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 value;
  u16 min;
  u16 max;
  u16 nrpn;
} mbcv_lre_enc_cfg_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_LRE_Init(u32 mode);

extern s32 MBNG_LRE_NotifyChange(u32 enc, s32 incrementer);

extern s32 MBNG_LRE_AutoSpeed(u32 enc, u32 range);

extern s32 MBNG_LRE_FastModeSet(u8 multiplier);
extern s32 MBNG_LRE_FastModeGet(void);

extern s32 MBNG_LRE_BankSet(u8 bank);
extern s32 MBNG_LRE_BankGet(void);

extern s32 MBCV_LRE_PatternSet(u8 num, u8 pos, u16 pattern);
extern u16 MBCV_LRE_PatternGet(u8 num, u8 pos);

extern s32 MBCV_LRE_EncCfgSet(u32 enc, u32 bank, const mbcv_lre_enc_cfg_t& cfg);
extern mbcv_lre_enc_cfg_t MBCV_LRE_EncCfgGet(u32 enc, u32 bank);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 mbcv_lre_ledring_select_inv;
extern u8 mbcv_lre_ledring_select_sr1[MBCV_LRE_NUM_ENC/16];
extern u8 mbcv_lre_ledring_select_sr2[MBCV_LRE_NUM_ENC/16];
extern u8 mbcv_lre_ledring_pattern_sr1[MBCV_LRE_NUM_ENC/16];
extern u8 mbcv_lre_ledring_pattern_sr2[MBCV_LRE_NUM_ENC/16];

#endif /* _MBCV_LRE_H */
