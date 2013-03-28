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
static const char MidiSliderCcNames[128][9] = {
  // 0x00..0x0f
  "Bank MSB",
  "ModWheel",
  "Breath  ",
  "Ctrl.  3",
  "FootCtrl",
  "PortTime",
  "Data MSB",
  "Volume  ",
  "Balance ",
  "Ctrl.  9",
  "Panorama",
  "Expr.   ",
  "Fx#1 MSB",
  "Fx#2 MSB",
  "Ctrl. 14",
  "Ctrl. 15",
  
  // 0x10..0x1f
  "GP #1   ",
  "GP #2   ",
  "GP #3   ",
  "GP #4   ",
  "Ctrl. 20",
  "Ctrl. 21",
  "Ctrl. 22",
  "Ctrl. 23",
  "Ctrl. 24",
  "Ctrl. 25",
  "Ctrl. 26",
  "Ctrl. 27",
  "Ctrl. 28",
  "Ctrl. 29",
  "Ctrl. 30",
  "Ctrl. 31",
  
  // 0x20..0x2f
  "Bank LSB",
  "Mod  LSB",
  "BreatLSB",
  "Ctrl. 35",
  "Foot.LSB",
  "PortaLSB",
  "Data LSB",
  "Vol. LSB",
  "Bal. LSB",
  "Ctrl. 41",
  "Pan. LSB",
  "Expr.LSB",
  "Fx#1 LSB",
  "Fx#2 LSB",
  "Ctrl. 46",
  "Ctrl. 47",
  
  // 0x30..0x3f
  "Ctrl. 48",
  "Ctrl. 49",
  "Ctrl. 50",
  "Ctrl. 51",
  "Ctrl. 52",
  "Ctrl. 53",
  "Ctrl. 54",
  "Ctrl. 55",
  "Ctrl. 56",
  "Ctrl. 57",
  "Ctrl. 58",
  "Ctrl. 59",
  "Ctrl. 60",
  "Ctrl. 61",
  "Ctrl. 62",
  "Ctrl. 63",
  
  // 0x40..0x4f
  "Sustain ",
  "Port. ON",
  "Sustenu.",
  "SoftPed.",
  "LegatoSw",
  "Hold2   ",
  "Ctrl. 70",
  "Harmonic",
  "Release ",
  "Attack  ",
  "CutOff  ",
  "Ctrl. 75",
  "Ctrl. 76",
  "Ctrl. 77",
  "Ctrl. 78",
  "Ctrl. 79",
  
  // 0x50..0x5f
  "Ctrl. 80",
  "Ctrl. 81",
  "Ctrl. 82",
  "Ctrl. 83",
  "Ctrl. 84",
  "Ctrl. 85",
  "Ctrl. 86",
  "Ctrl. 87",
  "Ctrl. 88",
  "Ctrl. 89",
  "Ctrl. 90",
  "Reverb  ",
  "Tremolo ",
  "Chorus  ",
  "Celeste ",
  "Phaser  ",
  
  // 0x60..0x6f
  "Data Inc",
  "Data Dec",
  "NRPN LSB",
  "NRPN MSB",
  "RPN LSB ",
  "RPN MSB ",
  "Ctrl.102",
  "Ctrl.103",
  "Ctrl.104",
  "Ctrl.105",
  "Ctrl.106",
  "Ctrl.107",
  "Ctrl.108",
  "Ctrl.109",
  "Ctrl.110",
  "Ctrl.111",
  
  // 0x70..0x7f
  "Ctrl.112",
  "Ctrl.113",
  "Ctrl.114",
  "Ctrl.115",
  "Ctrl.116",
  "Ctrl.117",
  "Ctrl.118",
  "Ctrl.119",
  "SoundOff",
  "ResetAll",
  "Local   ",
  "NotesOff",
  "Omni Off",
  "Omni On ",
  "Mono On ",
  "Poly On "
};


//==============================================================================
MidiSliderComponent::MidiSliderComponent(const String &componentName)
    : Slider(componentName)
    , snapsBackOnRelease(0)
{
    setRange(0, 127, 1);
    setTextBoxStyle(Slider::NoTextBox, true, 80, 20);
}

MidiSliderComponent::~MidiSliderComponent()
{
}

void MidiSliderComponent::mouseUp(const MouseEvent& e)
{
    if( snapsBackOnRelease )
        setValue(64);
}

//==============================================================================
MidiSlider::MidiSlider(MiosStudio *_miosStudio, int _num, String _functionName, int _functionArg, int _midiChannel, int initialValue, bool _vertical)
    : miosStudio(_miosStudio)
    , sliderNum(_num)
    , vertical(_vertical)
{
    addAndMakeVisible(slider = new MidiSliderComponent(T("Slider")));
    slider->setSliderStyle(vertical ? MidiSliderComponent::LinearVertical : MidiSliderComponent::LinearHorizontal);
    slider->addListener(this);

    // only used for horizontal sliders
    addAndMakeVisible(sliderFunction = new ComboBox(String::empty));
    sliderFunction->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
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
	deleteAndZero(sliderFunction);
}

//==============================================================================
void MidiSlider::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiSlider::resized()
{
    if( vertical ) {
        slider->setBounds(0, 0, getWidth(), getHeight());
    } else {
        sliderFunction->setBounds(0, 0, 128, 18);
        slider->setBounds(0, 18, 128, 24);
    }
}


//==============================================================================
void MidiSlider::setFunction(const String &_functionName, const int &_functionArg, const int &_midiChannel, const int &initialValue)
{
    functionName = _functionName;
    functionArg = _functionArg;
    midiChannel = _midiChannel;

    slider->snapsBackOnRelease = 0;
    if( functionName.containsWholeWord(T("CC")) ) {
        String labelStr(functionArg);
        sliderFunction->clear(true);
        for(int i=0; i<128; ++i) {
            sliderFunction->addItem(T("CC") + String(i) + T(": ") + String(MidiSliderCcNames[i]), i+1);
        }
        sliderFunction->setSelectedId(functionArg+1, true);
    } else if( functionName.containsWholeWord(T("PB")) ) {
        String labelStr(functionArg);
        slider->snapsBackOnRelease = 1;
        sliderFunction->clear();
    } else {
        sliderFunction->clear();
    }

    slider->setValue(initialValue, dontSendNotification); // don't send update message

    // store settings
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        propertiesFile->setValue("slider" + String(sliderNum) + "FunctionName",_functionName);
        propertiesFile->setValue("slider" + String(sliderNum) + "FunctionArg", functionArg);
        propertiesFile->setValue("slider" + String(sliderNum) + "MidiChannel", midiChannel);
        propertiesFile->setValue("slider" + String(sliderNum) + "InitialValue", slider->getValue());
    }
}


//==============================================================================
void MidiSlider::setMidiChannel(const int& _midiChannel)
{
    midiChannel = _midiChannel;
}

int MidiSlider::getMidiChannel(void)
{
    return midiChannel;
}

//==============================================================================
void MidiSlider::sliderValueChanged(Slider* sliderThatWasMoved)
{
    if( functionName.containsWholeWord(T("CC")) ) {
        MidiMessage message = MidiMessage::controllerEvent(midiChannel, functionArg, (uint8)slider->getValue());
        miosStudio->sendMidiMessage(message);
    } else if( functionName.containsWholeWord(T("PB")) ) {
        unsigned pitchValue = (int)slider->getValue() * 128;

        // I've seen this handling to fake an increased resolution on Korg and Yamaha Keyboards
        if( pitchValue != 0x2000 )
            pitchValue += (int)slider->getValue();

        MidiMessage message = MidiMessage::pitchWheel(midiChannel, pitchValue);
        miosStudio->sendMidiMessage(message);
    }
}


//==============================================================================
void MidiSlider::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
    setFunction(functionName, sliderFunction->getSelectedId()-1, midiChannel, slider->getValue());
}

