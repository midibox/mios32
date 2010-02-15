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
#include "../SysexHelper.h"
#include "LogBox.h"

class MiosStudio; // forward declaration

class MidiMonitor
    : public Component
    , public ComboBoxListener
{
public:
    //==============================================================================
    MidiMonitor(MiosStudio *_miosStudio, const bool _inPort);
    ~MidiMonitor();

    //==============================================================================
    void scanMidiDevices();

    //==============================================================================
    void paint(Graphics& g);
    void resized();
    void comboBoxChanged(ComboBox*);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    LogBox* monitorLogBox;
    ComboBox* midiPortSelector;
    Label* midiPortLabel;

    //==============================================================================
    MiosStudio *miosStudio;
    bool inPort;

    //==============================================================================
    bool filterMidiClock;
    bool filterActiveSense;
    bool filterMiosTerminalMessage;
    bool cutLongMessages;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiMonitor (const MidiMonitor&);
    const MidiMonitor& operator= (const MidiMonitor&);
};

#endif /* _MIDI_MONITOR_H */
