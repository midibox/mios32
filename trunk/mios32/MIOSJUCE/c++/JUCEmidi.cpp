/*
    FreeRTOS Emu
*/


#include <juce.h>

#include <mios32.h>

#include <JUCEmidi.h>
#include <JUCEmidi.hpp>


/*
MIOS32_MIDI_DirectRxCallback_Init
MIOS32_MIDI_SendCC
MIOS32_MIDI_SendClock
MIOS32_MIDI_SendStart
MIOS32_MIDI_SendContinue
MIOS32_MIDI_SendStop
MIOS32_MIDI_SendPackage
*/


#define dummyreturn 0 // just something to keep the compiler quiet for now

// pointer to the midi output being used
MidiOutput *MIDIOut;

// extern this so that emulation can see the MIDI Device Manager globally
AudioDeviceManager* midiDeviceManager;

long JUCE_MIDI_Init(long mode)
{
    // Get the MIDI output from the device manager
    MIDIOut = midiDeviceManager->getDefaultMidiOutput();
    
    // if it's failed (like when the app has just started)
    // try to just get the system default
    if (MIDIOut == NULL)
    {
        // this makes a new port on linux... 
        // f*** knows what it does elsewhere. Should just choose the default
        MIDIOut = MidiOutput::openDevice(MidiOutput::getDefaultDeviceIndex());
    }

    return dummyreturn;
}


long JUCE_MIDI_DirectRxCallback_Init(void *callback_rx)
{
    return dummyreturn;
}



long JUCE_MIDI_SendPackage(int port, long package)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::MidiMessage(((package>>16)&0x0f),
                                                        ((package>> 8)&0x0f),
                                                        ((package>> 0)&0x0f), 0));
    }
    return dummyreturn;
}


long JUCE_MIDI_SendNoteOn(int port, char chn, char note, char vel)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::noteOn(chn, note, (uint8) vel));
    }
    return dummyreturn;
}

long JUCE_MIDI_SendNoteOff(int port, char chn, char note, char vel)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::noteOff(chn, note));
    }
    return dummyreturn;
}

long JUCE_MIDI_SendPolyPressure(int port, char chn, char note, char val)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::aftertouchChange(chn, note, val));
    }
    return dummyreturn;
}

long JUCE_MIDI_SendCC(int port, char chn, char note, char val)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::controllerEvent(chn, note, val));
    }
    return dummyreturn;
}



long JUCE_MIDI_SendProgramChange(int port, char chn, char charprg)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::programChange(chn, charprg));
    }
    return dummyreturn;
}

long JUCE_MIDI_SendAftertouch(int port, char chn, char val)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::channelPressureChange(chn, val));
    }
    return dummyreturn;
}

long JUCE_MIDI_SendPitchBend(int port, char chn, int val)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::pitchWheel(chn, val));
    }
    return dummyreturn;
}



long JUCE_MIDI_SendClock(int port)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::midiClock());
    }
    return dummyreturn;
}

long JUCE_MIDI_SendStart(int port)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::midiStart());
    }
    return dummyreturn;
}

long JUCE_MIDI_SendContinue(int port)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::midiContinue());
    }
    return dummyreturn;
}

long JUCE_MIDI_SendStop(int port)
{
    if (MIDIOut != NULL)
    {
        MIDIOut->sendMessageNow(MidiMessage::midiStop());
    }
    return dummyreturn;
}

