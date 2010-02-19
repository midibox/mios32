/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * SysEx Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "SysexTool.h"
#include "MiosStudio.h"


//==============================================================================
SysexToolSend::SysexToolSend(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(sendBox = new LogBox(T("SysEx Send")));

	addAndMakeVisible(sendFileChooser = new FilenameComponent (T("syxfile"),
                                                               File::nonexistent,
                                                               true, false, false,
                                                               "*.syx",
                                                               String::empty,
                                                               T("(choose a .syx file to send)")));
	sendFileChooser->addListener(this);
	sendFileChooser->setBrowseButtonText(T("Browse"));

    addAndMakeVisible(sendStartButton = new TextButton(T("Send Button")));
    sendStartButton->setButtonText(T("Send"));
    sendStartButton->setEnabled(false);
    sendStartButton->addButtonListener(this);

    addAndMakeVisible(sendStopButton = new TextButton(T("Stop Button")));
    sendStopButton->setButtonText(T("Stop"));
    sendStopButton->setEnabled(false);
    sendStopButton->addButtonListener(this);

    addAndMakeVisible(sendEditButton = new TextButton(T("Edit Button")));
    sendEditButton->setButtonText(T("Edit"));
    sendEditButton->setEnabled(false);
    sendEditButton->addButtonListener(this);

    addAndMakeVisible(sendDelayLabel = new Label(T("Send Delay"), T("Send Delay")));
    sendDelayLabel->setJustificationType(Justification::centred);

    addAndMakeVisible(sendDelaySlider = new Slider(T("Send Delay (mS)")));
    sendDelaySlider->setRange(0, 5000, 50);
    sendDelaySlider->setValue(750);
    sendDelaySlider->setSliderStyle(Slider::IncDecButtons);
    sendDelaySlider->setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    sendDelaySlider->setDoubleClickReturnValue(true, 0);
    sendDelaySlider->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        sendDelaySlider->setValue(propertiesFile->getIntValue(T("sysexSendDelay"), 750));

        String recentlyUsedSyxFiles = propertiesFile->getValue(T("recentlyUsedSyxSendFiles"), String::empty);
        // seems that Juce doesn't provide a split function?
        StringArray recentlyUsedSyxFilesArray;
        int index = 0;
        while( (index=recentlyUsedSyxFiles.indexOfChar(';')) >= 0 ) {
            recentlyUsedSyxFilesArray.add(recentlyUsedSyxFiles.substring(0, index));
            recentlyUsedSyxFiles = recentlyUsedSyxFiles.substring(index+1);
        }
        if( recentlyUsedSyxFiles != String::empty )
            recentlyUsedSyxFilesArray.add(recentlyUsedSyxFiles);
        sendFileChooser->setRecentlyUsedFilenames(recentlyUsedSyxFilesArray);

        sendFileChooser->setDefaultBrowseTarget(propertiesFile->getValue(T("defaultFile"), String::empty));
    }
}

SysexToolSend::~SysexToolSend()
{
    deleteAllChildren();
}

//==============================================================================
void SysexToolSend::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexToolSend::resized()
{
    sendFileChooser->setBounds(4, 4, getWidth()-8, 24);
    sendBox->setBounds(100, 32, getWidth()-100-4, getHeight()-32-4);

    int sendButtonY = 4+32+8;
    int sendButtonWidth = 72;
    sendStartButton->setBounds(14, sendButtonY+0*36, sendButtonWidth, 24);
    sendStopButton->setBounds(14, sendButtonY+1*36, sendButtonWidth, 24);
    sendEditButton->setBounds(14, sendButtonY+2*36, sendButtonWidth, 24);
    sendDelayLabel->setBounds(14, sendButtonY+3*36-10, sendButtonWidth, 24);
    sendDelaySlider->setBounds(14, sendButtonY+3*36+10, sendButtonWidth, 40);
}

//==============================================================================
void SysexToolSend::textEditorTextChanged(TextEditor &editor)
{
}

void SysexToolSend::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void SysexToolSend::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void SysexToolSend::textEditorFocusLost(TextEditor &editor)
{
}


//==============================================================================
void SysexToolSend::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == sendStartButton ) {
        sendStartButton->setEnabled(false);
        sendStopButton->setEnabled(true);
        sendEditButton->setEnabled(false);
        sendDelaySlider->setEnabled(false);
        sendFileChooser->setEnabled(false);
    } else if( buttonThatWasClicked == sendStopButton ) {
        sendStopButton->setEnabled(false);
        sendStartButton->setEnabled(true);
        sendEditButton->setEnabled(sendBox->getNumRows() > 0);
        sendDelaySlider->setEnabled(true);
        sendFileChooser->setEnabled(true);
    } else if( buttonThatWasClicked == sendEditButton ) {
        // TODO
    }
}


//==============================================================================
void SysexToolSend::sliderValueChanged(Slider* slider)
{
    if( slider == sendDelaySlider ) {
        // store settings
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile ) {
            propertiesFile->setValue(T("sysexSendDelay"), sendDelaySlider->getValue());
        }
    }
}


//==============================================================================
void SysexToolSend::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
    if( fileComponentThatHasChanged == sendFileChooser ) {
        // TODO...

        sendStartButton->setEnabled(true);
        sendEditButton->setEnabled(true);
    }
}




//==============================================================================
SysexToolReceive::SysexToolReceive(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(receiveBox = new LogBox(T("SysEx Receive")));

	addAndMakeVisible(receiveFileChooser = new FilenameComponent (T("syxfile"),
                                                                  File::nonexistent,
                                                                  true, false, true,
                                                                  "*.syx",
                                                                  "*.syx",
                                                                  T("(choose a .syx file to save)")));
	receiveFileChooser->addListener(this);
	receiveFileChooser->setBrowseButtonText(T("Browse"));
    receiveFileChooser->setEnabled(false);

    addAndMakeVisible(receiveStartButton = new TextButton(T("Receive Button")));
    receiveStartButton->setButtonText(T("Receive"));
    receiveStartButton->setEnabled(true);
    receiveStartButton->addButtonListener(this);

    addAndMakeVisible(receiveStopButton = new TextButton(T("Stop Button")));
    receiveStopButton->setButtonText(T("Stop"));
    receiveStopButton->setEnabled(false);
    receiveStopButton->addButtonListener(this);

    addAndMakeVisible(receiveEditButton = new TextButton(T("Edit Button")));
    receiveEditButton->setButtonText(T("Edit"));
    receiveEditButton->setEnabled(false);
    receiveEditButton->addButtonListener(this);

    addAndMakeVisible(receiveClearButton = new TextButton(T("Clear Button")));
    receiveClearButton->setButtonText(T("Clear"));
    receiveClearButton->setEnabled(false);
    receiveClearButton->addButtonListener(this);
}

SysexToolReceive::~SysexToolReceive()
{
    deleteAllChildren();
}

//==============================================================================
void SysexToolReceive::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexToolReceive::resized()
{
    receiveFileChooser->setBounds(4, 4, getWidth()-8, 24);
    receiveBox->setBounds(100, 32, getWidth()-100-4, getHeight()-32-4);

    int receiveButtonY = 4+32+8;
    int receiveButtonWidth = 72;
    receiveStartButton->setBounds(14, receiveButtonY+0*36, receiveButtonWidth, 24);
    receiveStopButton->setBounds(14, receiveButtonY+1*36, receiveButtonWidth, 24);
    receiveEditButton->setBounds(14, receiveButtonY+2*36, receiveButtonWidth, 24);
    receiveClearButton->setBounds(14, receiveButtonY+3*36, receiveButtonWidth, 24);
}

//==============================================================================
void SysexToolReceive::textEditorTextChanged(TextEditor &editor)
{
}

void SysexToolReceive::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void SysexToolReceive::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void SysexToolReceive::textEditorFocusLost(TextEditor &editor)
{
}


//==============================================================================
void SysexToolReceive::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == receiveStartButton ) {
        receiveStartButton->setEnabled(false);
        receiveStopButton->setEnabled(true);
        receiveEditButton->setEnabled(false);
        receiveFileChooser->setEnabled(false);
        receiveClearButton->setEnabled(false);
    } else if( buttonThatWasClicked == receiveStopButton ) {
        receiveStopButton->setEnabled(false);
        receiveStartButton->setEnabled(true);
        receiveEditButton->setEnabled(receiveBox->getNumRows() > 0);
        receiveFileChooser->setEnabled(receiveBox->getNumRows() > 0);
        receiveClearButton->setEnabled(receiveBox->getNumRows() > 0);
    } else if( buttonThatWasClicked == receiveEditButton ) {
        // TODO
    } else if( buttonThatWasClicked == receiveClearButton ) {
        receiveBox->clear();
        receiveEditButton->setEnabled(false);
        receiveFileChooser->setEnabled(false);
        receiveClearButton->setEnabled(false);
    }
}


//==============================================================================
void SysexToolReceive::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
    if( fileComponentThatHasChanged == receiveFileChooser ) {
        // TODO...
    }
}


//==============================================================================
void SysexToolReceive::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    if( !receiveStartButton->isEnabled() ) { // !enabled means that we are ready for receive
        uint8 *data = message.getRawData();
        uint32 size = message.getRawDataSize();

        String hexStr = String::toHexString(data, size);
        receiveBox->addEntry(Colours::black, hexStr);

        receiveEditButton->setEnabled(true);
        receiveFileChooser->setEnabled(true);
        receiveClearButton->setEnabled(true);
    }
}


//==============================================================================
//==============================================================================
//==============================================================================
SysexTool::SysexTool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(sysexToolSend = new SysexToolSend(miosStudio));
    addAndMakeVisible(sysexToolReceive = new SysexToolReceive(miosStudio));

    //                             num   min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.005, -0.9, -0.25); // Send Window
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.005, -0.9, -0.25); // Receive Window

    horizontalDividerBar = new StretchableLayoutResizerBar(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalDividerBar);

    resizeLimits.setSizeLimits(200, 100, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(800, 400);
}

SysexTool::~SysexTool()
{
    deleteAllChildren();
}

//==============================================================================
void SysexTool::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexTool::resized()
{
    Component* hcomps[] = { sysexToolSend,
                            horizontalDividerBar,
                            sysexToolReceive,
    };

    horizontalLayout.layOutComponents(hcomps, 3,
                                      4, 4,
                                      getWidth() - 8, getHeight() - 8,
                                      true,  // lay out above each other
                                      true); // resize the components' heights as well as widths

    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}

