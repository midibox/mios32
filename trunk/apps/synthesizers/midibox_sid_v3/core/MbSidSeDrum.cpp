/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidSeDrum.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeDrum::MbSidSeDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeDrum::~MbSidSeDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr)
{
    // assign SID registers to voices
    MbSidVoiceDrum *v = mbSidVoiceDrum.first();
    for(int voice=0; voice < mbSidVoiceDrum.size; ++voice, ++v) {
        u8 physVoice = voice % 3;
        sid_voice_t *physSidVoice;
        if( voice >= 3 )
            physSidVoice = (sid_voice_t *)&sidRegRPtr->v[physVoice];
        else
            physSidVoice = (sid_voice_t *)&sidRegLPtr->v[physVoice];

        v->init(voice, physVoice, physSidVoice);
    }

    // assign SID registers to filters
    for(int filter=0; filter<mbSidFilter.size; ++filter)
        mbSidFilter[filter].init(filter ? sidRegRPtr : sidRegLPtr);

    // initialize clock and patch pointer
    mbSidClockPtr = _mbSidClockPtr;
    mbSidPatchPtr = _mbSidPatchPtr;
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the structures of a SID sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::initPatch(bool patchOnly)
{
    if( !patchOnly ) {
        for(int voice=0; voice<mbSidVoiceDrum.size; ++voice)
            mbSidVoiceDrum[voice].init();

        for(int wt=0; wt<mbSidWtDrum.size; ++wt)
            mbSidWtDrum[wt].init();

        for(int filter=0; filter<mbSidFilter.size; ++filter)
            mbSidFilter[filter].init();

        mbSidSeqDrum.init();

        // clear voice queue
        voiceQueue.init(&mbSidPatchPtr->body);
    }

    for(int drum=0; drum<mbSidDrum.size; ++drum)
        mbSidDrum[drum].init();

    // iterate over patch to transfer all SysEx parameters into SE objects
    for(int addr=0x10; addr<0x100; ++addr) // reduced range... loop over patch name and WT memory not required
        sysexSetParameter(addr, mbSidPatchPtr->body.ALL[addr]);

    mbSidSeqDrum.seqPatternMemory = &mbSidPatchPtr->body.D.seq_memory[0];
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeDrum::tick(const u8 &updateSpeedFactor)
{
    // Clock
    if( mbSidClockPtr->eventStart ) {
        mbSidSeqDrum.seqRestartReq = 1;
    }

    if( mbSidClockPtr->eventClock ) {
        // clock sequencer
        mbSidSeqDrum.seqClockReq = 1;
    }

    // Sequencer
    mbSidSeqDrum.tick(this);

    // Voices
    MbSidVoiceDrum *v = mbSidVoiceDrum.first();
    MbSidWtDrum *w = mbSidWtDrum.first();
    for(int voice=0; voice < mbSidVoiceDrum.size; ++voice, ++v, ++w)
        v->tick(updateSpeedFactor, this, w);

    // Filters and Volume
    MbSidFilter *f = mbSidFilter.first();
    for(int filter=0; filter<mbSidFilter.size; ++filter, ++f)
        f->tick(updateSpeedFactor);

    // currently no detection if SIDs have to be updated
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::triggerNoteOn(MbSidVoice *v, u8 no_wt)
{
    // restart gate
    v->voiceNoteRestartReq = 1;

    // set clear delay counter to control the gatelength
    v->voiceClrDelayCtr = 1;

    // restart assigned WT sequencer
    mbSidWtDrum[v->voiceNum].restartReq = 1;
}


void MbSidSeDrum::triggerNoteOff(MbSidVoice *v, u8 no_wt)
{
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    MbSidMidiVoice *mv = mbSidVoiceDrum[0].midiVoicePtr;

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    // check if MIDI channel and splitzone matches
    if( mv->isAssigned(chn, note) ) {
        if( velocity ) { // Note On
            // copy velocity into mod matrix source
            knobSet(SID_KNOB_VELOCITY, velocity << 1);

            noteOn(0, note, velocity, false);
        } else { // Note Off
            noteOff(0, note, false);
        }
    }

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    if( mbSidVoiceDrum[0].midiVoicePtr->isAssigned(chn) ) {
        switch( ccNumber ) {
        case  1: knobSet(SID_KNOB_1, value << 1); break;
        case 16: knobSet(SID_KNOB_2, value << 1); break;
        case 17: knobSet(SID_KNOB_3, value << 1); break;
        case 18: knobSet(SID_KNOB_4, value << 1); break;
        case 19: knobSet(SID_KNOB_5, value << 1); break;
        case 123: if( value == 0 ) noteAllOff(0, false); break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    u16 pitchbendValue8bit = pitchbendValue14bit >> 6;

    MbSidMidiVoice *mv = mbSidVoiceDrum[0].midiVoicePtr;
    for(int midiVoice=0; midiVoice<2; ++midiVoice, ++mv)
        if( mv->isAssigned(chn) ) {
            mv->midivoicePitchbender = pitchbendValue8bit;
            knobSet(SID_KNOB_PITCHBENDER, pitchbendValue8bit);
        }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbSidVoiceDrum[0].midiVoicePtr->isAssigned(chn) ) {
        knobSet(SID_KNOB_AFTERTOUCH, value << 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack)
{
    if( instrument >= 1 )
        return;

    MbSidMidiVoice *mv = (MbSidMidiVoice *)mbSidVoiceDrum[0].midiVoicePtr;

    // branch depending on normal/seq mode
    if( !bypassNotestack && mbSidSeqDrum.seqEnabled ) {

        // push note into stack
        mv->notestackPush(note, velocity);

        // reset sequencer if voice was not active before
        // (only done in Clock Master mode)
        if( !mbSidClockPtr->clockSlaveMode ) {
            if( !mbSidSeqDrum.seqRunning )
                mbSidSeqDrum.seqRestartReq = 1;
        }

        // change sequence
        seqChangePattern(instrument, note);

        // enable sequencer
        mbSidSeqDrum.seqRunning = 1;
    } else {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // add MIDI voice based transpose value
        int drum = note + (s32)mv->midivoiceTranspose - 64;

        // octave-wise saturation (only positive direction required)
        while( drum < 0 )
            note += 12;

        // normalize note number to 24 (2 octaves)
        drum %= 24;

        // ignore drums >= 16
        if( drum >= mbSidDrum.size )
            return;

        // get pointer to instrument
        MbSidDrum *d = &mbSidDrum[drum];

        // number of voices depends on mono/stereo mode (TODO: once ensemble available...)
        u8 numVoices = mbSidVoiceDrum.size;

        // check if drum instrument already played, in this case use the old voice
        MbSidVoiceDrum *v = mbSidVoiceDrum.first();
        int voice;
        for(voice=0; voice < mbSidVoiceDrum.size; ++voice, ++v)
            if( v->voiceAssignedInstrument == drum )
                break;

        bool voiceFound = 0;
        if( voice < numVoices ) {
            // check if voice assignment still valid (could have been changed meanwhile)
            switch( d->drumVoiceAssignment ) {
            case 0: voiceFound = 1; break; // All
            case 1: if( voice < 3 ) voiceFound = 1; break; // Left Only
            case 2: if( voice >= 3 ) voiceFound = 1; break; // Right Only
            default: if( voice == (d->drumVoiceAssignment-3) ) voiceFound = 1; break; // direct assignment
            }
        }

        // get new voice if required
        if( !voiceFound )
            voice = voiceQueue.get(drum, d->drumVoiceAssignment, numVoices);

#if 0
        DEBUG_MSG("[MbSidSeDrum] drum %d takes voice %d\n", drum, voice);
#endif

        // get pointer to voice
        v = &mbSidVoiceDrum[voice];

        // save instrument number
        v->voiceAssignedInstrument = drum;

        // set velocity assignment (if defined)
        // IMPORTANT TO DO IT BEFORE DRUM INITIALISATION!
        u8 sidlr = (voice >= 3) ? 2 : 1;
        bool scaleFrom16bit = true;
        parSet(d->drumVelocityAssignment, velocity << 9, sidlr, drum, scaleFrom16bit);

        // copy drum parameters into voice
        v->init(d, drum, velocity);

        // activate voice and request gate
        v->voiceActive = 1;
        triggerNoteOn(v, 0);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Disable a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::noteOff(u8 instrument, u8 note, bool bypassNotestack)
{
    if( instrument >= 1 )
        return;

    MbSidMidiVoice *mv = (MbSidMidiVoice *)mbSidVoiceDrum[0].midiVoicePtr;

    // branch depending on normal/seq mode
    if( !bypassNotestack && mbSidSeqDrum.seqEnabled ) {
        // pop note from stack
        if( mv->notestackPop(note) > 0 ) {
            // change sequence if there is still a note in stack
            if( mv->midivoiceNotestack.len )
                seqChangePattern(instrument, mv->midivoiceNotestack.note_items[0].note);

            // disable voice active flag if voice plays invalid sequence (seq off)
            // only used in master mode
            if( !mbSidClockPtr->clockSlaveMode && mbSidSeqDrum.seqPatternNumber >= 8 )
                mbSidSeqDrum.seqRunning = 0;
        }
    } else {
        // no check required if note was really in stack...
        if( !bypassNotestack )
            mv->notestackPop(note);

        // add MIDI voice based transpose value
        int drum = note + (s32)mv->midivoiceTranspose - 64;

        // octave-wise saturation (only positive direction required)
        while( drum < 0 )
            note += 12;

        // normalize note number to 24 (2 octaves)
        drum %= 24;

        if( drum >= mbSidDrum.size )
            return;

        // iterate through all voices which are assigned to the current instrument
        MbSidVoice *v = mbSidVoiceDrum.first();
        for(int voice=0; voice < mbSidVoiceDrum.size; ++voice, ++v) {
            if( v->voiceAssignedInstrument == drum ) {
                // release voice
                voiceQueue.release(voice);
#if 0
                DEBUG_MSG("[MbSidSeDrum] drum %d releases voice %d\n", drum, voice);
#endif
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::noteAllOff(u8 instrument, bool bypassNotestack)
{
    if( instrument >= 1 )
        return;

    MbSidVoice *v = mbSidVoiceDrum.first();
    for(int voice=0; voice < mbSidVoiceDrum.size; ++voice, ++v) {
        // release voice
        voiceQueue.release(voice);

        // deactivate gate
        v->voiceActive = 0;
        v->voiceGateClrReq = 1;
        v->voiceGateSetReq = 0;
        triggerNoteOff(v, 0);
    }

    if( !bypassNotestack )
        mbSidVoiceDrum[0].midiVoicePtr->notestackReset();
}


/////////////////////////////////////////////////////////////////////////////
// Bassline Sequencer: change pattern on keypress
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::seqChangePattern(u8 instrument, u8 note)
{
    // add MIDI voice based transpose value
    int transposedNote = (s32)note + (s32)mbSidVoiceDrum[0].midiVoicePtr->midivoiceTranspose - 64;

    // octavewise saturation if note is negative
    while( transposedNote < 0 )
        transposedNote += 12;

    // determine pattern based on note number
    u8 patternNumber = transposedNote % 12;

    // if number >= 8: set to 8 (sequence off)
    if( patternNumber >= 8 )
        patternNumber = 8;

    // set new sequence
    mbSidSeqDrum.seqPatternNumber = patternNumber;

    // also in patch
    mbSidPatchPtr->body.D.seq_num = patternNumber;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::knobSet(u8 knob, u8 value)
{
    // store new value into patch
    mbsid_knob_t *k = (mbsid_knob_t *)&mbSidPatchPtr->body.knob[knob];
    k->value = value;

    // get scaled value between min/max boundaries
    s32 factor = ((k->max > k->min) ? (k->max - k->min) : (k->min - k->max)) + 1;
    s32 knobScaledValue = k->min + (value * factor); // 16bit

    u8 sidlr = 3; // always modify both SIDs
    u8 ins = 0; // instrument n/a
    bool scaleFrom16bit = true;

    // forward to parameter handler
    // interrupts should be disabled to ensure atomic access
    MIOS32_IRQ_Disable();
    parSet(k->assign1, knobScaledValue, sidlr, ins, scaleFrom16bit);
    parSet(k->assign2, knobScaledValue, sidlr, ins, scaleFrom16bit);
    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// returns the current knob value
/////////////////////////////////////////////////////////////////////////////
u8 MbSidSeDrum::knobGet(u8 knob)
{
    mbsid_knob_t *k = (mbsid_knob_t *)&mbSidPatchPtr->body.knob[knob];
    return k->value;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// value contains unscaled value (range depends on parameter)
// sidlr selects left/right SID channel (0..1)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
// scaleFrom16bit optionally allows to input 16bit values instead of unscaled value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit)
{
    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            if( scaleFrom16bit ) value >>= 9;
            if( sidlr & 1 ) mbSidFilter[0].filterVolume = value;
            if( sidlr & 2 ) mbSidFilter[1].filterVolume = value;
            break;

        case 0x03: // OSC Detune
            if( scaleFrom16bit ) value >>= 8;
            sysexSetParameter(0x51, value);
            break;

        case 0x04: // Filter CutOff
            if( scaleFrom16bit ) value >>= 4;
            if( sidlr & 1 ) mbSidFilter[0].filterCutoff = value;
            if( sidlr & 2 ) mbSidFilter[1].filterCutoff = value;
            break;

        case 0x05: // Filter Resonance
            if( scaleFrom16bit ) value >>= 8;
            if( sidlr & 1 ) mbSidFilter[0].filterResonance = value;
            if( sidlr & 2 ) mbSidFilter[1].filterResonance = value;
            break;

        case 0x06: // Filter Channels
            if( scaleFrom16bit ) value >>= 12;
            if( sidlr & 1 ) mbSidFilter[0].filterChannels = value;
            if( sidlr & 2 ) mbSidFilter[1].filterChannels = value;
            break;

        case 0x07: // Filter Mode
            if( scaleFrom16bit ) value >>= 12;
            if( sidlr & 1 ) mbSidFilter[0].filterMode = value;
            if( sidlr & 2 ) mbSidFilter[1].filterMode = value;
            break;
        }
    } else if( par <= 0x0f ) { // Knobs
        if( scaleFrom16bit ) value >>= 8;
        knobSet(par & 0x07, value);
    } else if( par <= 0x1f ) {
        if( par <= 0x17 ) {
            // External Parameters (CV): TODO
        } else {
            // External Switches: TODO
        }
    } else if( par <= 0xbf ) {
        u8 drum = par & 0x0f;
        u8 drumAddr = (par-0x30) >> 4;
        MbSidDrum *d = &mbSidDrum[drum];

        switch( drumAddr ) {
        case 0:
            if( scaleFrom16bit ) value >>= 8;
            d->drumModel = value;
            break;

        case 1:
            if( scaleFrom16bit ) value >>= 12;
            d->drumAttackDecay.ATTACK = value;
            break;

        case 2:
            if( scaleFrom16bit ) value >>= 12;
            d->drumAttackDecay.DECAY = value;
            break;

        case 3:
            if( scaleFrom16bit ) value >>= 12;
            d->drumSustainRelease.SUSTAIN = value;
            break;

        case 4:
            if( scaleFrom16bit ) value >>= 12;
            d->drumSustainRelease.RELEASE = value;
            break;

        case 5:
            if( scaleFrom16bit ) value >>= 8;
            d->drumTuneModifier = (s32)value - 0x80;
            break;

        case 6:
            if( scaleFrom16bit ) value >>= 8;
            d->drumGatelengthModifier = (s32)value - 0x80;
            break;

        case 7:
            if( scaleFrom16bit ) value >>= 8;
            d->drumSpeedModifier = (s32)value - 0x80;
            break;

        case 8:
            if( scaleFrom16bit ) value >>= 8;
            d->drumParameter = (s32)value - 0x80;
            break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Returns a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// sidlr selects left/right SID channel (if 0, 1 or 3, SIDL register will be returned, with 2 the SIDR register)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
// scaleTo16bit optionally allows to return 16bit values instead of unscaled value
/////////////////////////////////////////////////////////////////////////////
u16 MbSidSeDrum::parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit)
{
    u16 value = 0;

    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            value = mbSidFilter[ins].filterVolume;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0x04: // Filter CutOff
            value = mbSidFilter[ins].filterCutoff;
            if( scaleTo16bit ) value <<= 4;
            break;

        case 0x05: // Filter Resonance
            value = mbSidFilter[ins].filterResonance;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x06: // Filter Channels
            value = mbSidFilter[ins].filterChannels;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x07: // Filter Mode
            value = mbSidFilter[ins].filterMode;
            if( scaleTo16bit ) value <<= 12;
            break;
        }
    } else if( par <= 0x0f ) { // Knobs
        knobGet(par & 0x07);
        if( scaleTo16bit ) value <<= 8;
    } else if( par <= 0x1f ) {
        if( par <= 0x17 ) {
            // External Parameters (CV): TODO
        } else {
            // External Switches: TODO
        }
    } else if( par <= 0xbf ) {
        u8 drum = par & 0x0f;
        u8 drumAddr = par >> 4;
        MbSidDrum *d = &mbSidDrum[drum];

        switch( drumAddr ) {
        case 0:
            value = d->drumModel;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 1:
            value = d->drumAttackDecay.ATTACK;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 2:
            value = d->drumAttackDecay.DECAY;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 3:
            value = d->drumSustainRelease.SUSTAIN;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 4:
            value = d->drumSustainRelease.RELEASE;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 5:
            value = d->drumTuneModifier + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 6:
            value = d->drumGatelengthModifier + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 7:
            value = d->drumSpeedModifier + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 8:
            value = d->drumParameter + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;
        }
    }

    return value;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from NRPN value
// sid selects the SID
// addr_[lsb|msb] selects parameter
// data_[lsb|msb] contains value
// value contains 8bit scaled value (0..255)
// sidlr selects left/right SID channel
// ins selects multi/drum instrument
/////////////////////////////////////////////////////////////////////////////
void MbSidSeDrum::parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins)
{
    return;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to set a dedicated SysEx parameter
// (forwarded from MbSidEnvironment and MbSid)
// returns false on invalid access
//
// This function is also used by init() to map the patch to sound engine 
// objects so that we avoid redundancies
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeDrum::sysexSetParameter(u16 addr, u8 data)
{
    if( addr <= 0x0f ) { // Patch Name
        return true; // no update required
    } else if( addr <= 0x012 ) { // global section
        if( addr == 0x012 ) { // opt_flags
            sid_se_opt_flags_t optFlags;
            optFlags.ALL = data;
            for(MbSidVoiceDrum *v = mbSidVoiceDrum.first(); v != NULL ; v=mbSidVoiceDrum.next(v)) {
                v->voiceAdsrBugWorkaround = optFlags.ABW;
            }
        }
        return true;
    } else if( addr <= 0x04f ) { // Custom Switches, Knobs and External Parameters (CV)
        return true; // already handled by MbSid before this function has been called
    } else if( addr <= 0x053 ) { // Engine specific parameters
        switch( addr ) {
        case 0x50:
            mbSidSeqDrum.seqClockDivider = data & 0x3f;
            mbSidSeqDrum.seqEnabled = (data >> 6) & 1;
            mbSidSeqDrum.seqSynchToMeasure = (data >> 7);
            break;

		case 0x51:
			mbSidSeqDrum.seqPatternNumber = data & 0x0f;
			break;
				
			case 0x52:
            for(MbSidFilter *f = mbSidFilter.first(); f != NULL ; f=mbSidFilter.next(f))
                f->filterVolume = data;

        case 0x53:
            mbSidSeqDrum.seqPatternLength = data & 0x0f;
            break;
        }
        return true;
    } else if( addr <= 0x05f ) { // filters
        u8 filter = (addr >= 0x05a) ? 1 : 0;
        MbSidFilter *f = &mbSidFilter[filter];

        switch( (addr - 0x54) % 6 ) {
        case 0x0: f->filterChannels = data & 0x0f; f->filterMode = data >> 4; break;
        case 0x1: f->filterCutoff = (f->filterCutoff & 0xff00) | data; break;
        case 0x2: f->filterCutoff = (f->filterCutoff & 0x00ff) | ((data & 0x0f) << 8); break;
        case 0x3: f->filterResonance = data; break;
        case 0x4: f->filterKeytrack = data; break;
        }
        return true;
    } else if( addr <= 0x0ff ) { // drum instruments
        u8 drum = (addr - 0x60) / 10;
        u8 drumAddr = (addr - 0x60) % 10;
        MbSidDrum *d = &mbSidDrum[drum];

        switch( drumAddr ) {
        case 0: d->drumVoiceAssignment = data >> 4; break;
        case 1: d->drumModel = data; break;
        case 2: d->drumAttackDecay.ALL = data; break;
        case 3: d->drumSustainRelease.ALL = data; break;
        case 4: d->drumTuneModifier = (s32)data - 0x80; break;
        case 5: d->drumGatelengthModifier = (s32)data - 0x80; break;
        case 6: d->drumSpeedModifier = (s32)data - 0x80; break;
        case 7: d->drumParameter = (s32)data - 0x80; break;
        case 8: d->drumVelocityAssignment = data; break;
        }

        return true;
    } else if( addr <= 0x1ff ) {
        return true; // no update required
    }

    // unsupported sysex address
    return false;
}
