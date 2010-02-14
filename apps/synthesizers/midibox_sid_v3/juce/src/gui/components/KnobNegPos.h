/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#ifndef _KNOB_NEGPOS_H
#define _KNOB_NEGPOS_H

#include "Knob.h"

class KnobNegPos
    : public Knob
{
public:
    //==============================================================================
    KnobNegPos(const String &name);
    ~KnobNegPos();

    // Binary resources:
    static const char* knob_negpos_png;
    static const int knob_negpos_pngSize;

protected:
};

#endif /* _KNOB_NEGPOS_H */
