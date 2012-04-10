/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_TOOL_H
#define _MB_CV_TOOL_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "ConfigTableComponents.h"

class MiosStudio; // forward declaration

class MbCvToolConfigGlobals
    : public Component
{
public:
    //==============================================================================
    MbCvToolConfigGlobals();
    ~MbCvToolConfigGlobals();

    //==============================================================================
    void resized();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    Label* mergerLabel;
    ComboBox* mergerComboBox;
    Label* clockDividerLabel;
    ComboBox* clockDividerComboBox;
    Label* nameLabel;
    TextEditor* nameEditor;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbCvToolConfigChannels
    : public Component
    , public TableListBoxModel
    , public ConfigTableController
{
public:
    //==============================================================================
    MbCvToolConfigChannels();
    ~MbCvToolConfigChannels();

    int getNumRows();
    void paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected);
    void paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected);
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate);

    void resized();

    //==============================================================================
    int getTableValue(const int rowNumber, const int columnId);
    void setTableValue(const int rowNumber, const int columnId, const int newValue);

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    TableListBox* table;
    Font font;

    Array<uint8> cvChannel;
    Array<uint8> cvEvent;
    Array<uint8> cvLegato;
    Array<uint8> cvPoly;
    Array<uint8> cvGateInverted;
    Array<uint8> cvPitchrange;
    Array<uint8> cvSplitLower;
    Array<uint8> cvSplitUpper;
    Array<int> cvTransposeOctave;
    Array<int> cvTransposeSemitones;
    Array<uint8> cvCcNumber;
    Array<uint8> cvCurve;
    Array<uint8> cvSlewRate;

    int numRows;
};



//==============================================================================
//==============================================================================
//==============================================================================
class MbCvToolConfig
    : public TabbedComponent
{
public:
    //==============================================================================
    MbCvToolConfig(MiosStudio *_miosStudio);
    ~MbCvToolConfig();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    MbCvToolConfigGlobals* configGlobals;
    MbCvToolConfigChannels* configChannels;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbCvToolControl
    : public Component
    , public ButtonListener
    , public SliderListener
    , public Timer
{
public:
    //==============================================================================
    MbCvToolControl(MiosStudio *_miosStudio, MbCvToolConfig *_mbCvToolConfig);
    ~MbCvToolControl();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged(Slider* slider);

    //==============================================================================
    void timerCallback();

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);


    //==============================================================================
    bool loadSyx(File &syxFile);
    bool saveSyx(File &syxFile);

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    Button* loadButton;
    Button* saveButton;
    Button* sendButton;
    Button* receiveButton;
    Label*      deviceIdLabel;
    Slider*     deviceIdSlider;
    Label*      patchLabel;
    Slider*     patchSlider;
    ProgressBar* progressBar;

    //==============================================================================
    File syxFile;
    bool receiveDump;
    bool dumpRequested;
    bool dumpReceived;
    bool checksumError;
    bool dumpSent;
    Array<uint8> currentSyxDump;

    //==============================================================================
    MiosStudio *miosStudio;
    MbCvToolConfig *mbCvToolConfig;

    //==============================================================================
    double progress;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbCvTool
    : public Component
{
public:
    //==============================================================================
    MbCvTool(MiosStudio *_miosStudio);
    ~MbCvTool();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    MbCvToolControl* mbCvToolControl;
    MbCvToolConfig* mbCvToolConfig;

protected:
    //==============================================================================
    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbCvToolWindow
    : public DocumentWindow
{
public:
    MbCvToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("MIDIbox CV V1 Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        mbCvTool = new MbCvTool(_miosStudio);
        setContentComponent(mbCvTool, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~MbCvToolWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    MbCvTool *mbCvTool;

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            mbCvTool->mbCvToolControl->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _MB_CV_TOOL_H */
