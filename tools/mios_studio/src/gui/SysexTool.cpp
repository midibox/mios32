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
    , numBytesToSend(0)
    , numBytesSent(0)
    , progress(0)
{
    addAndMakeVisible(statusLabel = new Label(T("Number of Bytes"), String::empty));
    statusLabel->setJustificationType(Justification::right);

    addAndMakeVisible(sendBox = new HexTextEditor(statusLabel));

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
    sendStartButton->addListener(this);

    addAndMakeVisible(sendStopButton = new TextButton(T("Stop Button")));
    sendStopButton->setButtonText(T("Stop"));
    sendStopButton->setEnabled(false);
    sendStopButton->addListener(this);

    addAndMakeVisible(sendClearButton = new TextButton(T("Clear Button")));
    sendClearButton->setButtonText(T("Clear"));
    sendClearButton->addListener(this);

    addAndMakeVisible(sendDelayLabel = new Label(T("Send Delay"), T("Send Delay:")));
    sendDelayLabel->setJustificationType(Justification::centred);

    addAndMakeVisible(sendDelaySlider = new Slider(T("Send Delay (mS)")));
    sendDelaySlider->setRange(0, 5000, 50);
    sendDelaySlider->setValue(750);
    sendDelaySlider->setSliderStyle(Slider::IncDecButtons);
    sendDelaySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    sendDelaySlider->setDoubleClickReturnValue(true, 0);
    sendDelaySlider->addListener(this);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

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

        sendFileChooser->setDefaultBrowseTarget(propertiesFile->getValue(T("defaultSyxSendFile"), String::empty));
    }

    setSize(800, 200);
}

SysexToolSend::~SysexToolSend()
{
}

//==============================================================================
void SysexToolSend::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexToolSend::resized()
{
    sendFileChooser->setBounds(4, 4, getWidth()-8, 24);

    int sendButtonY = 4+32;
    int sendButtonWidth = 72;
    sendStartButton->setBounds(10 + 0*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    sendStopButton->setBounds(10 + 1*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    sendClearButton->setBounds(10 + 2*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    sendDelayLabel->setBounds(10 + 4*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    sendDelaySlider->setBounds(10 + 5*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    statusLabel->setBounds(getWidth()-10-150, sendButtonY, 150, 24);

    sendBox->setBounds(4, 32+40, getWidth()-8, getHeight()-32-40-4-28-4);

    progressBar->setBounds(4, getHeight()-28, getWidth()-8, 24);
}

//==============================================================================
void SysexToolSend::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == sendStartButton ) {
        sendData = sendBox->getBinary();
        numBytesToSend = sendData.size();
        numBytesSent = 0;
        progress = 0;
        if( numBytesToSend > 0 ) {
            startTimer(1);
            sendStartButton->setEnabled(false);
            sendStopButton->setEnabled(true);
            sendDelaySlider->setEnabled(false);
            sendFileChooser->setEnabled(false);
            sendClearButton->setEnabled(false);
        }
    } else if( buttonThatWasClicked == sendStopButton ) {
        stopTimer();
        sendStopButton->setEnabled(false);
        sendStartButton->setEnabled(true);
        sendDelaySlider->setEnabled(true);
        sendFileChooser->setEnabled(true);
        sendClearButton->setEnabled(true);
        progress = 0;
    } else if( buttonThatWasClicked == sendClearButton ) {
        sendBox->clear();
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
        File inFile = sendFileChooser->getCurrentFile();

        FileInputStream *inFileStream = inFile.createInputStream();

        if( !inFileStream || inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("The file ") + inFile.getFileName(),
                                        T("doesn't exist!"),
                                        String::empty);
        } else if( inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("The file ") + inFile.getFileName(),
                                        T("is empty!"),
                                        String::empty);
        } else {
            int64 size = inFileStream->getTotalLength();
            uint8 *buffer = (uint8 *)juce_malloc(size);
            int64 readNumBytes = inFileStream->read(buffer, size);
            sendBox->setBinary(buffer, readNumBytes);
            juce_free(buffer);

            // store setting
            PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
            if( propertiesFile ) {
                String recentlyUsedHexFiles = sendFileChooser->getRecentlyUsedFilenames().joinIntoString(";");
                propertiesFile->setValue(T("recentlyUsedSyxSendFiles"), recentlyUsedHexFiles);
                propertiesFile->setValue(T("defaultSyxSendFile"), inFile.getFullPathName());
            }
        }

        deleteAndZero(inFileStream);
    }
}


//==============================================================================
void SysexToolSend::timerCallback()
{
    stopTimer(); // will be restarted if required

    if( sendData.size() == 0 ) {
        sendStopButton->setEnabled(false);
        sendStartButton->setEnabled(true);
        sendDelaySlider->setEnabled(true);
        sendFileChooser->setEnabled(true);
        sendClearButton->setEnabled(true);
        progress = 0;
    } else {
        int streamEnd = sendData.indexOf(0xf7);
        if( streamEnd < 0 ) {
            MidiMessage message = SysexHelper::createMidiMessage(sendData);
            miosStudio->sendMidiMessage(message);
            sendData.clear();
            numBytesSent += sendData.size();
        } else {
            // send string until 0xf7
            MidiMessage message = SysexHelper::createMidiMessage(sendData, streamEnd+1);
            miosStudio->sendMidiMessage(message);
            sendData.removeRange(0, streamEnd+1);
            numBytesSent += streamEnd+1;
        }

        progress = (double)numBytesSent / (double)numBytesToSend;

        if( sendData.size() > 0 ) {
            int delay = sendDelaySlider->getValue();
            startTimer((delay > 0) ? delay : 1);
        } else {
            startTimer(1000); // stop progress bar after 1 second
        }
    }
}


//==============================================================================
SysexToolReceive::SysexToolReceive(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(statusLabel = new Label(T("Number of Bytes"), String::empty));
    statusLabel->setJustificationType(Justification::right);

    addAndMakeVisible(receiveBox = new HexTextEditor(statusLabel));
    receiveBox->setTextToShowWhenEmpty(T("Received Data will be displayed once the stop button has been pressed!"),
                                       Colour(0xff808080));

	addAndMakeVisible(receiveFileChooser = new FilenameComponent (T("syxfile"),
                                                                  File::nonexistent,
                                                                  true, false, true,
                                                                  "*.syx",
                                                                  ".syx",
                                                                  T("(choose a .syx file to save)")));
	receiveFileChooser->addListener(this);
	receiveFileChooser->setBrowseButtonText(T("Browse"));

    addAndMakeVisible(receiveStartButton = new TextButton(T("Receive Button")));
    receiveStartButton->setButtonText(T("Receive"));
    receiveStartButton->setEnabled(true);
    receiveStartButton->addListener(this);

    addAndMakeVisible(receiveStopButton = new TextButton(T("Stop Button")));
    receiveStopButton->setButtonText(T("Stop"));
    receiveStopButton->setEnabled(false);
    receiveStopButton->addListener(this);

    addAndMakeVisible(receiveClearButton = new TextButton(T("Clear Button")));
    receiveClearButton->setButtonText(T("Clear"));
    receiveClearButton->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        String recentlyUsedSyxFiles = propertiesFile->getValue(T("recentlyUsedSyxReceiveFiles"), String::empty);
        // seems that Juce doesn't provide a split function?
        StringArray recentlyUsedSyxFilesArray;
        int index = 0;
        while( (index=recentlyUsedSyxFiles.indexOfChar(';')) >= 0 ) {
            recentlyUsedSyxFilesArray.add(recentlyUsedSyxFiles.substring(0, index));
            recentlyUsedSyxFiles = recentlyUsedSyxFiles.substring(index+1);
        }
        if( recentlyUsedSyxFiles != String::empty )
            recentlyUsedSyxFilesArray.add(recentlyUsedSyxFiles);
        receiveFileChooser->setRecentlyUsedFilenames(recentlyUsedSyxFilesArray);

        receiveFileChooser->setDefaultBrowseTarget(propertiesFile->getValue(T("defaultSyxReceiveFile"), String::empty));
    }

    setSize(800, 200);
}

SysexToolReceive::~SysexToolReceive()
{
}

//==============================================================================
void SysexToolReceive::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexToolReceive::resized()
{
    receiveFileChooser->setBounds(4, 4, getWidth()-8, 24);

    int receiveButtonY = 4+32;
    int receiveButtonWidth = 72;
    receiveStartButton->setBounds(10 + 0*(receiveButtonWidth+10), receiveButtonY, receiveButtonWidth, 24);
    receiveStopButton->setBounds(10 + 1*(receiveButtonWidth+10), receiveButtonY, receiveButtonWidth, 24);
    receiveClearButton->setBounds(10 + 2*(receiveButtonWidth+10), receiveButtonY, receiveButtonWidth, 24);
    statusLabel->setBounds(getWidth()-10-150, receiveButtonY, 150, 24);

    receiveBox->setBounds(4, 32+40, getWidth()-8, getHeight()-32-40-4);
}

//==============================================================================
void SysexToolReceive::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == receiveStartButton ) {
        receiveStartButton->setEnabled(false);
        receiveStopButton->setEnabled(true);
        receiveFileChooser->setEnabled(false);
        receivedData.clear();
        statusLabel->setText(String(receivedData.size()) + T(" bytes received"), true);
    } else if( buttonThatWasClicked == receiveStopButton ) {
        receiveStopButton->setEnabled(false);
        receiveStartButton->setEnabled(true);
        receiveFileChooser->setEnabled(true);
        if( receivedData.size() > 0 )
            receiveBox->addBinary(&receivedData.getReference(0), receivedData.size());
        receivedData.clear();
    } else if( buttonThatWasClicked == receiveClearButton ) {
        receiveBox->clear();
    }
}


//==============================================================================
void SysexToolReceive::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
    if( fileComponentThatHasChanged == receiveFileChooser ) {
        File outFile = receiveFileChooser->getCurrentFile();

        Array<uint8> saveData = receiveBox->getBinary();
        if( saveData.size() == 0 ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        String::empty,
                                        T("No data to save!"),
                                        String::empty);
        } else {
            outFile.deleteFile();
            FileOutputStream *outFileStream = outFile.createOutputStream();
            
            if( !outFileStream || outFileStream->failedToOpen() ) {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            String::empty,
                                            T("File cannot be created!"),
                                            String::empty);
            } else {
                uint8 *data = &saveData.getReference(0);
                outFileStream->write(data, saveData.size());
                delete outFileStream;
                statusLabel->setText(String(saveData.size()) + T(" bytes saved"), true);
            }
        }

        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile ) {
            String recentlyUsedHexFiles = receiveFileChooser->getRecentlyUsedFilenames().joinIntoString(";");
            propertiesFile->setValue(T("recentlyUsedSyxReceiveFiles"), recentlyUsedHexFiles);
            propertiesFile->setValue(T("defaultSyxReceiveFile"), outFile.getFullPathName());
        }
    }
}


//==============================================================================
void SysexToolReceive::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    if( !receiveStartButton->isEnabled() ) { // !enabled means that we are ready for receive
        Array<uint8> data(message.getRawData(), message.getRawDataSize());
        receivedData.addArray(data);
        statusLabel->setText(String(receivedData.size()) + T(" bytes received"), true);
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

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(860, 500);
}

SysexTool::~SysexTool()
{
}

//==============================================================================
void SysexTool::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
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
