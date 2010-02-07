/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  7 Feb 2010 12:17:46 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_MAINCOMPONENT_MAINCOMPONENT_A31C2676__
#define __JUCER_HEADER_MAINCOMPONENT_MAINCOMPONENT_A31C2676__

//[Headers]     -- You can add your own extra header files here --
#include "includes.h"
#include "BlmClass.h"
#include "SetupWindow.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MainComponent  : public Component,
                       public ButtonListener
{
public:
    //==============================================================================
    MainComponent ();
    ~MainComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void toggleTriggersButton(bool value) {triggersButton->setToggleState(value,false);}
	void toggleTracksButton(bool value) {tracksButton->setToggleState(value,false);}
	void togglePatternsButton(bool value) {patternsButton->setToggleState(value,false);}
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);


    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	SetupWindow* setupWindow;
    //[/UserVariables]

    //==============================================================================
    ResizableBorderComponent* resizer;
    TextButton* triggersButton;
    TextButton* tracksButton;
    TextButton* patternsButton;
    TextButton* setupButton;
    BlmClass* blmClass;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MainComponent (const MainComponent&);
    const MainComponent& operator= (const MainComponent&);
};


#endif   // __JUCER_HEADER_MAINCOMPONENT_MAINCOMPONENT_A31C2676__
