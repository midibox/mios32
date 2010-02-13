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
MidiSlider::MidiSlider(MiosStudio *_miosStudio, int _num, String _functionName, int _functionArg, int _midiChannel, int initialValue, bool _vertical)
    : miosStudio(_miosStudio)
    , sliderNum(_num)
    , vertical(_vertical)
{
    slider = new Slider(T("Slider"));
    addAndMakeVisible(slider);
    slider->setRange(0, 127, 1);
    slider->setSliderStyle(vertical ? Slider::LinearVertical : Slider::LinearHorizontal);
    slider->setTextBoxStyle(Slider::NoTextBox, true, 80, 20);
    slider->addListener(this);

    label = new Label("", "<no function>");
    addAndMakeVisible(label);
    label->setJustificationType(Justification::centred);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        _functionName = propertiesFile->getValue("slider" + String(sliderNum) + "FunctionName", _functionName);
        _functionArg = propertiesFile->getIntValue("slider" + String(sliderNum) + "FunctionArg", _functionArg);
        _midiChannel = propertiesFile->getIntValue("slider" + String(sliderNum) + "MidiChannel", _midiChannel);
        initialValue = propertiesFile->getIntValue("slider" + String(sliderNum) + "InitialValue", initialValue);
    }

    setFunction(_functionName, _functionArg, _midiChannel, initialValue);

    if( vertical )
        setSize(24, 18+80);
    else
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
    if( vertical ) {
        label->setBounds(0, 0, 24, 18);
        slider->setBounds(0, 18, 24, 80);
    } else {
        label->setBounds(0, 0, 128, 24);
        slider->setBounds(0, 18, 128, 24);
    }
}


//==============================================================================
void MidiSlider::setFunction(const String &_functionName, const int &_functionArg, const int &_midiChannel, const int &initialValue)
{
    functionName = _functionName;
    functionArg = _functionArg;
    midiChannel = _midiChannel;

    if( functionName.containsWholeWord(T("CC")) ) {
        String labelStr(functionArg);
        label->setText("CC#" + labelStr, true);
    } else if( functionName.containsWholeWord(T("PB")) ) {
        String labelStr(functionArg);
        label->setText("PB", true);
    } else {
        label->setText("<no function>", true);
    }

    slider->setValue(initialValue, false); // don't send update message

    // store settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        propertiesFile->setValue("slider" + String(sliderNum) + "FunctionName",_functionName);
        propertiesFile->setValue("slider" + String(sliderNum) + "FunctionArg", functionArg);
        propertiesFile->setValue("slider" + String(sliderNum) + "MidiChannel", midiChannel);
        propertiesFile->setValue("slider" + String(sliderNum) + "InitialValue", slider->getValue());
    }
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
