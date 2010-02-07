//
// MidiClass.h - blm_scalar_emulation 
//
// Created by Phil Taylor 2010
//
#ifndef _MIDI_H


class MidiClass : public Component,
                 public ButtonListener
{
public:
	MidiClass();
	~MidiClass();
	sendBLMLayout();
private:


};

#endif /* _BLM_H */