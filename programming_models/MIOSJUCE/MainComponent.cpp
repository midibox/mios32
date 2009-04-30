/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  13 Apr 2009 6:46:25 am

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.11

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...


// This file lets us set up any special config that we need for this app..
#include "juce_AppConfig.h"
// And this includes all the juce headers..
#include <juce.h>
// This is the stuff for the emulated main()
extern "C" {
    #include "mios32_main.h"
}
// and the app itself
#include <AppComponent.h>


//[/Headers]

#include "MainComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MainComponent::MainComponent ()
    : UserApp (0)
{
    addAndMakeVisible (UserApp = new AppComponent());
    UserApp->setExplicitFocusOrder (1);

    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 300);

    //[Constructor] You can add your own custom stuff here..

    // crank up the emulated MIOS32 main()
    MIOS32_Main();

    //[/Constructor]
}

MainComponent::~MainComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (UserApp);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::grey);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainComponent::resized()
{
    UserApp->setBounds ((getWidth() / 2) - ((getWidth() - 0) / 2), 0, getWidth() - 0, getHeight() - 0);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MainComponent::parentSizeChanged()
{
    //[UserCode_parentSizeChanged] -- Add your code here...
    //[/UserCode_parentSizeChanged]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MainComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="0" initialWidth="600" initialHeight="300">
  <METHODS>
    <METHOD name="parentSizeChanged()"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff808080"/>
  <JUCERCOMP name="Application" id="b9071d634967815e" memberName="UserApp"
             virtualName="" explicitFocusOrder="1" pos="0Cc 0 0M 0M" sourceFile="../../Application Skeleton/AppComponent.cpp"
             constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
