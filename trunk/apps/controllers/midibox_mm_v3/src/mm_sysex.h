// $Id$
/*
 * Header file for SysEx Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MM_SYSEX_H
#define _MM_SYSEX_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define SYSEX_CMD_STATE_BEGIN 0
#define SYSEX_CMD_STATE_CONT  1
#define SYSEX_CMD_STATE_END   2

/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u8 ALL;
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
    unsigned :1;
    unsigned CURSOR_POS_RECEIVED:1;
    unsigned :1;
    unsigned :1;
  };

  struct {
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned POINTER_TYPE_RECEIVED:1;
    unsigned POINTER_POS_RECEIVED:1;
    unsigned :1;
    unsigned :1;
  };
} sysex_state_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_SYSEX_Init(u32 mode);
extern s32 MM_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 MM_SYSEX_CmdFinished(void);
extern s32 MM_SYSEX_TimeOut(mios32_midi_port_t port);
extern s32 MM_SYSEX_SendResponse(u8 *buffer, u8 len);
extern s32 MM_SYSEX_Cmd(u8 cmd_state, u8 midi_in);
extern s32 MM_SYSEX_Cmd_WriteLCD(u8 cmd_state, u8 midi_in);
extern s32 MM_SYSEX_Cmd_WriteValue(u8 cmd_state, u8 midi_in);
extern s32 MM_SYSEX_Cmd_Digits(u8 cmd_state, u8 midi_in);

#endif /* _MM_SYSEX_H */
