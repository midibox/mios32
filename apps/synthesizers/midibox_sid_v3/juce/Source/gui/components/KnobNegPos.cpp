/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#include "KnobNegPos.h"

//==============================================================================
KnobNegPos::KnobNegPos(const String &name)
    : Knob(name, 48, 48, ImageCache::getFromMemory(BinaryData::knob_negpos_png, BinaryData::knob_negpos_pngSize))
{
}

KnobNegPos::~KnobNegPos()
{
}
