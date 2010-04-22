/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Osc Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _OSC_TOOL_H
#define _OSC_TOOL_H

#include "../includes.h"
#include "OscTextEditor.h"
#include "OscMonitor.h"
#include "UdpSocket.h"

class MiosStudio; // forward declaration

class OscToolConnect
    : public Component
    , public ButtonListener
    , public Timer
{
public:
    //==============================================================================
    OscToolConnect(MiosStudio *_miosStudio, OscMonitor* _oscMonitor);
    ~OscToolConnect();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);

    //==============================================================================
    void timerCallback();

    //==============================================================================
    UdpSocket* udpSocket;

protected:
    //==============================================================================
    Label* remoteHostLabel;
    TextEditor* remoteHostLine;
    Label* portNumberReadLabel;
    TextEditor* portNumberReadLine;
    Label* portNumberWriteLabel;
    TextEditor* portNumberWriteLine;
    Button* connectButton;
    Button* disconnectButton;

    //==============================================================================
    MiosStudio *miosStudio;
    OscMonitor* oscMonitor;

    //==============================================================================
};


//==============================================================================
//==============================================================================
//==============================================================================
class OscToolSend
    : public Component
    , public ButtonListener
{
public:
    //==============================================================================
    OscToolSend(MiosStudio *_miosStudio, OscToolConnect* _oscToolConnect);
    ~OscToolSend();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);

protected:
    //==============================================================================
    Label *statusLabel;
    OscTextEditor* sendBox;
    Button* sendStartButton;
    Button* sendClearButton;

    //==============================================================================
    MiosStudio* miosStudio;
    OscToolConnect* oscToolConnect;

    //==============================================================================
    Array<uint8> sendData;
};


//==============================================================================
//==============================================================================
//==============================================================================
class OscTool
    : public Component
{
public:
    //==============================================================================
    OscTool(MiosStudio *_miosStudio);
    ~OscTool();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    OscToolConnect* oscToolConnect;
    OscToolSend* oscToolSend;
    OscMonitor* oscMonitor;

protected:
    //==============================================================================
    StretchableLayoutManager horizontalLayout;
    StretchableLayoutResizerBar* horizontalDividerBar;

    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class OscToolWindow
    : public DocumentWindow
{
public:
    OscToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("Osc Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        oscTool = new OscTool(_miosStudio);
        setContentComponent(oscTool, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~OscToolWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    OscTool *oscTool;

    //==============================================================================
    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _OSC_TOOL_H */
