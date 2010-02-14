/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#ifndef _KNOB_ORANGE_H
#define _KNOB_ORANGE_H

#include "Knob.h"

class KnobOrange
    : public Knob
{
public:
    //==============================================================================
    KnobOrange(const String &name);
    ~KnobOrange();

    // Binary resources:
    static const char* knob_orange_png;
    static const int knob_orange_pngSize;

protected:
};

#endif /* _KNOB_ORANGE_H */
