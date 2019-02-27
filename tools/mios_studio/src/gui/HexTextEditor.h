/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Text Editor variant which allows to edit hex bytes
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _HEX_TEXT_EDITOR_H
#define _HEX_TEXT_EDITOR_H

#include "../includes.h"


class HexTextEditor
    : public TextEditor
    , public TextEditor::Listener
{
public:
    //==============================================================================
    HexTextEditor(Label *_statusLabel);
    ~HexTextEditor();

    //==============================================================================
    void clear();

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    //==============================================================================
    void setBinary(uint8 *buffer, const int &size);
    void addBinary(uint8 *buffer, const int &size);
    Array<uint8> getBinary(void);

protected:
    String bufferedText;

    //==============================================================================
    // note: this status label has to be placed into the window via setBounds() 
    // from external! See SysexTool.cpp as example
    Label* statusLabel;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    HexTextEditor (const HexTextEditor&);
    const HexTextEditor& operator= (const HexTextEditor&);
};

#endif /* _HEX_TEXT_EDITOR_H */
