/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID SysEx Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SYSEX_H
#define _MB_SID_SYSEX_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidPatch.h"


// must be more than 1024, since patch size is 2*512 bytes
#define MBSID_SYSEX_BUFFER_SIZE 1100


// command states
typedef enum {
    SYSEX_CMD_STATE_BEGIN,
    SYSEX_CMD_STATE_CONT,
    SYSEX_CMD_STATE_END
} mbsid_sysex_cmd_state_t;

typedef union {
    u32 ALL;

    struct {
        u8 CTR:3;
        u8 MY_SYSEX:1;
        u8 CMD:1;
    };

    struct {
        u8 dummy1_CTR:3;
        u8 dummy1_MY_SYSEX:1;
        u8 dummy1_CMD:1;
        u8 TYPE_RECEIVED:1; // used by most actions
    };

    struct {
        u8 dummy2_CTR:3;
        u8 dummy2_MY_SYSEX:1;
        u8 dummy2_CMD:1;
        u8 dummy2_TYPE_RECEIVED:1; // used by most actions
        u8 BANK_RECEIVED:1; // used by PATCH_[Read/Write]
        u8 PATCH_RECEIVED:1; // used by PATCH_[Read/Write]
        u8 CHECKSUM_RECEIVED:1; // used by PATCH_Write
        u8 LNIBBLE_RECV:1; // used by PATCH_Write
    };

    struct {
        u8 dummy3_CTR:3;
        u8 dummy3_MY_SYSEX:1;
        u8 dummy3_CMD:1;
        u8 dummy3_TYPE_RECEIVED:1; // used by most actions
        u8 AH_RECEIVED:1; // used by PAR_[Read/Write]
        u8 AL_RECEIVED:1; // used by PAR_[Read/Write]
        u8 DL_RECEIVED:1; // used by PAR_[Read/Write]
        u8 DH_RECEIVED:1; // used by PAR_[Read/Write]
    };

    struct {
        u8 dummy4_CTR:3;
        u8 dummy4_MY_SYSEX:1;
        u8 dummy4_CMD:1;
        u8 dummy4_TYPE_RECEIVED:1; // used by most actions
        u8 EXTRA_CMD_RECEIVED:1; // used by Patch_ExtraCmd
    };

} mbsid_sysex_state_t;


class MbSidEnvironment; // forward declaration

class MbSidSysEx
{
public:
    // Constructor
    MbSidSysEx();

    // Destructor
    ~MbSidSysEx();

    // pointer to environment - required for sysex callbacks
    MbSidEnvironment *mbSidEnvironmentPtr;

    bool parse(mios32_midi_port_t port, u8 midi_in);
    void timeOut(mios32_midi_port_t port);
    s32 sendDump(mios32_midi_port_t port, u8 type, u8 bank, u8 patch);

protected:
    void cmdFinished(void);
    void cmd(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    void cmdPatchRead(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    void cmdPatchWrite(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    void cmdParWrite(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    void cmdExtra(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    void cmdPing(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in);
    s32 sendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);
    s32 writePatchPar(u8 sid_selection, u8 wopt, u16 addr, u8 data);
    s32 writeEnsPar(u8 sid_selection, u16 addr, u8 data);

    u8 sysexBuffer[MBSID_SYSEX_BUFFER_SIZE]; 

    mbsid_sysex_state_t sysexState;
    u8 sysexDeviceId;
    u8 sysexCmd;
    mios32_midi_port_t lastSysexPort;


    u8 sysexPatchType;
    u8 sysexBank;
    u8 sysexPatch;
    u8 sysexChecksum;
    u8 sysexReceivedChecksum;
    u8 sysexSidSelection;
};

#endif /* _MB_SID_SYSEX_H */
