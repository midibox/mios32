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

#include "MbSidAsid.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////
static const u8 asid_reg_map[28] = {
  0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x04, 0x0b, 0x12, 0x04, 0x0b, 0x12
};


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidAsid::MbSidAsid()
{
    asid_state = MBSID_ASID_STATE_SYX0;
    asid_last_sysex_port = DEFAULT;
    asid_cmd = 0;
    asid_stream_ix = 0;
    asid_reg_ix = 0;

    modeSet(MBSID_ASID_MODE_OFF);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidAsid::~MbSidAsid()
{
}



/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream
// returns true if stream is taken - in this case, forwarding to
// APP_MIDI_NotifyPackage() can be prevented by returning 1 (instead of 0)
// from the MIOS32 SysEx Callback function
/////////////////////////////////////////////////////////////////////////////
bool MbSidAsid::parse(mios32_midi_port_t port, u8 midi_in)
{
    // ignore realtime messages (see MIDI spec - realtime messages can
    // always be injected into events/streams, and don't change the running status)
    if( midi_in >= 0xf8 )
        return false;


    // TODO: here we could send an error notification, that multiple devices are trying to access the device
    if( asid_mode != MBSID_ASID_MODE_OFF && port != asid_last_sysex_port )
        return false;

    asid_last_sysex_port = port;

    // clear state if status byte (like 0xf0 or 0xf7...) has been received
    if( midi_in & (1 << 7) )
        asid_state = MBSID_ASID_STATE_SYX0;

    // check SysEx sequence
    switch( asid_state ) {
    case MBSID_ASID_STATE_SYX0:
        if( midi_in == 0xf0 )
            asid_state = MBSID_ASID_STATE_SYX1;
        break;

    case MBSID_ASID_STATE_SYX1:
        if( midi_in == 0x2d )
            asid_state = MBSID_ASID_STATE_CMD;
        else
            asid_state = MBSID_ASID_STATE_SYX0;
        break;

    case MBSID_ASID_STATE_CMD:
        asid_cmd = midi_in;

        switch( asid_cmd ) {
        case 0x4c: // Start
            modeSet(MBSID_ASID_MODE_ON);
            break;

        case 0x4d: // Stop
            modeSet(MBSID_ASID_MODE_OFF);
            break;

        case 0x4e: // Sequence
            // ensure that SID player is properly initialized (some players don't send start command!)
            if( asid_mode == MBSID_ASID_MODE_OFF )
                modeSet(MBSID_ASID_MODE_ON);
            asid_stream_ix = 0;
            asid_reg_ix = 0;
            break;

        case 0x4f: // LCD
            // ensure that SID player is properly initialized (some players don't send start command!)
            if( asid_mode == MBSID_ASID_MODE_OFF )
                modeSet(MBSID_ASID_MODE_ON);
            break;
        }

        asid_state = MBSID_ASID_STATE_DATA;
        break;

    case MBSID_ASID_STATE_DATA:
        switch( asid_cmd ) {
        case 0x4c: // Start
            break;

        case 0x4d: // Stop
            break;

        case 0x4e: // Sequence
            newData(midi_in);
            break;

        case 0x4f: // LCD
            break;
        }
        break;

    default:
        // unexpected state
        timeOut(port);
    }

    return true; // don't forward package to APP_MIDI_NotifyPackage()
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
// MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
void MbSidAsid::timeOut(mios32_midi_port_t port)
{
    // called on SysEx timeout
    if( asid_state != MBSID_ASID_STATE_SYX0 && port == asid_last_sysex_port ) {
        asid_state = MBSID_ASID_STATE_SYX0;
        asid_last_sysex_port = DEFAULT;
        asid_cmd = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// ASID Mode Control
/////////////////////////////////////////////////////////////////////////////
mbsid_asid_mode_t MbSidAsid::modeGet(void)
{
    return asid_mode;
}

void MbSidAsid::modeSet(mbsid_asid_mode_t mode)
{
    asid_mode = mode;
}


/////////////////////////////////////////////////////////////////////////////
// Parses ASID Data Stream
/////////////////////////////////////////////////////////////////////////////
void MbSidAsid::newData(u8 midi_in)
{
    if( asid_stream_ix < 4 ) {
        asid_masks[asid_stream_ix] = midi_in;
    } else if( asid_stream_ix < 8 ) {
        asid_msbs[asid_stream_ix-4] = midi_in;
    } else {
        // scan for next mask flag
        u8 taken = 0;
        do {
            // exit if last mapped register already reached
            if( asid_reg_ix >= sizeof(asid_reg_map) )
                return; // all register values received

            // change register if mask flag set
            if( asid_masks[asid_reg_ix/7] & (1 << (asid_reg_ix % 7)) ) {
                u8 sid_value = midi_in;
                if( asid_msbs[asid_reg_ix/7] & (1 << (asid_reg_ix % 7)) )
                    sid_value |= (1 << 7);

                sid_regs_t *sid_l = (sid_regs_t *)&sid_regs[0];
                sid_regs_t *sid_r = (sid_regs_t *)&sid_regs[1];
                u8 sid_reg = asid_reg_map[asid_reg_ix];

                sid_l->ALL[sid_reg] = sid_value;
                sid_r->ALL[sid_reg] = sid_value;
                SID_Update(0);

                taken = 1;
            }

            // try next register
            ++asid_reg_ix;
        } while( !taken );
    }

    ++asid_stream_ix;
}
