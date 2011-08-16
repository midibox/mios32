// $Id$
/*
 * Patch Layer for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"

#include "mbcv_patch.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"


/////////////////////////////////////////////////////////////////////////////
// Preset patch
/////////////////////////////////////////////////////////////////////////////

mbcv_patch_cv_entry_t mbcv_patch_cv[MBCV_PATCH_NUM_CV] = {
  // ports  chn mode prange split l/u   +oct +semi    CC  curve slew
  { 0x1011,  0, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  1, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  2, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  3, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  4, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  5, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  6, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
  { 0x1011,  7, 0x10,   2,  0x00, 0x7f, 0x00, 0x00, 0x10, 0x00, 0x00 },
};

mbcv_patch_router_entry_t mbcv_patch_router[MBCV_PATCH_NUM_ROUTER] = {
  // src chn   dst chn
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
};

u8 mbcv_patch_gate_inverted[MBCV_PATCH_NUM_CV/8];

mbcv_patch_cfg_t mbcv_patch_cfg = {
#if 0
  // TK: doesn't work, these values are initialized in PATCH_Init()
  .flags = { .MERGER_MODE = 0,
  },
#endif
  .ext_clk_divider = 3,
};


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // init remaining config values
  mbcv_patch_cfg.flags.ALL = 0;

  int i;
  for(i=0; i<(MBCV_PATCH_NUM_CV/8); ++i)
    mbcv_patch_gate_inverted[i] = 0x00;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns a byte from patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
u8 MBCV_PATCH_ReadByte(u16 addr)
{
  if( addr >= MBCV_PATCH_SIZE )
    return 0;

  // Note: this patch structure only exists for compatibility reasons with MBCV V1
  // Don't enhance it! Meanwhile the complete patch can only be edited via SD Card!
  if( addr < 8 ) {
    switch( addr ) {
    case 0x00: return mbcv_patch_cfg.flags.MERGER_MODE;
    case 0x02: return mbcv_patch_gate_inverted[0];
    case 0x03: return mbcv_patch_cfg.ext_clk_divider;
    }
  } else {
    u8 cv = addr & 0x7;
    switch( addr >> 3 ) {
    case 0x01: return mbcv_patch_cv[cv].chn;
    case 0x02: return mbcv_patch_cv[cv].midi_mode.ALL;
    case 0x03: return mbcv_patch_cv[cv].pitchrange;
    case 0x04: return mbcv_patch_cv[cv].split_l;
    case 0x05: return mbcv_patch_cv[cv].split_u;
    case 0x06: return mbcv_patch_cv[cv].transpose_oct;
    case 0x07: return mbcv_patch_cv[cv].transpose_semi;
    case 0x08: return mbcv_patch_cv[cv].cc_number;
    case 0x09: return mbcv_patch_cv[cv].curve;
    case 0x0a: return mbcv_patch_cv[cv].slew_rate;
    }
  }

  return 0x00;
}


/////////////////////////////////////////////////////////////////////////////
// This function writes a byte into patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_WriteByte(u16 addr, u8 byte)
{
  if( addr >= MBCV_PATCH_SIZE )
    return -1; // invalid address

  if( addr >= MBCV_PATCH_SIZE )
    return 0;

  // Note: this patch structure only exists for compatibility reasons with MBCV V1
  // Don't enhance it! Meanwhile the complete patch can only be edited via SD Card!
  if( addr < 8 ) {
    switch( addr ) {
    case 0x00: mbcv_patch_cfg.flags.MERGER_MODE = byte; return 0;
    case 0x02: mbcv_patch_gate_inverted[0] = byte; return 0;
    case 0x03: mbcv_patch_cfg.ext_clk_divider = byte; return 0;
    }
    return 0x00;
  } else if( addr < 0x100 ) {
    u8 cv = addr & 0x7;
    switch( addr >> 3 ) {
    case 0x01: mbcv_patch_cv[cv].chn = byte; return 0;
    case 0x02: mbcv_patch_cv[cv].midi_mode.ALL = byte; return 0;
    case 0x03: mbcv_patch_cv[cv].pitchrange = byte; return 0;
    case 0x04: mbcv_patch_cv[cv].split_l = byte; return 0;
    case 0x05: mbcv_patch_cv[cv].split_u = byte; return 0;
    case 0x06: mbcv_patch_cv[cv].transpose_oct = byte; return 0;
    case 0x07: mbcv_patch_cv[cv].transpose_semi = byte; return 0;
    case 0x08: mbcv_patch_cv[cv].cc_number = byte; return 0;
    case 0x09: mbcv_patch_cv[cv].curve = byte; return 0;
    case 0x0a: mbcv_patch_cv[cv].slew_rate = byte; return 0;
    }
    return 0x00;
  }

  return -2; // value not mapped
}


/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Load(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Read(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Store(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}
