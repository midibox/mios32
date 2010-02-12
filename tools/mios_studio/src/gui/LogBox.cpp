/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Log Box (an optimized Log window)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "LogBox.h"


//==============================================================================
LogBox::LogBox(const String &componentName)
    : ListBox(componentName, 0)
    , logEntryFont(T("Helvetica Neue"), 14.0, 0)
{
    setModel(this);
    setMultipleSelectionEnabled(true);

    setRowHeight(14);
    setSize(300, 200);
}

LogBox::~LogBox()
{
}

//==============================================================================
// The following methods implement the necessary virtual functions from ListBoxModel,
// telling the listbox how many rows there are, painting them, etc.
int LogBox::getNumRows()
{
    return logEntries.size();
}

void LogBox::paintListBoxItem(int rowNumber,
                              Graphics& g,
                              int width, int height,
                              bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);

    g.setColour(Colours::black);
    g.setFont(logEntryFont);

    g.drawText(logEntries[rowNumber],
               5, 0, width, height,
               Justification::centredLeft, true);
}

//==============================================================================
void LogBox::paintOverChildren(Graphics& g)
{
    ListBox::paintOverChildren(g);

    // let it look like a text editor (I like the shadow... :-)

    // from getLookAndFeel().drawTextEditorOutline(g, getWidth(), getHeight(), *this);

    g.setColour(findColour(0x1000205)); // TextEditor::outlineColourId
    g.drawRect(0, 0, getWidth(), getHeight());

    g.setOpacity(1.0f);
    const Colour shadowColour(findColour(0x1000207)); // TextEditor::shadowColourId
    g.drawBevel(0, 0, getWidth(), getHeight() + 2, 3, shadowColour, shadowColour);
}

//==============================================================================
void LogBox::clear(void)
{
    logEntries.clear();

    updateContent();
    repaint(); // note: sometimes not updated without repaint()
    setVerticalPosition(1.0); // has to be done after updateContent()!
}

void LogBox::addEntry(String textLine)
{
    logEntries.add(textLine);

    updateContent();
    repaint(); // note: sometimes not updated without repaint()
    setVerticalPosition(1.0); // has to be done after updateContent()!
}
