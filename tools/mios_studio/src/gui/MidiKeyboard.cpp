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
            s = new MidiSlider(miosStudio, i, "CC", i+16-2, 1, 0, false);

        midiSlider.push_back(s);
        addAndMakeVisible(s);
    }

    addAndMakeVisible(midiKeyboardComponent
                      = new MidiKeyboardComponent(keyboardState,
                                                  MidiKeyboardComponent::horizontalKeyboard));

    keyboardState.addListener(this);

    midiKeyboardComponent->setLowestVisibleKey(24); // does match better with layout

    addAndMakeVisible(midiChannelLabel = new Label(T("Chn."), T("Chn.")));
    midiChannelLabel->setJustificationType(Justification::centred);

    addAndMakeVisible(midiChannelSlider = new Slider(T("MIDI Channel")));
    midiChannelSlider->setRange(1, 16, 1);
    midiChannelSlider->setSliderStyle(Slider::IncDecButtons);
    midiChannelSlider->setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    midiChannelSlider->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        int chn = propertiesFile->getIntValue("midiKeyboardChannel", 1);
        midiKeyboardComponent->setMidiChannel(chn);
        for(int i=0; i<midiSlider.size(); ++i)
            midiSlider[i]->setMidiChannel(chn);
        midiChannelSlider->setValue(chn, false);
    }

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
    // MIDI channel
    if( getWidth() > 764 ) {
        midiChannelLabel->setVisible(true);
        midiChannelLabel->setBounds(4, 6, 30, 12);
        midiChannelSlider->setVisible(true);
        midiChannelSlider->setBounds(4, 20, 30, 32);
    } else {
        midiChannelLabel->setVisible(false);
        midiChannelSlider->setVisible(false);
    }

    // CCs
    int sliderWidth = 128;
    int distanceBetweenSliders = 150;
    int sliderArrayWidth = (midiSlider.size()-1) * distanceBetweenSliders - (distanceBetweenSliders-sliderWidth);
    int sliderOffset = 20 + ((getWidth()-8-sliderArrayWidth) / 2);
    for(int i=1; i<midiSlider.size(); ++i)
        midiSlider[i]->setBounds(sliderOffset + distanceBetweenSliders*(i-1), 4, sliderWidth, 42);

    int keyboardX = 4+24+4;
    int keyboardWidth = getWidth()-8-24-4;
    int keyboardY = 4+44+4;
    int keyboardHeight = getHeight()-8-44-4;
    if( getWidth() >= (1024+4+24+4+4) ) {
        keyboardWidth = 1024;
        keyboardX = 4+24+4 + ((getWidth()-keyboardWidth-4-4) / 2);
    }

    midiKeyboardComponent->setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);
    midiSlider[0]->setBounds(keyboardX-24-4, keyboardY-4, 24, keyboardHeight+8);
}


//==============================================================================
void MidiKeyboard::setMidiChannel(const int& midiChannel)
{
    midiKeyboardComponent->setMidiChannel(midiChannel);
    midiKeyboardComponent->setMidiChannelsToDisplay(1 << midiChannel);

    for(int i=0; i<midiSlider.size(); ++i)
        midiSlider[i]->setMidiChannel(midiChannel);

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
void MidiKeyboard::sliderValueChanged(Slider* sliderThatWasMoved)
{
    setMidiChannel(midiChannelSlider->getValue());
}


//==============================================================================
void MidiKeyboard::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{

}

