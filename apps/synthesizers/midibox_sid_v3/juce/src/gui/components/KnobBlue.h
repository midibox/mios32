/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#ifndef _KNOB_BLUE_H
#define _KNOB_BLUE_H

#include "Knob.h"

class KnobBlue
    : public Knob
{
public:
    //==============================================================================
    KnobBlue(const String &name);
    ~KnobBlue();

    // Binary resources:
    static const char* knob_blue_png;
    static const int knob_blue_pngSize;

protected:
};

#endif /* _KNOB_BLUE_H */
