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
    addAndMakeVisible(uploadWindow = new UploadWindow(audioDeviceManager));
    addAndMakeVisible(midiInMonitor = new MidiMonitor(audioDeviceManager, true));
    addAndMakeVisible(midiOutMonitor = new MidiMonitor(audioDeviceManager, false));
    addAndMakeVisible(miosTerminal = new MiosTerminal(audioDeviceManager));
    addAndMakeVisible(midiKeyboard = new MidiKeyboard(audioDeviceManager));


    audioDeviceManager.addMidiInputCallback("MIDI IN Monitor", midiInMonitor);


    //                             num  min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.1, -0.9, -0.30); // Upload Window
    horizontalLayout.setItemLayout(1,    8,    8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.1, -0.9, -0.25); // MIDI In/Out Monitors
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
    Component* hcomps[] = { uploadWindow,
                            horizontalDividerBar1,
                            0,
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
                                            4 + horizontalLayout.getItemCurrentPosition(2),
                                            getWidth() - 8,
                                            horizontalLayout.getItemCurrentAbsoluteSize(2),
                                            false, // lay out side-by-side
                                            true); // resize the components' heights as well as widths
}
