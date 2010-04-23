/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Text Editor variant which allows to edit OSC messages
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "OscTextEditor.h"

#define OSC_BUFFER_MAX 8192

//==============================================================================
OscTextEditor::OscTextEditor(Label *_statusLabel)
    : TextEditor(String::empty)
    , statusLabel(_statusLabel)
{
    setMultiLine(true);
    setReturnKeyStartsNewLine(true);
    setReadOnly(false);
    setScrollbarsShown(true);
    setCaretVisible(true);
    setPopupMenuEnabled(true);
    //setInputRestrictions(1000000, T("0123456789ABCDEFabcdef \n"));
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
#if defined(JUCE_WIN32) || defined(JUCE_LINUX)
    setFont(Font(Typeface::defaultTypefaceNameMono, 10.0, 0));
#else
    setFont(Font(Typeface::defaultTypefaceNameMono, 13.0, 0));
#endif
#else
#if defined(JUCE_WIN32) || defined(JUCE_LINUX)
    setFont(Font(Font::getDefaultMonospacedFontName(), 10.0, 0));
#else
    setFont(Font(Font::getDefaultMonospacedFontName(), 13.0, 0));
#endif
#endif
	addListener(this);
    setTextToShowWhenEmpty(T("Syntax: [@<seconds>.<fraction>] <osc-path> [<arg1>] [<arg2>] ... [<osc-path] [<arg1>] [<arg2>]..."), Colours::grey);

    setSize(600, 200);
}

OscTextEditor::~OscTextEditor()
{
}

//==============================================================================
void OscTextEditor::clear()
{
    TextEditor::clear();
}

//==============================================================================
void OscTextEditor::textEditorTextChanged(TextEditor &editor)
{
    String statusMessage;
    currentBinary = OscHelper::string2Packet(getText(), statusMessage);
    statusLabel->setText(statusMessage, true);
}

void OscTextEditor::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void OscTextEditor::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void OscTextEditor::textEditorFocusLost(TextEditor &editor)
{
}


//==============================================================================
Array<uint8> OscTextEditor::getBinary(void)
{
#if 1
    return currentBinary;
#else
    // send dummy message for testing only!
    String oscStr = getText();

    uint8 packet[OSC_BUFFER_MAX];
    uint8 *endPtr = packet;
    uint8 *insertLenPtr;

    OscHelper::OscTimetagT timetag;
    timetag.seconds = 0;
    timetag.fraction = 0;

    endPtr = OscHelper::putString(endPtr, "#bundle");
    endPtr = OscHelper::putTimetag(endPtr, timetag);
    insertLenPtr = endPtr; // remember this address - we will insert the length later
    endPtr += 4;
    endPtr = OscHelper::putString(endPtr, "/foo/bar/xxx/yyy");
    endPtr = OscHelper::putString(endPtr, ",fisTF");
    endPtr = OscHelper::putFloat(endPtr, 0.123f);
    endPtr = OscHelper::putInt(endPtr, 42);
    endPtr = OscHelper::putString(endPtr, "Hi!");
    OscHelper::putWord(insertLenPtr, (unsigned)(endPtr-insertLenPtr-4));
    size_t len = (size_t)(endPtr-packet);

    Array<uint8> retArray(packet, len);
    return retArray;
#endif
}
