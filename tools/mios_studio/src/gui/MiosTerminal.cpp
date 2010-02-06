/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIOS Terminal Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MiosTerminal.h"


//==============================================================================
MiosTerminal::MiosTerminal(AudioDeviceManager &_audioDeviceManager)
    : audioDeviceManager(&_audioDeviceManager)
{
    addAndMakeVisible(terminalWindow = new TextEditor (String::empty));
    terminalWindow->setMultiLine(false);
    terminalWindow->setReturnKeyStartsNewLine(false);
    terminalWindow->setReadOnly(true);
    terminalWindow->setScrollbarsShown(true);
    terminalWindow->setCaretVisible(true);
    terminalWindow->setPopupMenuEnabled(false);
    terminalWindow->setText(T("MIOS Terminal ready."));

    setSize(400, 200);
}

MiosTerminal::~MiosTerminal()
{
    deleteAndZero(terminalWindow);
}

//==============================================================================
void MiosTerminal::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MiosTerminal::resized()
{
    terminalWindow->setBounds(4, 4, getWidth()-8, getHeight()-8);
}
