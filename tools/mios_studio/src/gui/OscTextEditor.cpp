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


//==============================================================================
OscTextEditor::OscTextEditor(Label *_statusLabel)
    : TextEditor(String::empty)
    , statusLabel(_statusLabel)
    , bufferedText(String::empty)
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

    setSize(600, 200);
}

OscTextEditor::~OscTextEditor()
{
}

//==============================================================================
void OscTextEditor::clear()
{
    bufferedText = String::empty;
    TextEditor::clear();
}

//==============================================================================
void OscTextEditor::textEditorTextChanged(TextEditor &editor)
{
    String oscStr = getText();
    bool invalidSyntax = false;
    int numParameters = 0;

    if( invalidSyntax )
        statusLabel->setText(T("Invalid Syntax!"), true);
    else if( numParameters == 0 )
        statusLabel->setText(String::empty, true);
    else if( numParameters == 1 )
        statusLabel->setText(T("1 parameter"), true);
    else
        statusLabel->setText(String(numParameters) + T(" parameters"), true);
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
    String oscStr = getText();
    Array<uint8> retArray;

    return retArray;
}
