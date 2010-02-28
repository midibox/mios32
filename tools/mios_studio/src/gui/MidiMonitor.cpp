/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDI Monitor Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MidiMonitor.h"
#include "MiosStudio.h"


//==============================================================================
MidiMonitor::MidiMonitor(MiosStudio *_miosStudio, const bool _inPort)
    : miosStudio(_miosStudio)
    , inPort(_inPort)
    , filterMidiClock(1)
    , filterActiveSense(1)
    , filterMiosTerminalMessage(1)
    , cutLongMessages(1)
{
	addAndMakeVisible(midiPortSelector = new ComboBox(String::empty));
	midiPortSelector->addListener(this);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< device scan running >>"), -1);
    midiPortSelector->setSelectedId(-1, true);
    midiPortSelector->setEnabled(false);
	midiPortLabel = new Label("", inPort ? T("MIDI IN: ") : T("MIDI OUT: "));
	midiPortLabel->attachToComponent(midiPortSelector, true);

    addAndMakeVisible(monitorLogBox = new LogBox(T("Midi Monitor")));
    monitorLogBox->addEntry(Colours::red, T("Connecting to MIDI driver - be patient!"));

    setSize(400, 200);
}

MidiMonitor::~MidiMonitor()
{
    deleteAllChildren();
}

//==============================================================================
// Should be called after startup once window is visible
void MidiMonitor::scanMidiDevices()
{
    monitorLogBox->clear();

    midiPortSelector->setEnabled(false);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< none >>"), -1);
    midiPortSelector->addSeparator();

    // restore settings
    String selectedPort;
    StringArray midiPorts;
    if( inPort ) {
        monitorLogBox->addEntry(Colours::red, T("Scanning for MIDI Inputs..."));
        selectedPort = miosStudio->getMidiInput();
        midiPorts = MidiInput::getDevices();
    } else {
        monitorLogBox->addEntry(Colours::red, T("Scanning for MIDI Outputs..."));
        selectedPort = miosStudio->getMidiOutput();
        midiPorts = MidiOutput::getDevices();
    }

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiPortSelector->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled )
            current = i + 1;

        monitorLogBox->addEntry(Colours::blue, "[" + String(i+1) + "] " + midiPorts[i] + (enabled ? " (*)" : ""));

        if( inPort )
            miosStudio->audioDeviceManager.setMidiInputEnabled(midiPorts[i], enabled);
        else if( enabled )
            miosStudio->audioDeviceManager.setDefaultMidiOutput(midiPorts[i]);
    }
    midiPortSelector->setSelectedId(current, true);
    midiPortSelector->setEnabled(true);

    if( current == -1 )
        if( inPort )
            miosStudio->setMidiInput(String::empty);
        else
            miosStudio->setMidiOutput(String::empty);

    monitorLogBox->addEntry(Colours::grey, T("MIDI Monitor ready."));
}

//==============================================================================
void MidiMonitor::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiMonitor::resized()
{
    midiPortLabel->setBounds(4, 4, 60-8, 22);
	midiPortSelector->setBounds(4+60+4, 4, getWidth()-8-60-4, 22);
    monitorLogBox->setBounds(4, 4+22+4, getWidth()-8, getHeight()-(4+22+4+4));
}


//==============================================================================
void MidiMonitor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiPortSelector ) {
        String portName = midiPortSelector->getText();
        if( portName == T("<< none >>") )
            portName = String::empty;

        if( inPort )
            miosStudio->setMidiInput(portName);
        else
            miosStudio->setMidiOutput(portName);
    }
}

//==============================================================================
void MidiMonitor::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint32 size = message.getRawDataSize();
    uint8 *data = message.getRawData();

    bool isMidiClock = data[0] == 0xf8;
    bool isActiveSense = data[0] == 0xfe;
    bool isMiosTerminalMessage = SysexHelper::isValidMios32DebugMessage(data, size, -1);

    if( !(isMidiClock && filterMidiClock) &&
        !(isActiveSense && filterActiveSense) &&
        !(isMiosTerminalMessage && filterMiosTerminalMessage) ) {

        double timeStamp = message.getTimeStamp() ? message.getTimeStamp() : ((double)Time::getMillisecondCounter() / 1000.0);
        String timeStampStr = (timeStamp > 0)
            ? String::formatted(T("%8.3f"), timeStamp)
            : T("now");

        String hexStr = String::toHexString(data, size);

        monitorLogBox->addEntry(Colours::black, "[" + timeStampStr + "] " + hexStr);
    }
}
