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

#ifndef _OSC_TEXT_EDITOR_H
#define _OSC_TEXT_EDITOR_H

#include "../includes.h"
#include "../OscHelper.h"


class OscTextEditor
    : public TextEditor
    , public TextEditorListener
{
public:
    //==============================================================================
    OscTextEditor(Label *_statusLabel);
    ~OscTextEditor();

    //==============================================================================
    void clear();

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    //==============================================================================
    Array<uint8> getBinary(void);

protected:
    String bufferedText;

    //==============================================================================
    // note: this status label has to be placed into the window via setBounds() 
    // from external! See SysexTool.cpp as example
    Label* statusLabel;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    OscTextEditor (const OscTextEditor&);
    const OscTextEditor& operator= (const OscTextEditor&);
};

#endif /* _OSC_TEXT_EDITOR_H */
