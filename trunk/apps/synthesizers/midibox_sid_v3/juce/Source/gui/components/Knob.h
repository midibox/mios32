/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
 * Prototype of a Knob
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _KNOB_H
#define _KNOB_H

#include <JuceHeader.h>

class Knob
    : public Slider
{
public:
    //==============================================================================
    Knob(const String &name, const int &_singleImageWidth, const int &_singleImageHeight, Image _image);
    ~Knob();

    void paint(Graphics& g);

    void setValue(double newValue, NotificationType notification=sendNotificationAsync);

protected:
    //==============================================================================
    Image image;
    int singleImageWidth;
    int singleImageHeight;
};

#endif /* _KNOB_H */
