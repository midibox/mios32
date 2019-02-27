/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Sysex Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_TOOL_H
#define _SYSEX_TOOL_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "HexTextEditor.h"

class MiosStudio; // forward declaration

class SysexToolSend
    : public Component
    , public Button::Listener
    , public Slider::Listener
    , public FilenameComponentListener
    , public Timer
{
public:
    //==============================================================================
    SysexToolSend(MiosStudio *_miosStudio);
    ~SysexToolSend();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    bool sendSyxFile(const String& filename, const bool& sendImmediately);
    bool sendSyxInProgress(void);

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged(Slider* slider);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

    //==============================================================================
    void timerCallback();

protected:
    //==============================================================================
    Label *statusLabel;
    HexTextEditor* sendBox;
    FilenameComponent* sendFileChooser;
    Button* sendStartButton;
    Button* sendStopButton;
    Button* sendClearButton;
    Label*  sendDelayLabel;
    Slider* sendDelaySlider;
    ProgressBar* progressBar;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    Array<uint8> sendData;
    int numBytesToSend;
    int numBytesSent;
    double progress;
    int previousProgress;
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexToolReceive
    : public Component
    , public Button::Listener
    , public FilenameComponentListener
{
public:
    //==============================================================================
    SysexToolReceive(MiosStudio *_miosStudio);
    ~SysexToolReceive();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    Label *statusLabel;
    HexTextEditor* receiveBox;
    FilenameComponent* receiveFileChooser;
    Button* receiveStartButton;
    Button* receiveStopButton;
    Button* receiveClearButton;
    Label*  receiveNumBytesLabel;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    Array<uint8> receivedData;
};



//==============================================================================
//==============================================================================
//==============================================================================
class SysexTool
    : public Component
{
public:
    //==============================================================================
    SysexTool(MiosStudio *_miosStudio);
    ~SysexTool();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    SysexToolSend* sysexToolSend;
    SysexToolReceive* sysexToolReceive;

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
class SysexToolWindow
    : public DocumentWindow
{
public:
    SysexToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("SysEx Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        sysexTool = new SysexTool(_miosStudio);
        setContentComponent(sysexTool, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~SysexToolWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    SysexTool *sysexTool;

    //==============================================================================
    bool sendSyxInProgress(void)
    {
        return sysexTool->sysexToolSend->sendSyxInProgress();
    }

    bool sendSyxFile(const String& filename)
    {
        return sysexTool->sysexToolSend->sendSyxFile(filename, true);
    }

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            sysexTool->sysexToolReceive->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _SYSEX_TOOL_H */
