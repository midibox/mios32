/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  7 Feb 2010 7:52:20 pm

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
      blmClass (0),
      midiInput (0),
      midiOutput (0),
      blmSize (0),
      label (0),
      label2 (0),
      label3 (0)
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

    addAndMakeVisible (blmClass = new BlmClass (32,16));
    blmClass->setName (T("blmClass"));

    addAndMakeVisible (midiInput = new ComboBox (T("new combo box")));
    midiInput->setEditableText (false);
    midiInput->setJustificationType (Justification::centredLeft);
    midiInput->setTextWhenNothingSelected (String::empty);
    midiInput->setTextWhenNoChoicesAvailable (T("(no choices)"));
    midiInput->addListener (this);

    addAndMakeVisible (midiOutput = new ComboBox (T("new combo box")));
    midiOutput->setEditableText (false);
    midiOutput->setJustificationType (Justification::centredLeft);
    midiOutput->setTextWhenNothingSelected (String::empty);
    midiOutput->setTextWhenNoChoicesAvailable (T("(no choices)"));
    midiOutput->addListener (this);

    addAndMakeVisible (blmSize = new ComboBox (T("new combo box")));
    blmSize->setEditableText (false);
    blmSize->setJustificationType (Justification::centredLeft);
    blmSize->setTextWhenNothingSelected (T("32x16"));
    blmSize->setTextWhenNoChoicesAvailable (T("(no choices)"));
    blmSize->addItem (T("32x16"), 1);
    blmSize->addItem (T("64x16"), 2);
    blmSize->addItem (T("128x16"), 3);
    blmSize->addItem (T("16x8"), 4);
    blmSize->addItem (T("32x8"), 5);
    blmSize->addItem (T("64x8"), 6);
    blmSize->addItem (T("16x4"), 7);
    blmSize->addItem (T("32x4"), 8);
    blmSize->addItem (T("64x4"), 9);
    blmSize->addListener (this);

    addAndMakeVisible (label = new Label (T("new label"),
                                          T("Midi In:")));
    label->setFont (Font (11.0000f, Font::plain));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label2 = new Label (T("new label"),
                                           T("Midi Out:")));
    label2->setFont (Font (11.0000f, Font::plain));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label3 = new Label (T("new label"),
                                           T("Size:")));
    label3->setFont (Font (11.0000f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x0));


    //[UserPreSize]
	const StringArray midiOuts (MidiOutput::getDevices());

	midiOutput->addItem (TRANS("<< none >>"), -1);
	midiOutput->addSeparator();
	int current = -1;
	for (int i = 0; i < midiOuts.size(); ++i) {
		midiOutput->addItem (midiOuts[i], i + 1);
	}
	midiOutput->setSelectedId(current, true);

	const StringArray midiIns (MidiInput::getDevices());

	midiInput->addItem (TRANS("<< none >>"), -1);
	midiInput->addSeparator();

	current = -1;
	for (int i = 0; i < midiIns.size(); ++i) {
		midiInput->addItem (midiIns[i], i + 1);
		bool enabled = false;
	}
	midiInput->setSelectedId(current, true);

    //[/UserPreSize]

    setSize (1024, 300);

    //[Constructor] You can add your own custom stuff here..
    setSize(117+(blmClass->getBlmColumns()*blmClass->getLedSize()),10+(blmClass->getBlmRows()*blmClass->getLedSize()));
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
    deleteAndZero (blmClass);
    deleteAndZero (midiInput);
    deleteAndZero (midiOutput);
    deleteAndZero (blmSize);
    deleteAndZero (label);
    deleteAndZero (label2);
    deleteAndZero (label3);
	deleteAllChildren();
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
    blmClass->setBounds (112, 5, getWidth() - 117, getHeight() - 10);
    midiInput->setBounds (8, 112, 88, 24);
    midiOutput->setBounds (8, 152, 88, 24);
    blmSize->setBounds (8, 192, 88, 24);
    label->setBounds (8, 96, 48, 16);
    label2->setBounds (8, 136, 48, 16);
    label3->setBounds (8, 176, 48, 16);
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
		blmClass->sendCCEvent(0,0x40, 0x7f);
        //[/UserButtonCode_triggersButton]
    }
    else if (buttonThatWasClicked == tracksButton)
    {
        //[UserButtonCode_tracksButton] -- add your button handler code here..
		blmClass->sendCCEvent(0,0x41,0x7f);
        //[/UserButtonCode_tracksButton]
    }
    else if (buttonThatWasClicked == patternsButton)
    {
        //[UserButtonCode_patternsButton] -- add your button handler code here..
		blmClass->sendCCEvent(0,0x42,0x7f);
        //[/UserButtonCode_patternsButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void MainComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == midiInput)
    {
        //[UserComboBoxCode_midiInput] -- add your combo box handling code here..
		blmClass->setMidiInput(midiInput->getText());
        //[/UserComboBoxCode_midiInput]
    }
    else if (comboBoxThatHasChanged == midiOutput)
    {
        //[UserComboBoxCode_midiOutput] -- add your combo box handling code here..
		blmClass->setMidiOutput(midiOutput->getText());
        //[/UserComboBoxCode_midiOutput]
    }
    else if (comboBoxThatHasChanged == blmSize)
    {
        //[UserComboBoxCode_blmSize] -- add your combo box handling code here..
		 int rows,cols;
         switch (blmSize->getSelectedId()) {
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
		 blmClass->setBlmDimensions(cols,rows);
		 setSize(117+(cols*blmClass->getLedSize()),10+(rows*blmClass->getLedSize()));
		 //blmClass->resized(); // Force call to resized to redraw new BLM
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
  <GENERICCOMPONENT name="blmClass" id="f588e2887ead362d" memberName="blmClass" virtualName=""
                    explicitFocusOrder="0" pos="112 5 117M 10M" class="BlmClass"
                    params="32,16"/>
  <COMBOBOX name="new combo box" id="d5b1c2215727b7d1" memberName="midiInput"
            virtualName="" explicitFocusOrder="0" pos="8 112 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <COMBOBOX name="new combo box" id="8075aa1c2b4cfb8b" memberName="midiOutput"
            virtualName="" explicitFocusOrder="0" pos="8 152 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <COMBOBOX name="new combo box" id="9468b7928e9045ee" memberName="blmSize"
            virtualName="" explicitFocusOrder="0" pos="8 192 88 24" editable="0"
            layout="33" items="32x16&#10;64x16&#10;128x16&#10;16x8&#10;32x8&#10;64x8&#10;16x4&#10;32x4&#10;64x4"
            textWhenNonSelected="32x16" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="8614fa3b9e19ab4c" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="8 96 48 16" edTextCol="ff000000"
         edBkgCol="0" labelText="Midi In:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="11"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="a6e208682370a799" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="8 136 48 16" edTextCol="ff000000"
         edBkgCol="0" labelText="Midi Out:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="11"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="75a4fdd6febd8a0b" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="8 176 48 16" edTextCol="ff000000"
         edBkgCol="0" labelText="Size:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="11"
         bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
