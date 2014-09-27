/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#include "KnobOrange.h"

//==============================================================================
KnobOrange::KnobOrange(const String &name)
    : Knob(name, 48, 48, ImageCache::getFromMemory(BinaryData::knob_orange_png, BinaryData::knob_orange_pngSize))
{
}

KnobOrange::~KnobOrange()
{
}
