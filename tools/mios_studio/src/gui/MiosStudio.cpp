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
    : uploadWindow(0)
    , midiInMonitor(0)
    , midiOutMonitor(0)
    , miosTerminal(0)
    , midiKeyboard(0)
{
    uploadHandler = new UploadHandler(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        setMidiInput(propertiesFile->getValue(T("midiIn"), String::empty));
        setMidiOutput(propertiesFile->getValue(T("midiOut"), String::empty));
    }

    addAndMakeVisible(uploadWindow = new UploadWindow(this));
    addAndMakeVisible(midiInMonitor = new MidiMonitor(this, true));
    addAndMakeVisible(midiOutMonitor = new MidiMonitor(this, false));
    addAndMakeVisible(miosTerminal = new MiosTerminal(this));
    addAndMakeVisible(midiKeyboard = new MidiKeyboard(this));

    //                             num   min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.005, -0.9, -0.25); // MIDI In/Out Monitors
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.005, -0.9, -0.30); // Upload Window
    horizontalLayout.setItemLayout(3,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(4, -0.005, -0.9, -0.25); // MIOS Terminal
    horizontalLayout.setItemLayout(5,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(6, -0.005, -0.2, -0.20); // MIDI Keyboard

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

    if( audioDeviceManager.getDefaultMidiOutputName() != String::empty )
        uploadWindow->queryCore();

    setSize (800, 600);
}

MiosStudio::~MiosStudio()
{
    deleteAndZero(uploadHandler);
    deleteAndZero(uploadWindow);
    deleteAndZero(midiInMonitor);
    deleteAndZero(midiOutMonitor);
    deleteAndZero(miosTerminal);
    deleteAndZero(midiKeyboard);
    deleteAndZero(horizontalDividerBar1);
    deleteAndZero(horizontalDividerBar2);
    deleteAndZero(horizontalDividerBar3);
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
    
    midiInQueue.push(message);

    // propagate to upload handler
    uploadHandler->handleIncomingMidiMessage(source, message);
}


//==============================================================================
void MiosStudio::sendMidiMessage(const MidiMessage &message)
{
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);

    midiOutQueue.push(message);
}


//==============================================================================
void MiosStudio::timerCallback()
{
    // important: only broadcast one message per timer tick to avoid GUI hangups when
    // a large bulk of data is received

    if( !midiInQueue.empty() ) {
        MidiMessage &message = midiInQueue.front();

        uint8 *data = message.getRawData();
        if( data[0] >= 0x80 && data[0] < 0xf8 )
            runningStatus = data[0];

        // propagate incoming event to MIDI components
        midiInMonitor->handleIncomingMidiMessage(message, runningStatus);

        // filter runtime events for following components to improve performance
        if( data[0] < 0xf8 ) {
            miosTerminal->handleIncomingMidiMessage(message, runningStatus);
            midiKeyboard->handleIncomingMidiMessage(message, runningStatus);
        }

        midiInQueue.pop();
    }

    if( !midiOutQueue.empty() ) {
        MidiMessage &message = midiOutQueue.front();
        
        midiOutMonitor->handleIncomingMidiMessage(message, message.getRawData()[0]);

        midiOutQueue.pop();
    }
}


//==============================================================================
void MiosStudio::setMidiInput(const String &port)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        bool enabled = allMidiIns[i] == port;
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
    }

    // propagate port change
    if( uploadWindow )
        uploadWindow->midiPortChanged();

    // store setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile )
        propertiesFile->setValue(T("midiIn"), port);
}

String MiosStudio::getMidiInput(void)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        if( audioDeviceManager.isMidiInputEnabled(allMidiIns[i]) )
            return allMidiIns[i];
    }

    return String::empty;
}

void MiosStudio::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);

    // propagate port change
    if( uploadWindow )
        uploadWindow->midiPortChanged();

    // store setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile )
        propertiesFile->setValue(T("midiOut"), port);
}

String MiosStudio::getMidiOutput(void)
{
    return audioDeviceManager.getDefaultMidiOutputName();
}
