// $Id$
/*
 * SysEx Parser for BLM_SCALAR
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_H
#define _SYSEX_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// command states
#define SYSEX_CMD_STATE_BEGIN 0
#define SYSEX_CMD_STATE_CONT  1
#define SYSEX_CMD_STATE_END   2

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define SYSEX_DISACK_INVALID_COMMAND      0x0c


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };

  struct {
    unsigned CTR:3;
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned CMD:1;
    unsigned MY_SYSEX:1;
  };

  struct {
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned DATA_RECEIVED:1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
  };
} sysex_state_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SYSEX_Init(u32 mode);
extern s32 SYSEX_Send(mios32_midi_port_t port, u8 bank, u8 patch);
extern s32 SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);

extern s32 SYSEX_SendLayoutInfo(mios32_midi_port_t port);

#endif /* _SYSEX_H */
