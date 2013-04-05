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

#include "CommandLineEditor.h"


//==============================================================================
CommandLineEditor::CommandLineEditor(const unsigned _maxLines)
    : TextEditor(String::empty)
    , maxLines(_maxLines)
    , positionInList(0)
{
    setMultiLine(false);
    setReturnKeyStartsNewLine(false);
    setReadOnly(false);
    setScrollbarsShown(false);
    setCaretVisible(true);
    setPopupMenuEnabled(true);

	addListener(this);
}

CommandLineEditor::~CommandLineEditor()
{
}

//==============================================================================
bool CommandLineEditor::keyPressed(const KeyPress& key)
{
    if( key == KeyPress::returnKey ) {
        String command(getText());

        if( command.length() ) {
            // add command if it's not equal to last command in list
            if( !commandList.size() || commandList[commandList.size()-1].compare(command) != 0 ) {
                commandList.add(command);
                while( commandList.size() >= maxLines ) {
                    commandList.remove(0);
                }
            }
        }

        positionInList = commandList.size() ? (commandList.size() - 1) : 0;
    } else if( key == KeyPress::upKey ) {
        if( commandList.size() && positionInList > 0 ) {
            String command(getText());

            if( command.length() &&
                positionInList >= (commandList.size()-1) &&
                commandList[commandList.size()-1].compare(command) != 0 ) {

                commandList.add(command);
                while( commandList.size() >= maxLines ) {
                    commandList.remove(0);
                }

                positionInList = commandList.size()-2;
                setText(commandList[positionInList]);
            } else {
                if( !command.length() || positionInList >= commandList.size() ) {
                    positionInList = commandList.size()-1;
                } else {
                    --positionInList;
                }
                setText(commandList[positionInList]);
            }
        }

        return true;
    } else if( key == KeyPress::downKey ) {
        if( commandList.size() ) {
            if( ++positionInList >= commandList.size() ) {
                positionInList = commandList.size();
                setText(String::empty);
            } else {
                setText(commandList[positionInList]);
            }
        }

        return true;
    }

    return TextEditor::keyPressed(key);
}


//==============================================================================
void CommandLineEditor::textEditorTextChanged(TextEditor &editor)
{
}

void CommandLineEditor::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void CommandLineEditor::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void CommandLineEditor::textEditorFocusLost(TextEditor &editor)
{
}
