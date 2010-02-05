/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSid.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSid::MbSid()
{
    currentMbSidSePtr = &mbSidSeLead;
    prevEngine = SID_SE_LEAD;

    u8 sid = 0;
    sid_regs_t *sidRegLPtr = &sid_regs[2*sid+0];
    sid_regs_t *sidRegRPtr = &sid_regs[2*sid+1];

    init(sid, sidRegLPtr, sidRegRPtr, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSid::~MbSid()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the sound engines
/////////////////////////////////////////////////////////////////////////////
void MbSid::init(u8 _sidNum, sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr)
{
    mbSidSeLead.init(sidRegLPtr, sidRegRPtr, _mbSidClockPtr, &mbSidPatch);
    mbSidSeBassline.init(sidRegLPtr, sidRegRPtr, _mbSidClockPtr, &mbSidPatch);
    mbSidSeDrum.init(sidRegLPtr, sidRegRPtr, _mbSidClockPtr, &mbSidPatch);
    mbSidSeMulti.init(sidRegLPtr, sidRegRPtr, _mbSidClockPtr, &mbSidPatch);

    for(int midiVoice=0; midiVoice<mbSidMidiVoice.size; ++midiVoice) {
        mbSidMidiVoice[midiVoice].init();
        mbSidSeLead.mbSidVoice[midiVoice].midiVoicePtr = &mbSidMidiVoice[midiVoice];
        mbSidSeBassline.mbSidVoice[midiVoice].midiVoicePtr = &mbSidMidiVoice[(midiVoice >= 3) ? 1 : 0];
        mbSidSeDrum.mbSidVoiceDrum[midiVoice].midiVoicePtr = &mbSidMidiVoice[0];
        mbSidSeMulti.mbSidVoice[midiVoice].midiVoicePtr = &mbSidMidiVoice[0]; // will be dynamically assigned by MIDI handler
        mbSidSeMulti.midiVoicePtr = &mbSidMidiVoice[0]; // therefore a reference to the first voice is required in SE as well
    }

    updatePatch(true);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSid::tick(const u8 &updateSpeedFactor)
{
    return currentMbSidSePtr->tick(updateSpeedFactor);
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceive(mios32_midi_package_t midi_package)
{
#ifndef MIOS32_FAMILY_EMULATION
    switch( midi_package.event ) {
    case NoteOff:
        midiReceiveNote(midi_package.chn, midi_package.note, 0x00);
        break;

    case NoteOn:
        midiReceiveNote(midi_package.chn, midi_package.note, midi_package.velocity);
        break;

    case PolyPressure:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt2);
        break;

    case CC:
        midiReceiveCC(midi_package.chn, midi_package.cc_number, midi_package.value);
        break;

    case PitchBend: {
        u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
        midiReceivePitchBend(midi_package.chn, pitchbend_value_14bit);
    } break;

    case Aftertouch:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt1);
        break;
  }
#else
	MIOS32_LCD_CursorSet(19,1);
    switch( midi_package.event ) {
    case NoteOff:
        midiReceiveNote(midi_package.chn, midi_package.note, 0x00);
		MIOS32_LCD_PrintString(" ");
        break;

    case NoteOn:
        midiReceiveNote(midi_package.chn, midi_package.note, midi_package.velocity);
		if (midi_package.velocity==0)
			MIOS32_LCD_PrintString(" ");
		else
			MIOS32_LCD_PrintString("*");
        break;

    case PolyPressure:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt2);
        break;

    case CC:
        midiReceiveCC(midi_package.chn, midi_package.cc_number, midi_package.value);
        break;

    case PitchBend: {
        u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
        midiReceivePitchBend(midi_package.chn, pitchbend_value_14bit);
    } break;

    case Aftertouch:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt1);
        break;
  }

#endif
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    return currentMbSidSePtr->midiReceiveNote(chn, note, velocity);
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    return currentMbSidSePtr->midiReceiveCC(chn, ccNumber, value);
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    return currentMbSidSePtr->midiReceivePitchBend(chn, pitchbendValue14bit);
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceiveAftertouch(u8 chn, u8 value)
{
    return currentMbSidSePtr->midiReceiveAftertouch(chn, value);
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenver the patch has been changed
/////////////////////////////////////////////////////////////////////////////
void MbSid::updatePatch(bool forceEngineInit)
{
    // disable interrupts to ensure atomic change
    MIOS32_IRQ_Disable();

    // force initialisation if engine has changed
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;
    if( engine != prevEngine ) {
        prevEngine = engine;
        forceEngineInit = true;

        switch( engine ) {
        case SID_SE_BASSLINE:
            currentMbSidSePtr = &mbSidSeBassline;

            // temporary code to configure MIDI voices - will be part of ensemble later
            mbSidMidiVoice[0].init();
            mbSidMidiVoice[0].midivoiceChannel = 0;
            mbSidMidiVoice[0].midivoiceSplitLower = 0x00;
            mbSidMidiVoice[0].midivoiceSplitUpper = 0x3b;

            mbSidMidiVoice[1].init();
            mbSidMidiVoice[1].midivoiceChannel = 0;
            mbSidMidiVoice[1].midivoiceSplitLower = 0x3c;
            mbSidMidiVoice[1].midivoiceSplitUpper = 0x7f;

            mbSidMidiVoice[2].init();
            mbSidMidiVoice[2].midivoiceChannel = 1;
            mbSidMidiVoice[2].midivoiceSplitLower = 0x00;
            mbSidMidiVoice[2].midivoiceSplitUpper = 0x3b;

            mbSidMidiVoice[3].init();
            mbSidMidiVoice[3].midivoiceChannel = 1;
            mbSidMidiVoice[3].midivoiceSplitLower = 0x3c;
            mbSidMidiVoice[3].midivoiceSplitUpper = 0x7f;
            break;

        case SID_SE_DRUM:
            currentMbSidSePtr = &mbSidSeDrum;
         
            // temporary code to configure MIDI voices - will be part of ensemble later
            mbSidMidiVoice[0].init();
			mbSidMidiVoice[1].init();
            mbSidMidiVoice[2].init();
            mbSidMidiVoice[3].init();
            mbSidMidiVoice[4].init();
            mbSidMidiVoice[5].init();
            break;

        case SID_SE_MULTI:
            currentMbSidSePtr = &mbSidSeMulti;

            // temporary code to configure MIDI voices - will be part of ensemble later
            mbSidMidiVoice[0].init();
            mbSidMidiVoice[0].midivoiceChannel = 0;
            mbSidMidiVoice[1].init();
            mbSidMidiVoice[1].midivoiceChannel = 1;
            mbSidMidiVoice[2].init();
            mbSidMidiVoice[2].midivoiceChannel = 2;
            mbSidMidiVoice[3].init();
            mbSidMidiVoice[3].midivoiceChannel = 3;
            mbSidMidiVoice[4].init();
            mbSidMidiVoice[4].midivoiceChannel = 4;
            mbSidMidiVoice[5].init();
            mbSidMidiVoice[5].midivoiceChannel = 5;
            break;

        default: // case SID_SE_LEAD
            currentMbSidSePtr = &mbSidSeLead;
            mbSidMidiVoice[0].init();
            mbSidMidiVoice[1].init();
            mbSidMidiVoice[2].init();
            mbSidMidiVoice[3].init();
            mbSidMidiVoice[4].init();
            mbSidMidiVoice[5].init();
        }
    }

    // transfer patch data to sound elements
    bool patchOnly = !forceEngineInit;
    currentMbSidSePtr->initPatch(patchOnly);

    // enable interrupts again
    MIOS32_IRQ_Enable();

#ifdef MIOS32_FAMILY_EMULATION
	MIOS32_LCD_CursorSet(0,0);

    char patchName[17];
    mbSidPatch.nameGet(patchName);
	MIOS32_LCD_PrintFormattedString("P:%s", patchName	);

#endif
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to take over a received patch
// returns false if initialisation failed
/////////////////////////////////////////////////////////////////////////////
bool MbSid::sysexSetPatch(sid_patch_t *p)
{
    mbSidPatch.copyToPatch(p);
    updatePatch(false);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to return the current patch
// returns false if patch not available
/////////////////////////////////////////////////////////////////////////////
bool MbSid::sysexGetPatch(sid_patch_t *p)
{
    // TODO: bank read
    mbSidPatch.copyFromPatch(p);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to set a dedicated SysEx parameter
// returns false on invalid access
/////////////////////////////////////////////////////////////////////////////
bool MbSid::sysexSetParameter(u16 addr, u8 data)
{
    // exit if address range not valid (just to ensure)
    if( addr >= sizeof(sid_patch_t) )
        return false;

    // change value in patch
    mbSidPatch.body.ALL[addr] = data;

    // forward to current engine for additional updates
    return currentMbSidSePtr->sysexSetParameter(addr, data);
}
