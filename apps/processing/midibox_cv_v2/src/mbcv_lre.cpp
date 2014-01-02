// $Id: mbcv_map.cpp 1895 2013-12-20 21:45:42Z tk $
/*
 * MIDIbox CV V2 LEDRing/Encoder functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <app.h>
#include <scs.h>
#include <MbCvEnvironment.h>

#include "mbcv_lre.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 mbcv_lre_ledring_select_inv; // set to 1 if ULN2803 installed
u8 mbcv_lre_ledring_select_sr1[MBCV_LRE_NUM_ENC/16];
u8 mbcv_lre_ledring_select_sr2[MBCV_LRE_NUM_ENC/16];
u8 mbcv_lre_ledring_pattern_sr1[MBCV_LRE_NUM_ENC/16];
u8 mbcv_lre_ledring_pattern_sr2[MBCV_LRE_NUM_ENC/16];

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SRIO_NUM_DOUT_PAGES != 16
# error "The LEDring driver requires 16 DOUT pages"
#endif

#if MBCV_LRE_NUM_DOUT_PATTERN_POS != 17
# error "The LEDring driver requires 17 ledring positions"
#endif

// the b'...' patterns have been converted with tools/patterns.pl to hexadecimal numbers
static const u16 dout_matrix_pattern_preload[MBCV_LRE_NUM_DOUT_PATTERNS][MBCV_LRE_NUM_DOUT_PATTERN_POS] = {
  {
    // 17 entries for LED pattern #1 - requires 11 LEDs
    0x0001, // [ 0] b'0000000000000001'
    0x0003, // [ 1] b'0000000000000011'
    0x0007, // [ 2] b'0000000000000111'
    0x000f, // [ 3] b'0000000000001111'
    0x001f, // [ 4] b'0000000000011111'
    0x003f, // [ 5] b'0000000000111111'
    0x007f, // [ 6] b'0000000001111111'
    0x00ff, // [ 7] b'0000000011111111'
    0x01ff, // [ 8] b'0000000111111111' // taken when mid value has been selected
    0x01ff, // [ 9] b'0000000111111111'
    0x03ff, // [10] b'0000001111111111'
    0x07ff, // [11] b'0000011111111111'
    0x0fff, // [12] b'0000111111111111'
    0x1fff, // [13] b'0001111111111111'
    0x3fff, // [14] b'0011111111111111'
    0x7fff, // [15] b'0111111111111111'
    0xffff, // [16] b'1111111111111111'
  },

  {
    // 17 entries for LED pattern #2 - requires 11 LEDs
    0x00ff, // [ 0] b'0000000011111111'
    0x00fe, // [ 1] b'0000000011111110'
    0x00fc, // [ 2] b'0000000011111100'
    0x00f8, // [ 3] b'0000000011111000'
    0x00f0, // [ 4] b'0000000011110000'
    0x00e0, // [ 5] b'0000000011100000'
    0x00c0, // [ 6] b'0000000011000000'
    0x0080, // [ 7] b'0000000010000000'
    0x0180, // [ 8] b'0000000110000000' // taken when mid value has been selected
    0x0100, // [ 9] b'0000000100000000'
    0x0300, // [10] b'0000001100000000'
    0x0700, // [11] b'0000011100000000'
    0x0f00, // [12] b'0000111100000000'
    0x1f00, // [13] b'0001111100000000'
    0x3f00, // [14] b'0011111100000000'
    0x7f00, // [15] b'0111111100000000'
    0xff00, // [16] b'1111111100000000'
  },

};

// stored in RAM so that it can be redefined in the .NGC file
AHB_SECTION static u16 dout_matrix_pattern[MBCV_LRE_NUM_DOUT_PATTERNS][MBCV_LRE_NUM_DOUT_PATTERN_POS];


AHB_SECTION static mbcv_lre_enc_cfg_t enc_cfg[MBCV_LRE_NUM_BANKS][MBCV_LRE_NUM_ENC];

static const u16 default_nrpn[MBCV_LRE_NUM_BANKS][MBCV_LRE_NUM_ENC] = {
  { // bank1
    0*0x400 + 0x100, // CV1 LFO1 Amplitude
    0*0x400 + 0x101, // CV1 LFO1 Rate
    0*0x400 + 0x180, // CV1 LFO2 Amplitude
    0*0x400 + 0x181, // CV1 LFO2 Rate
    0*0x400 + 0x200, // CV1 ENV1 Amplitude
    0*0x400 + 0x204, // CV1 ENV1 Decay
    0*0x400 + 0x280, // CV1 ENV2 Amplitude
    0*0x400 + 0x283, // CV1 ENV2 Rate

    1*0x400 + 0x100, // CV2 LFO1 Amplitude
    1*0x400 + 0x101, // CV2 LFO1 Rate
    1*0x400 + 0x180, // CV2 LFO2 Amplitude
    1*0x400 + 0x181, // CV2 LFO2 Rate
    1*0x400 + 0x200, // CV2 ENV1 Amplitude
    1*0x400 + 0x204, // CV2 ENV1 Decay
    1*0x400 + 0x280, // CV2 ENV2 Amplitude
    1*0x400 + 0x283, // CV2 ENV2 Rate

    0*0x400 + 0x0c0, // CV1 SEQ Key
    0*0x400 + 0x0c1, // CV1 SEQ Key
    0*0x400 + 0x0c2, // CV1 SEQ Key
    0*0x400 + 0x0c3, // CV1 SEQ Key
    0*0x400 + 0x0c4, // CV1 SEQ Key
    0*0x400 + 0x0c5, // CV1 SEQ Key
    0*0x400 + 0x0c6, // CV1 SEQ Key
    0*0x400 + 0x0c7, // CV1 SEQ Key
    0*0x400 + 0x0c8, // CV1 SEQ Key
    0*0x400 + 0x0c9, // CV1 SEQ Key
    0*0x400 + 0x0ca, // CV1 SEQ Key
    0*0x400 + 0x0cb, // CV1 SEQ Key
    0*0x400 + 0x0cc, // CV1 SEQ Key
    0*0x400 + 0x0cd, // CV1 SEQ Key
    0*0x400 + 0x0ce, // CV1 SEQ Key
    0*0x400 + 0x0cf, // CV1 SEQ Key
  },
  { // bank2
    2*0x400 + 0x100, // CV3 LFO1 Amplitude
    2*0x400 + 0x101, // CV3 LFO1 Rate
    2*0x400 + 0x180, // CV3 LFO2 Amplitude
    2*0x400 + 0x181, // CV3 LFO2 Rate
    2*0x400 + 0x200, // CV3 ENV1 Amplitude
    2*0x400 + 0x204, // CV3 ENV1 Decay
    2*0x400 + 0x280, // CV3 ENV2 Amplitude
    2*0x400 + 0x283, // CV3 ENV2 Rate

    3*0x400 + 0x100, // CV4 LFO1 Amplitude
    3*0x400 + 0x101, // CV4 LFO1 Rate
    3*0x400 + 0x180, // CV4 LFO2 Amplitude
    3*0x400 + 0x181, // CV4 LFO2 Rate
    3*0x400 + 0x200, // CV4 ENV1 Amplitude
    3*0x400 + 0x204, // CV4 ENV1 Decay
    3*0x400 + 0x280, // CV4 ENV2 Amplitude
    3*0x400 + 0x283, // CV4 ENV2 Rate

    1*0x400 + 0x0c0, // CV2 SEQ Key
    1*0x400 + 0x0c1, // CV2 SEQ Key
    1*0x400 + 0x0c2, // CV2 SEQ Key
    1*0x400 + 0x0c3, // CV2 SEQ Key
    1*0x400 + 0x0c4, // CV2 SEQ Key
    1*0x400 + 0x0c5, // CV2 SEQ Key
    1*0x400 + 0x0c6, // CV2 SEQ Key
    1*0x400 + 0x0c7, // CV2 SEQ Key
    1*0x400 + 0x0c8, // CV2 SEQ Key
    1*0x400 + 0x0c9, // CV2 SEQ Key
    1*0x400 + 0x0ca, // CV2 SEQ Key
    1*0x400 + 0x0cb, // CV2 SEQ Key
    1*0x400 + 0x0cc, // CV2 SEQ Key
    1*0x400 + 0x0cd, // CV2 SEQ Key
    1*0x400 + 0x0ce, // CV2 SEQ Key
    1*0x400 + 0x0cf, // CV2 SEQ Key
  },
  { // bank3
    4*0x400 + 0x100, // CV5 LFO1 Amplitude
    4*0x400 + 0x101, // CV5 LFO1 Rate
    4*0x400 + 0x180, // CV5 LFO2 Amplitude
    4*0x400 + 0x181, // CV5 LFO2 Rate
    4*0x400 + 0x200, // CV5 ENV1 Amplitude
    4*0x400 + 0x204, // CV5 ENV1 Decay
    4*0x400 + 0x280, // CV5 ENV2 Amplitude
    4*0x400 + 0x283, // CV5 ENV2 Rate

    5*0x400 + 0x100, // CV6 LFO1 Amplitude
    5*0x400 + 0x101, // CV6 LFO1 Rate
    5*0x400 + 0x180, // CV6 LFO2 Amplitude
    5*0x400 + 0x181, // CV6 LFO2 Rate
    5*0x400 + 0x200, // CV6 ENV1 Amplitude
    5*0x400 + 0x204, // CV6 ENV1 Decay
    5*0x400 + 0x280, // CV6 ENV2 Amplitude
    5*0x400 + 0x283, // CV6 ENV2 Rate

    2*0x400 + 0x0c0, // CV3 SEQ Key
    2*0x400 + 0x0c1, // CV3 SEQ Key
    2*0x400 + 0x0c2, // CV3 SEQ Key
    2*0x400 + 0x0c3, // CV3 SEQ Key
    2*0x400 + 0x0c4, // CV3 SEQ Key
    2*0x400 + 0x0c5, // CV3 SEQ Key
    2*0x400 + 0x0c6, // CV3 SEQ Key
    2*0x400 + 0x0c7, // CV3 SEQ Key
    2*0x400 + 0x0c8, // CV3 SEQ Key
    2*0x400 + 0x0c9, // CV3 SEQ Key
    2*0x400 + 0x0ca, // CV3 SEQ Key
    2*0x400 + 0x0cb, // CV3 SEQ Key
    2*0x400 + 0x0cc, // CV3 SEQ Key
    2*0x400 + 0x0cd, // CV3 SEQ Key
    2*0x400 + 0x0ce, // CV3 SEQ Key
    2*0x400 + 0x0cf, // CV3 SEQ Key
  },
  { // bank4
    6*0x400 + 0x100, // CV7 LFO1 Amplitude
    6*0x400 + 0x101, // CV7 LFO1 Rate
    6*0x400 + 0x180, // CV7 LFO2 Amplitude
    6*0x400 + 0x181, // CV7 LFO2 Rate
    6*0x400 + 0x200, // CV7 ENV1 Amplitude
    6*0x400 + 0x204, // CV7 ENV1 Decay
    6*0x400 + 0x280, // CV7 ENV2 Amplitude
    6*0x400 + 0x283, // CV7 ENV2 Rate

    7*0x400 + 0x100, // CV8 LFO1 Amplitude
    7*0x400 + 0x101, // CV8 LFO1 Rate
    7*0x400 + 0x180, // CV8 LFO2 Amplitude
    7*0x400 + 0x181, // CV8 LFO2 Rate
    7*0x400 + 0x200, // CV8 ENV1 Amplitude
    7*0x400 + 0x204, // CV8 ENV1 Decay
    7*0x400 + 0x280, // CV8 ENV2 Amplitude
    7*0x400 + 0x283, // CV8 ENV2 Rate

    3*0x400 + 0x0c0, // CV4 SEQ Key
    3*0x400 + 0x0c1, // CV4 SEQ Key
    3*0x400 + 0x0c2, // CV4 SEQ Key
    3*0x400 + 0x0c3, // CV4 SEQ Key
    3*0x400 + 0x0c4, // CV4 SEQ Key
    3*0x400 + 0x0c5, // CV4 SEQ Key
    3*0x400 + 0x0c6, // CV4 SEQ Key
    3*0x400 + 0x0c7, // CV4 SEQ Key
    3*0x400 + 0x0c8, // CV4 SEQ Key
    3*0x400 + 0x0c9, // CV4 SEQ Key
    3*0x400 + 0x0ca, // CV4 SEQ Key
    3*0x400 + 0x0cb, // CV4 SEQ Key
    3*0x400 + 0x0cc, // CV4 SEQ Key
    3*0x400 + 0x0cd, // CV4 SEQ Key
    3*0x400 + 0x0ce, // CV4 SEQ Key
    3*0x400 + 0x0cf, // CV4 SEQ Key
  },
};

static u8 enc_bank;
static u8 enc_speed_multiplier;
static u8 enc_config_mode;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_Init(u32 mode)
{
  enc_speed_multiplier = 0;
  enc_config_mode = 0;

  for(int i=1; i<MIOS32_ENC_NUM_MAX; ++i) { // start at 1 since the first encoder is allocated by SCS
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(i);
    enc_config.cfg.type = NON_DETENTED;
    enc_config.cfg.sr = 0;
    enc_config.cfg.pos = 0;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(i, enc_config);
  }

  // TODO: make this configurable
  for(int i=0; i<MBCV_LRE_NUM_ENC; ++i) {
    int enc_ix = i + 1;
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(enc_ix);
    enc_config.cfg.type = NON_DETENTED;
    enc_config.cfg.sr = 3 + (i/4); // a DIO_MATRIX module with 2 DIN/2 DOUT is connected between core and LRE8x2
    enc_config.cfg.pos = 2 * (i % 4);
    MIOS32_ENC_ConfigSet(enc_ix, enc_config);
  }

  // TODO: make this configurable
#if (MBCV_LRE_NUM_ENC/16) != 2
# error "adapt here"
#endif
  mbcv_lre_ledring_select_inv = 1;
  mbcv_lre_ledring_select_sr1[0] = 3;
  mbcv_lre_ledring_select_sr2[0] = 4;
  mbcv_lre_ledring_pattern_sr1[0] = 5;
  mbcv_lre_ledring_pattern_sr2[0] = 6;
  mbcv_lre_ledring_select_sr1[1] = 7;
  mbcv_lre_ledring_select_sr2[1] = 8;
  mbcv_lre_ledring_pattern_sr1[1] = 9;
  mbcv_lre_ledring_pattern_sr2[1] = 10;

  // initial LED patterns
  memcpy((u16 *)dout_matrix_pattern, (u16 *)dout_matrix_pattern_preload, 2*MBCV_LRE_NUM_DOUT_PATTERNS*MBCV_LRE_NUM_DOUT_PATTERN_POS);

  // initial enc values
  for(int bank=0; bank<MBCV_LRE_NUM_BANKS; ++bank) {
    for(int enc=0; enc<MBCV_LRE_NUM_ENC; ++enc) {
      MBCV_LRE_EncCfgSetFromDefault(enc, bank, default_nrpn[bank][enc]);
    }
  }

  enc_speed_multiplier = 0; // disabled
  enc_bank = 0; // first bank

  MBCV_LRE_UpdateAllLedRings();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LEDring update
/////////////////////////////////////////////////////////////////////////////
static s32 MBCV_LRE_UpdateLedRingPattern(u32 enc, u16 sr_pattern)
{
  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  u16 ledring_cluster = enc / 16;
#if MIOS32_SRIO_NUM_DOUT_PAGES != 16
# error "The LEDring driver requires 16 DOUT pages"
#endif
  u16 ledring_select = enc % 16;
  u16 sr_select = mbcv_lre_ledring_select_inv ? (1 << ledring_select) : ~(1 << ledring_select);

  if( mbcv_lre_ledring_select_sr1[ledring_cluster] ) {
    MIOS32_DOUT_PageSRSet(ledring_select, mbcv_lre_ledring_select_sr1[ledring_cluster] - 1, sr_select);
  }

  if( mbcv_lre_ledring_select_sr2[ledring_cluster] ) {
    MIOS32_DOUT_PageSRSet(ledring_select, mbcv_lre_ledring_select_sr2[ledring_cluster] - 1, sr_select >> 8);
  }

  if( mbcv_lre_ledring_pattern_sr1[ledring_cluster] ) {
    MIOS32_DOUT_PageSRSet(ledring_select, mbcv_lre_ledring_pattern_sr1[ledring_cluster] - 1, sr_pattern);
  }

  if( mbcv_lre_ledring_pattern_sr2[ledring_cluster] ) {
    MIOS32_DOUT_PageSRSet(ledring_select, mbcv_lre_ledring_pattern_sr2[ledring_cluster] - 1, sr_pattern >> 8);
  }

  return 0; // no error
}

static s32 MBCV_LRE_UpdateLedRing(u32 enc)
{
  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[enc_bank][enc];

  MbCvEnvironment* env = APP_GetEnv();
  u16 value;
  if( !env->getNRPN(e->nrpn, &value) ) {
    MBCV_LRE_UpdateLedRingPattern(enc, 0x0000);
  } else {
    int range = (e->min <= e->max) ? (e->max - e->min + 1) : (e->min - e->max + 1);
    int pos = (16 * value) / range;

    // middle pos
#if MBCV_LRE_NUM_DOUT_PATTERN_POS != 17
# error "The LEDring driver requires 17 ledring positions"
#endif
    if( value == (range / 2) ) {
      pos = 8;
    } else if( pos >= 8 ) {
      ++pos;
    }

    // transfer to DOUT
    MBCV_LRE_UpdateLedRingPattern(enc, dout_matrix_pattern[e->pattern][pos]);
  }

  return 0; // no error
}

s32 MBCV_LRE_UpdateAllLedRings(void)
{
  for(int enc=0; enc<MBCV_LRE_NUM_ENC; ++enc) {
    MBCV_LRE_UpdateLedRing(enc);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Enables/Disables fast mode with given multiplier
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_FastModeSet(u8 multiplier)
{
  enc_speed_multiplier = multiplier;
  return 0;
}

s32 MBCV_LRE_FastModeGet(void)
{
  return enc_speed_multiplier;
}


/////////////////////////////////////////////////////////////////////////////
// Enables/Disables configuration mode
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_ConfigModeSet(u8 configMode)
{
  enc_config_mode = configMode;
  return 0;
}

s32 MBCV_LRE_ConfigModeGet(void)
{
  return enc_config_mode;
}


/////////////////////////////////////////////////////////////////////////////
// Sets/Gets the encoder bank
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_BankSet(u8 bank)
{
  if( bank >= MBCV_LRE_NUM_BANKS )
    return -1; // invalid bank

  enc_bank = bank;

  return 0;
}

s32 MBCV_LRE_BankGet(void)
{
  return enc_bank;
}


/////////////////////////////////////////////////////////////////////////////
// Set/Get pattern
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_PatternSet(u8 num, u8 pos, u16 pattern)
{
  if( num >= MBCV_LRE_NUM_DOUT_PATTERNS )
    return -1;

  if( pos >= MBCV_LRE_NUM_DOUT_PATTERN_POS )
    return -2;

  dout_matrix_pattern[num][pos] = pattern;

  return 0; // no error
}

u16 MBCV_LRE_PatternGet(u8 num, u8 pos)
{
  if( num >= MBCV_LRE_NUM_DOUT_PATTERNS )
    return 0x0000;

  if( pos >= MBCV_LRE_NUM_DOUT_PATTERN_POS )
    return 0x0000;

  return dout_matrix_pattern[num][pos];
}


/////////////////////////////////////////////////////////////////////////////
// Set/Get Encoder Config
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_EncCfgSet(u32 enc, u32 bank, const mbcv_lre_enc_cfg_t& cfg)
{
  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  if( bank >= MBCV_LRE_NUM_BANKS )
    return -2; // invalid bank

  enc_cfg[bank][enc] = cfg;

  return 0; // no error
}

s32 MBCV_LRE_EncCfgSetFromDefault(u32 enc, u32 bank, u16 nrpnNumber)
{
  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  if( bank >= MBCV_LRE_NUM_BANKS )
    return -2; // invalid bank

  MbCvEnvironment* env = APP_GetEnv();
  MbCvNrpnInfoT info;
  if( !env->getNRPNInfo(nrpnNumber, &info) )
    return -3; // NRPN not available

  mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[bank][enc];

  e->nrpn = nrpnNumber;
  e->min = info.min;
  e->max = info.max;
  e->pattern = info.is_bidir ? 1 : 0;

  return 0; // no error
}

mbcv_lre_enc_cfg_t MBCV_LRE_EncCfgGet(u32 enc, u32 bank)
{
  const mbcv_lre_enc_cfg_t dummy_cfg = {
    0, // min
    127, // max
    0, // nrpn
    0  // pattern
  };

  if( enc >= MBCV_LRE_NUM_ENC )
    return dummy_cfg; // invalid encoder

  if( bank >= MBCV_LRE_NUM_BANKS )
    return dummy_cfg; // invalid encoder

  return enc_cfg[bank][enc];
}


/////////////////////////////////////////////////////////////////////////////
// Sets the encoder speed depending on value range
// should this be optional?
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_AutoSpeed(u32 enc, u32 range)
{
  int enc_ix = enc + 1; // app.cpp decrements -1 from encoder due to SCS

  mios32_enc_config_t enc_config;
  enc_config = MIOS32_ENC_ConfigGet(enc_ix); // add +1 since the first encoder is allocated by SCS

  mios32_enc_speed_t cfg_speed = NORMAL;
  int                cfg_speed_par = 0;

  if( enc_config.cfg.type == NON_DETENTED ) {
    if( range < 32  ) {
      cfg_speed = SLOW;
      cfg_speed_par = 3;
    } else if( range <= 256 ) {
      cfg_speed = NORMAL;
    } else {
      cfg_speed = FAST;
      cfg_speed_par = 2 + (range / 2048);
      if( cfg_speed_par > 7 )
	cfg_speed_par = 7;
    }
  } else {
    if( range < 32  ) {
      cfg_speed = SLOW;
      cfg_speed_par = 3;
    } else if( range <= 64 ) {
      cfg_speed = NORMAL;
    } else {
      cfg_speed = FAST;
      cfg_speed_par = 2 + (range / 2048);
      if( cfg_speed_par > 7 )
	cfg_speed_par = 7;
    }
  }

  if( enc_config.cfg.speed != cfg_speed || enc_config.cfg.speed_par != cfg_speed_par ) {
    enc_config.cfg.speed = cfg_speed;
    enc_config.cfg.speed_par = cfg_speed_par;
    MIOS32_ENC_ConfigSet(enc_ix, enc_config); // add +1 since the first encoder is allocated by SCS
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from APP_ENC_NotifyChange on encoder updates
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_NotifyChange(u32 enc, s32 incrementer)
{
  MbCvEnvironment* env = APP_GetEnv();

#if 0
  DEBUG_MSG("[MBCV_LRE] ENC#%d %d\n", enc, incrementer);
#endif

  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  if( enc_speed_multiplier > 1 ) {
    incrementer *= (s32)enc_speed_multiplier;
  }

  if( enc_config_mode ) {
    mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[enc_bank][enc];

    int nrpnNumber = e->nrpn + incrementer;
    if( nrpnNumber < 0 )
      nrpnNumber = 0;
    else if( nrpnNumber > 16383 )
      nrpnNumber = 16383;

    if( incrementer < 0 ) {
      while( 1 ) {
	u16 nrpnValue;
	if( env->getNRPN(nrpnNumber, &nrpnValue) ) {
	  break;
	} else {
	  if( --nrpnNumber < 0 )
	    return 0; // first value already selected
	}
      }
    } else {
      while( 1 ) {
	u16 nrpnValue;
	if( env->getNRPN(nrpnNumber, &nrpnValue) ) {
	  break;
	} else {
	  if( ++nrpnNumber > 16383 )
	    return 0; // last value already selected
	}
      }
    }
    e->nrpn = nrpnNumber;

    MbCvNrpnInfoT info;
    if( env->getNRPNInfo(e->nrpn, &info) ) {
      SCS_Msg(SCS_MSG_L, 1000, info.nameString, info.valueString);
    } else {
      char buffer1[21];
      char buffer2[21];
      sprintf(buffer1, "NRPN #%5d", nrpnNumber);
      sprintf(buffer2, "CVx Value: ----");
      SCS_Msg(SCS_MSG_L, 1000, buffer1, buffer2);
    }
  } else {
    mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[enc_bank][enc];
    int range = (e->min <= e->max) ? (e->max - e->min + 1) : (e->min - e->max + 1);
    MBCV_LRE_AutoSpeed(enc, range);

    MbCvEnvironment* env = APP_GetEnv();
    u16 nrpnValue;
    if( !env->getNRPN(e->nrpn, &nrpnValue) ) {
      MBCV_LRE_UpdateLedRing(enc);
      return 0; // no valid NRPN value mapped
    }

    int value = nrpnValue;
    int new_value;
    if( e->min <= e->max ) {
      new_value = value + incrementer;
      if( new_value < e->min )
      new_value = e->min;
      else if( new_value > e->max )
	new_value = e->max;
    } else { // reversed range
      new_value = value - incrementer;
      if( new_value < e->max )
	new_value = e->max;
      else if( new_value > e->min )
	new_value = e->min;
    }

    if( value != new_value ) {
#if 0
      DEBUG_MSG("[MBCV_LRE] ENC#%d nrpn=0x%04x  value=%d\n", enc, e->nrpn, e->value);
#endif

      env->setNRPN(e->nrpn, new_value);
      env->midiSendNRPNToActivePort(e->nrpn, new_value);
      
      MbCvNrpnInfoT info;
      if( env->getNRPNInfo(e->nrpn, &info) ) {
	SCS_Msg(SCS_MSG_L, 1000, info.nameString, info.valueString);
      }
    }
  }

  MBCV_LRE_UpdateLedRing(enc);

  return 0; // no error
}
