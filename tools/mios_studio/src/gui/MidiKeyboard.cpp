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
    for(int i=0; i<6; ++i) {
        MidiSlider *s;
        if( i == 0 )
            s = new MidiSlider(miosStudio, i, "PB", 0, 1, 0x40, true);
        else if( i == 1 )
            s = new MidiSlider(miosStudio, i, "CC", 1, 1, 0, false);
        else
            s = new MidiSlider(miosStudio, i, "CC", i+16-1, 1, 0, false);

        midiSlider.push_back(s);
        addAndMakeVisible(s);
    }

    addAndMakeVisible(midiKeyboardComponent
                      = new MidiKeyboardComponent(keyboardState,
                                                  MidiKeyboardComponent::horizontalKeyboard));

    keyboardState.addListener(this);

    midiKeyboardComponent->setLowestVisibleKey(24); // does match better with layout

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        midiKeyboardComponent->setMidiChannel(propertiesFile->getIntValue("midiKeyboardChannel", 1));
    }

    setSize(400, 200);
}

MidiKeyboard::~MidiKeyboard()
{
    deleteAllChildren();
}

//==============================================================================
void MidiKeyboard::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiKeyboard::resized()
{
    // pitchbender
    midiSlider[0]->setBounds(4, 24, 24, getHeight()-8);

    // CCs
    int sliderWidth = 128;
    int distanceBetweenSliders = 150;
    int sliderArrayWidth = (midiSlider.size()-1) * distanceBetweenSliders - (distanceBetweenSliders-sliderWidth);
    int sliderOffset = 20 + ((getWidth()-8-sliderArrayWidth) / 2);
    for(int i=1; i<midiSlider.size(); ++i)
        midiSlider[i]->setBounds(sliderOffset + distanceBetweenSliders*(i-1), 4, sliderWidth, 42);

    midiKeyboardComponent->setBounds(4+24+4, 4+44+4, getWidth()-8-24-4, getHeight()-8-44-4);
}


//==============================================================================
void MidiKeyboard::setMidiChannel(const int midiChannel)
{
    midiKeyboardComponent->setMidiChannel(midiChannel);
    midiKeyboardComponent->setMidiChannelsToDisplay(1 << midiChannel);

    // store setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile )
        propertiesFile->setValue("midiKeyboardChannel", midiChannel);
}


int MidiKeyboard::getMidiChannel(void)
{
    return midiKeyboardComponent->getMidiChannel();
}


//==============================================================================
void MidiKeyboard::handleNoteOn(MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity)
{
    MidiMessage message = MidiMessage::noteOn(midiChannel, midiNoteNumber, (uint8)(velocity*128));
    miosStudio->sendMidiMessage(message);
}

void MidiKeyboard::handleNoteOff(MidiKeyboardState *source, int midiChannel, int midiNoteNumber)
{
    MidiMessage message = MidiMessage::noteOn(midiChannel, midiNoteNumber, (uint8)0);
    miosStudio->sendMidiMessage(message);
}


//==============================================================================
void MidiKeyboard::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{

}

