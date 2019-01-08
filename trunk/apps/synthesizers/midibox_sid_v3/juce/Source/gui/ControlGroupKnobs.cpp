/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Knobs Control Group
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "ControlGroupKnobs.h"

//==============================================================================
ControlGroupKnobs::ControlGroupKnobs(const String &name)
    : ControlGroup(name, Colour(0xff000000), Colour(0xff808080))
{
    knobArray.clear();

    for(int knob=0; knob<8; ++knob) {
        printf("K%d\n", knob);
        KnobOrange *k = new KnobOrange("CC#"); // TODO: get name from MBSID
        knobArray.add(k);
        addAndMakeVisible(k);
        k->setRange(0, 127, 1);
        k->setValue(64); // TODO
    }
}

ControlGroupKnobs::~ControlGroupKnobs()
{
    deleteAllChildren();
}

//==============================================================================
void ControlGroupKnobs::resized()
{
    ControlGroup::resized();

    int knobOffset = 58;
    int lrSpace = (getWidth() - knobArray.size() * knobOffset) / 2;
    for(int knob=0; knob<knobArray.size(); ++knob)
        knobArray[knob]->setBounds(lrSpace + knob*knobOffset, 30, 48, 48);
}
