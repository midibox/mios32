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
    , gotFirstMessage(0)
    , filterMidiClock(1)
    , filterActiveSense(1)
    , filterMiosTerminalMessage(1)
    , cutLongMessages(1)
{
	addAndMakeVisible(midiPortSelector = new ComboBox(String::empty));
	midiPortSelector->addListener(this);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< none >>"), -1);
    midiPortSelector->addSeparator();
	
    const StringArray midiPorts = inPort ? MidiInput::getDevices() : MidiOutput::getDevices();
    String inPortName = miosStudio->getMidiInput();
    String outPortName = miosStudio->getMidiOutput();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiPortSelector->addItem(midiPorts[i], i+1);
        bool enabled = inPort ? (inPortName == midiPorts[i]) : (outPortName == midiPorts[i]);

        if( enabled )
            current = i + 1;
    }
    midiPortSelector->setSelectedId(current, true);

	midiPortLabel = new Label("", inPort ? T("MIDI IN: ") : T("MIDI OUT: "));
	midiPortLabel->attachToComponent(midiPortSelector, true);

    addAndMakeVisible(monitorLogBox = new LogBox(T("Midi Monitor")));
    monitorLogBox->addEntry(T("MIDI Monitor ready."));

    setSize(400, 200);
}

MidiMonitor::~MidiMonitor()
{
    deleteAndZero(monitorLogBox);
}

//==============================================================================
void MidiMonitor::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiMonitor::resized()
{
    midiPortLabel->setBounds(4, 4, 50-8, 22);
	midiPortSelector->setBounds(4+50+4, 4, getWidth()-8-50-4, 22);
    monitorLogBox->setBounds(4, 4+22+4, getWidth()-8, getHeight()-(4+22+4+4));
}


//==============================================================================
void MidiMonitor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiPortSelector ) {
        if( inPort )
            miosStudio->setMidiInput(midiPortSelector->getText());
        else
            miosStudio->setMidiOutput(midiPortSelector->getText());
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

        if( !gotFirstMessage )
            monitorLogBox->clear();

        double timeStamp = message.getTimeStamp() ? message.getTimeStamp() : ((double)Time::getMillisecondCounter() / 1000.0);
        String timeStampStr = (timeStamp > 0)
            ? String::formatted(T("%8.3f"), timeStamp)
            : T("now");

        String hexStr = String::toHexString(data, size);

        monitorLogBox->addEntry("[" + timeStampStr + "] " + hexStr);
        gotFirstMessage = 1;
    }
}
