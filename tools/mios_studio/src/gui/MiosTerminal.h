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

#ifndef _MIOS_TERMINAL_H
#define _MIOS_TERMINAL_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "LogBox.h"

class MiosStudio; // forward declaration

class MiosTerminal
    : public Component
    , public TextEditorListener
{
public:
    //==============================================================================
    MiosTerminal(MiosStudio *_miosStudio);
    ~MiosTerminal();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    LogBox* terminalLogBox;
    TextEditor* inputLine;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    bool gotFirstMessage;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosTerminal (const MiosTerminal&);
    const MiosTerminal& operator= (const MiosTerminal&);
};

#endif /* _MIOS_TERMINAL_H */
