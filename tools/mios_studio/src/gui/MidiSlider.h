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

class MidiSlider
    : public Component
    , public SliderListener
{
public:
    //==============================================================================
    MidiSlider(AudioDeviceManager &_audioDeviceManager);
    ~MidiSlider();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void setFunction(String _functionName, int _functionArg);

    //==============================================================================
    void sliderValueChanged(Slider* sliderThatWasMoved);


private:
    //==============================================================================
    Slider *slider;
    Label *label;

    String functionName;
    int functionArg;

    //==============================================================================
    AudioDeviceManager *audioDeviceManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiSlider (const MidiSlider&);
    const MidiSlider& operator= (const MidiSlider&);
};

#endif /* _MIDI_SLIDER_H */
