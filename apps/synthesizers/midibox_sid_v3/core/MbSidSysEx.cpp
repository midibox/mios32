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
#include "MbSidEnvironment.h"
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SID_DISACK_LESS_BYTES_THAN_EXP	0x01
#define SID_DISACK_WRONG_CHECKSUM	    0x03
#define SID_DISACK_BS_NOT_AVAILABLE	    0x0a
#define SID_DISACK_PAR_NOT_AVAILABLE	0x0b
#define SID_DISACK_INVALID_COMMAND	    0x0c
#define SID_DISACK_NO_RAM_ACCESS	    0x10
#define SID_DISACK_BS_TOO_SMALL		    0x11
#define SID_DISACK_WRONG_TYPE		    0x12
#define SID_DISACK_SID_NOT_AVAILABLE    0x13


/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////
static const u8 sidSysexHeader[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4b };


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSysEx::MbSidSysEx()
{
    lastSysexPort = DEFAULT;
    sysexState.ALL = 0;
    sysexDeviceId = MIOS32_MIDI_DeviceIDGet(); // taken from MIOS32
    sysexSidSelection = 0x0f;
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
    if( sysexState.MY_SYSEX && port != lastSysexPort )
        return false;

    lastSysexPort = port;

    // branch depending on state
    if( !sysexState.MY_SYSEX ) {
        if( (sysexState.CTR < sizeof(sidSysexHeader) && midi_in != sidSysexHeader[sysexState.CTR]) ||
            (sysexState.CTR == sizeof(sidSysexHeader) && midi_in != sysexDeviceId) ) {
            // incoming byte doesn't match
            cmdFinished();
        } else {
            if( ++sysexState.CTR > sizeof(sidSysexHeader) ) {
                // complete header received, waiting for data
                sysexState.MY_SYSEX = 1;
            }
        }
    } else {
        // check for end of SysEx message or invalid status byte
        if( midi_in >= 0x80 ) {
            if( midi_in == 0xf7 && sysexState.CMD ) {
                cmd(port, SYSEX_CMD_STATE_END, midi_in);
            }
            cmdFinished();
        } else {
            // check if command byte has been received
            if( !sysexState.CMD ) {
                sysexState.CMD = 1;
                sysexCmd = midi_in;
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
    if( sysexState.MY_SYSEX && port == lastSysexPort )
        cmdFinished();
}


/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmdFinished(void)
{
    // clear all status variables
    sysexState.ALL = 0;
    sysexCmd = 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
void MbSidSysEx::cmd(mios32_midi_port_t port, mbsid_sysex_cmd_state_t cmd_state, u8 midi_in)
{
    switch( sysexCmd ) {
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
        sysexState.TYPE_RECEIVED = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysexState.TYPE_RECEIVED ) {
            sysexState.TYPE_RECEIVED = 1;
            sysexPatchType = midi_in;
        } else if( !sysexState.BANK_RECEIVED ) {
            sysexState.BANK_RECEIVED = 1;
            sysexBank = midi_in;
        } else if( !sysexState.PATCH_RECEIVED ) {
            sysexState.PATCH_RECEIVED = 1;
            sysexPatch = midi_in;
        }

        break;

    default: // SYSEX_CMD_STATE_END
        if( sysexPatchType >= 0x10 && sysexPatchType != 0x70 ) {
            // invalid if unsupported type
            sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_TYPE);
        } else if( !sysexState.PATCH_RECEIVED ) {
            // invalid if patch number has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else {
            // try to send dump, send disack if this failed
            if( sendDump(port, sysexPatchType, sysexBank, sysexPatch) < 0 ) {
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
    static int bufferIx = 0;
    static int bufferIxMax = 0;

    switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
        sysexState.TYPE_RECEIVED = 0;
        sysexState.LNIBBLE_RECV = 0;
        sysexChecksum = 0;
        bufferIx = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysexState.TYPE_RECEIVED ) {
            sysexState.TYPE_RECEIVED = 1;
            sysexPatchType = midi_in;
            if( sysexPatchType < 0x10 ) {
                bufferIxMax = 512; // patch
            } else if( sysexPatchType == 0x70 ) {
                bufferIxMax = 256; // ensemble
            } else {
                bufferIxMax = 0; // invalid type
            }
        } else if( !sysexState.BANK_RECEIVED ) {
            sysexState.BANK_RECEIVED = 1;
            sysexBank = midi_in;
        } else if( !sysexState.PATCH_RECEIVED ) {
            sysexState.PATCH_RECEIVED = 1;
            sysexPatch = midi_in;
        } else if( !sysexState.CHECKSUM_RECEIVED ) {
            if( bufferIx >= bufferIxMax ) {
                sysexState.CHECKSUM_RECEIVED = 1;
                sysexReceivedChecksum = midi_in;
            } else {
                // add to checksum
                sysexChecksum += midi_in;

                if( !sysexState.LNIBBLE_RECV ) {
                    sysexState.LNIBBLE_RECV = 1;
                    sysexBuffer[bufferIx] = midi_in & 0x0f;
                } else {
                    sysexState.LNIBBLE_RECV = 0;
                    sysexBuffer[bufferIx] |= (midi_in & 0x0f) << 4;
                    ++bufferIx;
                }
            }
        }

        break;

    default: // SYSEX_CMD_STATE_END
        if( !sysexState.CHECKSUM_RECEIVED ) {
            // invalid if checksum has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else if( sysexReceivedChecksum != (-sysexChecksum & 0x7f) ) {
            // invalid if wrong checksum has been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_WRONG_CHECKSUM);
        } else {
            switch( sysexPatchType & 0xf8 ) {
            case 0x00: // Bank Write
            case 0x08: { // RAM Write
                u8 sid = sysexPatchType & 0x07; // prepared for up to 8 mbSids
                bool toBank = sysexPatchType < 0x08;
                if( mbSidEnvironmentPtr->sysexSetPatch(sid, (sid_patch_t *)sysexBuffer, toBank, sysexBank, sysexPatch) ) {
                    // notify that bytes have been written
                    sendAck(port, SYSEX_ACK, sysexReceivedChecksum);
                } else {
                    // SID not available
                    sendAck(port, SYSEX_DISACK, SID_DISACK_SID_NOT_AVAILABLE);
                }
            } break;

            // TODO: Ensemble write

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
        sysexState.TYPE_RECEIVED = 0;
        write_status = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysexState.TYPE_RECEIVED ) {
            sysexState.TYPE_RECEIVED = 1;
            sysexPatchType = midi_in;
        } else if( !sysexState.AH_RECEIVED ) {
            sysexState.AH_RECEIVED = 1;
            addr = (midi_in & 0x7f) << 7;
        } else if( !sysexState.AL_RECEIVED ) {
            sysexState.AL_RECEIVED = 1;
            addr |= (midi_in & 0x7f);
        } else if( !sysexState.DL_RECEIVED ) {
            sysexState.DL_RECEIVED = 1;
            sysexState.DH_RECEIVED = 0; // for the case that DH_RECEIVED switched back to DL_RECEIVED
            data = midi_in & 0x0f;
        } else if( !sysexState.DH_RECEIVED ) {
            sysexState.DH_RECEIVED = 1;
            data |= (midi_in & 0x0f) << 4;
            switch( sysexPatchType & 0xf8 ) {
            case 0x00: { // write parameter data for WOPT(0..7)
                u8 wopt = sysexPatchType & 7; // unnecessary masking, variable only used for documentation reasons
                MIOS32_IRQ_Disable(); // ensure atomic change of all selected addresses
                write_status |= writePatchPar(sysexSidSelection, wopt, addr, data);
                MIOS32_IRQ_Enable();
            } break;

            case 0x70: // Ensemble Write
                MIOS32_IRQ_Disable(); // ensure atomic change of all selected addresses
                write_status |= writeEnsPar(sysexSidSelection, addr, data);
                MIOS32_IRQ_Enable();

            default:
                write_status |= -1;
            }

            // clear DL_RECEIVED. This allows to send additional bytes to the next address
            sysexState.DL_RECEIVED = 0;
            ++addr;
        }
        break;

    default: // SYSEX_CMD_STATE_END
        if( !sysexState.DH_RECEIVED ) {
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
        sysexState.TYPE_RECEIVED = 0;
        extra_cmd = 0;
        play_instrument = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        if( !sysexState.TYPE_RECEIVED ) {
            sysexState.TYPE_RECEIVED = 1;
            extra_cmd = midi_in;
        } else if( !sysexState.EXTRA_CMD_RECEIVED ) {
            sysexState.EXTRA_CMD_RECEIVED = 1;
        } else {
            switch( extra_cmd ) {
            case 0x00: // select SIDs
                sysexSidSelection = midi_in;
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
        if( !sysexState.EXTRA_CMD_RECEIVED ) {
            // invalid if command has not been received
            sendAck(port, SYSEX_DISACK, SID_DISACK_LESS_BYTES_THAN_EXP);
        } else {
            switch( extra_cmd ) {
            case 0x00: { // select SIDs
                // return available SIDs (TODO)
                sendAck(port, SYSEX_ACK, 0x01 & sysexSidSelection); // only SID1 available
            } break;

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
        sysexState.TYPE_RECEIVED = 0;
        break;

    case SYSEX_CMD_STATE_CONT:
        sysexState.TYPE_RECEIVED = 1;
        break;

    default: // SYSEX_CMD_STATE_END
        // send acknowledge if no additional byte has been received
        // to avoid feedback loop if two cores are directly connected
        if( !sysexState.TYPE_RECEIVED )
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
    u8 sysexBuffer[32]; // should be enough?
    u8 *sysexBuffer_ptr = &sysexBuffer[0];

    for(int i=0; i<(int)sizeof(sidSysexHeader); ++i)
        *sysexBuffer_ptr++ = sidSysexHeader[i];

    // device ID
    *sysexBuffer_ptr++ = sysexDeviceId;

    // send ack code and argument
    *sysexBuffer_ptr++ = ack_code;
    *sysexBuffer_ptr++ = ack_arg;

    // send footer
    *sysexBuffer_ptr++ = 0xf7;

    // finally send SysEx stream
    MUTEX_MIDIOUT_TAKE;
    s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysexBuffer, (u32)sysexBuffer_ptr - ((u32)&sysexBuffer[0]));
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
        s32 status = 0;
        // write directly into patch with wopt==0
        for(int sid=0; sid<8; ++sid)
            if( sid_selection & (1 << sid) )
                status |= (mbSidEnvironmentPtr->sysexSetParameter(sid, addr, data) ? 0 : -1);
        // important: we *must* return here to avoid endless recursion!
        // workaround for current java based Midibox SID Editor version:
        // don't check for SID availability...
        // seems that we should never remove this workaround to avoid user confusion (they probably won't update the editor)
#if 0
        return status;
#else
        return 0;
#endif
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
    int sysexBufferIx = 0;

    // TODO: currently only support for patch
    if( type >= 0x10 )
        return -1;

    // prepared for up to 8 SIDs
    u8 sid = type & 0x07;
    bool fromBank = type < 0x08;

    // send header
    for(int i=0; i<(int)sizeof(sidSysexHeader); ++i)
        sysexBuffer[sysexBufferIx++] = sidSysexHeader[i];

    // device ID
    sysexBuffer[sysexBufferIx++] = sysexDeviceId;

    // "write patch" command (so that dump could be sent back to overwrite EEPROM w/o modifications)
    sysexBuffer[sysexBufferIx++] = 0x02;

    // send type, bank and patch number
    sysexBuffer[sysexBufferIx++] = type;
    sysexBuffer[sysexBufferIx++] = patch;
    sysexBuffer[sysexBufferIx++] = bank;

    // copy patch into SysEx buffer
    u8 *patchRange = &sysexBuffer[sysexBufferIx];
    if( !mbSidEnvironmentPtr->sysexGetPatch(sid, (sid_patch_t *)patchRange, fromBank, sysexBank, sysexPatch) )
        return -2; // SID not available

    // this is tricky: nibbleize the patch inside buffer
    u8 checksum = 0;
    for(int i=511; i>=0; --i) {
        u8 b = patchRange[i];
        u8 upper = b >> 4;
        patchRange[2*i + 1] = upper;
        checksum += upper;
        u8 lower = b & 0x0f;
        patchRange[2*i + 0] = lower;
        checksum += lower;
    }
    sysexBufferIx += 2*512;

    // send checksum
    sysexBuffer[sysexBufferIx++] = -checksum & 0x7f;

    // send footer
    sysexBuffer[sysexBufferIx++] = 0xf7;

    // finally send SysEx stream and return error status
    MUTEX_MIDIOUT_TAKE;
    s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysexBuffer, sysexBufferIx);
    MUTEX_MIDIOUT_GIVE;
    return status;
}
