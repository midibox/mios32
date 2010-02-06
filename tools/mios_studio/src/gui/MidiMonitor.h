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

#ifndef _MIDI_MONITOR_H
#define _MIDI_MONITOR_H

#include "../includes.h"

class MidiMonitor
    : public Component
    , public ComboBoxListener
    , public MidiInputCallback
{
public:
    //==============================================================================
    MidiMonitor(AudioDeviceManager &_audioDeviceManager, const bool _inPort);
    ~MidiMonitor();

    //==============================================================================
    void paint(Graphics& g);
    void resized();
    void comboBoxChanged(ComboBox*);

    //==============================================================================
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);
    void handlePartialSysexMessage(MidiInput* source, const uint8 *messageData, const int numBytesSoFar, const double timestamp);

private:
    //==============================================================================
    TextEditor* monitorWindow;
    ComboBox* midiPortSelector;
    Label* midiPortLabel;

    //==============================================================================
    AudioDeviceManager *audioDeviceManager;
    bool inPort;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiMonitor (const MidiMonitor&);
    const MidiMonitor& operator= (const MidiMonitor&);
};

#endif /* _MIDI_MONITOR_H */
