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

#include <mbcv_patch.h>

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
    // initialize global timestamp
    timestamp = 0;

    // initialize NRPN address/values
    for(int i=0; i<16; ++i) {
        nrpnAddress[i] = 0;
        nrpnValue[i] = 0;
    }
    lastSentNrpnAddressMsb = 0xff;
    lastSentNrpnAddressLsb = 0xff;

    // for delayed ack messages (only used if lastNrpnMidiPort != 0)
    lastNrpnMidiPort = DEFAULT;
    lastNrpnCvChannels = (1 << 0);

    // initialize structures of each CV channel
    MbCv *s = mbCv.first();
    for(int cv=0; cv < mbCv.size; ++cv, ++s) {
        s->init(cv, &mbCvClock);
    }

    // sets the default speed factor
    updateSpeedFactorSet(2);

    // restart clock generator
    bpmRestart();

    // default content of copy buffer
    channelCopy(0, copyBuffer);

    // scope assignments
    {
        MbCvScope *scope = mbCvScope.first();
        for(int i=0; i < mbCvScope.size; ++i, ++scope) {
            scope->init(i);
        }

        scopeUpdateCtr = 0;
    }
    updateScopeParameters();
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

    // global timestamp
    ++timestamp;

    // Tempo Clock
    mbCvClock.tick();

    // Synchronized patch change request?
    if( mbCvClock.eventClock && mbCvPatch.reqChange ) {
        u32 tick = mbCvClock.clkTickCtr;
        u32 atStep = (u32)mbCvPatch.synchedChangeStep + 1;

#if 0
        if( (tick % 24) == 0 && ((tick/24) % atStep) == 0 )
            mbCvPatch.reqChangeAck = true;
#else
        // change 8 ticks before step change
        if( (tick % 24) == 16 && ((tick/24) % atStep) == (atStep-1) )
            mbCvPatch.reqChangeAck = true;        
#endif
    }

    // Engines
    for(MbCv *s = mbCv.first(); s != NULL ; s=mbCv.next(s)) {
        if( s->tick(updateSpeedFactor) )
            updateRequired = true;
    }

    // Transfer values to scope
    // we do this as a second step, so that it will be possible to map values
    {
        MbCvScope *scope = mbCvScope.first();
        for(int i=0; i < mbCvScope.size; ++i, ++scope) {
            u8 cvNumber = scope->getAssignedFunction();
            if( cvNumber > 0 && cvNumber <= cvOut.size ) {
                s16 out = cvOut[cvNumber-1] - 0x8000;
                scope->addValue(timestamp, out);
            } else {
                scope->addValue(timestamp, 0);
            }
        }
    }

    if( updateRequired ) {
        // map engine parameters to CV outputs
        // we do this as a second step, so that it will be possible to map values
        // of a single engine to different channels in future
        cvGates = 0;
        MbCv *s = mbCv.first();
        u16 *out = cvOut.first();
        u16 *outMeter = cvOutMeter.first();
        for(int cv=0; cv < cvOut.size; ++cv, ++s, ++out, ++outMeter) {
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

            if( *out > *outMeter ) {
                *outMeter = *out;
            }
        }
    }

    return updateRequired;
}


/////////////////////////////////////////////////////////////////////////////
// Should be called each mS from a thread, e.g. for synchronized patch changes
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::tick_1mS(void)
{
    // synchronized patch change?
    if( mbCvPatch.reqChangeAck )
        bankLoad(mbCvPatch.nextBankNum, mbCvPatch.nextPatchNum, true);

    // handle meters
    {
        u16 *out = cvOut.first();
        u16 *outMeter = cvOutMeter.first();
        for(int cv=0; cv < cvOut.size; ++cv, ++out, ++outMeter) {
            if( *outMeter > *out ) {
                MIOS32_IRQ_Disable(); // must be atomic
                s32 newValue = *outMeter - 1000;
                if( newValue < *out )
                    newValue = *out;
                *outMeter = newValue;
                MIOS32_IRQ_Enable();
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Should be called each mS from a low-prio thread to update scope displays
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::tickScopes(void)
{
    // with each round we update another display
    if( ++scopeUpdateCtr >= mbCvScope.size )
        scopeUpdateCtr = 0;

    mbCvScope[scopeUpdateCtr].tick();
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever the scope mapping has been changed
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::updateScopeParameters(void)
{
    u32 scopeAssigned = 0;

    {
        MbCvScope *scope = mbCvScope.first();
        for(int i=0; i < mbCvScope.size; ++i, ++scope) {
            scope->setAssignedFunction(0);
        }
    }

    MbCv *s = mbCv.first();
    for(int cv=0; cv < cvOut.size; ++cv, ++s) {
        if( s->scopeSelect > 0 ) {
            u8 scopeNumber = s->scopeSelect - 1;
            if( scopeNumber < mbCvScope.size ) {
                if( !(scopeAssigned & (1 << scopeNumber)) ) {
                    scopeAssigned |= (1 << scopeNumber);

                    MbCvScope *scope = &mbCvScope[scopeNumber];
                    scope->setAssignedFunction(cv+1);
                    scope->setOversamplingFactor(s->scopeOversamplingFactor);
                    scope->setTriggerLevelPercent(s->scopeTriggerLevelPercent);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Write a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankSave(u8 bank, u8 patch)
{
#if 1
    DEBUG_MSG("[CV_BANK_PatchSave] write patch %c%03d\n", 'A'+bank, patch);
#endif

    // change to new bank/patch
    mbCvPatch.bankNum = bank;
    mbCvPatch.patchNum = patch;

    // file operation
    MBCV_PATCH_Store(bank, patch);

    // send confirmation (e.g. to Lemur)
    if( lastNrpnMidiPort ) {
        midiSendGlobalNRPNDump(lastNrpnMidiPort);
    }

    return -2; // not supported yet
}

/////////////////////////////////////////////////////////////////////////////
// Read a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankLoad(u8 bank, u8 patch, bool forceImmediateChange)
{
    if( bank >= CV_BANK_NUM )
        return -2; // invalid bank

    if( patch >= 128 )
        return -3; // invalid patch

#if 1
    DEBUG_MSG("[CV_BANK_PatchRead] read patch %c%03d\n", 'A'+bank, patch);
#endif

    if( mbCvPatch.synchedChange && !forceImmediateChange ) {
        // request change
        MIOS32_IRQ_Disable();
        mbCvPatch.reqChange = true;
        mbCvPatch.nextBankNum = bank;
        mbCvPatch.nextPatchNum = patch;
        MIOS32_IRQ_Enable();
    } else {
        // do immediate change
        mbCvPatch.bankNum = bank;
        mbCvPatch.patchNum = patch;

        // file operation
        MBCV_PATCH_Load(bank, patch);

#if 0
        // update patch structures
        MbCv *s = mbCv.first();
        for(int cv=0; cv < mbCv.size; ++cv, ++s) {
            s->updatePatch(false);
        }
#endif

        // send confirmation (e.g. to Lemur)
        if( lastNrpnMidiPort ) {
            midiSendNRPNDump(lastNrpnMidiPort, lastNrpnCvChannels, 0);
            midiSendGlobalNRPNDump(lastNrpnMidiPort);
        }

        // change done
        MIOS32_IRQ_Disable();
        mbCvPatch.reqChange = false;
        mbCvPatch.reqChangeAck = false;
        MIOS32_IRQ_Enable();
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns the name of a preset patch as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
s32 MbCvEnvironment::bankPatchNameGet(u8 bank, u8 patch, char *buffer)
{
    if( bank >= CV_BANK_NUM ) {
        sprintf(buffer, "<Invalid Bank %c>", 'A'+bank);
        return -1; // invalid bank
    }

    if( patch >= 128 ) {
        sprintf(buffer, "<WrongPatch %03d>", patch);
        return -2; // invalid patch
    }

#if 0
    for(i=0; i<16; ++i)
        buffer[i] = p->name[i] >= 0x20 ? p->name[i] : ' ';
    buffer[i] = 0;
#endif

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
            // remember this port for delayed ack messages
            lastNrpnMidiPort = port;

            switch( address % CV_PATCH_SIZE ) {
            case 0x000: { // Dump All: 0x3c00 <channels>
                lastNrpnCvChannels = value; // for delayed ack messages
                midiSendNRPNDump(port, value, 0);
                midiSendGlobalNRPNDump(port);
            } break;

            case 0x001: { // Dump Seq Only: 0x3c00 <channels>
                lastNrpnCvChannels = value; // for delayed ack messages
                midiSendNRPNDump(port, value, 1);
            } break;

            case 0x008: channelCopy(value, copyBuffer);  break;
            case 0x009: channelPaste(value, copyBuffer); midiSendNRPNDump(port, 1 << value, 0); break;
            case 0x00a: channelClear(value); midiSendNRPNDump(port, 1 << value, 0); break;

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

            case 0x018: {                                     // Tempo: 0x3c18 <tempo>
                mbCvClock.bpmSet((float)value);
            } break;

            case 0x019: {                                     // TempoMode: 0x3c19 <tempoMode>
                mbCvClock.clockModeSet((mbcv_clock_mode_t)value);                
            } break;

            case 0x01a: {                                     // Seq Start/Stop/Continue: 0x3c1a <1|0|2>
                mios32_midi_port_t dummyPort = (mios32_midi_port_t)0xff;

                if( value == 0 )
                    mbCvClock.midiReceiveRealTimeEvent(dummyPort, 0xfc); // stop
                else if( value == 1 )
                    mbCvClock.midiReceiveRealTimeEvent(dummyPort, 0xfa); // start
                else if( value == 2 )
                    mbCvClock.midiReceiveRealTimeEvent(dummyPort, 0xfb); // continue
            } break;

            case 0x020: {                                     // Patch load
                bankLoad(mbCvPatch.bankNum, value);
            } break;

            case 0x021: {                                     // Bank load
                bankLoad(value, mbCvPatch.patchNum);
            } break;

            case 0x022: {                                     // Patch and Bank load
                bankLoad(value >> 7, value & 0x7f);
            } break;

            case 0x023: {                                     // Patch and Bank save
                bankSave(value >> 7, value & 0x7f);
            } break;

            case 0x024: {                                     // Synched Change
                mbCvPatch.synchedChange = value;
            } break;

            case 0x025: {                                     // Synch Change Step
                mbCvPatch.synchedChangeStep = value;
            } break;
            }
        }
        return;
    }

    if( midi_package.event == ProgramChange ) {
        // TODO: check port and channel for program change
        bankLoad(mbCvPatch.bankNum, midi_package.evnt1);
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
// sends global parameters
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::midiSendGlobalNRPNDump(mios32_midi_port_t port)
{
    midiSendNRPN(port, 0x3c18, (u16)mbCvClock.bpmGet());
    midiSendNRPN(port, 0x3c19, (u16)mbCvClock.clockModeGet());

    midiSendNRPN(port, 0x3c20, mbCvPatch.patchNum);
    midiSendNRPN(port, 0x3c21, mbCvPatch.bankNum);
    //midiSendNRPN(port, 0x3c22, ((u16)mbCvPatch.bankNum << 7) | mbCvPatch.patchNum);
    midiSendNRPN(port, 0x3c24, mbCvPatch.synchedChange);
    midiSendNRPN(port, 0x3c25, mbCvPatch.synchedChangeStep);
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
// Copy/Paste/Clear operations
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvironment::channelCopy(u8 channel, u16* buffer)
{
    if( channel < mbCv.size ) {
        MbCv *s = &mbCv[channel];

        for(int par=0; par<CV_PATCH_SIZE; ++par) {
            u16 value = 0;
            s->getNRPN(par, &value);
            buffer[par] = value;
        }
    }
}

void MbCvEnvironment::channelPaste(u8 channel, u16* buffer)
{
    if( channel < mbCv.size ) {
        MbCv *s = &mbCv[channel];

        for(int par=0; par<CV_PATCH_SIZE; ++par) {
            s->setNRPN(par, buffer[par]);
        }
    }
}

void MbCvEnvironment::channelClear(u8 channel)
{
    if( channel < mbCv.size ) {
        mbCv[channel].updatePatch(false);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to take over a received patch
// returns false if CV not available
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexSetPatch(u8 cv, cv_patch_t *p, bool toBank, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return false;

#if 0
    if( toBank ) {
        bankSave(bank, patch);
    } else {
        // forward to selected CV
        return mbCv[cv].sysexSetPatch(p);
    }
#endif
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to return the current patch
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexGetPatch(u8 cv, cv_patch_t *p, bool fromBank, u8 bank, u8 patch)
{
    if( cv >= mbCv.size )
        return false;

#if 0
    if( fromBank )
        bankLoad(bank, patch);

    return mbCv[cv].sysexGetPatch(p);
#endif
    return false;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to set a dedicated SysEx parameter
// returns false if selected CV or parameter not available
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvironment::sysexSetParameter(u8 cv, u16 addr, u8 data)
{
    if( cv >= mbCv.size )
        return false;

#if 0
    // forward to selected CV (no [cv] range checking required, this is done by array template)
    return mbCv[cv].sysexSetParameter(addr, data);
#endif
    return false;
}
