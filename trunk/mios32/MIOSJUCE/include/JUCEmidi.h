/*
	FreeRTOS Emu
*/

#ifndef JUCEMIDI_H
#define JUCEMIDI_H

#ifdef __cplusplus
extern "C" {
#endif


extern long JUCE_MIDI_Init(long mode);

extern long JUCE_MIDI_DirectRxCallback_Init(void *callback_rx);

extern long JUCE_MIDI_SendPackage(int port, long package);


extern long JUCE_MIDI_SendNoteOn(int port, char chn, char note, char vel);

extern long JUCE_MIDI_SendNoteOff(int port, char chn, char note, char vel);

extern long JUCE_MIDI_SendPolyPressure(int port, char chn, char note, char val);

extern long JUCE_MIDI_SendCC(int port, char chn, char note, char val);

extern long JUCE_MIDI_SendProgramChange(int port, char chn, char charprg);

extern long JUCE_MIDI_SendAftertouch(int port, char chn, char val);

extern long JUCE_MIDI_SendPitchBend(int port, char chn, int val);

extern long JUCE_MIDI_SendClock(int port);

extern long JUCE_MIDI_SendStart(int port);

extern long JUCE_MIDI_SendContinue(int port);

extern long JUCE_MIDI_SendStop(int port);


#ifdef __cplusplus
}
#endif


#endif /* JUCEMIDI_H */

