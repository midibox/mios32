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
#include "LogBox.h"

class MiosStudio; // forward declaration

class SysexToolSend
    : public Component
    , public TextEditorListener
    , public ButtonListener
    , public SliderListener
    , public FilenameComponentListener
{
public:
    //==============================================================================
    SysexToolSend(MiosStudio *_miosStudio);
    ~SysexToolSend();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged(Slider* slider);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

protected:
    //==============================================================================
    LogBox* sendBox;
    FilenameComponent* sendFileChooser;
    Button* sendStartButton;
    Button* sendStopButton;
    Button* sendEditButton;
    Label*  sendDelayLabel;
    Slider* sendDelaySlider;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    SysexToolSend (const SysexToolSend&);
    const SysexToolSend& operator= (const SysexToolSend&);
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexToolReceive
    : public Component
    , public TextEditorListener
    , public ButtonListener
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
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    void buttonClicked (Button* buttonThatWasClicked);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    LogBox* receiveBox;
    FilenameComponent* receiveFileChooser;
    Button* receiveStartButton;
    Button* receiveStopButton;
    Button* receiveEditButton;
    Button* receiveClearButton;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    SysexToolReceive (const SysexToolReceive&);
    const SysexToolReceive& operator= (const SysexToolReceive&);
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

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    SysexTool (const SysexTool&);
    const SysexTool& operator= (const SysexTool&);
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexToolWindow
    : public DocumentWindow
{
public:
    SysexToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("Sysex Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
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
