/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  6 Feb 2010 1:29:13 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_SETUPWINDOW_SETUPWINDOW_86F66210__
#define __JUCER_HEADER_SETUPWINDOW_SETUPWINDOW_86F66210__

//[Headers]     -- You can add your own extra header files here --
#include "juce.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SetupWindow  : public Component,
                     public ButtonListener,
                     public ComboBoxListener
{
public:
    //==============================================================================
    SetupWindow ();
    ~SetupWindow();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	int getBlmSize(void) {return blmSize->getSelectedId();}
	int getMidiInput(void) {return midiInput->getSelectedId();}
	int getMidiOutput(void) {return midiOutput->getSelectedId();}
	//[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);


    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    TextButton* okButton;
    TextButton* cancelButton;
    ComboBox* midiInput;
    Label* label;
    ComboBox* midiOutput;
    Label* label3;
    ComboBox* blmSize;
    Label* label4;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    SetupWindow (const SetupWindow&);
    const SetupWindow& operator= (const SetupWindow&);
};


#endif   // __JUCER_HEADER_SETUPWINDOW_SETUPWINDOW_86F66210__
