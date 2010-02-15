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
    , maxRowWidth(0)
    , logEntryFont(Typeface::defaultTypefaceNameMono, 13.0, 0)
{
    setModel(this);
    setMultipleSelectionEnabled(true);

    setRowHeight(14);
    setSize(300, 200);
}

LogBox::~LogBox()
{
    logEntries.clear();
    deleteAllChildren();
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
    std::pair<Colour, String> p = logEntries[rowNumber];

    int fontWidth = 6; // DIRTY - how to find out the real width without generating the complete graphic?
    int rowWidth = 30 + fontWidth * p.second.length();
    if( rowWidth > maxRowWidth )
        setMinimumContentWidth(maxRowWidth = rowWidth);

    if( rowIsSelected )
        g.fillAll(Colours::lightblue);

    g.setFont(logEntryFont);

    g.setColour(p.first);
    g.drawText(p.second,
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
    setMinimumContentWidth(maxRowWidth = 1);

    updateContent();
    repaint(); // note: sometimes not updated without repaint()
    setVerticalPosition(2.0); // has to be done after updateContent()!
}

void LogBox::addEntry(const Colour &colour, const String &textLine)
{
    std::pair<Colour, String> p;
    p.first = colour;
    p.second = textLine;
    logEntries.add(p);

    updateContent();
    setVerticalPosition(2.0); // has to be done after updateContent()!
} 


//==============================================================================
void LogBox::copy(void)
{
    String selectedText;

    for(int row=0; row<getNumRows(); ++row)
        if( isRowSelected(row) ) {
            std::pair<Colour, String> p = logEntries[row];

#if JUCE_WIN32
            if( selectedText != String::empty )
                selectedText += T("\r\n");
#else
            if( selectedText != String::empty )
                selectedText += T("\n");
#endif
            selectedText += p.second;
        }

    if( selectedText != String::empty )
        SystemClipboard::copyTextToClipboard(selectedText);
}

void LogBox::cut(void)
{
    for(int row=0; row<getNumRows(); ++row)
        if( isRowSelected(row) )
            logEntries.remove(row);

    repaint(); // note: sometimes not updated without repaint()
    setVerticalPosition(2.0); // has to be done after updateContent()!
}

//==============================================================================
void LogBox::mouseDown(const MouseEvent& e)
{
    listBoxItemClicked(-1, e);
}

void LogBox::listBoxItemClicked(int row, const MouseEvent& e)
{
    if( e.mouseWasClicked() ) {
        if( e.mods.isLeftButtonDown() ) {
            if( row == -1 )
                deselectAllRows();
        } else if( e.mods.isRightButtonDown() ) {
            PopupMenu m;
            addPopupMenuItems(m, &e);

            const int result = m.show();

            if( result != 0 )
                performPopupMenuAction(result);
        }
    }
}


const int baseMenuItemId = 0x7fff0000;

void LogBox::addPopupMenuItems(PopupMenu& m, const MouseEvent*)
{
    m.addItem(baseMenuItemId + 1, TRANS("cut"), true);
    m.addItem(baseMenuItemId + 2, TRANS("copy"), true);
    m.addSeparator();
    m.addItem(baseMenuItemId + 3, TRANS("select all"), true);
    m.addItem(baseMenuItemId + 4, TRANS("delete all"), true);
}

void LogBox::performPopupMenuAction(const int menuItemId)
{
    switch (menuItemId)
    {
    case baseMenuItemId + 1:
        copy();
        cut();
        break;

    case baseMenuItemId + 2:
        copy();
        break;

    case baseMenuItemId + 3:
        for(int row=0; row<getNumRows(); ++row)
            selectRow(row, false, false);
        break;

    case baseMenuItemId + 4:
        clear();
        break;

    default:
        break;
    }
}
