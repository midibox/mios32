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

#ifndef _LC_SYSEX_H
#define _LC_SYSEX_H

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
    unsigned METER_CHN_RECEIVED:1;
    unsigned METER_MODE_RECEIVED:1;
    unsigned :1;
    unsigned :1;
  };
} sysex_state_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_SYSEX_Init(u32 mode);
extern s32 LC_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 LC_SYSEX_CmdFinished(void);
extern s32 LC_SYSEX_SendResponse(u8 *buffer, u8 len);
extern s32 LC_SYSEX_Cmd(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_Query(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_MCQuery(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_HostReply(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_VersionReq(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_WriteMTC(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_WriteStatus(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_WriteLCD(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_MeterMode(u8 cmd_state, u8 midi_in);
extern s32 LC_SYSEX_Cmd_MeterGMode(u8 cmd_state, u8 midi_in);

#endif /* _LC_SYSEX_H */
