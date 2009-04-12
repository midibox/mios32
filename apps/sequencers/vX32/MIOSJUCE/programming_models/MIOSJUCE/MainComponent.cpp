/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  12 Apr 2009 10:44:35 pm

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


// This is the stuff for the emulated main() and the app itself
extern "C" {
    #include "mios32_main.h"
}

#include <AppComponent.h>


//[/Headers]

#include "MainComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

MainComponent* contentComponent_p;

//[/MiscUserDefs]

//==============================================================================
MainComponent::MainComponent ()
    : UserApp (0),
      MIOSJUCELabel (0),
      credits (0),
      quitButton (0),
      ioButton (0)
{
    addAndMakeVisible (UserApp = new AppComponent());
    UserApp->setExplicitFocusOrder (1);
    addAndMakeVisible (MIOSJUCELabel = new Label (String::empty,
                                                  T("MIOSJUCE")));
    MIOSJUCELabel->setFont (Font (29.5000f, Font::bold));
    MIOSJUCELabel->setJustificationType (Justification::centredBottom);
    MIOSJUCELabel->setEditable (false, false, false);
    MIOSJUCELabel->setColour (Label::textColourId, Colours::black);
    MIOSJUCELabel->setColour (TextEditor::textColourId, Colours::black);
    MIOSJUCELabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (credits = new Label (T("credits label"),
                                            T("by stryd_one\nThanks lucem and TK!")));
    credits->setTooltip (T("TK codes faster than you"));
    credits->setFont (Font (11.2000f, Font::bold));
    credits->setJustificationType (Justification::centred);
    credits->setEditable (false, false, false);
    credits->setColour (Label::textColourId, Colours::black);
    credits->setColour (TextEditor::textColourId, Colours::black);
    credits->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (quitButton = new TextButton (String::empty));
    quitButton->setExplicitFocusOrder (3);
    quitButton->setButtonText (T("Quit"));
    quitButton->setConnectedEdges (Button::ConnectedOnRight | Button::ConnectedOnBottom);
    quitButton->addButtonListener (this);

    addAndMakeVisible (ioButton = new TextButton (T("IO Button")));
    ioButton->setExplicitFocusOrder (2);
    ioButton->setButtonText (T("MIDI Devices"));
    ioButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnBottom);
    ioButton->addButtonListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 300);

    //[Constructor] You can add your own custom stuff here..

    contentComponent_p = this;

    MIOS32_Main();

    addMouseListener(this, true);

    //[/Constructor]
}

MainComponent::~MainComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (UserApp);
    deleteAndZero (MIOSJUCELabel);
    deleteAndZero (credits);
    deleteAndZero (quitButton);
    deleteAndZero (ioButton);

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
    UserApp->setBounds ((getWidth() / 2) - ((getWidth() - 0) / 2), 0, getWidth() - 0, getHeight() - 23);
    MIOSJUCELabel->setBounds ((getWidth() / 2) - ((proportionOfWidth (0.1836f)) / 2), getHeight() - 39, proportionOfWidth (0.1836f), 39);
    credits->setBounds ((getWidth() / 2) - ((144) / 2), getHeight() - 24, 144, 24);
    quitButton->setBounds (getWidth() - 120, getHeight() - 24, 120, 24);
    ioButton->setBounds (0, getHeight() - 24, 120, 24);
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
    else if (buttonThatWasClicked == ioButton)
    {
        //[UserButtonCode_ioButton] -- add your button handler code here..
        AudioDeviceSelectorComponent audioSettingsComp (midiDeviceManager,
                                                0, 0,
                                                0, 0,
                                                true,
                                                true,
                                                false,
                                                false);

        // ...and show it in a DialogWindow...
        audioSettingsComp.setSize (400, 200);

        MIDIDialog::showModalDialog (T("MIDI Settings"),
                                       &audioSettingsComp,
                                       this,
                                       Colours::azure,
                                       true);
        //[/UserButtonCode_ioButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void MainComponent::parentSizeChanged()
{
    //[UserCode_parentSizeChanged] -- Add your code here...
    //[/UserCode_parentSizeChanged]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void MainComponent::mouseMove(const MouseEvent &e)
{

    if (e.y < ((getBounds().getBottom())-50))
    {
        MIOSJUCELabel->setVisible(false);
        credits->setVisible(false);
        quitButton->setVisible(false);
        ioButton->setVisible(false);
    }
    else
    {
        MIOSJUCELabel->setVisible(true);
        credits->setVisible(true);
        quitButton->setVisible(true);
        ioButton->setVisible(true);
    }

}




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
             virtualName="" explicitFocusOrder="1" pos="0Cc 0 0M 23M" sourceFile="../../Application Skeleton/AppComponent.cpp"
             constructorParams=""/>
  <LABEL name="" id="be4f6f2e5725a063" memberName="MIOSJUCELabel" virtualName=""
         explicitFocusOrder="0" pos="0Cc 0Rr 18.362% 39" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="MIOSJUCE" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="29.5" bold="1" italic="0" justification="20"/>
  <LABEL name="credits label" id="bf8adb21dcd75d9f" memberName="credits"
         virtualName="" explicitFocusOrder="0" pos="0Cc 0Rr 144 24" tooltip="TK codes faster than you"
         textCol="ff000000" edTextCol="ff000000" edBkgCol="0" labelText="by stryd_one&#10;Thanks lucem and TK!"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="11.2" bold="1" italic="0" justification="36"/>
  <TEXTBUTTON name="" id="bcf4f7b0888effe5" memberName="quitButton" virtualName=""
              explicitFocusOrder="3" pos="0Rr 0Rr 120 24" buttonText="Quit"
              connectedEdges="10" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="IO Button" id="12f8fc5ccc3dac90" memberName="ioButton"
              virtualName="" explicitFocusOrder="2" pos="0 0Rr 120 24" buttonText="MIDI Devices"
              connectedEdges="9" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
