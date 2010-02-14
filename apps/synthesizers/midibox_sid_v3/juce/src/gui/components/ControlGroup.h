/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
 * Prototype of a Control Group
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _CONTROL_GROUP_H
#define _CONTROL_GROUP_H


class ControlGroup
    : public Component
{
public:
    //==============================================================================
    ControlGroup(const String &name, const Colour& _headerTextColour, const Colour& _headerBackgroundColour);
    ~ControlGroup();

    void paint(Graphics& g);
    void resized();

protected:
    Label *headerLabel;
};

#endif /* _CONTROL_GROUP_H */
