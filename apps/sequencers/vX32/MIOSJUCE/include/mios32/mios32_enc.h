// $Id: mios32_enc.h 144 2008-12-02 00:07:05Z tk $
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
  DISABLED,
  NON_DETENTED,
  DETENTED1,
  DETENTED2,
  DETENTED3
} mios32_enc_type_t;

typedef enum {
  SLOW,
  NORMAL,
  FAST
} mios32_enc_speed_t;

typedef union {
  struct {
    unsigned ALL:32;
  } all;
  struct {
    mios32_enc_type_t   type:3;
    mios32_enc_speed_t  speed:2;
    unsigned            speed_par:3;
    unsigned            sr:4;
    unsigned            pos:3;
  } cfg;
} mios32_enc_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_ENC_Init(u32 mode);

extern s32 MIOS32_ENC_ConfigSet(u32 encoder, mios32_enc_config_t config);
extern mios32_enc_config_t MIOS32_ENC_ConfigGet(u32 encoder);

extern s32 MIOS32_ENC_UpdateStates(void);
extern s32 MIOS32_ENC_Handler(void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_ENC_H */
