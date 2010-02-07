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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "setupWindow.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SetupWindow::SetupWindow ()
    : okButton (0),
      cancelButton (0),
      midiInput (0),
      label (0),
      midiOutput (0),
      label3 (0),
      blmSize (0),
      label4 (0)
{
    addAndMakeVisible (okButton = new TextButton (T("OK")));
    okButton->addButtonListener (this);

    addAndMakeVisible (cancelButton = new TextButton (T("Cancel")));
    cancelButton->addButtonListener (this);

    addAndMakeVisible (midiInput = new ComboBox (T("new combo box")));
    midiInput->setEditableText (false);
    midiInput->setJustificationType (Justification::centredLeft);
    midiInput->setTextWhenNothingSelected (String::empty);
    midiInput->setTextWhenNoChoicesAvailable (T("(no choices)"));
    midiInput->addListener (this);

    addAndMakeVisible (label = new Label (T("new label"),
                                          T("MIDI IN:")));
    label->setFont (Font (23.0000f, Font::plain));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (midiOutput = new ComboBox (T("new combo box")));
    midiOutput->setEditableText (false);
    midiOutput->setJustificationType (Justification::centredLeft);
    midiOutput->setTextWhenNothingSelected (String::empty);
    midiOutput->setTextWhenNoChoicesAvailable (T("(no choices)"));
    midiOutput->addListener (this);

    addAndMakeVisible (label3 = new Label (T("new label"),
                                           T("MIDI OUT:")));
    label3->setFont (Font (23.0000f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (blmSize = new ComboBox (T("new combo box")));
    blmSize->setEditableText (false);
    blmSize->setJustificationType (Justification::centredLeft);
    blmSize->setTextWhenNothingSelected (T("Please Select Size"));
    blmSize->setTextWhenNoChoicesAvailable (T("(no choices)"));
    blmSize->addItem (T("Columns: 32, Rows 16"), 1);
    blmSize->addItem (T("Columns: 64, Rows 16"), 2);
    blmSize->addItem (T("Columns: 128, Rows 16"), 3);
    blmSize->addItem (T("Columns: 16, Rows 8"), 4);
    blmSize->addItem (T("Columns: 32, Rows 8"), 5);
    blmSize->addItem (T("Columns: 64, Rows 8"), 6);
    blmSize->addItem (T("Columns: 16, Rows 4"), 7);
    blmSize->addItem (T("Columns: 32, Rows 4"), 8);
    blmSize->addItem (T("Columns: 64, Rows 4"), 9);
    blmSize->addSeparator();
    blmSize->addListener (this);

    addAndMakeVisible (label4 = new Label (T("new label"),
                                           T("Size of BLM")));
    label4->setFont (Font (23.0000f, Font::plain));
    label4->setJustificationType (Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x0));


    //[UserPreSize]
	const StringArray midiOuts (MidiOutput::getDevices());

	midiOutput->addItem (TRANS("<< none >>"), -1);
	midiOutput->addSeparator();
	int current = -1;
	for (int i = 0; i < midiOuts.size(); ++i) {
		midiOutput->addItem (midiOuts[i], i + 1);
		/*if( midiOuts[i] == ownerSidEmu->lastSysexMidiOut ) {
			current = i+1;
			ownerSidEmu->midiProcessing.audioDeviceManager.setDefaultMidiOutput(midiOuts[i]);
		}*/
	}
	midiOutput->setSelectedId(current, true);

	const StringArray midiIns (MidiInput::getDevices());

	midiInput->addItem (TRANS("<< none >>"), -1);
	midiInput->addSeparator();

	current = -1;
	for (int i = 0; i < midiIns.size(); ++i) {
		midiInput->addItem (midiIns[i], i + 1);
		bool enabled = false;
		/*if( midiIns[i] == ownerSidEmu->lastSysexMidiIn ) {
			enabled = true;
			current = i+1;
		}
		ownerSidEmu->midiProcessing.audioDeviceManager.setMidiInputEnabled (midiIns[i], enabled);
	*/
	}
	midiInput->setSelectedId(current, true);

	

    //[/UserPreSize]

    setSize (300, 400);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

SetupWindow::~SetupWindow()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (okButton);
    deleteAndZero (cancelButton);
    deleteAndZero (midiInput);
    deleteAndZero (label);
    deleteAndZero (midiOutput);
    deleteAndZero (label3);
    deleteAndZero (blmSize);
    deleteAndZero (label4);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SetupWindow::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::beige);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SetupWindow::resized()
{
    okButton->setBounds (16, 360, 103, 24);
    cancelButton->setBounds (176, 360, 103, 24);
    midiInput->setBounds (104, 24, 184, 24);
    label->setBounds (16, 16, 80, 40);
    midiOutput->setBounds (104, 72, 184, 24);
    label3->setBounds (16, 64, 80, 40);
    blmSize->setBounds (16, 152, 272, 24);
    label4->setBounds (16, 112, 208, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SetupWindow::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == okButton)
    {
        //[UserButtonCode_okButton] -- add your button handler code here..
        //[/UserButtonCode_okButton]
    }
    else if (buttonThatWasClicked == cancelButton)
    {
        //[UserButtonCode_cancelButton] -- add your button handler code here..
        //[/UserButtonCode_cancelButton]
    }

    //[UserbuttonClicked_Post]
	// Whatever happens we will close the window.
	findParentComponentOfClass ((DialogWindow*) 0)->exitModalState(0);
    //[/UserbuttonClicked_Post]
}

void SetupWindow::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == midiInput)
    {
        //[UserComboBoxCode_midiInput] -- add your combo box handling code here..
        //[/UserComboBoxCode_midiInput]
    }
    else if (comboBoxThatHasChanged == midiOutput)
    {
        //[UserComboBoxCode_midiOutput] -- add your combo box handling code here..
        //[/UserComboBoxCode_midiOutput]
    }
    else if (comboBoxThatHasChanged == blmSize)
    {
        //[UserComboBoxCode_blmSize] -- add your combo box handling code here..
        //[/UserComboBoxCode_blmSize]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SetupWindow" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="1" initialWidth="300" initialHeight="400">
  <BACKGROUND backgroundColour="fff5f5dc"/>
  <TEXTBUTTON name="OK" id="83fcaba94ba53277" memberName="okButton" virtualName=""
              explicitFocusOrder="0" pos="16 360 103 24" buttonText="OK" connectedEdges="0"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="Cancel" id="21f50d46a9d84e54" memberName="cancelButton"
              virtualName="" explicitFocusOrder="0" pos="176 360 103 24" buttonText="Cancel"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <COMBOBOX name="new combo box" id="978f0cf6b8342d3d" memberName="midiInput"
            virtualName="" explicitFocusOrder="0" pos="104 24 184 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="483cc57031801181" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="16 16 80 40" edTextCol="ff000000"
         edBkgCol="0" labelText="MIDI IN:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="23"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="new combo box" id="1e6204cd4e862587" memberName="midiOutput"
            virtualName="" explicitFocusOrder="0" pos="104 72 184 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="a1301391b32a697c" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="16 64 80 40" edTextCol="ff000000"
         edBkgCol="0" labelText="MIDI OUT:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="23"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="new combo box" id="a89cfac5a4c9e838" memberName="blmSize"
            virtualName="" explicitFocusOrder="0" pos="16 152 272 24" editable="0"
            layout="33" items="Columns: 32, Rows 16&#10;Columns: 64, Rows 16&#10;Columns: 128, Rows 16&#10;Columns: 16, Rows 8&#10;Columns: 32, Rows 8&#10;Columns: 64, Rows 8&#10;Columns: 16, Rows 4&#10;Columns: 32, Rows 4&#10;Columns: 64, Rows 4&#10;&#10;"
            textWhenNonSelected="Please Select Size" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="ced6e6f8e98ab45e" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="16 112 208 40" edTextCol="ff000000"
         edBkgCol="0" labelText="Size of BLM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="23" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
