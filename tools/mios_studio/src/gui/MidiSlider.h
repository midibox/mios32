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


class MidiSliderComponent
    : public Slider
{
public:
    MidiSliderComponent(const String &componentName);
    ~MidiSliderComponent();
    void mouseUp(const MouseEvent& e);

    bool snapsBackOnRelease;

};



class MidiSlider
    : public Component
    , public SliderListener
    , public ComboBoxListener
{
public:
    //==============================================================================
    MidiSlider(MiosStudio *_miosStudio, int _num, String _functionName, int _functionArg, int _midiChannel, int initialValue, bool vertical);
    ~MidiSlider();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void setFunction(const String &_functionName, const int &_functionArg, const int &_midiChannel, const int &initialValue);
    String getFunctionName(void);
    int getFunctionArg(void);
    int getMidiChannel(void);
    uint8 getInitialValue(void);

    //==============================================================================
    void sliderValueChanged(Slider* sliderThatWasMoved);
    void comboBoxChanged(ComboBox *comboBoxThatHasChanged);


protected:
    //==============================================================================
    MidiSliderComponent *slider;
    ComboBox* sliderFunction; // only used for horizontal sliders

    int sliderNum;
    bool vertical;

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
