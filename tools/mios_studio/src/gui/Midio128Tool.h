/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIO128 Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO128_TOOL_H
#define _MIDIO128_TOOL_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "ConfigTableComponents.h"

class MiosStudio; // forward declaration

class Midio128ToolConfigGlobals
    : public Component
{
public:
    //==============================================================================
    Midio128ToolConfigGlobals();
    ~Midio128ToolConfigGlobals();

    //==============================================================================
    void resized();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    Label* mergerLabel;
    ComboBox* mergerComboBox;
    Label* debounceLabel;
    Slider* debounceSlider;
    Label* altPrgChangesLabel;
    ComboBox* altPrgChangesComboBox;
    Label* forwardInToOutLabel;
    ComboBox* forwardInToOutComboBox;
    Label* inverseInputsLabel;
    ComboBox* inverseInputsComboBox;
    Label* inverseOutputsLabel;
    ComboBox* inverseOutputsComboBox;
    Label* allNotesOffChannelLabel;
    ComboBox* allNotesOffChannelComboBox;
    Label* touchsensorSensitivityLabel;
    Slider* touchsensorSensitivitySlider;
};


//==============================================================================
//==============================================================================
//==============================================================================
class Midio128ToolConfigDout
    : public Component
    , public TableListBoxModel
    , public ConfigTableController
{
public:
    //==============================================================================
    Midio128ToolConfigDout();
    ~Midio128ToolConfigDout();

    int getNumRows();
    void paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected);
    void paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected);
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate);

    void resized();

    //==============================================================================
    int getTableValue(const int rowNumber, const int columnId);
    void setTableValue(const int rowNumber, const int columnId, const int newValue);
    String getTableString(const int rowNumber, const int columnId) { return String(T("???")); };
    void setTableString(const int rowNumber, const int columnId, const String newString) { };

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    TableListBox* table;
    Font font;

    Array<uint8> doutChannel;
    Array<uint8> doutEvent;
    Array<uint8> doutParameter;

    int numRows;
};


//==============================================================================
//==============================================================================
//==============================================================================
class Midio128ToolConfigDin
    : public Component
    , public TableListBoxModel
    , public ConfigTableController
{
public:
    //==============================================================================
    Midio128ToolConfigDin();
    ~Midio128ToolConfigDin();

    int getNumRows();
    void paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected);
    void paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected);
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate);

    void resized();

    //==============================================================================
    int getTableValue(const int rowNumber, const int columnId);
    void setTableValue(const int rowNumber, const int columnId, const int newValue);
    String getTableString(const int rowNumber, const int columnId) { return String(T("???")); };
    void setTableString(const int rowNumber, const int columnId, const String newString) { };

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    TableListBox* table;
    Font font;

    Array<uint8> dinChannel;
    Array<uint8> dinOnEvent;
    Array<uint8> dinOnParameter1;
    Array<uint8> dinOnParameter2;
    Array<uint8> dinOffEvent;
    Array<uint8> dinOffParameter1;
    Array<uint8> dinOffParameter2;
    Array<uint8> dinMode;

    int numRows;
};



//==============================================================================
//==============================================================================
//==============================================================================
class Midio128ToolConfig
    : public TabbedComponent
{
public:
    //==============================================================================
    Midio128ToolConfig(MiosStudio *_miosStudio);
    ~Midio128ToolConfig();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    //==============================================================================
    Midio128ToolConfigGlobals* configGlobals;
    Midio128ToolConfigDin* configDin;
    Midio128ToolConfigDout* configDout;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class Midio128ToolControl
    : public Component
    , public ButtonListener
    , public SliderListener
    , public Timer
{
public:
    //==============================================================================
    Midio128ToolControl(MiosStudio *_miosStudio, Midio128ToolConfig *_midio128ToolConfig);
    ~Midio128ToolControl();

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
    ProgressBar* progressBar;

    //==============================================================================
    File syxFile;
    int syxBlock;
    bool receiveDump;
    bool dumpReceived;
    bool checksumError;
    Array<uint8> currentSyxDump;

    //==============================================================================
    MiosStudio *miosStudio;
    Midio128ToolConfig *midio128ToolConfig;

    //==============================================================================
    double progress;
};


//==============================================================================
//==============================================================================
//==============================================================================
class Midio128Tool
    : public Component
{
public:
    //==============================================================================
    Midio128Tool(MiosStudio *_miosStudio);
    ~Midio128Tool();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    Midio128ToolControl* midio128ToolControl;
    Midio128ToolConfig* midio128ToolConfig;

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
class Midio128ToolWindow
    : public DocumentWindow
{
public:
    Midio128ToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("MIDIO128 V2 Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        midio128Tool = new Midio128Tool(_miosStudio);
        setContentComponent(midio128Tool, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~Midio128ToolWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    Midio128Tool *midio128Tool;

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            midio128Tool->midio128ToolControl->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _MIDIO128_TOOL_H */
