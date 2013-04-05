/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Text Editor variant which handles a command line (with up/down key)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _COMMAND_LINE_EDITOR_H
#define _COMMAND_LINE_EDITOR_H

#include "../includes.h"


class CommandLineEditor
    : public TextEditor
    , public TextEditorListener
{
public:
    //==============================================================================
    CommandLineEditor(const unsigned maxLines = 1000);
    ~CommandLineEditor();

    //==============================================================================
    bool keyPressed(const KeyPress& key);

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

protected:
    unsigned maxLines;
    unsigned positionInList;
    StringArray commandList;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    CommandLineEditor (const CommandLineEditor&);
    const CommandLineEditor& operator= (const CommandLineEditor&);
};

#endif /* _COMMAND_LINE_EDITOR_H */
