/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "app_lcd.h"

// for faster startup
#define DISABLE_MIDI 0

// because editor is not integrated yet...
#define MINIMAL_GUI 1

//==============================================================================
MidiboxSidAudioProcessorEditor::MidiboxSidAudioProcessorEditor (MidiboxSidAudioProcessor* ownerSidEmu)
    : AudioProcessorEditor (ownerSidEmu)
{
	///////////////////////////////////////////////////////////////////////////
	// Patch Selection
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible(patchComboBox = new ComboBox("Patch"));
    patchComboBox->addListener(this);
    patchComboBox->setEditableText(false);
    patchComboBox->setTextWhenNothingSelected (String::empty);
    patchComboBox->setTextWhenNoChoicesAvailable ("(no choices)");
    for(int patch=0; patch<128; ++patch)
        patchComboBox->addItem(ownerSidEmu->getPatchNameFromBank(0, patch), patch+1);
    patchComboBox->addSeparator();
    patchComboBox->setTooltip ("selects a patch");
    patchComboBox->setSelectedId((int)ownerSidEmu->getParameter(2)+1, false);

#if MINIMAL_GUI
    gainSlider = NULL;
#else
	///////////////////////////////////////////////////////////////////////////
    // create our gain slider..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (gainSlider = new KnobBlue("gain"));
    gainSlider->addListener (this);
    gainSlider->setRange (0.0, 1.0, 0.01);
    gainSlider->setTooltip ("changes the volume");
    gainSlider->setValue (ownerSidEmu->getParameter(0));
#endif

	///////////////////////////////////////////////////////////////////////////
    // create MIDI input selector
	///////////////////////////////////////////////////////////////////////////
	addAndMakeVisible (midiInputSelector = new ComboBox (String::empty));
	midiInputSelector->addListener (this);
	midiInputSelector->clear();

	midiInputLabel = new Label ("lm", TRANS("SysEx Input:"));
	midiInputLabel->attachToComponent (midiInputSelector, true);

	midiInputSelector->addItem (TRANS("<< none >>"), -1);
	midiInputSelector->addSeparator();
	
#if !DISABLE_MIDI
	const StringArray midiIns (MidiInput::getDevices());
	
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
#else
	midiInputSelector->setSelectedId(-1, true);
#endif


	///////////////////////////////////////////////////////////////////////////
    // create MIDI output selector
	///////////////////////////////////////////////////////////////////////////
	addAndMakeVisible (midiOutputSelector = new ComboBox (String::empty));
	midiOutputSelector->addListener (this);
	midiOutputSelector->clear();
	
	midiOutputLabel = new Label ("lm", TRANS("SysEx Output:"));
	midiOutputLabel->attachToComponent (midiOutputSelector, true);
	
	midiOutputSelector->addItem (TRANS("<< none >>"), -1);
	midiOutputSelector->addSeparator();
	
#if !DISABLE_MIDI
	const StringArray midiOuts (MidiOutput::getDevices());
	
	current = -1;
	for (int i = 0; i < midiOuts.size(); ++i) {
		midiOutputSelector->addItem (midiOuts[i], i + 1);
		if( midiOuts[i] == ownerSidEmu->lastSysexMidiOut ) {
			current = i+1;
			ownerSidEmu->midiProcessing.audioDeviceManager.setDefaultMidiOutput(midiOuts[i]);
		}
	}
	midiOutputSelector->setSelectedId (current, true);
#else
	midiOutputSelector->setSelectedId(-1, true);
#endif
	
	///////////////////////////////////////////////////////////////////////////
    // create and add the midi keyboard component..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (midiKeyboard
                       = new MidiKeyboardComponent (ownerSidEmu->keyboardState,
                                                    MidiKeyboardComponent::horizontalKeyboard));
    midiKeyboard->setLowestVisibleKey(24); // does match better with layout
  
	///////////////////////////////////////////////////////////////////////////
    // add a label that will display the current timecode and status..
	///////////////////////////////////////////////////////////////////////////
    addAndMakeVisible (infoLabel = new Label (String::empty, String::empty));


	///////////////////////////////////////////////////////////////////////////
    // Control Groups
	///////////////////////////////////////////////////////////////////////////
#if MINIMAL_GUI
    controlGroupKnobs = NULL;
#else
    addAndMakeVisible(controlGroupKnobs = new ControlGroupKnobs("Knobs"));
#endif

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
    // set component size (always the same, not stored in .xml anymore)
	///////////////////////////////////////////////////////////////////////////
    setSize(600, 200);
  
    // register ourselves with the SidEmu - it will use its ChangeBroadcaster base
    // class to tell us when something has changed, and this will call our changeListenerCallback()
    // method.
    ownerSidEmu->addChangeListener (this);
}

MidiboxSidAudioProcessorEditor::~MidiboxSidAudioProcessorEditor()
{
    getSidEmu()->removeChangeListener (this);
  
    deleteAllChildren();
}

//==============================================================================
void MidiboxSidAudioProcessorEditor::paint (Graphics& g)
{
    // just clear the window
    g.fillAll(Colour::greyLevel(0.25f));
}

void MidiboxSidAudioProcessorEditor::resized()
{
    infoLabel->setBounds (15, 0, getWidth(), 15);

    if( gainSlider )
        gainSlider->setBounds (30, 10, 48, 48);

    patchComboBox->setBounds     (100, 15+0*32, getWidth()-100-4, 24);
	midiInputSelector->setBounds (100, 15+1*32, getWidth()-100-4, 24);
	midiOutputSelector->setBounds(100, 15+2*32, getWidth()-100-4, 24);

    if( controlGroupKnobs )
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
void MidiboxSidAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster *source)
{
    // this is the SidEmu telling us that it's changed, so we'll update our
    // display of the time, midi message, etc.
    updateParametersFromSidEmu();
}

void MidiboxSidAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
    if( slider == gainSlider ) {
        getSidEmu()->setParameterNotifyingHost (0, (float) slider->getValue());
    }
}

void MidiboxSidAudioProcessorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiOutputSelector ) {
#if !DISABLE_MIDI
        getSidEmu()->midiProcessing.audioDeviceManager.setDefaultMidiOutput(midiOutputSelector->getText());
		getSidEmu()->lastSysexMidiOut = midiOutputSelector->getText();
#endif
	} else if( comboBoxThatHasChanged == midiInputSelector ) {
#if !DISABLE_MIDI
        const StringArray allMidiIns (MidiInput::getDevices());
        for (int i = allMidiIns.size(); --i >= 0;) {
			bool enabled = allMidiIns[i] == midiInputSelector->getText();
            getSidEmu()->midiProcessing.audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
            if( enabled )
                getSidEmu()->lastSysexMidiIn = allMidiIns[i];
		}
#endif
	} else {
		getSidEmu()->setParameterNotifyingHost (2, (float) patchComboBox->getSelectedId()-1);
	}
        
}

//==============================================================================
void MidiboxSidAudioProcessorEditor::updateParametersFromSidEmu()
{
    MidiboxSidAudioProcessor* const sidEmu = getSidEmu();
  
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
    if( gainSlider )
        gainSlider->setValue(newGain);
    patchComboBox->setSelectedId((int)newPatch+1, false);
}
