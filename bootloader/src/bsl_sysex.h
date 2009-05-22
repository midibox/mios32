// $Id: sysex.h 78 2008-10-12 22:09:23Z tk $
/*
 * BSL SysEx Header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BSL_SYSEX_H
#define _BSL_SYSEX_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// max. received bytes
#define BSL_SYSEX_MAX_BYTES 1024

// buffer size (due to scrambling, must be larger than received bytes)
// + some bytes to send the header
#define BSL_SYSEX_BUFFER_SIZE (((BSL_SYSEX_MAX_BYTES*8)/7) + 20)


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


// receive state
typedef enum {
  BSL_SYSEX_REC_A3,
  BSL_SYSEX_REC_A2,
  BSL_SYSEX_REC_A1,
  BSL_SYSEX_REC_A0,
  BSL_SYSEX_REC_L3,
  BSL_SYSEX_REC_L2,
  BSL_SYSEX_REC_L1,
  BSL_SYSEX_REC_L0,
  BSL_SYSEX_REC_PAYLOAD,
  BSL_SYSEX_REC_CHECKSUM,
  BSL_SYSEX_REC_ID,
  BSL_SYSEX_REC_ID_OK,
  BSL_SYSEX_REC_INVALID
} bsl_sysex_rec_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BSL_SYSEX_Init(u32 mode);
extern s32 BSL_SYSEX_HaltStateGet(void);
extern s32 BSL_SYSEX_ReleaseHaltState(void);
extern s32 BSL_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in, u8 sysex_cmd);
extern s32 BSL_SYSEX_SendUploadReq(mios32_midi_port_t port);

#endif /* _BSL_SYSEX_H */
