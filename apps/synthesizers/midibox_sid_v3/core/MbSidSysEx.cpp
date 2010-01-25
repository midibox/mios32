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

#include <string.h>
#include "MbSidSysEx.h"
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SID_DISACK_LESS_BYTES_THAN_EXP	0x01
#define SID_DISACK_WRONG_CHECKSUM	0x03
#define SID_DISACK_BS_NOT_AVAILABLE	0x0a
#define SID_DISACK_PAR_NOT_AVAILABLE	0x0b
#define SID_DISACK_INVALID_COMMAND	0x0c
#define SID_DISACK_NO_RAM_ACCESS	0x10
#define SID_DISACK_BS_TOO_SMALL		0x11
#define SID_DISACK_WRONG_TYPE		0x12


/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////
static const u8 sid_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4b };


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSysEx::MbSidSysEx()
{
    last_sysex_port = DEFAULT;
    sysex_state.ALL = 0;
    sysex_device_id = MIOS32_MIDI_DeviceIDGet(); // taken from MIOS32
    sysex_sid_selection = 0x0f;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSysEx::~MbSidSysEx()
{
}



/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream
// returns true if stream is taken - in this case, forwarding to
// APP_MIDI_NotifyPackage() can be prevented by returning 1 (instead of 0)
// from the MIOS32 SysEx Callback function
/////////////////////////////////////////////////////////////////////////////
bool MbSidSysEx::parse(mios32_midi_port_t port, u8 midi_in)
{
    // ignore realtime messages (see MIDI spec - realtime messages can
    // always be injected into events/streams, and don't change the running status)
    if( midi_in >= 0xf8 )
        return false;


    // TODO: here we could send an error notification, that multiple devices are trying to access the device
    if( sysex_state.MY_SYSEX && port != last_sysex_port )
        return false;

    last_sysex_port = port;

    // branch depending on state
    if( !sysex_state.MY_SYSEX ) {
        if( (sysex_state.CTR < sizeof(sid_sysex_header) && midi_in != sid_sysex_header[sysex_state.CTR]) ||
            (sysex_state.CTR == sizeof(sid_sysex_header) && midi_in != sysex_device_id) ) {
            // incoming byte doesn't match
            cmdFinished();
        } else {
            if( ++sysex_state.CTR > sizeof(sid_sysex_header) ) {
                // complete header received, waiting for data
                sysex_state.MY_SYSEX = 1;
            }
        }
    } else {
        // check for end of SysEx message or invalid status byte
        if( midi_in >= 0x80 ) {
            if( midi_in == 0xf7 && sysex_state.CMD ) {
                cmd(port, SYSEX_CMD_STATE_END, midi_in);
            }
            cmdFinished();
        } else {
            // check if command byte has been received
            if( !sysex_state.CMD ) {
                sysex_state.CMD = 1;
                sysex_cmd = midi_in;
                cmd(port, SYSEX_CMD_STATE_BEGIN, midi_in);
            }
            else
                cmd(port, SYSEX_CMD_STATE_CONT, midi_in);
        }
    }

    return true; // don't forward package to APP_MIDI_NotifyPackage()
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
// MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::timeOut(mios32_midi_port_t port)
{
    // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
    if( sysex_state.MY_SYSEX && port == last_sysex_port )
        cmdFinished();
}


/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdFinished(void)
{
    // clear all status variables
    sysex_state.ALL = 0;
    sysex_cmd = 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmd(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    switch( sysex_cmd ) {
    case 0x01:
        cmdPatchRead(port, cmd_state, midi_in);
        break;

    case 0x02:
        cmdPatchWrite(port, cmd_state, midi_in);
        break;

    case 0x06:
        cmdParWrite(port, cmd_state, midi_in);
        break;

    case 0x0c:
        cmdExtra(port, cmd_state, midi_in);
        break;

    case 0x0e: // ignore to avoid loopbacks
        break;

    case 0x0f:
        cmdPing(port, cmd_state, midi_in);
        break;

    default:
        // unknown command
        sendAck(port, SYSEX_DISACK, SID_DISACK_INVALID_COMMAND);
        cmdFinished();      
    }
}



/////////////////////////////////////////////////////////////////////////////
// Command 01: Patch Read
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdPatchRead(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysex_state.TYPE_RECEIVED = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysex_state.TYPE_RECEIVED ) {
            sysex_state.TYPE_RECEIVED = 1;
            sysex_patch_type = midi_in;
        } else if( !sysex_state.BANK_RECEIVED ) {
            sysex_state.BANK_RECEIVED = 1;
            sysex_bank = midi_in;
        } else if( !sysex_state.PATCH_RECEIVED ) {
            sysex_state.PATCH_RECEIVED = 1;
            sysex_patch = midi_in;
        }

        break;

    default: // SYSEX_CMD_STATE_END
        if( sysex_patch_type != 0x00 && sysex_patch_type != 0x08 && sysex_patch_type != 0x70 ) {
            // invalid if unsupported type
            sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_TYPE);
        } else if( !sysex_state.PATCH_RECEIVED ) {
            // invalid if patch number has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else {
            // try to send dump, send disack if this failed
            if( sendDump(port, sysex_patch_type, sysex_bank, sysex_patch) < 0 ) {
                sendAck(port, SYSEX_DISACK, SID_DISACK_BS_NOT_AVAILABLE);
            }
        }
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Command 02: Patch Write
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdPatchWrite(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    static int buffer_ix = 0;
    static int buffer_ix_max = 0;

    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysex_state.TYPE_RECEIVED = 0;
        sysex_state.LNIBBLE_RECV = 0;
        sysex_checksum = 0;
        buffer_ix = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysex_state.TYPE_RECEIVED ) {
            sysex_state.TYPE_RECEIVED = 1;
            sysex_patch_type = midi_in;
            if( sysex_patch_type == 0x00 || sysex_patch_type == 0x08 ) {
                buffer_ix_max = 512; // patch
            } else if( sysex_patch_type == 0x70 ) {
                buffer_ix_max = 256; // ensemble
            } else {
                buffer_ix_max = 0; // invalid type
            }
        } else if( !sysex_state.BANK_RECEIVED ) {
            sysex_state.BANK_RECEIVED = 1;
            sysex_bank = midi_in;
        } else if( !sysex_state.PATCH_RECEIVED ) {
            sysex_state.PATCH_RECEIVED = 1;
            sysex_patch = midi_in;
        } else if( !sysex_state.CHECKSUM_RECEIVED ) {
            if( buffer_ix >= buffer_ix_max ) {
                sysex_state.CHECKSUM_RECEIVED = 1;
                sysex_received_checksum = midi_in;
            } else {
                // add to checksum
                sysex_checksum += midi_in;

                if( !sysex_state.LNIBBLE_RECV ) {
                    sysex_state.LNIBBLE_RECV = 1;
                    sysex_buffer[buffer_ix] = midi_in & 0x0f;
                } else {
                    sysex_state.LNIBBLE_RECV = 0;
                    sysex_buffer[buffer_ix] |= (midi_in & 0x0f) << 4;
                    ++buffer_ix;
                }
            }
        }

        break;

    default: // SYSEX_CMD_STATE_END
        if( !sysex_state.CHECKSUM_RECEIVED ) {
            // invalid if checksum has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
            // invalid if wrong checksum has been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_CHECKSUM);
        } else {
            switch( sysex_patch_type ) {
            case 0x00: // Bank Write (tmp. write into RAM as well)
            case 0x08: // RAM Write
#if 0
                // TODO: tmp. copy routine
                memcpy((u8 *)&sid_patch[0].ALL[0], (u8 *)sysex_buffer, 512);
                SID_PATCH_Changed(0); // sid==0
#endif
                // notify that bytes have been written
                sendAck(port, SYSEX_ACK, sysex_received_checksum);
                break;
            default:
                // unsuported type
                sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_TYPE);
            }
        }
        break;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Command 06: Parameter Write
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdParWrite(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    static u16 addr = 0;
    static u8 data = 0;
    static s32 write_status = 0;

    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysex_state.TYPE_RECEIVED = 0;
        write_status = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysex_state.TYPE_RECEIVED ) {
            sysex_state.TYPE_RECEIVED = 1;
            sysex_patch_type = midi_in;
        } else if( !sysex_state.AH_RECEIVED ) {
            sysex_state.AH_RECEIVED = 1;
            addr = (midi_in & 0x7f) << 7;
        } else if( !sysex_state.AL_RECEIVED ) {
            sysex_state.AL_RECEIVED = 1;
            addr |= (midi_in & 0x7f);
        } else if( !sysex_state.DL_RECEIVED ) {
            sysex_state.DL_RECEIVED = 1;
            sysex_state.DH_RECEIVED = 0; // for the case that DH_RECEIVED switched back to DL_RECEIVED
            data = midi_in & 0x0f;
        } else if( !sysex_state.DH_RECEIVED ) {
            sysex_state.DH_RECEIVED = 1;
            data |= (midi_in & 0x0f) << 4;
            switch( sysex_patch_type ) {
            case 0x00: // write parameter data for WOPT(0..3)
            case 0x01:
            case 0x02:
            case 0x03: {
                u8 wopt = sysex_patch_type & 3; // unnecessary masking, variable only used for documentation reasons
                MIOS32_IRQ_Disable(); // ensure atomic change of all selected addresses
                write_status |= writePatchPar(sysex_sid_selection, wopt, addr, data);
                MIOS32_IRQ_Enable();
            } break;

            case 0x70: // Ensemble Write
                MIOS32_IRQ_Disable(); // ensure atomic change of all selected addresses
                write_status |= writeEnsPar(sysex_sid_selection, addr, data);
                MIOS32_IRQ_Enable();

            default:
                write_status |= -1;
            }

            // clear DL_RECEIVED. This allows to send additional bytes to the next address
            sysex_state.DL_RECEIVED = 0;
            ++addr;
        }
        break;

    default: // SYSEX_CMD_STATE_END
        if( !sysex_state.DH_RECEIVED ) {
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else if( write_status < 0 ) {
            sendAck(port, SYSEX_DISACK, SID_DISACK_PAR_NOT_AVAILABLE);
        } else {
            sendAck(port, SYSEX_ACK, 0x00);
        }
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Command 0C: Extra Commands
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdExtra(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    static int extra_cmd = 0;
    static int play_instrument = 0;

    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysex_state.TYPE_RECEIVED = 0;
        extra_cmd = 0;
        play_instrument = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysex_state.TYPE_RECEIVED ) {
            sysex_state.TYPE_RECEIVED = 1;
            extra_cmd = midi_in;
        } else if( !sysex_state.EXTRA_CMD_RECEIVED ) {
            sysex_state.EXTRA_CMD_RECEIVED = 1;
        } else {
            switch( extra_cmd ) {
            case 0x00: // select SIDs
                sysex_sid_selection = midi_in;
                break;

            case 0x08: // All Notes On/Off
                break; // nothing else to do

            case 0x09: // Play current patch
                play_instrument = midi_in;
                break;
            }
        }
        break;

    default: // SYSEX_CMD_STATE_END
        if( !sysex_state.EXTRA_CMD_RECEIVED ) {
            // invalid if command has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else {
            switch( extra_cmd ) {
            case 0x00: // select SIDs
                // return available SIDs (TODO)
                sendAck(port, SYSEX_ACK, 0x01 & sysex_sid_selection); // only SID1 available
                break;

            case 0x08: // All Notes On/Off
                sendAck(port, SYSEX_ACK, 0x00);
                break; // nothing else to do

            case 0x09: // Play current patch
                sendAck(port, SYSEX_ACK, 0x00);
                break;

            default:
                // unsuported type
                sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_TYPE);
            }
        }
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdPing(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysex_state.TYPE_RECEIVED = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        sysex_state.TYPE_RECEIVED = 1;
        break;

    default: // SYSEX_CMD_STATE_END
        // send acknowledge if no additional byte has been received
        // to avoid feedback loop if two cores are directly connected
        if( !sysex_state.TYPE_RECEIVED )
            sendAck(port, SYSEX_ACK, 0x00);

        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
s32 MbSidSysEx::sendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
    u8 sysex_buffer[32]; // should be enough?
    u8 *sysex_buffer_ptr = &sysex_buffer[0];

    for(int i=0; i<(int)sizeof(sid_sysex_header); ++i)
        *sysex_buffer_ptr++ = sid_sysex_header[i];

    // device ID
    *sysex_buffer_ptr++ = sysex_device_id;

    // send ack code and argument
    *sysex_buffer_ptr++ = ack_code;
    *sysex_buffer_ptr++ = ack_arg;

    // send footer
    *sysex_buffer_ptr++ = 0xf7;

    // finally send SysEx stream
    MUTEX_MIDIOUT_TAKE;
    s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
    MUTEX_MIDIOUT_GIVE;
    return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function writes into a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbSidSysEx::writePatchPar(u8 sid_selection, u8 wopt, u16 addr, u8 data)
{
    s32 status = 0;

    if( addr >= 512 )
        return -1;

    if( wopt == 0 ) {
        // write directly into patch with wopt==0
        //sid_patch[0].ALL[addr] = data;
        //sid_patch_shadow[0].ALL[addr] = data;
        // TODO: transfer to multiple SIDs depending on sid_selection
        return 0; // no error
        // important: we *must* return here to avoid endless recursion!
    }

    // WOPT == 1: SID L/R write
    // WOPT == 2: Voice123/456 Flag
    // WOPT == 3: Combined Left/Right and Voice123/456 Flag

    // check address ranges
    if( (wopt == 1 || wopt == 3) && addr >= 0x040 && addr <= 0x04f ) {
        // External Values: share EXT1/2, EXT3/4, EXT5/6, EXT7/8
        addr &= ~(1 << 1);
        status |= writePatchPar(sid_selection, 0, addr + 0, data);
        status |= writePatchPar(sid_selection, 0, addr + 2, data);
        return status;
    }

    if( (wopt == 1 || wopt == 3) && addr >= 0x054 && addr <= 0x05f ) {
        // Filter: share FIL1/2
        addr = ((addr - 0x54) % 6) + 0x54;
        status |= writePatchPar(sid_selection, 0, addr + 0, data);
        status |= writePatchPar(sid_selection, 0, addr + 6, data);
        return status;
    }

    // Engine dependend ranges
    //TODO sid_se_engine_t engine = (sid_se_engine_t)sid_patch_shadow[0].engine;
    sid_se_engine_t engine = SID_SE_LEAD;
    switch( engine ) {
    case SID_SE_LEAD:
        if( addr >= 0x60 && addr <= 0xbf ) {
            // Voice Parameters
            if( wopt == 1 ) { // SIDL/R
                addr = ((addr - 0x60) % 0x30) + 0x60;
                status |= writePatchPar(sid_selection, 0, addr + 0x00, data);
                status |= writePatchPar(sid_selection, 0, addr + 0x30, data);
                return status;
            } else if( wopt == 2 ) { // Voice 123/456
                if( addr >= 0x90 )
                    addr = ((addr - 0x90) % 0x30) + 0x90;
                else
                    addr = ((addr - 0x60) % 0x30) + 0x60;

                status |= writePatchPar(sid_selection, 0, addr + 0*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 1*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 2*0x10, data);
                return status;
            } else if( wopt == 3 ) { // Combined Left/Right and Voice123/456 Flag
                addr = ((addr - 0x60) % 0x10) + 0x60;
                status |= writePatchPar(sid_selection, 0, addr + 0*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 1*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 2*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 3*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 4*0x10, data);
                status |= writePatchPar(sid_selection, 0, addr + 5*0x10, data);
                return status;
            }
        } else if( addr >= 0x100 && addr <= 0x13f ) {
            if( wopt == 1 || wopt == 3 ) {
                u16 mod_base_addr = addr & ~0x7;
                u16 mod_offset = addr & 0x7;
                if( mod_offset >= 0x4 && mod_offset <= 0x5 ) {
                    // Modulation Target Assignments
                    status |= writePatchPar(sid_selection, 0, mod_base_addr + 0x4, data);
                    status |= writePatchPar(sid_selection, 0, mod_base_addr + 0x5, data);
                    return status;
                }
            }
        }
        break;

    case SID_SE_BASSLINE:
        if( addr >= 0x60 && addr <= 0xff ) {
            // Voice Parameters
            if( wopt == 1 ) { // SIDL/R
                addr = ((addr - 0x60) % 0x50) + 0x60;
                status |= writePatchPar(sid_selection, 0, addr + 0x00, data);
                status |= writePatchPar(sid_selection, 0, addr + 0x50, data);
                return status;
            }
        }
        break;

    case SID_SE_DRUM:
        // no support for additional ranges
        break;

    case SID_SE_MULTI:
        if( addr >= 0x060 && addr <= 0x17f ) {
            if( wopt == 1 ) { // SIDL/R
                // Voice Parameters
                addr = ((addr - 0x060) % 0x090) + 0x060;
                status |= writePatchPar(sid_selection, 0, addr + 0x00, data);
                status |= writePatchPar(sid_selection, 0, addr + 0x90, data);
                return status;
            } else if( wopt == 2 ) { // Voice 123/456
                if( addr >= 0x0f0 )
                    addr = ((addr - 0x0f0) % 0x090) + 0x0f0;
                else
                    addr = ((addr - 0x060) % 0x090) + 0x060;

                status |= writePatchPar(sid_selection, 0, addr + 0*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 1*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 2*0x30, data);
                return status;
            } else if( wopt == 3 ) { // Combined Left/Right and Voice123/456 Flag
                addr = ((addr - 0x060) % 0x030) + 0x060;
                status |= writePatchPar(sid_selection, 0, addr + 0*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 1*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 2*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 3*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 4*0x30, data);
                status |= writePatchPar(sid_selection, 0, addr + 5*0x30, data);
                return status;
            }
        }
        break;
    }

    // no WOPT support -> write single value
    return writePatchPar(sid_selection, 0, addr, data);
}


/////////////////////////////////////////////////////////////////////////////
// This function writes into an ensemble
/////////////////////////////////////////////////////////////////////////////
s32 MbSidSysEx::writeEnsPar(u8 sid_selection, u16 addr, u8 data)
{
    if( addr >= 256 )
        return -1;

    // TODO: ensembles

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MbSidSysEx::sendDump(mios32_midi_port_t port, u8 type, u8 bank, u8 patch)
{
    int sysex_buffer_ix = 0;
    u8 checksum;
    u8 c;

    // TODO: currently only support for patch
    if( type != 0x00 && type != 0x08 )
        return -1;

    // send header
    for(int i=0; i<(int)sizeof(sid_sysex_header); ++i)
        sysex_buffer[sysex_buffer_ix++] = sid_sysex_header[i];

    // "write patch" command (so that dump could be sent back to overwrite EEPROM w/o modifications)
    sysex_buffer[sysex_buffer_ix++] = 0x02;

    // send type, bank and patch number
    sysex_buffer[sysex_buffer_ix++] = type;
    sysex_buffer[sysex_buffer_ix++] = patch;
    sysex_buffer[sysex_buffer_ix++] = bank;

    // send patch content
    checksum = 0;
    for(int i=0; i<512; ++i) {
#if 0
        // TODO
        c = PATCH_ReadByte((u8)i);
#else
        //c = sid_patch_shadow[0].ALL[i];
        c = 0;
#endif

        sysex_buffer[sysex_buffer_ix++] = c & 0x0f;
        checksum += c & 0x0f;
        sysex_buffer[sysex_buffer_ix++] = c >> 4;
        checksum += c >> 4;
    }

    // send checksum
    sysex_buffer[sysex_buffer_ix++] = -checksum & 0x7f;

    // send footer
    sysex_buffer[sysex_buffer_ix++] = 0xf7;

    // finally send SysEx stream and return error status
    MUTEX_MIDIOUT_TAKE;
    s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, sysex_buffer_ix);
    MUTEX_MIDIOUT_GIVE;
    return status;
}
