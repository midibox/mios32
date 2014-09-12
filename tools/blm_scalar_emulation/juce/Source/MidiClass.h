/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UploadHandler.cpp 928 2010-02-20 23:38:06Z tk $
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
	void sendBLMLayout(void);
private:


};

#endif /* _BLM_H */
