/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDI processing routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "includes.h"
#include "MidiProcessing.h"

#include <mios32.h>


// temporary!!!
AudioDeviceManager *audioDeviceManagerForMidiTmp;


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MidiProcessing::MidiProcessing()
{
    audioDeviceManagerForMidiTmp = &audioDeviceManager;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MidiProcessing::~MidiProcessing()
{
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Input Events
/////////////////////////////////////////////////////////////////////////////

// inherited from MidiInputCallback
void MidiProcessing::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    processNextMidiEvent(message);
}

// inherited from MidiInputCallback
void MidiProcessing::processNextMidiEvent(const MidiMessage& message)
{
    int size = message.getRawDataSize();
    u8 *data = message.getRawData();

    // TODO: integrate this properly into $MIOS32_PATH/mios32/juce/ !!!
    if( size >= 1 && data[0] == 0xf0 ) {
        for(int i=0; i<size; ++i)
            mbSidEnvironment->midiReceiveSysEx(DEFAULT, data[i]);
    } else {
        sendMidiEvent(data[0],
                      (size >= 2) ? data[1] : 0x00,
                      (size >= 3) ? data[2] : 0x00);
    }
}


// inherited from MidiKeyboardStateListener
void MidiProcessing::processNextMidiBuffer (MidiBuffer& buffer,
                                            const int startSample,
                                            const int numSamples,
                                            const bool injectIndirectEvents)
{
    MidiBuffer::Iterator i (buffer);
    MidiMessage message (0xf4, 0.0);
    int time;
  
#if 0
    const ScopedLock sl (lock);
#endif

    while (i.getNextEvent (message, time))
        processNextMidiEvent (message);

#if 0
    if (injectIndirectEvents)
        {
            MidiBuffer::Iterator i2 (eventsToAdd);
            const int firstEventToAdd = eventsToAdd.getFirstEventTime();
            const double scaleFactor = numSamples / (double) (eventsToAdd.getLastEventTime() + 1 - firstEventToAdd);
    
            while (i2.getNextEvent (message, time))
                {
                    const int pos = jlimit (0, numSamples - 1, roundDoubleToInt ((time - firstEventToAdd) * scaleFactor));
                    buffer.addEvent (message, startSample + pos);
                }
        }
  
    eventsToAdd.clear();
#endif
}


// inherited from MidiKeyboardStateListener
void MidiProcessing::handleNoteOn (MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    unsigned char evnt0 = 0x90 + ((midiChannel > 15) ? 15 : midiChannel);
    unsigned char evnt1 = (midiNoteNumber > 127) ? 127 : midiNoteNumber;
    unsigned char evnt2 = (velocity >= 1.0) ? 127 : (velocity*127);
    sendMidiEvent(evnt0, evnt1, evnt2);
}

// inherited from MidiKeyboardStateListener
void MidiProcessing::handleNoteOff (MidiKeyboardState *source, int midiChannel, int midiNoteNumber)
{
    unsigned char evnt0 = 0x90 + ((midiChannel > 15) ? 15 : midiChannel);
    unsigned char evnt1 = (midiNoteNumber > 127) ? 127 : midiNoteNumber;
    unsigned char evnt2 = 0x00;
    sendMidiEvent(evnt0, evnt1, evnt2);
}


/////////////////////////////////////////////////////////////////////////////
// sends a MIDI event to MIOS32
/////////////////////////////////////////////////////////////////////////////
void MidiProcessing::sendMidiEvent(unsigned char evnt0, unsigned char evnt1, unsigned char evnt2)
{
    mios32_midi_package_t p;
    p.type = evnt0 >> 4;
    p.evnt0 = evnt0;
    p.evnt1 = evnt1;
    p.evnt2 = evnt2;

    // temporary ignore channel
    p.evnt0 &= 0xf0;

    mbSidEnvironment->midiReceive(DEFAULT, p);
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Output Events
/////////////////////////////////////////////////////////////////////////////

s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count)
{
    MidiMessage message(stream, count);
    MidiOutput *out = audioDeviceManagerForMidiTmp->getDefaultMidiOutput();

    if( out ) {
        out->sendMessageNow(message);
        return 0; // no error
    }

    fprintf(stderr, "MIOS32_MIDI_SendSysEx: No MIDI Output, %u bytes *not* sent\n", count);
    return -1; // error
}
