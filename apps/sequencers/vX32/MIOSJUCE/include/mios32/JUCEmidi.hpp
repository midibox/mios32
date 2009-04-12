/*
	FreeRTOS Emu
*/


#ifndef JUCEMIDI_HPP
#define JUCEMIDI_HPP


// needed to see the globally selected MIDI devices
#include <MainComponent.h>

// extern this so that emulation can see the member vars of the main component
// normally i'd put this in the header file above
// but there's no space in the bottom and keeping it jucer friendly is a must
extern MainComponent* contentComponent_p;

// pointer to the midi output being used
extern MidiOutput *MIDIOut;


#endif /* JUCEMIDI_HPP */


