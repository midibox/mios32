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
#include "MiosStudio.h"


//==============================================================================
MiosTerminal::MiosTerminal(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , ongoingMidiMessage(0)
    , gotFirstMessage(0)
{
    addAndMakeVisible(terminalWindow = new TextEditor (String::empty));
    terminalWindow->setMultiLine(true);
    terminalWindow->setReturnKeyStartsNewLine(true);
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

//==============================================================================
void MiosTerminal::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    int messageOffset = 0;

    if( runningStatus == 0xf0 && !ongoingMidiMessage &&
        SysexHelper::isValidMios32DebugMessage(data, size, -1) ) {
            ongoingMidiMessage = 1;
            messageOffset = 8;
    } else
        ongoingMidiMessage = 0;

    if( ongoingMidiMessage ) {
        String str = "";

        for(int i=messageOffset; i<size; ++i) {
            if( data[i] == 0xf7 ) {
                ongoingMidiMessage = 0;
            } else {
                if( data[i] != '\n' || size < (i+1) )
                    str += String::formatted(T("%c"), data[i] & 0x7f);
            }
        }

        if( !gotFirstMessage )
            terminalWindow->clear();

        double timeStamp = message.getTimeStamp();
        String timeStampStr = (timeStamp > 0)
            ? String::formatted(T("%8.3f"), timeStamp)
            : T("now");

        String out = String::formatted(T("%s[%s] %s"),
                                       gotFirstMessage ? "\n" : "",
                                       (const char *)timeStampStr,
                                       (const char *)str);

        terminalWindow->insertTextAtCursor(out);
        gotFirstMessage = 1;
    }
}
