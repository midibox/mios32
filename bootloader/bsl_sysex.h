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

// buffer size (due to scrambling, must be larger than received bytes)
#define BSL_SYSEX_BUFFER_SIZE 1024

// max. received bytes
#define BSL_SYSEX_MAX_BYTES 1024

// ack/disack code
#define BSL_SYSEX_DISACK   0x0e
#define BSL_SYSEX_ACK      0x0f

// disacknowledge arguments
#define BSL_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define BSL_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define BSL_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define BSL_SYSEX_DISACK_WRITE_FAILED         0x04
#define BSL_SYSEX_DISACK_WRITE_ACCESS         0x05
#define BSL_SYSEX_DISACK_MIDI_TIMEOUT         0x06
#define BSL_SYSEX_DISACK_WRONG_DEBUG_CMD      0x07
#define BSL_SYSEX_DISACK_WRONG_ADDR_RANGE     0x08
#define BSL_SYSEX_DISACK_ADDR_NOT_ALIGNED     0x09
#define BSL_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define BSL_SYSEX_DISACK_OVERRUN              0x0b
#define BSL_SYSEX_DISACK_FRAME_ERROR          0x0c
#define BSL_SYSEX_DISACK_WRONG_UNLOCK_SEQ     0x0d
#define BSL_SYSEX_DISACK_INVALID_COMMAND      0x0e


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  BSL_SYSEX_CMD_STATE_BEGIN,
  BSL_SYSEX_CMD_STATE_CONT,
  BSL_SYSEX_CMD_STATE_END
} bsl_sysex_cmd_state_t;


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
  BSL_SYSEX_REC_UNLOCK1,
  BSL_SYSEX_REC_UNLOCK2,
  BSL_SYSEX_REC_UNLOCK3,
  BSL_SYSEX_REC_UNLOCK_OK,
  BSL_SYSEX_REC_ID,
  BSL_SYSEX_REC_ID_OK,
  BSL_SYSEX_REC_INVALID
} bsl_sysex_rec_state_t;


typedef union {
  struct {
    unsigned ALL:8;
  };

  struct {
    unsigned CTR:3;
    unsigned CMD:1;
    unsigned MY_SYSEX:1;
  };
} sysex_state_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BSL_SYSEX_Init(u32 mode);
extern s32 BSL_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);

extern s32 BSL_SYSEX_SendUploadReq(mios32_midi_port_t port);

#endif /* _BSL_SYSEX_H */
