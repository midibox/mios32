/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Midi Keyboard
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MidiKeyboard.h"


//==============================================================================
MidiKeyboard::MidiKeyboard(AudioDeviceManager &_audioDeviceManager)
    : audioDeviceManager(&_audioDeviceManager)
{
    for(int i=0; i<5; ++i) {
        MidiSlider *s = new MidiSlider(_audioDeviceManager);
        midiSlider.push_back(s);
        s->setFunction("CC", i+16);
        addAndMakeVisible(s);
    }

    addAndMakeVisible(midiKeyboardComponent
                      = new MidiKeyboardComponent(keyboardState,
                                                  MidiKeyboardComponent::horizontalKeyboard));

    setSize(400, 200);
}

MidiKeyboard::~MidiKeyboard()
{
}

//==============================================================================
void MidiKeyboard::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiKeyboard::resized()
{
    int sliderWidth = 128;
    int distanceBetweenSliders = 150;
    int sliderArrayWidth = midiSlider.size() * distanceBetweenSliders - (distanceBetweenSliders-sliderWidth);
    int sliderOffset = 4 + ((getWidth()-8-sliderArrayWidth) / 2);
    for(int i=0; i<midiSlider.size(); ++i)
        midiSlider[i]->setBounds(sliderOffset + distanceBetweenSliders*i, 4, sliderWidth, 42);

    midiKeyboardComponent->setBounds(4, 4+44+4, getWidth()-8, getHeight()-8-44-4);
}
