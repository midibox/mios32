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
static u16 dout_matrix_pattern[MBCV_LRE_NUM_DOUT_PATTERNS][MBCV_LRE_NUM_DOUT_PATTERN_POS];


static mbcv_lre_enc_cfg_t enc_cfg[MBCV_LRE_NUM_BANKS][MBCV_LRE_NUM_ENC];

static u8 enc_bank;
static u8 enc_speed_multiplier;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MBNG_LRE_UpdateAllLedRings(void);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_LRE_Init(u32 mode)
{
  enc_speed_multiplier = 0;

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
      mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[bank][enc];
      e->value = 0;
      e->min = 0;
      e->max = 127;
      e->nrpn = 0;
    }
  }

  enc_speed_multiplier = 0; // disabled
  enc_bank = 0; // first bank

  MBNG_LRE_UpdateAllLedRings();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LEDring update
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_LRE_UpdateLedRingPattern(u32 enc, u16 sr_pattern)
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

static s32 MBNG_LRE_UpdateLedRing(u32 enc)
{
  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[enc_bank][enc];
  int range = (e->min <= e->max) ? (e->max - e->min + 1) : (e->min - e->max + 1);
  int pos = (16 * e->value) / range;

  // middle pos
#if MBCV_LRE_NUM_DOUT_PATTERN_POS != 17
# error "The LEDring driver requires 17 ledring positions"
#endif
  if( e->value == (range / 2) ) {
    pos = 8;
  } else if( pos >= 8 ) {
    ++pos;
  }

  // transfer to DOUT
  int pattern = 0; // TODO: select depending on value type
  MBNG_LRE_UpdateLedRingPattern(enc, dout_matrix_pattern[pattern][pos]);

  return 0; // no error
}

static s32 MBNG_LRE_UpdateAllLedRings(void)
{
  for(int enc=0; enc<MBCV_LRE_NUM_ENC; ++enc) {
    MBNG_LRE_UpdateLedRing(enc);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Enables/Disables fast mode with given multiplier
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LRE_FastModeSet(u8 multiplier)
{
  enc_speed_multiplier = multiplier;
  return 0;
}

s32 MBNG_LRE_FastModeGet(void)
{
  return enc_speed_multiplier;
}


/////////////////////////////////////////////////////////////////////////////
// Sets/Gets the encoder bank
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LRE_BankSet(u8 bank)
{
  if( bank >= MBCV_LRE_NUM_BANKS )
    return -1; // invalid bank

  enc_bank = bank;

  return 0;
}

s32 MBNG_LRE_BankGet(void)
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

#if 0
  enc_cfg[bank][enc] = enc_cfg;
#else
  memcpy(&enc_cfg[bank][enc], enc_cfg, sizeof(mbcv_lre_enc_cfg_t));
#endif

  return 0; // no error
}

mbcv_lre_enc_cfg_t MBCV_LRE_EncCfgGet(u32 enc, u32 bank)
{
  const mbcv_lre_enc_cfg_t dummy_cfg = {
    0, // value
    0, // min
    127, // max
    0, // nrpn
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
s32 MBNG_LRE_AutoSpeed(u32 enc, u32 range)
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
s32 MBNG_LRE_NotifyChange(u32 enc, s32 incrementer)
{
#if 0
  DEBUG_MSG("[MBCV_LRE] ENC#%d %d\n", enc, incrementer);
#endif

  if( enc >= MBCV_LRE_NUM_ENC )
    return -1; // invalid encoder

  if( enc_speed_multiplier > 1 ) {
    incrementer *= (s32)enc_speed_multiplier;
  }

  mbcv_lre_enc_cfg_t *e = (mbcv_lre_enc_cfg_t *)&enc_cfg[enc_bank][enc];
  int range = (e->min <= e->max) ? (e->max - e->min + 1) : (e->min - e->max + 1);
  MBNG_LRE_AutoSpeed(enc, range);

  int value = e->value;
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

  if( e->value != new_value ) {
    e->value = new_value;
#if 0
    DEBUG_MSG("[MBCV_LRE] ENC#%d value %d\n", enc, e->value);
#endif

    // TODO: forward to MbCvEnvironment

    MBNG_LRE_UpdateLedRing(enc);
  }

  return 0; // no error
}
