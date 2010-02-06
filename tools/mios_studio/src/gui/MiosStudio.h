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

#include "UploadWindow.h"
#include "MidiMonitor.h"
#include "MiosTerminal.h"
#include "MidiKeyboard.h"

class MiosStudio
    : public Component
{
public:
    //==============================================================================
    MiosStudio();
    ~MiosStudio();

    //==============================================================================
    void paint (Graphics& g);
    void resized();


private:
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

    AudioDeviceManager audioDeviceManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosStudio (const MiosStudio&);
    const MiosStudio& operator= (const MiosStudio&);
};

#endif /* _MIOS_STUDIO_H */
