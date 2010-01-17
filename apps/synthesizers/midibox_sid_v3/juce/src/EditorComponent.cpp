/*
  ==============================================================================
  Derived from DemoEditorComponent.cpp of Juce project
 
 
  ==============================================================================
*/

#include "includes.h"
#include "EditorComponent.h"

//==============================================================================
EditorComponent::EditorComponent (AudioProcessing* const ownerSidEmu)
: AudioProcessorEditor (ownerSidEmu)
{
  // patch selection
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

  // create our gain slider..
  addAndMakeVisible (gainSlider = new Slider (T("gain")));
  gainSlider->addListener (this);
  gainSlider->setSliderStyle(Slider::RotaryVerticalDrag);
  gainSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  gainSlider->setRange (0.0, 1.0, 0.01);
  gainSlider->setTooltip (T("changes the volume"));
  gainSlider->setValue (ownerSidEmu->getParameter(0), false);
  
  // create and add the midi keyboard component..
  addAndMakeVisible (midiKeyboard
		     = new MidiKeyboardComponent (ownerSidEmu->keyboardState,
						  MidiKeyboardComponent::horizontalKeyboard));
  
  // add a label that will display the current timecode and status..
  addAndMakeVisible (infoLabel = new Label (String::empty, String::empty));
  
  // add the triangular resizer component for the bottom-right of the UI
  addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
  resizeLimits.setSizeLimits (150, 150, 800, 300);
  
  // set our component's initial size to be the last one that was stored in the SidEmu's settings
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
  g.fillAll (Colour::greyLevel (0.9f));
}

void EditorComponent::resized()
{
  gainSlider->setBounds (10, 10, 30, 30);
  patchComboBox->setBounds (50, 13, 200, 22);
  infoLabel->setBounds (10, 35, 450, 20);
  
  const int keyboardHeight = 70;
  midiKeyboard->setBounds (4, getHeight() - keyboardHeight - 4,
			   getWidth() - 8, keyboardHeight);
  
  resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
  
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

void EditorComponent::comboBoxChanged (ComboBox*)
{
  getSidEmu()->setParameterNotifyingHost (2, (float) patchComboBox->getSelectedId()-1);
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
