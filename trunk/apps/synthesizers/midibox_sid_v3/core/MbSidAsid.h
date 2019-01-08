/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID ASID SysEx Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_ASID_H
#define _MB_SID_ASID_H

#include <mios32.h>
#include "MbSidStructs.h"


// command states
typedef enum {
    MBSID_ASID_STATE_SYX0,
    MBSID_ASID_STATE_SYX1,
    MBSID_ASID_STATE_CMD,
    MBSID_ASID_STATE_DATA
} mbsid_asid_state_t;


// ASID Mode
typedef enum {
    MBSID_ASID_MODE_OFF,
    MBSID_ASID_MODE_ON
} mbsid_asid_mode_t;


class MbSidAsid
{
public:
    // Constructor
    MbSidAsid();

    // Destructor
    ~MbSidAsid();

    bool parse(mios32_midi_port_t port, u8 midi_in);
    void timeOut(mios32_midi_port_t port);
    mbsid_asid_mode_t modeGet(void);
    void modeSet(mbsid_asid_mode_t mode);

protected:
    void newData(u8 midi_in);

    mbsid_asid_mode_t asid_mode;
    mbsid_asid_state_t asid_state;
    mios32_midi_port_t asid_last_sysex_port;
    u8 asid_cmd;
    u8 asid_stream_ix;
    u8 asid_reg_ix;
    u8 asid_masks[4];
    u8 asid_msbs[4];
};

#endif /* _MB_SID_ASID_H */
