/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  5 Apr 2009 3:22:42 pm

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

extern "C" {

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>


#include "tasks.h"
#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>
#include <seq_bpm.h>

}


//[/Headers]

#include "MainComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MainComponent::MainComponent ()
    : MIOSJUCELabel (0),
      quitButton (0)
{
    addAndMakeVisible (MIOSJUCELabel = new Label (String::empty,
                                                  T("MIOSJUCE\n\nstryd_one\nlucem")));
    MIOSJUCELabel->setFont (Font (28.4000f, Font::bold));
    MIOSJUCELabel->setJustificationType (Justification::centred);
    MIOSJUCELabel->setEditable (false, false, false);
    MIOSJUCELabel->setColour (Label::textColourId, Colours::white);
    MIOSJUCELabel->setColour (Label::outlineColourId, Colours::black);
    MIOSJUCELabel->setColour (TextEditor::textColourId, Colours::black);
    MIOSJUCELabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (quitButton = new TextButton (String::empty));
    quitButton->setButtonText (T("Quit"));
    quitButton->addButtonListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 300);

    //[Constructor] You can add your own custom stuff here..
    APP_Init();
    //[/Constructor]
}

MainComponent::~MainComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (MIOSJUCELabel);
    deleteAndZero (quitButton);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::black);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainComponent::resized()
{
    MIOSJUCELabel->setBounds (152, 40, 296, 128);
    quitButton->setBounds (getWidth() - 176, getHeight() - 60, 120, 32);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MainComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == quitButton)
    {
        //[UserButtonCode_quitButton] -- add your button handler code here..

        JUCEApplication::quit();

        //[/UserButtonCode_quitButton]
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

<JUCER_COMPONENT documentType="Component" className="MainComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="1" initialWidth="600" initialHeight="300">
  <BACKGROUND backgroundColour="ff000000"/>
  <LABEL name="" id="be4f6f2e5725a063" memberName="MIOSJUCELabel" virtualName=""
         explicitFocusOrder="0" pos="152 40 296 128" textCol="ffffffff"
         outlineCol="ff000000" edTextCol="ff000000" edBkgCol="0" labelText="MIOSJUCE&#10;&#10;stryd_one&#10;lucem"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="28.4" bold="1" italic="0" justification="36"/>
  <TEXTBUTTON name="" id="bcf4f7b0888effe5" memberName="quitButton" virtualName=""
              explicitFocusOrder="0" pos="176R 60R 120 32" buttonText="Quit"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
