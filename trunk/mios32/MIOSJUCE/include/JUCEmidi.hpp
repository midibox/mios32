/*
    FreeRTOS Emu
*/


#ifndef JUCEMIDI_HPP
#define JUCEMIDI_HPP


// needed to see the globally selected MIDI devices
#include <MainComponent.h>

// extern this so that emulation can see the MIDI Device Manager globally
extern AudioDeviceManager* midiDeviceManager;

// pointer to the midi output being used
extern MidiOutput *MIDIOut;


#endif /* JUCEMIDI_HPP */


