/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Audio Processing Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef EDITOR_COMPONENT_H
#define EDITOR_COMPONENT_H

#include "AudioProcessing.h"
#include "CLCDView.h"

//==============================================================================
/**
   This is the Component that our sidEmu will use as its UI.
 
   One or more of these is created by the AudioProcessing::createEditor() method,
   and they will be deleted at some later time by the wrapper code.
 
   To demonstrate the correct way of connecting a sidEmu to its UI, this
   class is a ChangeListener, and our SidEmu is a ChangeBroadcaster. The
   editor component registers with the SidEmu when it's created and deregisters
   when it's destroyed. When the SidEmu's parameters are changed, it broadcasts
   a message and this editor responds by updating its display.
*/
class EditorComponent   : public AudioProcessorEditor,
                          public ChangeListener,
                          public SliderListener,
                          public ComboBoxListener
{
public:
    /** Constructor.
   
    When created, this will register itself with the SidEmu for changes. It's
    safe to assume that the SidEmu won't be deleted before this object is.
    */
    EditorComponent (AudioProcessing* const ownerSidEmu);
  
    /** Destructor. */
    ~EditorComponent();
  
    //==============================================================================
    /** Our demo SidEmu is a ChangeBroadcaster, and will call us back when one of
        its parameters changes.
    */
    void changeListenerCallback (void* source);
  
    void sliderValueChanged (Slider*);
    void comboBoxChanged (ComboBox*);

    //==============================================================================
    /** Standard Juce paint callback. */
    void paint (Graphics& g);
  
    /** Standard Juce resize callback. */
    void resized();
  
	// CLCD
	CLcdView *clcdView;

  
private:
    //==============================================================================
    Slider* gainSlider;
    ComboBox* patchComboBox;
    MidiKeyboardComponent* midiKeyboard;
    Label* infoLabel;
    ResizableCornerComponent* resizer;
    ComponentBoundsConstrainer resizeLimits;
    TooltipWindow tooltipWindow;
  
    void updateParametersFromSidEmu();
  
    // handy wrapper method to avoid having to cast the SidEmu to a AudioProcessing
    // every time we need it..
    AudioProcessing* getSidEmu() const throw()       { return (AudioProcessing*) getAudioProcessor(); }
};


#endif
