// $Id$
/*
 * Header file for Rotary Encoder Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_ENC_H
#define _MIOS32_ENC_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// maximal number of rotary encoders
#ifndef MIOS32_ENC_NUM_MAX
#define MIOS32_ENC_NUM_MAX 64
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  DISABLED=0x00,
  NON_DETENTED=0xff,
  DETENTED1=0xaa,
  DETENTED2=0x22,
  DETENTED3=0x88,
  DETENTED4=0xa5,
  DETENTED5=0x5a
} mios32_enc_type_t;

typedef enum {
  SLOW,
  NORMAL,
  FAST
} mios32_enc_speed_t;

typedef union {
  struct {
    u32 ALL;
  } all;
  struct {
    mios32_enc_type_t   type:8;
    mios32_enc_speed_t  speed:2;
    u8                  speed_par:3;
    u8                  pos:3;
    u8                  sr;
  } cfg;
} mios32_enc_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_ENC_Init(u32 mode);

extern s32 MIOS32_ENC_ConfigSet(u32 encoder, mios32_enc_config_t config);
extern mios32_enc_config_t MIOS32_ENC_ConfigGet(u32 encoder);

extern s32 MIOS32_ENC_StateSet(u32 encoder, u8 new_state);
extern s32 MIOS32_ENC_StateGet(u32 encoder);

extern s32 MIOS32_ENC_UpdateStates(void);
extern s32 MIOS32_ENC_Handler(void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_ENC_H */
