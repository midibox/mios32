/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Midi Slider
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MidiSlider.h"
#include "MiosStudio.h"


//==============================================================================
MidiSlider::MidiSlider(MiosStudio *_miosStudio, String _functionName, int _functionArg, int _midiChannel, int initialValue)
    : miosStudio(_miosStudio)
{
    slider = new Slider(T("Slider"));
    addAndMakeVisible(slider);
    slider->setRange(0, 127, 1);
    slider->setSliderStyle(Slider::LinearHorizontal);
    slider->setTextBoxStyle(Slider::NoTextBox, true, 80, 20);
    slider->addListener(this);

    label = new Label("", "<no function>");
    addAndMakeVisible(label);
    label->setJustificationType(Justification::centred);

    setFunction(_functionName, _functionArg, _midiChannel, initialValue);

    setSize(128, 24+18);
}

MidiSlider::~MidiSlider()
{
    deleteAndZero(slider);
    deleteAndZero(label);
}

//==============================================================================
void MidiSlider::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiSlider::resized()
{
    label->setBounds(0, 0, 128, 24);
    slider->setBounds(0, 18, 128, 24);
}


//==============================================================================
void MidiSlider::setFunction(String _functionName, int _functionArg, int _midiChannel, int initialValue)
{
    functionName = _functionName;
    functionArg = _functionArg;
    midiChannel = _midiChannel;

    if( functionName.containsWholeWord(T("CC")) ) {
        String labelStr(functionArg);
        label->setText("CC#" + labelStr, true);
    } else if( functionName.containsWholeWord(T("PB")) ) {
        String labelStr(functionArg);
        label->setText("PitchBender", true);
    } else {
        label->setText("<no function>", true);
    }

    slider->setValue(initialValue, false); // don't send update message
}

//==============================================================================
void MidiSlider::sliderValueChanged(Slider* sliderThatWasMoved)
{
    if( functionName.containsWholeWord(T("CC")) ) {
        MidiMessage message = MidiMessage::controllerEvent(midiChannel, functionArg, (uint8)slider->getValue());
        miosStudio->sendMidiMessage(message);
    } else if( functionName.containsWholeWord(T("PB")) ) {
        MidiMessage message = MidiMessage::pitchWheel(midiChannel, (uint8)slider->getValue());
        miosStudio->sendMidiMessage(message);
    }
}
