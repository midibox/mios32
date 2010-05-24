/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UploadHandler.cpp 928 2010-02-20 23:38:06Z tk $


#include "MainComponent.h"


//==============================================================================
MainComponent::MainComponent ()
    : blmClass (0),
      midiInput (0),
      midiOutput (0),
      label (0),
      label2 (0),
      initialMidiScanCounter(1) // start step-wise MIDI port scan
{
    addAndMakeVisible (blmClass = new BlmClass (16,16));
    blmClass->setName (T("blmClass"));

    addAndMakeVisible(midiInput = new ComboBox (T("MIDI Inputs")));
    midiInput->setEditableText(false);
    midiInput->setJustificationType(Justification::centredLeft);
    midiInput->setTextWhenNothingSelected(String::empty);
    midiInput->setTextWhenNoChoicesAvailable(T("(no choices)"));
    midiInput->addItem(TRANS("<< device scan running >>"), -1);
    midiInput->addListener(this);

    addAndMakeVisible(label = new Label(T(""), T("MIDI IN: ")));
    label->attachToComponent(midiInput, true);

    addAndMakeVisible (midiOutput = new ComboBox (T("MIDI Output")));
    midiOutput->setEditableText(false);
    midiOutput->setJustificationType(Justification::centredLeft);
    midiOutput->setTextWhenNothingSelected(String::empty);
    midiOutput->setTextWhenNoChoicesAvailable(T("(no choices)"));
    midiOutput->addItem(TRANS("<< device scan running >>"), -1);
    midiOutput->addListener(this);

    addAndMakeVisible(label = new Label(T(""), T("MIDI OUT: ")));
    label->attachToComponent(midiOutput, true);

    Timer::startTimer(1);

    setSize(10 + blmClass->getWidth(), 5 + 24 + blmClass->getHeight());
}

MainComponent::~MainComponent()
{
    deleteAndZero (blmClass);
    deleteAndZero (midiInput);
    deleteAndZero (midiOutput);
    deleteAndZero (label);
    deleteAndZero (label2);
	deleteAllChildren();
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll(Colour(0xffd0d0d0));
}

void MainComponent::resized()
{
    int middle = getWidth() / 2;
    midiInput->setBounds(5+80, 5, middle-10-80, 24);
    midiOutput->setBounds(middle+5+80, 5, middle-10-80, 24);

    blmClass->setTopLeftPosition(5, 5+24+10);
}


//==============================================================================
void MainComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == midiInput)
    {
		blmClass->setMidiInput(midiInput->getText());
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("midiIn"), midiInput->getText());
    }
    else if (comboBoxThatHasChanged == midiOutput)
    {
		blmClass->setMidiOutput(midiOutput->getText());
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("midiOut"), midiOutput->getText());
    }
}


//==============================================================================
// Should be called after startup once window is visible
void MainComponent::scanMidiInDevices()
{
    midiInput->setEnabled(false);
	midiInput->clear();
    midiInput->addItem (TRANS("<< none >>"), -1);
    midiInput->addSeparator();

    // restore MIDI input port from .xml file
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue(T("midiIn"), String::empty) : String::empty;

    StringArray midiPorts = MidiInput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiInput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled )
            current = i + 1;

        blmClass->audioDeviceManager.setMidiInputEnabled(midiPorts[i], enabled);
    }
    midiInput->setSelectedId(current, true);
    midiInput->setEnabled(true);
}


//==============================================================================
// Should be called after startup once window is visible
void MainComponent::scanMidiOutDevices()
{
    midiOutput->setEnabled(false);
	midiOutput->clear();
    midiOutput->addItem (TRANS("<< none >>"), -1);
    midiOutput->addSeparator();

    // restore MIDI output port from .xml file
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue(T("midiOut"), String::empty) : String::empty;

    StringArray midiPorts = MidiOutput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiOutput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled ) {
            current = i + 1;
            blmClass->audioDeviceManager.setDefaultMidiOutput(midiPorts[i]);
        }
    }
    midiOutput->setSelectedId(current, true);
    midiOutput->setEnabled(true);
}

//==============================================================================
void MainComponent::timerCallback()
{
    // step-wise MIDI port scan after startup
    if( initialMidiScanCounter ) {
        switch( initialMidiScanCounter ) {
        case 1:
            scanMidiInDevices();
            ++initialMidiScanCounter;
            break;

        case 2:
            scanMidiOutDevices();
            ++initialMidiScanCounter;
            break;

        case 3:
            blmClass->sendBLMLayout();

            initialMidiScanCounter = 0; // stop scan
            Timer::stopTimer();
            break;
        }
    }
}
