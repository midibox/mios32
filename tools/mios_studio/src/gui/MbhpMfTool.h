/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvTool.h 935 2010-02-28 01:05:46Z tk $
/*
 * MBHP_MF Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBHP_MF_TOOL_H
#define _MBHP_MF_TOOL_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "ConfigTableComponents.h"

class MiosStudio; // forward declaration
class MbhpMfTool; // forward declaration

class MbhpMfToolConfigGlobals
    : public Component
    , public Slider::Listener
    , public ComboBox::Listener
{
public:
    //==============================================================================
    MbhpMfToolConfigGlobals(MbhpMfTool* _mbhpMfTool);
    ~MbhpMfToolConfigGlobals();

    //==============================================================================
    void resized();
    void sliderValueChanged(Slider* slider);
    void comboBoxChanged(ComboBox*);

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

protected:
    MbhpMfTool* mbhpMfTool;
    //==============================================================================
    Label* nameLabel;
    TextEditor* nameEditor;
    Label* numberFadersLabel;
    Slider* numberFadersSlider;
    Label* operationModeLabel;
    ComboBox* operationModeComboBox;
    Label* midiChannelLabel;
    Slider* midiChannelSlider;
    Label* mergerLabel;
    ComboBox* mergerComboBox;
    Label* pwmStepsLabel;
    Slider* pwmStepsSlider;
    Label* ainDeadbandLabel;
    Slider* ainDeadbandSlider;
    Label* mfDeadbandLabel;
    Slider* mfDeadbandSlider;
    Label* touchSensorModeLabel;
    ComboBox* touchSensorModeComboBox;
    Label* touchSensorSensitivityLabel;
    Slider* touchSensorSensitivitySlider;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolCalibrationTable
    : public Component
    , public TableListBoxModel
    , public ConfigTableController
{
public:
    //==============================================================================
    MbhpMfToolCalibrationTable(MbhpMfTool* _mbhpMfTool);
    ~MbhpMfToolCalibrationTable();

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
    Array<uint8> mfMode;
    Array<uint16> mfMinValue;
    Array<uint16> mfMaxValue;
    Array<uint8> mfMinDutyUp;
    Array<uint8> mfMaxDutyUp;
    Array<uint8> mfMinDutyDown;
    Array<uint8> mfMaxDutyDown;

    int numRows;

    //==============================================================================
    TableListBox* table;
    Font font;

protected:
    MbhpMfTool* mbhpMfTool;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolCalibrationCurve
    : public Component
{
public:
    //==============================================================================
    MbhpMfToolCalibrationCurve();
    ~MbhpMfToolCalibrationCurve();

    void paint(Graphics& g);

    void setTrace(const Array<uint16>& newTrace);

protected:
    //==============================================================================
    Array<uint16> traceMem;
};



//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolCalibration
    : public Component
    , public Button::Listener
    , public Slider::Listener
    , public Timer
{
public:
    //==============================================================================
    MbhpMfToolCalibration(MbhpMfTool* _mbhpMfTool);
    ~MbhpMfToolCalibration();

    void resized();

    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged(Slider* slider);

    //==============================================================================
    void timerCallback();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

    //==============================================================================
    MbhpMfToolCalibrationTable* calibrationTable;
    MbhpMfToolCalibrationCurve* calibrationCurve;

protected:
    MbhpMfTool* mbhpMfTool;
    //==============================================================================
    Label*                      calibrationCurveLabel;
    Label*                      traceSliderLabel;
    Slider*                     traceSlider;
    Label*                      traceScaleSliderLabel;
    Slider*                     traceScaleSlider;
    Label*                      directMoveSliderLabel;
    Slider*                     directMoveSlider;

    TextButton* upperButton;
    TextButton* lowerButton;
    TextButton* middleButton;
    TextButton* slashButton;
    TextButton* backslashButton;
    TextButton* slowUpperButton;
    TextButton* slowLowerButton;
    TextButton* slowMiddleButton;
    TextButton* slowSineButton;

    Label*      delayLabel;
    Slider*     delaySlider;
    Label*      delayStepsLabel;
    Slider*     delayStepsSlider;

    Array<uint16> targetMfValues;
    Array<uint16> currentMfValues;

    bool timerGeneratesSineWaveform;
    float sinePhase;
};



//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolConfig
    : public TabbedComponent
{
public:
    //==============================================================================
    MbhpMfToolConfig(MbhpMfTool *_mbhpMfTool);
    ~MbhpMfToolConfig();

    //==============================================================================
    void getDump(Array<uint8> &syxDump);
    void setDump(const Array<uint8> &syxDump);

    //==============================================================================
    MbhpMfToolConfigGlobals* configGlobals;
    MbhpMfToolCalibration* calibration;

protected:
    //==============================================================================
    MbhpMfTool* mbhpMfTool;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolControl
    : public Component
    , public Button::Listener
    , public Slider::Listener
    , public Timer
{
public:
    //==============================================================================
    MbhpMfToolControl(MiosStudio *_miosStudio, MbhpMfToolConfig *_mbhpMfToolConfig);
    ~MbhpMfToolControl();

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

    //==============================================================================
    void sysexUpdateRequest(void);

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
    bool sendCompleteDump;
    bool sendChangedDump;
    bool dumpRequested;
    bool dumpReceived;
    bool checksumError;
    bool dumpSent;

    Array<uint8> currentSyxDump;

    //==============================================================================
    MiosStudio *miosStudio;
    MbhpMfToolConfig *mbhpMfToolConfig;

    //==============================================================================
    double progress;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfTool
    : public Component
{
public:
    //==============================================================================
    MbhpMfTool(MiosStudio *_miosStudio);
    ~MbhpMfTool();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    MbhpMfToolControl* mbhpMfToolControl;
    MbhpMfToolConfig* mbhpMfToolConfig;

    //==============================================================================
    MiosStudio *miosStudio;

protected:
    //==============================================================================
    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

};


//==============================================================================
//==============================================================================
//==============================================================================
class MbhpMfToolWindow
    : public DocumentWindow
{
public:
    MbhpMfToolWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("MBHP_MF_NG Tool"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        mbhpMfTool = new MbhpMfTool(_miosStudio);
        setContentComponent(mbhpMfTool, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~MbhpMfToolWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    MbhpMfTool *mbhpMfTool;

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            mbhpMfTool->mbhpMfToolControl->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _MBHP_MF_TOOL_H */
