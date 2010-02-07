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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "MainComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MainComponent::MainComponent ()
    : resizer (0),
      triggersButton (0),
      tracksButton (0),
      patternsButton (0),
      setupButton (0),
      blmClass (0)
{
    addAndMakeVisible (resizer = new ResizableBorderComponent (this,0));

    addAndMakeVisible (triggersButton = new TextButton (String::empty));
    triggersButton->setButtonText (T("Triggers"));
    triggersButton->addButtonListener (this);
    triggersButton->setColour (TextButton::buttonColourId, Colours::azure);
    triggersButton->setColour (TextButton::buttonOnColourId, Colours::red);

    addAndMakeVisible (tracksButton = new TextButton (String::empty));
    tracksButton->setButtonText (T("Tracks"));
    tracksButton->addButtonListener (this);
    tracksButton->setColour (TextButton::buttonColourId, Colours::azure);
    tracksButton->setColour (TextButton::buttonOnColourId, Colours::red);

    addAndMakeVisible (patternsButton = new TextButton (String::empty));
    patternsButton->setButtonText (T("Patterns"));
    patternsButton->addButtonListener (this);
    patternsButton->setColour (TextButton::buttonColourId, Colours::azure);
    patternsButton->setColour (TextButton::buttonOnColourId, Colours::red);

    addAndMakeVisible (setupButton = new TextButton (String::empty));
    setupButton->setButtonText (T("Setup"));
    setupButton->addButtonListener (this);
    setupButton->setColour (TextButton::buttonColourId, Colour (0xffc5f5f1));
    setupButton->setColour (TextButton::buttonOnColourId, Colour (0xff18b7e8));

    addAndMakeVisible (blmClass = new BlmClass (32,16));
    blmClass->setName (T("blmClass"));


    //[UserPreSize]
	triggersButton->setClickingTogglesState(true);
	tracksButton->setClickingTogglesState(true);
	patternsButton->setClickingTogglesState(true);
	setupWindow=new SetupWindow; // Create setup window but don't display.

    //[/UserPreSize]

    setSize (1024, 300);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MainComponent::~MainComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (resizer);
    deleteAndZero (triggersButton);
    deleteAndZero (tracksButton);
    deleteAndZero (patternsButton);
    deleteAndZero (setupButton);
    deleteAndZero (blmClass);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffdae2f9));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MainComponent::resized()
{
    resizer->setBounds (0, 0, getWidth() - 0, getHeight() - 0);
    triggersButton->setBounds (8, 8, 88, 24);
    tracksButton->setBounds (8, 40, 88, 24);
    patternsButton->setBounds (8, 72, 88, 24);
    setupButton->setBounds (8, getHeight() - 31, 88, 24);
    blmClass->setBounds (104, 5, getWidth() - 109, getHeight() - 10);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MainComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == triggersButton)
    {
        //[UserButtonCode_triggersButton] -- add your button handler code here..
		blmClass->sendCCEvent(0,0x40,(buttonThatWasClicked->getToggleState() ? 0x7f:0x00));
        //[/UserButtonCode_triggersButton]
    }
    else if (buttonThatWasClicked == tracksButton)
    {
        //[UserButtonCode_tracksButton] -- add your button handler code here..
		blmClass->sendCCEvent(0,0x41,(buttonThatWasClicked->getToggleState() ? 0x7f:0x00));
        //[/UserButtonCode_tracksButton]
    }
    else if (buttonThatWasClicked == patternsButton)
    {
        //[UserButtonCode_patternsButton] -- add your button handler code here..
		blmClass->sendCCEvent(0,0x42,(buttonThatWasClicked->getToggleState() ? 0x7f:0x00));
        //[/UserButtonCode_patternsButton]
    }
    else if (buttonThatWasClicked == setupButton)
    {
        //[UserButtonCode_setupButton] -- add your button handler code here..
		DialogWindow::showModalDialog (T("BLM Setup"),
                                   setupWindow,
                                   this,
                                   Colours::azure,
                                   true);
		int rows,cols;
		switch (setupWindow->getBlmSize()) {
		case 2: cols=64; rows=16; break;
		case 3: cols=128; rows=16; break;
		case 4: cols=16; rows=8; break;
		case 5: cols=32; rows=8; break;
		case 6: cols=64; rows=8; break;
		case 7: cols=16; rows=4; break;
		case 8: cols=32; rows=4; break;
		case 9: cols=64; rows=4; break;
		default: cols=32; rows=16; // 1 was selected!
		}
		deleteAndZero(blmClass);
	    addAndMakeVisible (blmClass = new BlmClass (cols,rows));

		if (setupWindow->getMidiInput())
			blmClass->setMidiInput(setupWindow->getMidiInput()-1);
		else
			blmClass->setMidiInput(MidiInput::getDefaultDeviceIndex());

		if (setupWindow->getMidiOutput())
			blmClass->setMidiOutput(setupWindow->getMidiOutput()-1);
		else
			blmClass->setMidiOutput(MidiOutput::getDefaultDeviceIndex());

		resized(); // force call to resized!

        //[/UserButtonCode_setupButton]
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
                 fixedSize="0" initialWidth="1024" initialHeight="300">
  <BACKGROUND backgroundColour="ffdae2f9"/>
  <GENERICCOMPONENT name="" id="9fab54123dfb0c53" memberName="resizer" virtualName=""
                    explicitFocusOrder="0" pos="0 0 0M 0M" class="ResizableBorderComponent"
                    params="this,0"/>
  <TEXTBUTTON name="" id="bcf4f7b0888effe5" memberName="triggersButton" virtualName=""
              explicitFocusOrder="0" pos="8 8 88 24" bgColOff="fff0ffff" bgColOn="ffff0000"
              buttonText="Triggers" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="624a688771700b76" memberName="tracksButton" virtualName=""
              explicitFocusOrder="0" pos="8 40 88 24" bgColOff="fff0ffff" bgColOn="ffff0000"
              buttonText="Tracks" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="68a0577452511158" memberName="patternsButton" virtualName=""
              explicitFocusOrder="0" pos="8 72 88 24" bgColOff="fff0ffff" bgColOn="ffff0000"
              buttonText="Patterns" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="d34028cb0db05000" memberName="setupButton" virtualName=""
              explicitFocusOrder="0" pos="8 31R 88 24" bgColOff="ffc5f5f1"
              bgColOn="ff18b7e8" buttonText="Setup" connectedEdges="0" needsCallback="1"
              radioGroupId="0"/>
  <GENERICCOMPONENT name="blmClass" id="f588e2887ead362d" memberName="blmClass" virtualName=""
                    explicitFocusOrder="0" pos="104 5 109M 10M" class="BlmClass"
                    params="32,16"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
