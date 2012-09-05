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

    addAndMakeVisible(portNumberWriteLabel = new Label(T("Port:"), T("Remote Port:")));
    portNumberWriteLabel->setJustificationType(Justification::right);

    addAndMakeVisible(portNumberWriteLine = new TextEditor(String::empty));
    portNumberWriteLine->setMultiLine(false);
    portNumberWriteLine->setReturnKeyStartsNewLine(false);
    portNumberWriteLine->setReadOnly(false);
    portNumberWriteLine->setScrollbarsShown(false);
    portNumberWriteLine->setCaretVisible(true);
    portNumberWriteLine->setPopupMenuEnabled(true);
    portNumberWriteLine->setTextToShowWhenEmpty(T("(port number)"), Colours::grey);
    portNumberWriteLine->setInputRestrictions(1000000, T("0123456789"));

    addAndMakeVisible(portNumberReadLabel = new Label(T("Port:"), T("Local Port:")));
    portNumberReadLabel->setJustificationType(Justification::right);

    addAndMakeVisible(portNumberReadLine = new TextEditor(String::empty));
    portNumberReadLine->setMultiLine(false);
    portNumberReadLine->setReturnKeyStartsNewLine(false);
    portNumberReadLine->setReadOnly(false);
    portNumberReadLine->setScrollbarsShown(false);
    portNumberReadLine->setCaretVisible(true);
    portNumberReadLine->setPopupMenuEnabled(true);
    portNumberReadLine->setTextToShowWhenEmpty(T("(port number)"), Colours::grey);
    portNumberReadLine->setInputRestrictions(1000000, T("0123456789"));

    addAndMakeVisible(connectButton = new TextButton(T("Connect Button")));
    connectButton->setButtonText(T("Connect"));
    connectButton->addListener(this);

    addAndMakeVisible(disconnectButton = new TextButton(T("Disconnect Button")));
    disconnectButton->setButtonText(T("Disconnect"));
    disconnectButton->setEnabled(false);
    disconnectButton->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        remoteHostLine->setText(propertiesFile->getValue(T("oscRemoteHost"), String::empty));
        portNumberWriteLine->setText(propertiesFile->getValue(T("oscPortWrite"), "10000"));
        portNumberReadLine->setText(propertiesFile->getValue(T("oscPortRead"), "10000"));
    }

    setSize(800, 4+24+4);
}

OscToolConnect::~OscToolConnect()
{
    deleteAndZero(udpSocket);
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
    portNumberWriteLabel->setBounds(10+75+4+200+10, buttonY, 100, 24);
    portNumberWriteLine->setBounds(10+75+4+200+10+100+4, buttonY, 50, 24);
    portNumberReadLabel->setBounds(10+75+4+200+10+100+4+50+4, buttonY, 75, 24);
    portNumberReadLine->setBounds(10+75+4+200+10+100+4+50+4+75+4, buttonY, 50, 24);
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
            propertiesFile->setValue(T("oscPortWrite"), portNumberWriteLine->getText());
            propertiesFile->setValue(T("oscPortRead"), portNumberReadLine->getText());
        }

        connectButton->setEnabled(false);
        disconnectButton->setEnabled(false); // will be set to true by timer once connection available
        remoteHostLine->setEnabled(false);
        portNumberWriteLine->setEnabled(false);
        portNumberReadLine->setEnabled(false);
        startTimer(1); // check for next message each 1 mS
    } else if( buttonThatWasClicked == disconnectButton ) {
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        remoteHostLine->setEnabled(true);
        portNumberWriteLine->setEnabled(true);
        portNumberReadLine->setEnabled(true);
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
        if( !udpSocket->connect(remoteHostLine->getText(), portNumberReadLine->getText().getIntValue(), portNumberWriteLine->getText().getIntValue()) ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("No connection to remote host"),
                                        T("Check remote host name (or IP) and port numbers!"),
                                        String::empty);

            deleteAndZero(udpSocket);
            connectButton->setEnabled(true);
            disconnectButton->setEnabled(false);
            remoteHostLine->setEnabled(true);
            portNumberWriteLine->setEnabled(true);
            portNumberReadLine->setEnabled(true);
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
OscToolSend::OscToolSend(MiosStudio *_miosStudio, OscToolConnect *_oscToolConnect)
    : miosStudio(_miosStudio)
    , oscToolConnect(_oscToolConnect)
{
    addAndMakeVisible(sendStartButton = new TextButton(T("Send Button")));
    sendStartButton->setButtonText(T("Send"));
    sendStartButton->addListener(this);

    addAndMakeVisible(sendClearButton = new TextButton(T("Clear Button")));
    sendClearButton->setButtonText(T("Clear"));
    sendClearButton->addListener(this);

    addAndMakeVisible(statusLabel = new Label(T("Status"), String::empty));
    statusLabel->setJustificationType(Justification::right);
    addAndMakeVisible(sendBox = new OscTextEditor(statusLabel));

    setSize(800, 200);
}

OscToolSend::~OscToolSend()
{
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
    statusLabel->setBounds(getWidth()-10-500, sendButtonY, 500, 24);

    sendBox->setBounds(4, 32, getWidth()-8, getHeight()-32);
}

//==============================================================================
void OscToolSend::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == sendStartButton ) {
        if( oscToolConnect->udpSocket ) {
            Array<uint8> packet = sendBox->getBinary();

            if( packet.size() ) {
                oscToolConnect->udpSocket->write((unsigned char*)&packet.getReference(0), packet.size());
            } else {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Invalid OSC packet"),
                                            T("The entered packet is invalid (or empty) - check syntax!"),
                                            String::empty);
            }
        } else {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("No connection to remote host"),
                                        T("Please press the connect button to establish connection!"),
                                        String::empty);
        }
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
    addAndMakeVisible(oscToolSend = new OscToolSend(miosStudio, oscToolConnect));

    //                             num   min   max  prefered  
    horizontalLayout.setItemLayout(0, -0.005, -0.9, -0.25); // Send Window
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.005, -0.9, -0.25); // Receive Window

    horizontalDividerBar = new StretchableLayoutResizerBar(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalDividerBar);

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(850, 500);
}

OscTool::~OscTool()
{
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
