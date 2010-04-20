/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * OSC Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "OscTool.h"
#include "MiosStudio.h"
#include "UdpSocket.h"

#define OSC_BUFFER_SIZE 2000

//==============================================================================
OscToolConnect::OscToolConnect(MiosStudio *_miosStudio, OscMonitor* _oscMonitor)
    : miosStudio(_miosStudio)
    , oscMonitor(_oscMonitor)
    , udpSocket(NULL)
{
    addAndMakeVisible(remoteHostLabel = new Label(T("Remote Host:"), T("Remote Host:")));
    remoteHostLabel->setJustificationType(Justification::right);

    addAndMakeVisible(remoteHostLine = new TextEditor(String::empty));
    remoteHostLine->setMultiLine(false);
    remoteHostLine->setReturnKeyStartsNewLine(false);
    remoteHostLine->setReadOnly(false);
    remoteHostLine->setScrollbarsShown(false);
    remoteHostLine->setCaretVisible(true);
    remoteHostLine->setPopupMenuEnabled(true);
    remoteHostLine->setTextToShowWhenEmpty(T("(hostname)"), Colours::grey);

    addAndMakeVisible(portNumberLabel = new Label(T("Port:"), T("Port:")));
    portNumberLabel->setJustificationType(Justification::right);

    addAndMakeVisible(portNumberLine = new TextEditor(String::empty));
    portNumberLine->setMultiLine(false);
    portNumberLine->setReturnKeyStartsNewLine(false);
    portNumberLine->setReadOnly(false);
    portNumberLine->setScrollbarsShown(false);
    portNumberLine->setCaretVisible(true);
    portNumberLine->setPopupMenuEnabled(true);
    portNumberLine->setTextToShowWhenEmpty(T("(port number)"), Colours::grey);
    portNumberLine->setInputRestrictions(1000000, T("0123456789"));

    addAndMakeVisible(connectButton = new TextButton(T("Connect Button")));
    connectButton->setButtonText(T("Connect"));
    connectButton->addButtonListener(this);

    addAndMakeVisible(disconnectButton = new TextButton(T("Disconnect Button")));
    disconnectButton->setButtonText(T("Disconnect"));
    disconnectButton->setEnabled(false);
    disconnectButton->addButtonListener(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        remoteHostLine->setText(propertiesFile->getValue(T("oscRemoteHost"), String::empty));
        portNumberLine->setText(propertiesFile->getValue(T("oscPort"), "10000"));
    }

    setSize(800, 4+24+4);
}

OscToolConnect::~OscToolConnect()
{
    deleteAndZero(udpSocket);
    deleteAllChildren();
}

//==============================================================================
void OscToolConnect::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void OscToolConnect::resized()
{
    int buttonY = 4;
    int buttonWidth = 90;
    int buttonX = getWidth() - 2*buttonWidth - 10-4;
    remoteHostLabel->setBounds(10, buttonY, 75, 24);
    remoteHostLine->setBounds(10+75+4, buttonY, 200, 24);
    portNumberLabel->setBounds(10+75+4+200+10, buttonY, 50, 24);
    portNumberLine->setBounds(10+75+4+200+10+50+4, buttonY, 50, 24);
    connectButton->setBounds(buttonX, buttonY, buttonWidth, 24);
    disconnectButton->setBounds(buttonX + buttonWidth + 10, buttonY, buttonWidth, 24);
}

//==============================================================================
void OscToolConnect::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == connectButton ) {
        // store settings
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile ) {
            propertiesFile->setValue(T("oscRemoteHost"), remoteHostLine->getText());
            propertiesFile->setValue(T("oscPort"), portNumberLine->getText());
        }

        connectButton->setEnabled(false);
        disconnectButton->setEnabled(false); // will be set to true by timer once connection available
        startTimer(1); // check for next message each 1 mS
    } else if( buttonThatWasClicked == disconnectButton ) {
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        stopTimer();
        if( udpSocket )
            udpSocket->disconnect();
        deleteAndZero(udpSocket);
    }
}


//==============================================================================
void OscToolConnect::timerCallback()
{
    unsigned char oscBuffer[OSC_BUFFER_SIZE];

    if( udpSocket == NULL ) {
        stopTimer(); // will be continued if connection established

        udpSocket = new UdpSocket();
        if( !udpSocket->connect(remoteHostLine->getText(), portNumberLine->getText().getIntValue()) ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("No connection to remote host"),
                                        T("Check remote host name (or IP) and port number!"),
                                        String::empty);

            deleteAndZero(udpSocket);
            connectButton->setEnabled(true);
            disconnectButton->setEnabled(false);
        } else {
            disconnectButton->setEnabled(true);
            startTimer(1); // continue timer
        }

        return;
    }

    unsigned size;
    if( (size=udpSocket->read(oscBuffer, OSC_BUFFER_SIZE)) )
        oscMonitor->handleIncomingOscMessage(oscBuffer, size);
}



//==============================================================================
//==============================================================================
//==============================================================================
OscToolSend::OscToolSend(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(sendStartButton = new TextButton(T("Send Button")));
    sendStartButton->setButtonText(T("Send"));
    sendStartButton->addButtonListener(this);

    addAndMakeVisible(sendClearButton = new TextButton(T("Clear Button")));
    sendClearButton->setButtonText(T("Clear"));
    sendClearButton->addButtonListener(this);

    addAndMakeVisible(statusLabel = new Label(T("Status"), String::empty));
    statusLabel->setJustificationType(Justification::right);
    addAndMakeVisible(sendBox = new OscTextEditor(statusLabel));

    setSize(800, 200);
}

OscToolSend::~OscToolSend()
{
    deleteAllChildren();
}

//==============================================================================
void OscToolSend::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void OscToolSend::resized()
{
    int sendButtonY = 4;
    int sendButtonWidth = 72;
    sendStartButton->setBounds(10 + 0*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    sendClearButton->setBounds(10 + 1*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    statusLabel->setBounds(getWidth()-10-150, sendButtonY, 150, 24);

    sendBox->setBounds(4, 32, getWidth()-8, getHeight()-32);
}

//==============================================================================
void OscToolSend::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == sendStartButton ) {
    } else if( buttonThatWasClicked == sendClearButton ) {
        sendBox->clear();
    }
}


//==============================================================================
//==============================================================================
//==============================================================================
OscTool::OscTool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(oscMonitor = new OscMonitor(miosStudio));
    addAndMakeVisible(oscToolConnect = new OscToolConnect(miosStudio, oscMonitor));
    addAndMakeVisible(oscToolSend = new OscToolSend(miosStudio));

    //                             num   min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.005, -0.9, -0.25); // Send Window
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.005, -0.9, -0.25); // Receive Window

    horizontalDividerBar = new StretchableLayoutResizerBar(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalDividerBar);

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(650, 500);
}

OscTool::~OscTool()
{
    deleteAllChildren();
}

//==============================================================================
void OscTool::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void OscTool::resized()
{
    oscToolConnect->setBounds(4, 4, getWidth()-8, oscToolConnect->getHeight());

    Component* hcomps[] = { oscToolSend,
                            horizontalDividerBar,
                            oscMonitor,
    };

    horizontalLayout.layOutComponents(hcomps, 3,
                                      4, oscToolConnect->getHeight()+8,
                                      getWidth() - 8, getHeight() - 8 - oscToolConnect->getHeight()-8,
                                      true,  // lay out above each other
                                      true); // resize the components' heights as well as widths

    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}
