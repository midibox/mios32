/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDI Keyboard
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDI_KEYBOARD_H
#define _MIDI_KEYBOARD_H

#include "../includes.h"
#include "MidiSlider.h"
#include <vector>

class MidiKeyboard
    : public Component
{
public:
    //==============================================================================
    MidiKeyboard(AudioDeviceManager &_audioDeviceManager);
    ~MidiKeyboard();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

private:
    //==============================================================================
    std::vector<MidiSlider*> midiSlider;
    MidiKeyboardComponent *midiKeyboardComponent;
    MidiKeyboardState keyboardState;

    //==============================================================================
    AudioDeviceManager *audioDeviceManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiKeyboard (const MidiKeyboard&);
    const MidiKeyboard& operator= (const MidiKeyboard&);
};

#endif /* _MIDI_KEYBOARD_H */
