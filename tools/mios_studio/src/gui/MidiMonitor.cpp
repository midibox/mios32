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


//==============================================================================
MidiMonitor::MidiMonitor(AudioDeviceManager &_audioDeviceManager, const bool _inPort)
    : audioDeviceManager(&_audioDeviceManager)
    , inPort(_inPort)
{
	addAndMakeVisible(midiPortSelector = new ComboBox(String::empty));
	midiPortSelector->addListener(this);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< none >>"), -1);
    midiPortSelector->addSeparator();
	
    const StringArray midiPorts = inPort ? MidiInput::getDevices() : MidiOutput::getDevices();

    int current = -1;
    for(int i = 0; i < midiPorts.size(); ++i) {
        midiPortSelector->addItem (midiPorts[i], i + 1);
        bool enabled = inPort
            ? audioDeviceManager->isMidiInputEnabled(midiPorts[i])
            : (audioDeviceManager->getDefaultMidiOutputName() == midiPorts[i]);

        if( enabled )
            current = i + 1;
    }
    midiPortSelector->setSelectedId(current, true);

	midiPortLabel = new Label("", inPort ? T("MIDI IN: ") : T("MIDI OUT: "));
	midiPortLabel->attachToComponent(midiPortSelector, true);

    addAndMakeVisible(monitorWindow = new TextEditor (String::empty));
    monitorWindow->setMultiLine(false);
    monitorWindow->setReturnKeyStartsNewLine(false);
    monitorWindow->setReadOnly(true);
    monitorWindow->setScrollbarsShown(true);
    monitorWindow->setCaretVisible(true);
    monitorWindow->setPopupMenuEnabled(false);
    monitorWindow->setText(T("MIDI Monitor ready."));

    setSize(400, 200);
}

MidiMonitor::~MidiMonitor()
{
    deleteAndZero(monitorWindow);
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
    monitorWindow->setBounds(4, 4+22+4, getWidth()-8, getHeight()-(4+22+4+4));
}


//==============================================================================
void MidiMonitor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiPortSelector ) {
        if( inPort ) {
            const StringArray allMidiIns(MidiInput::getDevices());
            for (int i = allMidiIns.size(); --i >= 0;) {
                bool enabled = allMidiIns[i] == midiPortSelector->getText();
                audioDeviceManager->setMidiInputEnabled(allMidiIns[i], enabled);
                // lastSysexMidiIn = allMidiIns[i];
            }
        } else {
            audioDeviceManager->setDefaultMidiOutput(midiPortSelector->getText());
            // lastSysexMidiOut = midiOutputSelector->getText();
        }
    }
}

//==============================================================================
void MidiMonitor::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
}

void MidiMonitor::handlePartialSysexMessage(MidiInput* source, const uint8 *messageData, const int numBytesSoFar, const double timestamp)
{
}
