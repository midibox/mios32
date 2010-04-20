/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
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

#include "../../includes.h"
#include "Knob.h"

//==============================================================================
Knob::Knob(const String &name, const int &_singleImageWidth, const int &_singleImageHeight, Image *_image)
    : Slider(name)
    , singleImageWidth(_singleImageWidth)
    , singleImageHeight(_singleImageHeight)
    , image(_image)
{
    setSliderStyle(Slider::RotaryVerticalDrag);
    setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    setSize(singleImageWidth, singleImageHeight);
}

Knob::~Knob()
{
    deleteAllChildren();
}

//==============================================================================
void Knob::paint(Graphics& g)
{
    g.fillAll(Colours::white);

    g.setColour(Colours::black);

    jassert( image != 0 );
    if( image ) {
        double range = getMaximum() - getMinimum();
        int numImagePics = image->getWidth() / singleImageWidth;
        int imageOffset = singleImageWidth * (int)((numImagePics-1) * getValue()/range);

        g.drawImage(image,
                    0, 0, singleImageWidth, singleImageHeight,
                    imageOffset, 0, singleImageWidth, singleImageHeight,
                    false);
    }
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
    g.setFont(Font(Typeface::defaultTypefaceNameSans, 13.0, 0));
#else
    g.setFont(Font(Font::getDefaultSansSerifFontName(), 13.0, 0));
#endif
    g.setColour(Colours::white);
    g.drawText(getName(), 0, singleImageHeight-16, singleImageWidth, 16, Justification::verticallyCentred | Justification::horizontallyCentred, true);

}

//==============================================================================
void Knob::setValue(double newValue, const bool sendUpdateMessage, const bool sendMessageSynchronously)
{
    Slider::setValue(newValue, sendUpdateMessage, sendMessageSynchronously);

    // TODO: send SysEx
}
