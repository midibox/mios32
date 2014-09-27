/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
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

#ifndef _CONTROL_GROUP_KNOBS_H
#define _CONTROL_GROUP_KNOBS_H

#include "components/ControlGroup.h"
#include "components/KnobOrange.h"

class ControlGroupKnobs
    : public ControlGroup
{
public:
    //==============================================================================
    ControlGroupKnobs(const String &name);
    ~ControlGroupKnobs();

    void resized();

protected:
    //==============================================================================
    Array<KnobOrange*> knobArray;

};

#endif /* _CONTROL_GROUP_KNOBS_H */
