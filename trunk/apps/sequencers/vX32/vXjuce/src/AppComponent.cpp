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

//[Headers] You can add your own extra header files here...


// this includes all the juce headers..
#include <juce.h>

// We need this so we can find the global stuff like MIDI IO
// so don't remove this

#include <MainComponent.h>


// and the reason for needing these should be pretty obvious ;)

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>


// and these are for the user to have an easy way
// to put all the core32 includes in the emulation
extern "C" {

#include <MIOS32_App_Headers.h>

}



//[/Headers]

#include "AppComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AppComponent::AppComponent ()
    : StartButton (0),
      StopButton (0),
      contButton (0),
      testButton (0)
{
    addAndMakeVisible (StartButton = new TextButton (T("Start Button")));
    StartButton->setButtonText (T("Start"));
    StartButton->setConnectedEdges (Button::ConnectedOnRight);
    StartButton->addButtonListener (this);
    StartButton->setColour (TextButton::buttonColourId, Colour (0xffffe6ba));
    StartButton->setColour (TextButton::buttonOnColourId, Colour (0xfff3ff44));

    addAndMakeVisible (StopButton = new TextButton (T("Stop Button")));
    StopButton->setButtonText (T("Pause"));
    StopButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    StopButton->addButtonListener (this);
    StopButton->setColour (TextButton::buttonColourId, Colour (0xffffbbbb));
    StopButton->setColour (TextButton::buttonOnColourId, Colour (0xffff4444));

    addAndMakeVisible (contButton = new TextButton (T("Continue button")));
    contButton->setButtonText (T("Play"));
    contButton->setConnectedEdges (Button::ConnectedOnLeft);
    contButton->addButtonListener (this);
    contButton->setColour (TextButton::buttonColourId, Colour (0xffbbffbc));
    contButton->setColour (TextButton::buttonOnColourId, Colour (0xff44ff4e));

    addAndMakeVisible (testButton = new TextButton (T("Test button")));
    testButton->setButtonText (T("Create modules"));
    testButton->addButtonListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 300);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

AppComponent::~AppComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (StartButton);
    deleteAndZero (StopButton);
    deleteAndZero (contButton);
    deleteAndZero (testButton);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AppComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AppComponent::resized()
{
    StartButton->setBounds ((getWidth() / 2) + -46 - ((48) / 2), (getHeight() / 2) + -59, 48, 32);
    StopButton->setBounds ((getWidth() / 2) + 46 - ((48) / 2), (getHeight() / 2) + -59, 48, 32);
    contButton->setBounds ((getWidth() / 2) - ((46) / 2), (getHeight() / 2) + -59, 46, 32);
    testButton->setBounds ((getWidth() / 2) - ((144) / 2), (getHeight() / 2) + 21, 144, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AppComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == StartButton)
    {
        //[UserButtonCode_StartButton] -- add your button handler code here..
        UI_Start();
        //[/UserButtonCode_StartButton]
    }
    else if (buttonThatWasClicked == StopButton)
    {
        //[UserButtonCode_StopButton] -- add your button handler code here..
        UI_Stop();
        //[/UserButtonCode_StopButton]
    }
    else if (buttonThatWasClicked == contButton)
    {
        //[UserButtonCode_contButton] -- add your button handler code here..
        UI_Continue();
        //[/UserButtonCode_contButton]
    }
    else if (buttonThatWasClicked == testButton)
    {
        //[UserButtonCode_testButton] -- add your button handler code here..


static unsigned char testmodule1; //FIXME TESTING
static unsigned char testmodule2; //FIXME TESTING
static edge_t *testedge1; //FIXME TESTING
static unsigned char testmodule3; //FIXME TESTING
static unsigned char testmodule4; //FIXME TESTING
static edge_t *testedge2; //FIXME TESTING
static unsigned char testmodule5; //FIXME TESTING
static unsigned char testmodule6; //FIXME TESTING
static edge_t *testedge3; //FIXME TESTING

static edge_t *testedge4; //FIXME TESTING
static edge_t *testedge5; //FIXME TESTING


    testmodule1 = UI_NewModule(MOD_MODULETYPE_SCLK);
    testmodule2 = UI_NewModule(MOD_MODULETYPE_SEQ);

    node[testmodule1].ports[MOD_SCLK_PORT_NUMERATOR] = 4;
    node[testmodule1].process_req++;

    testedge1 = UI_NewCable(testmodule1, MOD_SCLK_PORT_NEXTTICK, testmodule2, MOD_SEQ_PORT_NEXTTICK);



    testmodule3 = UI_NewModule(MOD_MODULETYPE_SCLK);
    testmodule4 = UI_NewModule(MOD_MODULETYPE_SEQ);

    node[testmodule3].ports[MOD_SCLK_PORT_NUMERATOR] = 8;
    node[testmodule3].process_req++;

    testedge2 = UI_NewCable(testmodule3, MOD_SCLK_PORT_NEXTTICK, testmodule4, MOD_SEQ_PORT_NEXTTICK);



    testmodule5 = UI_NewModule(0);
    testmodule6 = UI_NewModule(1);

    testedge3 = UI_NewCable(testmodule5, MOD_SCLK_PORT_NEXTTICK, testmodule6, MOD_SEQ_PORT_NEXTTICK);

    node[testmodule5].ports[MOD_SCLK_PORT_NUMERATOR] = 5;
    node[testmodule5].process_req++;



    testedge4 = UI_NewCable(testmodule2, MOD_SEQ_PORT_CURRENTSTEP, testmodule4, MOD_SEQ_PORT_NOTE0_NOTE);
    testedge5 = UI_NewCable(testmodule4, MOD_SEQ_PORT_CURRENTSTEP, testmodule6, MOD_SEQ_PORT_NOTE0_NOTE);


        //[/UserButtonCode_testButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AppComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="0" initialWidth="600" initialHeight="300">
  <BACKGROUND backgroundColour="0"/>
  <TEXTBUTTON name="Start Button" id="a8b18d1dc20a0a30" memberName="StartButton"
              virtualName="" explicitFocusOrder="0" pos="-46Cc -59C 48 32"
              bgColOff="ffffe6ba" bgColOn="fff3ff44" buttonText="Start" connectedEdges="2"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="Stop Button" id="5e58d7ae4b3b00e2" memberName="StopButton"
              virtualName="" explicitFocusOrder="0" pos="46Cc -59C 48 32" bgColOff="ffffbbbb"
              bgColOn="ffff4444" buttonText="Pause" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="Continue button" id="6749b4d227ced44f" memberName="contButton"
              virtualName="" explicitFocusOrder="0" pos="0Cc -59C 46 32" bgColOff="ffbbffbc"
              bgColOn="ff44ff4e" buttonText="Play" connectedEdges="1" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="Test button" id="bd5f20ad9c4057b2" memberName="testButton"
              virtualName="" explicitFocusOrder="0" pos="0Cc 21C 144 24" buttonText="Create modules"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
