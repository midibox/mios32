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

#ifndef _LOG_BOX_H
#define _LOG_BOX_H

#include "../includes.h"

#include <utility>

class LogBox
    : public ListBox
    , public ListBoxModel
{
public:
    //==============================================================================
    LogBox(const String &componentName);
    ~LogBox();


    //==============================================================================
    int getNumRows();
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected);

    //==============================================================================
    void paintOverChildren(Graphics& g);

    //==============================================================================
    void clear(void);
    void addEntry(const Colour &colour, const String &textLine);

    //==============================================================================
    void copy(void);
    void cut(void);

    //==============================================================================
    void mouseDown(const MouseEvent& e);
    void listBoxItemClicked(int row, const MouseEvent& e);

    void addPopupMenuItems(PopupMenu& m, const MouseEvent*);
    void performPopupMenuAction(const int menuItemId);

protected:
    Font logEntryFont;

    Array<std::pair<Colour, String> > logEntries;

    int maxRowWidth;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    LogBox (const LogBox&);
    const LogBox& operator= (const LogBox&);
};

#endif /* _LOG_BOX_H */
