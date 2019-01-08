/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  12 Apr 2009 10:44:22 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.11

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_APPCOMPONENT_APPCOMPONENT_1B34A945__
#define __JUCER_HEADER_APPCOMPONENT_APPCOMPONENT_1B34A945__

//[Headers]     -- You can add your own extra header files here --
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AppComponent  : public Component,
                      public ButtonListener
{
public:
    //==============================================================================
    AppComponent ();
    ~AppComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);


    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    TextButton* StartButton;
    TextButton* StopButton;
    TextButton* contButton;
    TextButton* testButton;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    AppComponent (const AppComponent&);
    const AppComponent& operator= (const AppComponent&);
};


#endif   // __JUCER_HEADER_APPCOMPONENT_APPCOMPONENT_1B34A945__
