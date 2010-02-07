/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDI Slider
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDI_SLIDER_H
#define _MIDI_SLIDER_H

#include "../includes.h"
#include <vector>

class MiosStudio; // forward declaration

class MidiSlider
    : public Component
    , public SliderListener
{
public:
    //==============================================================================
    MidiSlider(MiosStudio *_miosStudio, String _functionName, int _functionArg, int _midiChannel, int initialValue);
    ~MidiSlider();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void setFunction(String _functionName, int _functionArg, int _midiChannel, int initialValue);

    //==============================================================================
    void sliderValueChanged(Slider* sliderThatWasMoved);


protected:
    //==============================================================================
    Slider *slider;
    Label *label;

    String functionName;
    int functionArg;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    int midiChannel;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiSlider (const MidiSlider&);
    const MidiSlider& operator= (const MidiSlider&);
};

#endif /* _MIDI_SLIDER_H */
