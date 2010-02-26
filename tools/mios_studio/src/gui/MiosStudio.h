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

#include "../includes.h"
#include <queue>

#include "UploadWindow.h"
#include "MidiMonitor.h"
#include "MiosTerminal.h"
#include "MidiKeyboard.h"
#include "SysexTool.h"
#include "Midio128Tool.h"
#include "../UploadHandler.h"

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

    void sendMidiMessage(MidiMessage &message);

    void setMidiInput(const String &port);
    String getMidiInput(void);
    void setMidiOutput(const String &port);
    String getMidiOutput(void);

    AudioDeviceManager audioDeviceManager;

    UploadHandler *uploadHandler;

    // Windows opened by Tools button in Upload Window
    SysexToolWindow *sysexToolWindow;
    Midio128ToolWindow *midio128ToolWindow;

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

    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiInQueue;
    CriticalSection midiInQueueLock;
    uint8 runningStatus;

    std::queue<MidiMessage> midiOutQueue;
    CriticalSection midiOutQueueLock;

    int initialMidiScanCounter;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosStudio (const MiosStudio&);
    const MiosStudio& operator= (const MiosStudio&);
};

#endif /* _MIOS_STUDIO_H */
