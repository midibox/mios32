/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Toplevel
 * Instantiates multiple MIDIbox CV Units
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvEnvironment.h"
#include <string.h>

#include <osc_client.h>


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define CV_BANK_NUM 1  // currently only a single bank is available in ROM


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvironment::MbCvEnvironment()
{
    // initialize NRPN address/values
    for(int i=0; i<16; ++i) {
        nrpnAddress[i] = 0;
        nrpnValue[i] = 0;
    }
    lastSentNrpnAddressMsb = 0xff;
    lastSentNrpnAddressLsb = 0xff;

    // initialize structures of each CV channel
    MbCv *s = mbCv.first();
    for(int cv=0; cv < mbCv.size; ++cv, ++s) {
        s->init(cv, &mbCvClock);
    }

    // sets the default speed factor
    updateSpeedFactorSet(2);

    // restart clock generator
    bpmRestart();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvironment::~MbCvEnvironment()
{
}


/////////////////////////////////////////////////////////////////////////////
// Changes the update speed factor
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::updateSpeedFactorSet(u8 _updateSpeedFactor)
{
    updateSpeedFactor = _updateSpeedFactor;
    mbCvClock.updateSpeedFactor = _updateSpeedFactor;
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engines Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::tick(void)
{
    bool updateRequired = false;

    // Tempo Clock
    mbCvClock.tick();

    // Engines
    for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s)) {
        if( s->tick(updateSpeedFactor) )
            updateRequired = true;
    }

    if( updateRequired ) {
        // map engine parameters to CV outputs
        // we do this as a second step, so that it will be possible to map values
        // of a single engine to different channels in future
        cvGates = 0;
        MbCv *s = mbCv.first();
        u16 *out = cvOut.first();
        for(int cv=0; cv < cvOut.size; ++cv, ++s, ++out) {
            MbCvVoice *v = &s->mbCvVoice;
            MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

            if( v->voicePhysGateActive ^ v->voiceGateInverted )
                cvGates |= (1 << cv);

            if( v->voiceEventMode == MBCV_MIDI_EVENT_MODE_NOTE ) {
                *out = v->voiceFrq;
            } else {
                switch( v->voiceEventMode ) {
                case MBCV_MIDI_EVENT_MODE_VELOCITY: *out = v->transpose(v->voiceVelocity << 9); break;
                case MBCV_MIDI_EVENT_MODE_AFTERTOUCH: *out = v->transpose(mv->midivoiceAftertouch << 9); break;
                case MBCV_MIDI_EVENT_MODE_CC: *out = v->transpose(mv->midivoiceCCValue << 9); break;
                case MBCV_MIDI_EVENT_MODE_NRPN: *out = v->transpose(mv->midivoiceNRPNValue << 2); break;
                case MBCV_MIDI_EVENT_MODE_PITCHBENDER: *out = v->transpose((mv->midivoicePitchbender + 8192) << 2); break;
                case MBCV_MIDI_EVENT_MODE_CONST_MIN: *out = v->transpose(0x0000); break;
                case MBCV_MIDI_EVENT_MODE_CONST_MID: *out = v->transpose(0x8000); break;
                case MBCV_MIDI_EVENT_MODE_CONST_MAX: *out = v->transpose(0xffff); break;
                }
            }
        }
    }

    return updateRequired;
}


/////////////////////////////////////////////////////////////////////////////
// Write a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankSave(u8 cv, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return -1; // CV not available

    return -2; // not supported yet
}

/////////////////////////////////////////////////////////////////////////////
// Read a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankLoad(u8 cv, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return -1; // CV not available

    MbCv *s = &mbCv[cv];

    if( bank >= CV_BANK_NUM )
        return -2; // invalid bank

    if( patch >= 128 )
        return -3; // invalid patch

#if 0
    DEBUG_MSG("[CV_BANK_PatchRead] CV %d reads patch %c%03d\n", cv, 'A'+bank, patch);
#endif

#if 0
    switch( bank ) {
    case 0: {
        cv_patch_t *bankPatch = (cv_patch_t *)cv_bank_preset_0[patch];
        s->mbCvPatch.copyToPatch(bankPatch);
    } break;

    default:
        return -4; // no bank in ROM
    }
#endif

    s->updatePatch(false);

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns the name of a preset patch as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankPatchNameGet(u8 bank, u8 patch, char *buffer)
{
    int i;
    cv_patch_t *p;

    if( bank >= CV_BANK_NUM ) {
        sprintf(buffer, "<Invalid Bank %c>", 'A'+bank);
        return -1; // invalid bank
    }

    if( patch >= 128 ) {
        sprintf(buffer, "<WrongPatch %03d>", patch);
        return -2; // invalid patch
    }

#if 0
    switch( bank ) {
    case 0:
        p = (cv_patch_t *)&cv_bank_preset_0[patch];
        break;

    default:
        sprintf(buffer, "<Empty Bank %c>  ", 'A'+bank);
        return -3; // no bank in ROM
    }
#endif

    for(i=0; i<16; ++i)
        buffer[i] = p->name[i] >= 0x20 ? p->name[i] : ' ';
    buffer[i] = 0;

    return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Forwards incoming MIDI events to all MBCVs
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    u8 handle_nrpn = 0;
    if( midi_package.event == CC ) {
        // NRPN handling
        switch( midi_package.cc_number ) {
        case 0x63: { // Address MSB
            nrpnAddress[midi_package.chn] &= ~0x3f80;
            nrpnAddress[midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);
            return;
        } break;

        case 0x62: { // Address LSB
            nrpnAddress[midi_package.chn] &= ~0x007f;
            nrpnAddress[midi_package.chn] |= (midi_package.value & 0x007f);
            return;
        } break;

        case 0x06: { // Data MSB
            nrpnValue[midi_package.chn] &= ~0x3f80;
            nrpnValue[midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);
            //handle_nrpn = 1; // pass to synth engine
            // MEMO: it's better to update only when LSB has been received
        } break;

        case 0x26: { // Data LSB
            nrpnValue[midi_package.chn] &= ~0x007f;
            nrpnValue[midi_package.chn] |= (midi_package.value & 0x007f);
            handle_nrpn = 1; // pass to synth engine
        } break;
        }
    }

    // process received NRPN value
    if( handle_nrpn ) {
        // decode address
        u16 address = nrpnAddress[midi_package.chn];
        u16 select = address >> 10;
        u16 value = nrpnValue[midi_package.chn];

        // channel independent access
        if( select >= 0 && select < CV_SE_NUM ) { // direct channel selection
            mbCv[select].setNRPN(address, value);

        } else if( select == 0xf ) { // special commands (0x3c00..0x3fff)
            switch( address & 0x3ff ) {
            case 0x000: midiSendNRPNDump(port, value, 0); break; // Dump All: 0x3c00 <channels>

            case 0x001: midiSendNRPNDump(port, value, 1); break; // Dump Seq Only: 0x3c00 <channels>

            case 0x010: {                                     // Play Off: 0x3c10 <channels>
                MIOS32_IRQ_Disable();
                int cv = 0;
                for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s), ++cv)
                    if( value & (1 << cv) )
                        s->noteAllOff(false);
                MIOS32_IRQ_Enable();
            } break;

            case 0x011: {                                     // Play On: 0x3c11 <channels>
                MIOS32_IRQ_Disable();
                int cv = 0;
                for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s), ++cv)
                    if( value & (1 << cv) ) {
                        s->noteOn(0x3c, 0x7f, false);
                        if( s->mbCvArp.arpEnabled ) {
                            s->noteOn(0x3c+4, 0x7f, false);
                            s->noteOn(0x3c+7, 0x7f, false);
                        }
                    }
                MIOS32_IRQ_Enable();
            } break;
            }
        }
        return;
    }

    if( midi_package.event == ProgramChange ) {
        // TODO: check port and channel for program change
        int cv = 0;
        for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s), ++cv)
            bankLoad(cv, s->mbCvPatch.bankNum, midi_package.evnt1);
    } else {
        for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s))
            s->midiReceive(port, midi_package);
    }
}


/////////////////////////////////////////////////////////////////////////////
// sends a NRPN dump for selected CV channels
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiSendNRPNDump(mios32_midi_port_t port, u16 cvChannels, bool seqOnly)
{
    // ensure that we are starting with MSB/LSB address
    lastSentNrpnAddressMsb = 0xff;
    lastSentNrpnAddressLsb = 0xff;

    const int parBegin = seqOnly ? 0x0c0 : 0x000;
    const int parEnd = seqOnly ? 0x0ff : 0x3ff;

    int cv = 0;
    for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s), ++cv) {
        if( cvChannels & (1 << cv) ) {
            u16 par;
            for(par=parBegin; par<=parEnd; ++par) {
                u16 value;
                if( s->getNRPN(par, &value) ) {
                    u16 nrpnNumber = (cv << 10) | par;
                    midiSendNRPN(port, nrpnNumber, value);
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// sends a NRPN value (with bandwidth optimization)
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiSendNRPN(mios32_midi_port_t port, u16 nrpnNumber, u16 value)
{
    if( (port & 0xf0) == OSC0 ) {
        OSC_CLIENT_SendNRPNEvent(port & 0xf, Chn1, nrpnNumber, value);
    } else {
        u8 nrpnNumberMsb = (nrpnNumber >> 7) & 0x7f;
        u8 nrpnNumberLsb = (nrpnNumber >> 0) & 0x7f;
        u8 nrpnValueMsb = (value >> 7) & 0x7f;
        u8 nrpnValueLsb = (value >> 0) & 0x7f;

        if( nrpnNumberMsb != lastSentNrpnAddressMsb ) {
            lastSentNrpnAddressMsb = nrpnNumberMsb;
            MIOS32_MIDI_SendCC(port, Chn1, 0x63, nrpnNumberMsb);
        }

        if( nrpnNumberLsb != lastSentNrpnAddressLsb ) {
            lastSentNrpnAddressLsb = nrpnNumberLsb;
            MIOS32_MIDI_SendCC(port, Chn1, 0x62, nrpnNumberLsb);
        }

        // should we optimize this as well?
        // would work with Lemur, but also with other controllers?
        MIOS32_MIDI_SendCC(port, Chn1, 0x06, nrpnValueMsb);
        MIOS32_MIDI_SendCC(port, Chn1, 0x26, nrpnValueLsb);                    
    }
}


/////////////////////////////////////////////////////////////////////////////
// called on incoming SysEx bytes
// should return 1 if SysEx data has been taken (and should not be forwarded
// to midiReceive)
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::midiReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
    s32 status = 0;

    //status |= mbCvSysEx.parse(port, midi_in);

    return status;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a realtime event to service MbCvClock
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in)
{
    mbCvClock.midiReceiveRealTimeEvent(port, midi_in);
}


/////////////////////////////////////////////////////////////////////////////
// called on MIDI timeout
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiTimeOut(mios32_midi_port_t port)
{
    //mbCvSysEx.timeOut(port);
}


/////////////////////////////////////////////////////////////////////////////
// Forwards to clock generator
/////////////////////////////////////////////////////////////////////////////

void MbCvEnvironment::bpmSet(float bpm)
{
    mbCvClock.bpmSet(bpm);
}

float MbCvEnvironment::bpmGet(void)
{
    return mbCvClock.bpmGet();
}

void MbCvEnvironment::bpmRestart(void)
{
    mbCvClock.bpmRestart();
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to take over a received patch
// returns false if CV not available
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexSetPatch(u8 cv, cv_patch_t *p, bool toBank, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return false;

    if( toBank ) {
        bankSave(cv, bank, patch);
    } else {
        // forward to selected CV
        return mbCv[cv].sysexSetPatch(p);
    }
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to return the current patch
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexGetPatch(u8 cv, cv_patch_t *p, bool fromBank, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return false;

    if( fromBank )
        bankLoad(cv, bank, patch);

    return mbCv[cv].sysexGetPatch(p);
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to set a dedicated SysEx parameter
// returns false if selected CV or parameter not available
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexSetParameter(u8 cv, u16 addr, u8 data)
{
    if( cv >= mbCv.size )
        return false;

    // forward to selected CV (no [cv] range checking required, this is done by array template)
    return mbCv[cv].sysexSetParameter(addr, data);
}
