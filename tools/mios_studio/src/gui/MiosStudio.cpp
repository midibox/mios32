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

#include "MiosStudio.h"

//==============================================================================
MiosStudio::MiosStudio()
{
    addAndMakeVisible(uploadWindow = new UploadWindow(this));
    addAndMakeVisible(midiInMonitor = new MidiMonitor(this, true));
    addAndMakeVisible(midiOutMonitor = new MidiMonitor(this, false));
    addAndMakeVisible(miosTerminal = new MiosTerminal(this));
    addAndMakeVisible(midiKeyboard = new MidiKeyboard(this));

    //                             num  min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.1, -0.9, -0.25); // MIDI In/Out Monitors
    horizontalLayout.setItemLayout(1,    8,    8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.1, -0.9, -0.30); // Upload Window
    horizontalLayout.setItemLayout(3,    8,    8,     8); // Resizer
    horizontalLayout.setItemLayout(4, -0.1, -0.9, -0.25); // MIOS Terminal
    horizontalLayout.setItemLayout(5,    8,    8,     8); // Resizer
    horizontalLayout.setItemLayout(6, -0.1, -0.9, -0.20); // MIDI Keyboard

    horizontalDividerBar1 = new StretchableLayoutResizerBar(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalDividerBar1);
    horizontalDividerBar2 = new StretchableLayoutResizerBar(&horizontalLayout, 3, false);
    addAndMakeVisible(horizontalDividerBar2);
    horizontalDividerBar3 = new StretchableLayoutResizerBar(&horizontalLayout, 5, false);
    addAndMakeVisible(horizontalDividerBar3);

    //                           num  min   max   prefered  
    verticalLayoutMonitors.setItemLayout(0, -0.2, -0.8, -0.5); // MIDI In Monitor
    verticalLayoutMonitors.setItemLayout(1,    8,    8,    8); // resizer
    verticalLayoutMonitors.setItemLayout(2, -0.2, -0.8, -0.5); // MIDI Out Monitor

    verticalDividerBarMonitors = new StretchableLayoutResizerBar(&verticalLayoutMonitors, 1, true);
    addAndMakeVisible(verticalDividerBarMonitors);


    audioDeviceManager.addMidiInputCallback(String::empty, this);
    Timer::startTimer(1);

    setSize (800, 600);
}

MiosStudio::~MiosStudio()
{
    deleteAndZero(midiInMonitor);
    deleteAndZero(midiOutMonitor);
    deleteAndZero(uploadWindow);
    deleteAndZero(horizontalDividerBar1);
    deleteAndZero(horizontalDividerBar2);
    deleteAndZero(verticalDividerBarMonitors);
}

//==============================================================================
void MiosStudio::paint (Graphics& g)
{
    g.fillAll(Colour (0xffc1d0ff));
}

void MiosStudio::resized()
{
    Component* hcomps[] = { 0,
                            horizontalDividerBar1,
                            uploadWindow,
                            horizontalDividerBar2,
                            miosTerminal,
                            horizontalDividerBar3,
                            midiKeyboard
    };

    horizontalLayout.layOutComponents(hcomps, 7,
                                       4, 4,
                                       getWidth() - 8, getHeight() - 8,
                                       true,  // lay out above each other
                                       true); // resize the components' heights as well as widths

    Component* vcomps[] = { midiInMonitor, verticalDividerBarMonitors, midiOutMonitor };

    verticalLayoutMonitors.layOutComponents(vcomps, 3,
                                            4,
                                            4 + horizontalLayout.getItemCurrentPosition(0),
                                            getWidth() - 8,
                                            horizontalLayout.getItemCurrentAbsoluteSize(0),
                                            false, // lay out side-by-side
                                            true); // resize the components' heights as well as widths
}


//==============================================================================
void MiosStudio::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    
    midiQueue.push(message);
}


//==============================================================================
void MiosStudio::timerCallback()
{
    MidiMessage message(0xfe);    

    while( !midiQueue.empty() ) {
        message = midiQueue.front();
        midiQueue.pop();

        uint8 *data = message.getRawData();
        if( data[0] >= 0x80 && data[0] < 0xf8 )
            runningStatus = data[0];

        // propagate incoming event to MIDI components
        midiInMonitor->handleIncomingMidiMessage(message, runningStatus);

        // filter runtime events for following components to improve performance
        if( data[0] < 0xf8 ) {
            uploadWindow->handleIncomingMidiMessage(message, runningStatus);
            miosTerminal->handleIncomingMidiMessage(message, runningStatus);
        }
    }
}


//==============================================================================
void MiosStudio::sendMidiMessage(const MidiMessage &message)
{
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);

    midiOutMonitor->handleIncomingMidiMessage(message, message.getRawData()[0]);
}


//==============================================================================
void MiosStudio::setMidiInput(const String &port)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        bool enabled = allMidiIns[i] == port;
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
        // lastSysexMidiIn = allMidiIns[i];
    }

    // propagate port change
    uploadWindow->midiPortChanged();
}

void MiosStudio::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);

    // propagate port change
    uploadWindow->midiPortChanged();
}

