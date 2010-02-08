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

#ifndef _MIOS_STUDIO_H
#define _MIOS_STUDIO_H

#include "includes.h"
#include <queue>

#include "UploadWindow.h"
#include "MidiMonitor.h"
#include "MiosTerminal.h"
#include "MidiKeyboard.h"

class MiosStudio
    : public Component
    , public MidiInputCallback
    , public Timer
{
public:
    //==============================================================================
    MiosStudio();
    ~MiosStudio();

    //==============================================================================
    void paint (Graphics& g);
    void resized();

    //==============================================================================
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);

    void timerCallback();

    void sendMidiMessage(const MidiMessage &message);

    void setMidiInput(const String &port);
    void setMidiOutput(const String &port);

    AudioDeviceManager audioDeviceManager;

protected:
    //==============================================================================
    UploadWindow *uploadWindow;
    MidiMonitor *midiInMonitor;
    MidiMonitor *midiOutMonitor;
    MiosTerminal *miosTerminal;
    MidiKeyboard *midiKeyboard;

    StretchableLayoutManager horizontalLayout;
    StretchableLayoutResizerBar* horizontalDividerBar1;
    StretchableLayoutResizerBar* horizontalDividerBar2;
    StretchableLayoutResizerBar* horizontalDividerBar3;

    StretchableLayoutManager verticalLayoutMonitors;
    StretchableLayoutResizerBar* verticalDividerBarMonitors;

    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiQueue;
    uint8 runningStatus;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosStudio (const MiosStudio&);
    const MiosStudio& operator= (const MiosStudio&);
};

#endif /* _MIOS_STUDIO_H */