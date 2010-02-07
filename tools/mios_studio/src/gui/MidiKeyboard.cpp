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
#include "MiosStudio.h"


//==============================================================================
MidiKeyboard::MidiKeyboard(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    for(int i=0; i<5; ++i) {
        MidiSlider *s;
        if( i == 0 )
            s = new MidiSlider(miosStudio, "PB", 0, 1, 0x40);
        else
            s = new MidiSlider(miosStudio, "CC", i+16-1, 1, 0);

        midiSlider.push_back(s);
        addAndMakeVisible(s);
    }

    addAndMakeVisible(midiKeyboardComponent
                      = new MidiKeyboardComponent(keyboardState,
                                                  MidiKeyboardComponent::horizontalKeyboard));

    keyboardState.addListener(this);

    midiKeyboardComponent->setLowestVisibleKey(24); // does match better with layout

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


//==============================================================================
void MidiKeyboard::handleNoteOn(MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage message = MidiMessage::noteOn(midiChannel, midiNoteNumber, (uint8)(velocity*128));
    miosStudio->sendMidiMessage(message);
}

// inherited from MidiKeyboardStateListener
void MidiKeyboard::handleNoteOff(MidiKeyboardState *source, int midiChannel, int midiNoteNumber)
{
    MidiMessage message = MidiMessage::noteOn(midiChannel, midiNoteNumber, (uint8)0);
    miosStudio->sendMidiMessage(message);
}
