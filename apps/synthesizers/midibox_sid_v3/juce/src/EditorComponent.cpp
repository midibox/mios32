/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
  ==============================================================================
  Derived from DemoEditorComponent.cpp of Juce project
 
 
  ==============================================================================
*/

#include "includes.h"
#include "EditorComponent.h"
#include "app_lcd.h"


//==============================================================================
EditorComponent::EditorComponent (AudioProcessing* const ownerSidEmu)
    : AudioProcessorEditor (ownerSidEmu)
{
	///////////////////////////////////////////////////////////////////////////
	// Patch Selection
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible(patchComboBox = new ComboBox(T("Patch")));
    patchComboBox->addListener(this);
    patchComboBox->setEditableText(false);
    patchComboBox->setJustificationType (Justification::centred);
    patchComboBox->setTextWhenNothingSelected (String::empty);
    patchComboBox->setTextWhenNoChoicesAvailable (T("(no choices)"));
    for(int patch=0; patch<128; ++patch)
        patchComboBox->addItem(ownerSidEmu->getPatchNameFromBank(0, patch), patch+1);
    patchComboBox->addSeparator();
    patchComboBox->setTooltip (T("selects a patch"));
    patchComboBox->setSelectedId((int)ownerSidEmu->getParameter(2)+1, false);


	///////////////////////////////////////////////////////////////////////////
    // create our gain slider..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (gainSlider = new KnobBlue(T("gain")));
    gainSlider->addListener (this);
    gainSlider->setRange (0.0, 1.0, 0.01);
    gainSlider->setTooltip (T("changes the volume"));
    gainSlider->setValue (ownerSidEmu->getParameter(0), false);


	///////////////////////////////////////////////////////////////////////////
    // create MIDI input selector
	///////////////////////////////////////////////////////////////////////////
	addAndMakeVisible (midiInputSelector = new ComboBox (String::empty));
	midiInputSelector->addListener (this);
	midiInputSelector->clear();

	midiInputLabel = new Label ("lm", TRANS("SysEx Input:"));
	midiInputLabel->attachToComponent (midiInputSelector, true);
	
	const StringArray midiIns (MidiInput::getDevices());
	
	midiInputSelector->addItem (TRANS("<< none >>"), -1);
	midiInputSelector->addSeparator();
	
	int current = -1;
	for (int i = 0; i < midiIns.size(); ++i) {
		midiInputSelector->addItem (midiIns[i], i + 1);
		bool enabled = false;
		if( midiIns[i] == ownerSidEmu->lastSysexMidiIn ) {
			enabled = true;
			current = i+1;
		}
		ownerSidEmu->midiProcessing.audioDeviceManager.setMidiInputEnabled (midiIns[i], enabled);
	}
	midiInputSelector->setSelectedId(current, true);

	// add MIDI collector to audio device manager
	ownerSidEmu->midiProcessing.audioDeviceManager.addMidiInputCallback(String::empty, &ownerSidEmu->midiProcessing);


	///////////////////////////////////////////////////////////////////////////
    // create MIDI output selector
	///////////////////////////////////////////////////////////////////////////
	addAndMakeVisible (midiOutputSelector = new ComboBox (String::empty));
	midiOutputSelector->addListener (this);
	midiOutputSelector->clear();
	
	midiOutputLabel = new Label ("lm", TRANS("SysEx Output:"));
	midiOutputLabel->attachToComponent (midiOutputSelector, true);
	
	const StringArray midiOuts (MidiOutput::getDevices());
	
	midiOutputSelector->addItem (TRANS("<< none >>"), -1);
	midiOutputSelector->addSeparator();
	
	current = -1;
	for (int i = 0; i < midiOuts.size(); ++i) {
		midiOutputSelector->addItem (midiOuts[i], i + 1);
		if( midiOuts[i] == ownerSidEmu->lastSysexMidiOut ) {
			current = i+1;
			ownerSidEmu->midiProcessing.audioDeviceManager.setDefaultMidiOutput(midiOuts[i]);
		}
	}
	midiOutputSelector->setSelectedId (current, true);
	
	
	///////////////////////////////////////////////////////////////////////////
    // create and add the midi keyboard component..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (midiKeyboard
                       = new MidiKeyboardComponent (ownerSidEmu->keyboardState,
                                                    MidiKeyboardComponent::horizontalKeyboard));
  
	///////////////////////////////////////////////////////////////////////////
    // add a label that will display the current timecode and status..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (infoLabel = new Label (String::empty, String::empty));


	///////////////////////////////////////////////////////////////////////////
    // Control Groups
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible(controlGroupKnobs = new ControlGroupKnobs(T("Knobs")));

#if 0
	///////////////////////////////////////////////////////////////////////////
	// create a CLCD via APP_LCD module and add it
	///////////////////////////////////////////////////////////////////////////
	if (!cLcdView->isValidComponent()) 
		addAndMakeVisible(cLcdView = APP_LCD_GetComponentPtr(0, 300, 50));
#endif

	///////////////////////////////////////////////////////////////////////////
    // add the triangular resizer component for the bottom-right of the UI
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
    resizeLimits.setSizeLimits (150, 150, 800, 300);
  
	
	///////////////////////////////////////////////////////////////////////////
    // set our component's initial size to be the last one that was stored in the SidEmu's settings
	///////////////////////////////////////////////////////////////////////////
    setSize (ownerSidEmu->lastUIWidth,
             ownerSidEmu->lastUIHeight);
  
    // register ourselves with the SidEmu - it will use its ChangeBroadcaster base
    // class to tell us when something has changed, and this will call our changeListenerCallback()
    // method.
    ownerSidEmu->addChangeListener (this);

}

EditorComponent::~EditorComponent()
{
    getSidEmu()->removeChangeListener (this);
  
    deleteAllChildren();
}

//==============================================================================
void EditorComponent::paint (Graphics& g)
{
    // just clear the window
    g.fillAll(Colour::greyLevel(0.25f));
}

void EditorComponent::resized()
{
    gainSlider->setBounds (30, 10, 48, 48);
    patchComboBox->setBounds (100, 13, 200, 22);
    infoLabel->setBounds (10, 35, 450, 20);

	midiInputSelector->setBounds (100, 50, 200, 22);
	midiOutputSelector->setBounds (100, 75, 200, 22);

    controlGroupKnobs->setBounds(10, 100, 500, 100);

    const int keyboardHeight = 70;
    midiKeyboard->setBounds (4, getHeight() - keyboardHeight - 4,
                             getWidth() - 8, keyboardHeight);
  
    //resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
  
    // if we've been resized, tell the SidEmu so that it can store the new size
    // in its settings
    getSidEmu()->lastUIWidth = getWidth();
    getSidEmu()->lastUIHeight = getHeight();

}

//==============================================================================
void EditorComponent::changeListenerCallback (void* source)
{
    // this is the SidEmu telling us that it's changed, so we'll update our
    // display of the time, midi message, etc.
    updateParametersFromSidEmu();
}

void EditorComponent::sliderValueChanged (Slider*)
{
    getSidEmu()->setParameterNotifyingHost (0, (float) gainSlider->getValue());
}

void EditorComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiOutputSelector ) {
        getSidEmu()->midiProcessing.audioDeviceManager.setDefaultMidiOutput(midiOutputSelector->getText());
		getSidEmu()->lastSysexMidiOut = midiOutputSelector->getText();
	} else if( comboBoxThatHasChanged == midiInputSelector ) {
        const StringArray allMidiIns (MidiInput::getDevices());
        for (int i = allMidiIns.size(); --i >= 0;) {
			bool enabled = allMidiIns[i] == midiInputSelector->getText();
            getSidEmu()->midiProcessing.audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
            if( enabled )
                getSidEmu()->lastSysexMidiIn = allMidiIns[i];
		}
	} else {
		getSidEmu()->setParameterNotifyingHost (2, (float) patchComboBox->getSelectedId()-1);
	}
        
}

//==============================================================================
void EditorComponent::updateParametersFromSidEmu()
{
    AudioProcessing* const sidEmu = getSidEmu();
  
    // we use this lock to make sure the processBlock() method isn't writing to the
    // lastMidiMessage variable while we're trying to read it, but be extra-careful to
    // only hold the lock for a minimum amount of time..
    sidEmu->getCallbackLock().enter();

    // take a local copy of the info we need while we've got the lock..
    const AudioPlayHead::CurrentPositionInfo positionInfo (sidEmu->lastPosInfo);
    const float newGain = sidEmu->getParameter(0);
    const float newBank = sidEmu->getParameter(1);
    const float newPatch = sidEmu->getParameter(2);
  
    // ..release the lock ASAP
    sidEmu->getCallbackLock().exit();
  
#if 0
    // ..and after releasing the lock, we're free to do the time-consuming UI stuff..
    String infoText;
    infoText << sidEmu->getPatchName();  
    infoLabel->setText (infoText, false);
#endif

    /* Update our slider.
   
    (note that it's important here to tell the slider not to send a change
    message, because that would cause it to call the sidEmu with a parameter
    change message again, and the values would drift out.
    */
    gainSlider->setValue(newGain, false);
    patchComboBox->setSelectedId((int)newPatch+1, false);
  
    setSize (sidEmu->lastUIWidth,
             sidEmu->lastUIHeight);
}
