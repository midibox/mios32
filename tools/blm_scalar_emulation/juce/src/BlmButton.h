/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
 * Bitmap based button for BLM
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_BUTTON_H
#define _BLM_BUTTON_H

#include "JuceHeader.h"

class BlmButton
    : public TextButton
{
public:
    //==============================================================================
    BlmButton(const String &name);
    ~BlmButton();

    void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown);

    void setButtonState(const unsigned& newState);
    unsigned getButtonState(void);

protected:
    //==============================================================================
    Image cachedImage_leds_png;

    unsigned buttonState;

    // Binary resources:
    static const char* leds_png;
    static const int leds_pngSize;

    int singleImageWidth;
    int singleImageHeight;
};

#endif /* _BlmButton_H */
