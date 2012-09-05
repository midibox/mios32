/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: MbCvTool.cpp 936 2010-02-28 01:27:18Z tk $
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

#include "MbhpMfTool.h"
#include "MiosStudio.h"


MbhpMfToolConfigGlobals::MbhpMfToolConfigGlobals(MbhpMfTool* _mbhpMfTool)
    : mbhpMfTool(_mbhpMfTool)
{
    addAndMakeVisible(nameLabel = new Label(String::empty, T("Patch Name:")));
    nameLabel->setJustificationType(Justification::right);
    addAndMakeVisible(nameEditor = new TextEditor(String::empty));
    nameEditor->setTextToShowWhenEmpty(T("<No Name>"), Colours::grey);
    nameEditor->setInputRestrictions(16);

    addAndMakeVisible(numberFadersLabel = new Label(String::empty, T("Number of Faders:")));
    numberFadersLabel->setJustificationType(Justification::right);
    addAndMakeVisible(numberFadersSlider = new Slider(T("Number of Faders")));
    numberFadersSlider->setWantsKeyboardFocus(true);
    numberFadersSlider->setSliderStyle(Slider::LinearHorizontal);
    numberFadersSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    numberFadersSlider->setRange(1, 8, 1);
    numberFadersSlider->setValue(8);
    numberFadersSlider->addListener(this);

    addAndMakeVisible(operationModeLabel = new Label(String::empty, T("Operation Mode:")));
    operationModeLabel->setJustificationType(Justification::right);
    addAndMakeVisible(operationModeComboBox = new ComboBox(String::empty));
    operationModeComboBox->setWantsKeyboardFocus(true);
    operationModeComboBox->addItem(T("PitchBender Chn#1..#8"), 1);
    operationModeComboBox->addItem(T("PitchBender Chn#9..#16"), 2);
    operationModeComboBox->addItem(T("CC#7 Chn#1..#8"), 3);
    operationModeComboBox->addItem(T("CC#7 Chn#9..#16"), 4);
    operationModeComboBox->addItem(T("CC#0..#7 Chn#1"), 5);
    operationModeComboBox->addItem(T("CC#8..#15 Chn#1"), 6);
    operationModeComboBox->addItem(T("CC#16..#23 Chn#1"), 7);
    operationModeComboBox->addItem(T("CC#24..#31 Chn#1"), 8);
    operationModeComboBox->addItem(T("CC#32..#39 Chn#1"), 9);
    operationModeComboBox->addItem(T("CC#40..#47 Chn#1"), 10);
    operationModeComboBox->addItem(T("CC#48..#55 Chn#1"), 11);
    operationModeComboBox->addItem(T("CC#56..#63 Chn#1"), 12);
    operationModeComboBox->addItem(T("CC#64..#71 Chn#1"), 13);
    operationModeComboBox->addItem(T("CC#72..#79 Chn#1"), 14);
    operationModeComboBox->addItem(T("CC#80..#87 Chn#1"), 15);
    operationModeComboBox->addItem(T("CC#88..#95 Chn#1"), 16);
    operationModeComboBox->addItem(T("CC#96..#103 Chn#1"), 17);
    operationModeComboBox->addItem(T("CC#104..#111 Chn#1"), 18);
    operationModeComboBox->addItem(T("CC#112..#119 Chn#1"), 19);
    operationModeComboBox->addItem(T("CC#120..#127 Chn#1"), 20);
    operationModeComboBox->addItem(T("Emulated Logic Control"), 21);
    operationModeComboBox->addItem(T("Emulated Logic Control Extension"), 22);
    operationModeComboBox->addItem(T("Emulated Mackie Control"), 23);
    operationModeComboBox->addItem(T("Emulated Mackie Control Extension"), 24);
    operationModeComboBox->addItem(T("Emulated Mackie HUI"), 25);
    operationModeComboBox->addItem(T("Emulated Motormix"), 26);
    operationModeComboBox->setSelectedId(1, true);
    operationModeComboBox->addListener(this);

    addAndMakeVisible(mergerLabel = new Label(String::empty, T("MIDI Merger:")));
    mergerLabel->setJustificationType(Justification::right);
    addAndMakeVisible(mergerComboBox = new ComboBox(String::empty));
    mergerComboBox->setWantsKeyboardFocus(true);
    mergerComboBox->addItem(T("Disabled"), 1);
    mergerComboBox->addItem(T("Enabled (received MIDI events forwarded to MIDI Out)"), 2);
    mergerComboBox->addItem(T("MIDIbox Link Endpoint (last core in the MIDIbox chain)"), 3);
    mergerComboBox->addItem(T("MIDIbox Link Forwarding Point (core in a MIDIbox chain)"), 4);
    mergerComboBox->setSelectedId(1, true);
    mergerComboBox->addListener(this);

    addAndMakeVisible(pwmStepsLabel = new Label(String::empty, T("PWM Steps:")));
    pwmStepsLabel->setJustificationType(Justification::right);
    addAndMakeVisible(pwmStepsSlider = new Slider(T("PWM Period")));
    pwmStepsSlider->setWantsKeyboardFocus(true);
    pwmStepsSlider->setSliderStyle(Slider::LinearHorizontal);
    pwmStepsSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    pwmStepsSlider->setRange(0, 255, 1);
    pwmStepsSlider->setValue(64);
    pwmStepsSlider->addListener(this);

    addAndMakeVisible(ainDeadbandLabel = new Label(String::empty, T("AIN Deadband:")));
    ainDeadbandLabel->setJustificationType(Justification::right);
    addAndMakeVisible(ainDeadbandSlider = new Slider(T("AIN Deadband")));
    ainDeadbandSlider->setWantsKeyboardFocus(true);
    ainDeadbandSlider->setSliderStyle(Slider::LinearHorizontal);
    ainDeadbandSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    ainDeadbandSlider->setRange(0, 63, 1);
    ainDeadbandSlider->setValue(3);
    ainDeadbandSlider->addListener(this);

    addAndMakeVisible(mfDeadbandLabel = new Label(String::empty, T("MF Deadband:")));
    mfDeadbandLabel->setJustificationType(Justification::right);
    addAndMakeVisible(mfDeadbandSlider = new Slider(T("MF Deadband")));
    mfDeadbandSlider->setWantsKeyboardFocus(true);
    mfDeadbandSlider->setSliderStyle(Slider::LinearHorizontal);
    mfDeadbandSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    mfDeadbandSlider->setRange(0, 63, 1);
    mfDeadbandSlider->setValue(3);
    mfDeadbandSlider->addListener(this);

    addAndMakeVisible(touchSensorModeLabel = new Label(String::empty, T("TouchSensor Mode:")));
    touchSensorModeLabel->setJustificationType(Justification::right);
    addAndMakeVisible(touchSensorModeComboBox = new ComboBox(String::empty));
    touchSensorModeComboBox->setWantsKeyboardFocus(true);
    touchSensorModeComboBox->addItem(T("Disabled"), 1);
    touchSensorModeComboBox->addItem(T("Pressed/Depressed Events forwarded to MIDI Out"), 2);
    touchSensorModeComboBox->addItem(T("Like previous, but additionally motors will be suspended"), 3);
    touchSensorModeComboBox->addItem(T("Like previous, but additionally no fader event as long as sensor not pressed"), 4);
    touchSensorModeComboBox->setSelectedId(2, true);
    touchSensorModeComboBox->addListener(this);

    addAndMakeVisible(touchSensorSensitivityLabel = new Label(String::empty, T("Touchsensor Sensitivity:")));
    touchSensorSensitivityLabel->setJustificationType(Justification::right);
    addAndMakeVisible(touchSensorSensitivitySlider = new Slider(T("Touchsensor Sensitivity")));
    touchSensorSensitivitySlider->setWantsKeyboardFocus(true);
    touchSensorSensitivitySlider->setSliderStyle(Slider::LinearHorizontal);
    touchSensorSensitivitySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    touchSensorSensitivitySlider->setRange(1, 32, 1);
    touchSensorSensitivitySlider->setValue(3);
    touchSensorSensitivitySlider->addListener(this);

}


MbhpMfToolConfigGlobals::~MbhpMfToolConfigGlobals()
{
}

//==============================================================================
void MbhpMfToolConfigGlobals::resized()
{
    int labelX = 10;
    int labelY0 = 10;
    int labelYOffset = 28;
    int labelWidth = 250;
    int labelHeight = 24;

    int controlX = labelX + labelWidth + 20;
    int controlY0 = labelY0;
    int controlYOffset = labelYOffset;
    int controlWidth = getWidth()-labelWidth-40-2*labelX;
    int controlHeight = labelHeight;

    nameLabel->setBounds(labelX, labelY0 + 0*labelYOffset, labelWidth, labelHeight);
    nameEditor->setBounds(controlX, controlY0 + 0*controlYOffset, controlWidth, controlHeight);

    numberFadersLabel->setBounds(labelX, labelY0 + 1*labelYOffset, labelWidth, labelHeight);
    numberFadersSlider->setBounds(controlX, controlY0 + 1*controlYOffset, controlWidth, controlHeight);

    operationModeLabel->setBounds(labelX, labelY0 + 2*labelYOffset, labelWidth, labelHeight);
    operationModeComboBox->setBounds(controlX, controlY0 + 2*controlYOffset, controlWidth, controlHeight);

    mergerLabel->setBounds(labelX, labelY0 + 3*labelYOffset, labelWidth, labelHeight);
    mergerComboBox->setBounds(controlX, controlY0 + 3*controlYOffset, controlWidth, controlHeight);

    pwmStepsLabel->setBounds(labelX, labelY0 + 4*labelYOffset, labelWidth, labelHeight);
    pwmStepsSlider->setBounds(controlX, controlY0 + 4*controlYOffset, controlWidth, controlHeight);

    ainDeadbandLabel->setBounds(labelX, labelY0 + 5*labelYOffset, labelWidth, labelHeight);
    ainDeadbandSlider->setBounds(controlX, controlY0 + 5*controlYOffset, controlWidth, controlHeight);

    mfDeadbandLabel->setBounds(labelX, labelY0 + 6*labelYOffset, labelWidth, labelHeight);
    mfDeadbandSlider->setBounds(controlX, controlY0 + 6*controlYOffset, controlWidth, controlHeight);

    touchSensorModeLabel->setBounds(labelX, labelY0 + 7*labelYOffset, labelWidth, labelHeight);
    touchSensorModeComboBox->setBounds(controlX, controlY0 + 7*controlYOffset, controlWidth, controlHeight);

    touchSensorSensitivityLabel->setBounds(labelX, labelY0 + 8*labelYOffset, labelWidth, labelHeight);
    touchSensorSensitivitySlider->setBounds(controlX, controlY0 + 8*controlYOffset, controlWidth, controlHeight);

}


//==============================================================================
void MbhpMfToolConfigGlobals::sliderValueChanged(Slider* slider)
{
    if( slider == pwmStepsSlider ) {
        pwmStepsLabel->setText(String::formatted(T("PWM Steps (resulting period: %4.2f mS):"), (float)(slider->getValue()*0.05)), true);
    }

    // request SysEx update
    if( mbhpMfTool->mbhpMfToolControl != NULL )
        mbhpMfTool->mbhpMfToolControl->sysexUpdateRequest();
}

void MbhpMfToolConfigGlobals::comboBoxChanged(ComboBox*)
{
    // request SysEx update
    if( mbhpMfTool->mbhpMfToolControl != NULL )
        mbhpMfTool->mbhpMfToolControl->sysexUpdateRequest();
}


//==============================================================================
void MbhpMfToolConfigGlobals::getDump(Array<uint8> &syxDump)
{
    int numberFaders = numberFadersSlider->getValue();
    int operationMode = operationModeComboBox->getSelectedId()-1;
    int merger = mergerComboBox->getSelectedId()-1;
    int pwmSteps = pwmStepsSlider->getValue();
    int ainDeadband = ainDeadbandSlider->getValue();
    int mfDeadband = mfDeadbandSlider->getValue();
    int touchSensorMode = touchSensorModeComboBox->getSelectedId()-1;
    int touchSensorSensitivity = touchSensorSensitivitySlider->getValue();

    int i;
    String txt = nameEditor->getText();
    for(i=0; i<16 && i<txt.length(); ++i)
        syxDump.set(i, txt[i]);
    while( i < 16 ) {
        syxDump.set(i, 0);
        i++;
    }
    syxDump.set(0x10, numberFaders);
    syxDump.set(0x11, operationMode);
    syxDump.set(0x12, merger);
    syxDump.set(0x13, pwmSteps);
    syxDump.set(0x14, ainDeadband);
    syxDump.set(0x15, mfDeadband);
    syxDump.set(0x16, touchSensorMode);
    syxDump.set(0x17, touchSensorSensitivity);
    
}

void MbhpMfToolConfigGlobals::setDump(const Array<uint8> &syxDump)
{
    int numberFaders = syxDump[0x10];
    int operationMode = syxDump[0x11];
    int merger = syxDump[0x12];
    int pwmSteps = syxDump[0x13];
    int ainDeadband = syxDump[0x14];
    int mfDeadband = syxDump[0x15];
    int touchSensorMode = syxDump[0x16];
    int touchSensorSensitivity = syxDump[0x17];

    String txt;
    for(int i=0; i<16 && syxDump[i]; ++i) {
        char dummy = syxDump[i];
        txt += String(&dummy, 1);
    }
    nameEditor->setText(txt);

    numberFadersSlider->setValue(numberFaders);
    operationModeComboBox->setSelectedId(operationMode+1);
    mergerComboBox->setSelectedId(merger+1);
    pwmStepsSlider->setValue(pwmSteps);
    ainDeadbandSlider->setValue(ainDeadband);
    mfDeadbandSlider->setValue(mfDeadband);
    touchSensorModeComboBox->setSelectedId(touchSensorMode+1);
    touchSensorSensitivitySlider->setValue(touchSensorSensitivity);
}


//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfToolCalibrationTable::MbhpMfToolCalibrationTable(MbhpMfTool* _mbhpMfTool)
    : mbhpMfTool(_mbhpMfTool)
    , font(14.0f)
    , numRows(8)
{
    for(int fader=0; fader<numRows; ++fader) {
        mfMode.add(0x01); // enabled and not inverted
        mfMinValue.add(20); // Default Min Value
        mfMaxValue.add(1000); // Default Max Value
        mfMinDutyUp.add(48); // Default Min Duty Upward Direction
        mfMaxDutyUp.add(64); // Default Max Duty Upward Direction
        mfMinDutyDown.add(48); // Default Min Duty Upward Direction
        mfMaxDutyDown.add(64); // Default Max Duty Upward Direction
    }

    addAndMakeVisible(table = new TableListBox(T("Motorfader Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    table->getHeader().addColumn(T("MF"), 1, 25);
    table->getHeader().addColumn(T("Use"), 2, 40);
    table->getHeader().addColumn(T("InvM"), 3, 40);
    table->getHeader().addColumn(T("MinValue"), 4, 80);
    table->getHeader().addColumn(T("MaxValue"), 5, 80);
    table->getHeader().addColumn(T("MinDutyUp"), 6, 80);
    table->getHeader().addColumn(T("MaxDutyUp"), 7, 80);
    table->getHeader().addColumn(T("MinDutyDn"), 8, 80);
    table->getHeader().addColumn(T("MaxDutyDn"), 9, 80);

    setSize(400, 200);
}

MbhpMfToolCalibrationTable::~MbhpMfToolCalibrationTable()
{
}

//==============================================================================
int MbhpMfToolCalibrationTable::getNumRows()
{
    return numRows;
}

void MbhpMfToolCalibrationTable::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);
}

void MbhpMfToolCalibrationTable::paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(Colours::black);
    g.setFont(font);

    switch( columnId ) {
    case 1: {
        const String text(rowNumber);
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;
    }

    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}


Component* MbhpMfToolCalibrationTable::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    switch( columnId ) {
    case 2:
    case 3: {
        ConfigTableToggleButton *toggleButton = (ConfigTableToggleButton *)existingComponentToUpdate;

        if( toggleButton == 0 )
            toggleButton = new ConfigTableToggleButton(*this);

        toggleButton->setRowAndColumn(rowNumber, columnId);
        return toggleButton;
    } break;

    case 4: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 255);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 5: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(768, 1023);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 6:
    case 7:
    case 8:
    case 9: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 255);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;
    }

    return 0;
}


//==============================================================================
void MbhpMfToolCalibrationTable::resized()
{
    // position our table with a gap around its edge
    table->setBoundsInset(BorderSize(8));
}


//==============================================================================
int MbhpMfToolCalibrationTable::getTableValue(const int rowNumber, const int columnId)
{
    switch( columnId ) {
    case 1: return rowNumber + 1; // doesn't work! :-/
    case 2: return (mfMode[rowNumber] & 1) ? 1 : 0;
    case 3: return (mfMode[rowNumber] & 2) ? 1 : 0;
    case 4: return mfMinValue[rowNumber];
    case 5: return mfMaxValue[rowNumber];
    case 6: return mfMinDutyUp[rowNumber];
    case 7: return mfMaxDutyUp[rowNumber];
    case 8: return mfMinDutyDown[rowNumber];
    case 9: return mfMaxDutyDown[rowNumber];
    }
    return 0;
}

void MbhpMfToolCalibrationTable::setTableValue(const int rowNumber, const int columnId, const int newValue)
{
    switch( columnId ) {
    case 2: mfMode.set(rowNumber, (mfMode[rowNumber] & 0xfe) | (newValue ? 1 : 0)); break;
    case 3: mfMode.set(rowNumber, (mfMode[rowNumber] & 0xfd) | (newValue ? 2 : 0)); break;
    case 4: mfMinValue.set(rowNumber, newValue); break;
    case 5: mfMaxValue.set(rowNumber, newValue); break;
    case 6: mfMinDutyUp.set(rowNumber, newValue); break;
    case 7: mfMaxDutyUp.set(rowNumber, newValue); break;
    case 8: mfMinDutyDown.set(rowNumber, newValue); break;
    case 9: mfMaxDutyDown.set(rowNumber, newValue); break;
    }

    // request SysEx update
    if( mbhpMfTool->mbhpMfToolControl != NULL )
        mbhpMfTool->mbhpMfToolControl->sysexUpdateRequest();
}


//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfToolCalibrationCurve::MbhpMfToolCalibrationCurve()
{
}


MbhpMfToolCalibrationCurve::~MbhpMfToolCalibrationCurve()
{
}

//==============================================================================
// set from MbhpMfToolControl::handleIncomingMidiMessage once trace has been received
void MbhpMfToolCalibrationCurve::setTrace(const Array<uint16>& newTrace)
{
    traceMem = newTrace;
    repaint(); // update view
}

//==============================================================================
void MbhpMfToolCalibrationCurve::paint(Graphics& g)
{
    unsigned X0 = 0;
    unsigned Y0 = getHeight();

    g.fillAll(Colour(0xffeeeeee));

    // frame around diagram
    {
        Path p;
        p.startNewSubPath(0, 0);
        p.lineTo(getWidth(), 0);
        p.lineTo(getWidth(), getHeight());
        p.lineTo(0, getHeight());
        //p.lineTo(0, 0);
        p.closeSubPath();

        PathStrokeType stroke(2.0f);
        g.setColour(Colour(0xff222222));
        g.strokePath (p, stroke, AffineTransform::identity);
    }


    // getWidth() usually 256, so that each traced mS can be displayed
    // vertical lines w/ 50 mS distance
    for(int x=50; x<getWidth(); x+=50) {
        Path p;
        p.startNewSubPath(x+0, Y0-0);
        p.lineTo(x+0, 0);
        //p.closeSubPath();

        PathStrokeType stroke(1.0f);
        g.setColour(Colour(0xff888888));
        g.strokePath (p, stroke, AffineTransform::identity);
    }

    // getHeight() usually 256 (mfValue / 4)
    // horizontal lines w/ 64 pixel distance
    for(int y=64; y<getHeight(); y+=64) {
        Path p;
        p.startNewSubPath(0, y);
        p.lineTo(getWidth(), y);
        //p.closeSubPath();

        PathStrokeType stroke(1.0f);
        g.setColour(Colour(0xff888888));
        g.strokePath (p, stroke, AffineTransform::identity);
    }

    // the final curve
    if( traceMem.size() && traceMem[0] < 0x400 ) {
        Path p;
        p.startNewSubPath(X0, Y0 - (traceMem[0] / 4));

        for(int i=1; i<traceMem.size(); ++i) {
            if( traceMem[i] >= 0x400 ) // end marker
                break;
            p.lineTo(X0+i, Y0 - (traceMem[i] / 4));
        }
        //p.closeSubPath();

        PathStrokeType stroke(3.0f);
        g.setColour(Colours::purple.withAlpha ((float)1.0));
        g.strokePath (p, stroke, AffineTransform::identity);
    }

}


//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfToolCalibration::MbhpMfToolCalibration(MbhpMfTool* _mbhpMfTool)
    : mbhpMfTool(_mbhpMfTool)
    , timerGeneratesSineWaveform(false)
    , sinePhase(0.0)
{
    addAndMakeVisible(calibrationTable = new MbhpMfToolCalibrationTable(_mbhpMfTool));

    addAndMakeVisible(calibrationCurve = new MbhpMfToolCalibrationCurve);
    addAndMakeVisible(calibrationCurveLabel = new Label(String::empty, T("x=50 mS per unit, y=256 steps per unit")));
    calibrationCurveLabel->setJustificationType(Justification::horizontallyCentred);

    addAndMakeVisible(traceSliderLabel = new Label(String::empty, T("Fader which should be traced:")));
    traceSliderLabel->setJustificationType(Justification::left);

    addAndMakeVisible(traceSlider = new Slider(T("Trace Fader")));
    traceSlider->setRange(1, 8, 1);
    traceSlider->setValue(1);
    traceSlider->setSliderStyle(Slider::IncDecButtons);
    traceSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 50, 20);
    traceSlider->setDoubleClickReturnValue(true, 0);
    traceSlider->addListener(this);

    addAndMakeVisible(traceScaleSliderLabel = new Label(String::empty, T("Trace Scale (x * mS):")));
    traceScaleSliderLabel->setJustificationType(Justification::left);

    addAndMakeVisible(traceScaleSlider = new Slider(T("TraceScale Fader")));
    traceScaleSlider->setRange(1, 10, 1);
    traceScaleSlider->setValue(1);
    traceScaleSlider->setSliderStyle(Slider::IncDecButtons);
    traceScaleSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 50, 20);
    traceScaleSlider->setDoubleClickReturnValue(true, 0);
    traceScaleSlider->addListener(this);

    addAndMakeVisible(directMoveSliderLabel = new Label(String::empty, T("Direct Move:")));
    directMoveSliderLabel->setJustificationType(Justification::left);

    addAndMakeVisible(directMoveSlider = new Slider(T("DirectMove Fader")));
    directMoveSlider->setRange(0, 1023, 1);
    directMoveSlider->setValue(512);
    directMoveSlider->setSliderStyle(Slider::IncDecButtons);
    directMoveSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 50, 20);
    directMoveSlider->setDoubleClickReturnValue(true, 0);
    directMoveSlider->addListener(this);

    addAndMakeVisible(upperButton = new TextButton(T("Upper")));
    upperButton->addListener(this);

    addAndMakeVisible(middleButton = new TextButton(T("Middle")));
    middleButton->addListener(this);

    addAndMakeVisible(lowerButton = new TextButton(T("Lower")));
    lowerButton->addListener(this);

    addAndMakeVisible(slashButton = new TextButton(T("Slash")));
    slashButton->addListener(this);

    addAndMakeVisible(backslashButton = new TextButton(T("Backslash")));
    backslashButton->addListener(this);

    addAndMakeVisible(slowUpperButton = new TextButton(T("Slow Upper")));
    slowUpperButton->addListener(this);

    addAndMakeVisible(slowMiddleButton = new TextButton(T("Slow Middle")));
    slowMiddleButton->addListener(this);

    addAndMakeVisible(slowLowerButton = new TextButton(T("Slow Lower")));
    slowLowerButton->addListener(this);

    addAndMakeVisible(slowSineButton = new TextButton(T("Slow Sine")));
    slowSineButton->addListener(this);

    addAndMakeVisible(delayLabel = new Label(String::empty, T("Slow Delay:")));
    delayLabel->setJustificationType(Justification::left);

    addAndMakeVisible(delaySlider = new Slider(T("Delay")));
    delaySlider->setRange(0, 200, 1);
    delaySlider->setValue(10);
    delaySlider->setSliderStyle(Slider::IncDecButtons);
    delaySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    delaySlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(delayStepsLabel = new Label(String::empty, T("Slow Steps:")));
    delayStepsLabel->setJustificationType(Justification::left);

    addAndMakeVisible(delayStepsSlider = new Slider(T("DelaySteps")));
    delayStepsSlider->setRange(1, 100, 1);
    delayStepsSlider->setValue(4);
    delayStepsSlider->setSliderStyle(Slider::IncDecButtons);
    delayStepsSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    delayStepsSlider->setDoubleClickReturnValue(true, 0);

    // for slow calibration moves
    for(int mf=0; mf<8; ++mf) {
        targetMfValues.set(mf, 0);
        currentMfValues.set(mf, 0xffff); // ensure that current values are invalid
    }

    setSize(800, 300);
}

MbhpMfToolCalibration::~MbhpMfToolCalibration()
{
}

//==============================================================================
void MbhpMfToolCalibration::resized()
{
    int buttonWidth = 72;
    int buttonHeight = 20;

    int tableX0 = 0;
    int tableY0 = 0;
    int tableWidth = 700;
    int tableHeight = 230;

    calibrationTable->setBounds(tableX0, tableY0, tableWidth, tableHeight);

    int curveX0 = 8;
    int curveY0 = tableY0 + tableHeight;
    int curveWidth = 256;
    int curveHeight = 256;
    calibrationCurve->setBounds(curveX0, curveY0, curveWidth, curveHeight);
    calibrationCurveLabel->setBounds(curveX0, curveY0+curveHeight, curveWidth, buttonHeight);

    int traceSliderWidth = 90;
    traceSliderLabel->setBounds(curveX0, curveY0+curveHeight+1*buttonHeight, curveWidth-traceSliderWidth-10, buttonHeight);
    traceSlider->setBounds(curveX0 + curveWidth-traceSliderWidth, curveY0+curveHeight+1*buttonHeight, traceSliderWidth, buttonHeight);
    traceScaleSliderLabel->setBounds(curveX0, curveY0+curveHeight+2*buttonHeight, curveWidth-traceSliderWidth-10, buttonHeight);
    traceScaleSlider->setBounds(curveX0 + curveWidth-traceSliderWidth, curveY0+curveHeight+2*buttonHeight, traceSliderWidth, buttonHeight);
    directMoveSliderLabel->setBounds(curveX0, curveY0+curveHeight+3*buttonHeight, curveWidth-traceSliderWidth-10, buttonHeight);
    directMoveSlider->setBounds(curveX0 + curveWidth-traceSliderWidth, curveY0+curveHeight+3*buttonHeight, traceSliderWidth, buttonHeight);

    int buttonX0 = curveX0 + curveWidth + 10;
    int buttonY0 = curveY0 + 10;
    int buttonYd = buttonHeight + 4;

    upperButton->setBounds(buttonX0, buttonY0 + 0*buttonYd, buttonWidth, buttonHeight);
    middleButton->setBounds(buttonX0, buttonY0 + 1*buttonYd, buttonWidth, buttonHeight);
    lowerButton->setBounds(buttonX0, buttonY0 + 2*buttonYd, buttonWidth, buttonHeight);
    slashButton->setBounds(buttonX0, buttonY0 + 3*buttonYd, buttonWidth, buttonHeight);
    backslashButton->setBounds(buttonX0, buttonY0 + 4*buttonYd, buttonWidth, buttonHeight);

    slowUpperButton->setBounds(buttonX0+buttonWidth+8, buttonY0 + 0*buttonYd, buttonWidth, buttonHeight);
    slowMiddleButton->setBounds(buttonX0+buttonWidth+8, buttonY0 + 1*buttonYd, buttonWidth, buttonHeight);
    slowLowerButton->setBounds(buttonX0+buttonWidth+8, buttonY0 + 2*buttonYd, buttonWidth, buttonHeight);
    slowSineButton->setBounds(buttonX0+buttonWidth+8, buttonY0 + 3*buttonYd, buttonWidth, buttonHeight);

    delayLabel->setBounds(buttonX0, buttonY0 + 6*buttonYd, buttonWidth, buttonHeight);
    delaySlider->setBounds(buttonX0+buttonWidth+8, buttonY0 + 6*buttonYd, buttonWidth, buttonHeight);
    delayStepsLabel->setBounds(buttonX0, buttonY0 + 7*buttonYd, buttonWidth, buttonHeight);
    delayStepsSlider->setBounds(buttonX0+buttonWidth+8, buttonY0 + 7*buttonYd, buttonWidth, buttonHeight);

}

//==============================================================================
void MbhpMfToolCalibration::sliderValueChanged(Slider* slider)
{
    if( slider == traceScaleSlider ) {
        calibrationCurveLabel->setText(String::
formatted(T("x=%d mS per unit, y=256 steps per unit"), (unsigned)traceScaleSlider->getValue()*50), true);
        Array<uint16> emptyTrace;
        calibrationCurve->setTrace(emptyTrace);
    } else if( slider == traceSlider ) {
        Array<uint16> emptyTrace;
        calibrationCurve->setTrace(emptyTrace);
    } else if( slider == directMoveSlider ) {
        uint8 traceFader = traceSlider->getValue() - 1;
        uint8 traceFaderScale = traceScaleSlider->getValue();

        // request trace update
        {

            Array<uint16> emptyTrace;
            calibrationCurve->setTrace(emptyTrace);

            // request values
            Array<uint8> requestTraceSyx = SysexHelper::createMbhpMfTraceRequest((uint8)mbhpMfTool->mbhpMfToolControl->deviceIdSlider->getValue(),
                                                                                 traceFader,
                                                                                 traceFaderScale);
            MidiMessage message = SysexHelper::createMidiMessage(requestTraceSyx);
            mbhpMfTool->miosStudio->sendMidiMessage(message);
        }

        // send new fader pos
        Array<uint16> faderValue;
        faderValue.add(directMoveSlider->getValue());

        Array<uint8> faderSnapshotSyx = SysexHelper::createMbhpMfFadersSet((uint8)mbhpMfTool->mbhpMfToolControl->deviceIdSlider->getValue(),
                                                                           traceFader,
                                                                           faderValue);
        MidiMessage message = SysexHelper::createMidiMessage(faderSnapshotSyx);
        mbhpMfTool->miosStudio->sendMidiMessage(message);
    }
}

//==============================================================================
void MbhpMfToolCalibration::buttonClicked(Button* buttonThatWasClicked)
{
    unsigned delay = 0;
    uint8 traceFader = traceSlider->getValue() - 1;
    uint8 traceFaderScale = traceScaleSlider->getValue();

    if( buttonThatWasClicked == upperButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 1023);
    } else if( buttonThatWasClicked == middleButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 512);
    } else if( buttonThatWasClicked == lowerButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 0);
    } else if( buttonThatWasClicked == slashButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, mf*128 + 64);
    } else if( buttonThatWasClicked == backslashButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 1024 - (mf*128 + 64));
    } else if( buttonThatWasClicked == slowUpperButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 1023);
        delay = delaySlider->getValue();
    } else if( buttonThatWasClicked == slowMiddleButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 512);
        delay = delaySlider->getValue();
    } else if( buttonThatWasClicked == slowLowerButton ) {
        for(int mf=0; mf<8; ++mf)
            targetMfValues.set(mf, 0);
        delay = delaySlider->getValue();
    } else if( buttonThatWasClicked == slowSineButton ) {
        for(int mf=0; mf<8; ++mf) // values generated by timer
            targetMfValues.set(mf, 0);
        delay = delaySlider->getValue();
    }

    if( targetMfValues.size() ) {
        timerGeneratesSineWaveform = buttonThatWasClicked == slowSineButton;

        // request trace update
        {
            Array<uint16> emptyTrace;
            calibrationCurve->setTrace(emptyTrace);

            // request values
            Array<uint8> requestTraceSyx = SysexHelper::createMbhpMfTraceRequest((uint8)mbhpMfTool->mbhpMfToolControl->deviceIdSlider->getValue(),
                                                                                 traceFader,
                                                                                 traceFaderScale);
            MidiMessage message = SysexHelper::createMidiMessage(requestTraceSyx);
            mbhpMfTool->miosStudio->sendMidiMessage(message);
        }

        if( !delay ) {
            // stop any timer operation
            stopTimer();

            // set fader values
            Array<uint8> faderSnapshotSyx = SysexHelper::createMbhpMfFadersSet((uint8)mbhpMfTool->mbhpMfToolControl->deviceIdSlider->getValue(),
                                                                           0x00,
                                                                           targetMfValues);
            MidiMessage message = SysexHelper::createMidiMessage(faderSnapshotSyx);
            mbhpMfTool->miosStudio->sendMidiMessage(message);

            // store new values for slow moves
            for(int mf=0; mf<8; ++mf)
                currentMfValues.set(mf, targetMfValues[mf]);
        } else {
            // initial state: if currentMfValues 0xffff, we have to initialize them somehow
            for(int mf=0; mf<8; ++mf)
                if( currentMfValues[mf] == 0xffff )
                    currentMfValues.set(mf, 0x3ff-targetMfValues[mf]);

            // start timer which moves the faders slowly
            startTimer(1);
        }
    }

    // update "direct move" slider
    directMoveSlider->setValue(currentMfValues[traceFader]);
}

//==============================================================================
void MbhpMfToolCalibration::timerCallback()
{
    uint8 traceFader = traceSlider->getValue() - 1;

    stopTimer(); // will be restarted if required
    bool targetReached = true;

    // move faders into target direction depending on specified range
    int stepRange = delayStepsSlider->getValue();
    if( !stepRange ) stepRange = 1;

    if( timerGeneratesSineWaveform ) {
        targetReached = false; // target never reached... endless waveform generation (until another button has been pressed)
        sinePhase += (float)stepRange;
        while( sinePhase >= 360 )
            sinePhase -= 360;

        for(int mf=0; mf<8; ++mf) {
            int value = 0x200 + 0x200 * sin(2*3.14159 * ((sinePhase+mf*20) / 360));
            if( value >= 0x400 ) value = 0x3ff;
            if( value < 0 ) value = 0;
            currentMfValues.set(mf, value);
        }
    } else {
        for(int mf=0; mf<8; ++mf) {
            int current = currentMfValues[mf];
            int target = targetMfValues[mf];

            if( current != target ) {
                if( target > current ) {
                    current += stepRange;
                    if( current > target )
                        current = target;
                } else {
                    current -= stepRange;
                    if( current < target )
                        current = target;
                }

                currentMfValues.set(mf, current);
            }
        }

        // restart timer if target values not reached yet
        for(int mf=0; mf<8 && targetReached; ++mf)
            if( currentMfValues[mf] != targetMfValues[mf] )
                targetReached = false;
    }

    // set fader values
    Array<uint8> faderSnapshotSyx = SysexHelper::createMbhpMfFadersSet((uint8)mbhpMfTool->mbhpMfToolControl->deviceIdSlider->getValue(),
                                                                           0x00,
                                                                           currentMfValues);
    MidiMessage message = SysexHelper::createMidiMessage(faderSnapshotSyx);
    mbhpMfTool->miosStudio->sendMidiMessage(message);

    // update "direct move" slider
    directMoveSlider->setValue(currentMfValues[traceFader]);

    // restart timer?
    if( !targetReached ) {
        int delay = delaySlider->getValue();
        if( !delay ) delay = 1;
        startTimer(delay);
    }
}

//==============================================================================
void MbhpMfToolCalibration::getDump(Array<uint8> &syxDump)
{
    for(int mf=0; mf<8; ++mf) {
        syxDump.set(0x20 + 0*8+mf, calibrationTable->mfMode[mf]);
        syxDump.set(0x20 + 1*8+mf, calibrationTable->mfMinValue[mf] & 0xff); // only low-byte will be stored (0..255)
        syxDump.set(0x20 + 2*8+mf, calibrationTable->mfMaxValue[mf] & 0xff); // only low-byte will be stored (768..1023)
        syxDump.set(0x20 + 3*8+mf, calibrationTable->mfMinDutyUp[mf]);
        syxDump.set(0x20 + 4*8+mf, calibrationTable->mfMaxDutyUp[mf]);
        syxDump.set(0x20 + 5*8+mf, calibrationTable->mfMinDutyDown[mf]);
        syxDump.set(0x20 + 6*8+mf, calibrationTable->mfMaxDutyDown[mf]);
    }
}

void MbhpMfToolCalibration::setDump(const Array<uint8> &syxDump)
{
    for(int mf=0; mf<8; ++mf) {
        calibrationTable->mfMode.set(mf, syxDump[0x20 + 0*8+mf]);
        unsigned minValue = syxDump[0x20 + 1*8+mf]; // only low-byte will be stored (0..255)
        calibrationTable->mfMinValue.set(mf, minValue);
        unsigned maxValue = syxDump[0x20 + 2*8+mf] + 768; // only low-byte will be stored (768..1023)
        calibrationTable->mfMaxValue.set(mf, maxValue);
        calibrationTable->mfMinDutyUp.set(mf, syxDump[0x20 + 3*8+mf]);
        calibrationTable->mfMaxDutyUp.set(mf, syxDump[0x20 + 4*8+mf]);
        calibrationTable->mfMinDutyDown.set(mf, syxDump[0x20 + 5*8+mf]);
        calibrationTable->mfMaxDutyDown.set(mf, syxDump[0x20 + 6*8+mf]);
    }
    calibrationTable->table->updateContent();
}



//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfToolConfig::MbhpMfToolConfig(MbhpMfTool *_mbhpMfTool)
    : mbhpMfTool(_mbhpMfTool)
    , TabbedComponent(TabbedButtonBar::TabsAtTop)
{
    addTab(T("Global"),   Colour(0xfff0f0e0), configGlobals = new MbhpMfToolConfigGlobals(mbhpMfTool), true);
    addTab(T("Calibration"), Colour(0xfff0f0d0), calibration = new MbhpMfToolCalibration(mbhpMfTool), true);
    setSize(800, 300);
}

MbhpMfToolConfig::~MbhpMfToolConfig()
{
}

//==============================================================================
void MbhpMfToolConfig::getDump(Array<uint8> &syxDump)
{
    for(int addr=0x000; addr<0x460; ++addr)
        syxDump.set(addr, 0x00);
    for(int addr=0x460; addr<0x600; ++addr)
        syxDump.set(addr, 0x7f);

    configGlobals->getDump(syxDump);
    calibration->getDump(syxDump);
}

void MbhpMfToolConfig::setDump(const Array<uint8> &syxDump)
{
    configGlobals->setDump(syxDump);
    calibration->setDump(syxDump);
}



//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfToolControl::MbhpMfToolControl(MiosStudio *_miosStudio, MbhpMfToolConfig *_mbhpMfToolConfig)
    : miosStudio(_miosStudio)
    , mbhpMfToolConfig(_mbhpMfToolConfig)
    , progress(0)
    , receiveDump(false)
    , sendCompleteDump(false)
    , sendChangedDump(false)
    , dumpRequested(false)
    , dumpReceived(false)
    , checksumError(false)
    , dumpSent(false)
{
    addAndMakeVisible(loadButton = new TextButton(T("Load")));
    loadButton->addListener(this);

    addAndMakeVisible(saveButton = new TextButton(T("Save")));
    saveButton->addListener(this);

    addAndMakeVisible(deviceIdLabel = new Label(T("Device ID"), T("Device ID:")));
    deviceIdLabel->setJustificationType(Justification::right);
    
    addAndMakeVisible(deviceIdSlider = new Slider(T("Device ID")));
    deviceIdSlider->setRange(0, 127, 1);
    deviceIdSlider->setSliderStyle(Slider::IncDecButtons);
    deviceIdSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    deviceIdSlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(patchLabel = new Label(T("Patch"), T("Patch:")));
    patchLabel->setJustificationType(Justification::right);
    patchLabel->setVisible(false); // no patches provided by MBHP_MF
    
    addAndMakeVisible(patchSlider = new Slider(T("Patch")));
    patchSlider->setRange(1, 128, 1);
    patchSlider->setSliderStyle(Slider::IncDecButtons);
    patchSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    patchSlider->setDoubleClickReturnValue(true, 0);
    patchSlider->setVisible(false); // no patches provided by MBHP_MF

    addAndMakeVisible(receiveButton = new TextButton(T("Receive")));
    receiveButton->addListener(this);

    addAndMakeVisible(sendButton = new TextButton(T("Send")));
    sendButton->addListener(this);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        deviceIdSlider->setValue(propertiesFile->getIntValue(T("mbhpMfDeviceId"), 0) & 0x7f);
        patchSlider->setValue(propertiesFile->getIntValue(T("mbhpMfPatch"), 0) & 0x7f);
        String syxFileName(propertiesFile->getValue(T("mbhpMfSyxFile"), String::empty));
        if( syxFileName != String::empty )
            syxFile = File(syxFileName);
    }

    // revert patch number change - no patches provided by MBHP_MF
    patchSlider->setValue(0);

    setSize(800, 300);
}

MbhpMfToolControl::~MbhpMfToolControl()
{
}

//==============================================================================
void MbhpMfToolControl::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MbhpMfToolControl::resized()
{
    int buttonX0 = 10;
    int buttonXOffset = 80;
    int buttonY = 8;
    int buttonWidth = 72;
    int buttonHeight = 24;

    loadButton->setBounds(buttonX0 + 0*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    saveButton->setBounds(buttonX0 + 1*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    deviceIdLabel->setBounds(buttonX0 + 20+2*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    deviceIdSlider->setBounds(buttonX0 + 20+3*buttonXOffset, buttonY, buttonWidth, buttonHeight);

#if 0
    patchLabel->setBounds(buttonX0 + 20+4*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    patchSlider->setBounds(buttonX0 + 20+5*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    receiveButton->setBounds(buttonX0 + 40+6*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    sendButton->setBounds(buttonX0 + 40+7*buttonXOffset, buttonY, buttonWidth, buttonHeight);
#else
    receiveButton->setBounds(buttonX0 + 40+4*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    sendButton->setBounds(buttonX0 + 40+5*buttonXOffset, buttonY, buttonWidth, buttonHeight);
#endif

    int progressBarX = sendButton->getX() + buttonWidth + 20;
    progressBar->setBounds(progressBarX, buttonY, getWidth()-progressBarX-buttonX0, buttonHeight);
}

//==============================================================================
void MbhpMfToolControl::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == loadButton ) {
        FileChooser fc(T("Choose a .syx file that you want to open..."),
                       syxFile,
                       T("*.syx"));

        if( fc.browseForFileToOpen() ) {
            syxFile = fc.getResult();
            if( loadSyx(syxFile) ) {
                PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
                if( propertiesFile )
                    propertiesFile->setValue(T("mbhpMfSyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == saveButton ) {
        FileChooser fc(T("Choose a .syx file that you want to save..."),
                       syxFile,
                       T("*.syx"));
        if( fc.browseForFileToSave(true) ) {
            syxFile = fc.getResult();
            if( saveSyx(syxFile) ) {
                PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
                if( propertiesFile )
                    propertiesFile->setValue(T("mbhpMfSyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == sendButton || buttonThatWasClicked == receiveButton ) {
        loadButton->setEnabled(false);
        saveButton->setEnabled(false);
        sendButton->setEnabled(false);
        receiveButton->setEnabled(false);
        progress = 0;
        checksumError = false;
        if( buttonThatWasClicked == receiveButton ) {
            dumpRequested = false;
            receiveDump = true;
            sendCompleteDump = false;
            sendChangedDump = false;
            currentSyxDump.clear();
        } else {
            receiveDump = false;
            sendCompleteDump = true;
            sendChangedDump = false;
            dumpSent = false;
        }
        startTimer(1);
    }
}

//==============================================================================
void MbhpMfToolControl::sliderValueChanged(Slider* slider)
{
    if( slider == deviceIdSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("mbhpMfDeviceId"), (int)slider->getValue());
    } else if( slider == patchSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("mbhpMfPatch"), (int)slider->getValue());
    }
}


//==============================================================================
void MbhpMfToolControl::sysexUpdateRequest(void)
{
    // get initial dump
    // this will already happen after startup of MIOS Studio, the function is called
    // from MbhpMfTool constructor as well!
    if( !currentSyxDump.size() )
        mbhpMfToolConfig->getDump(currentSyxDump);

    // send changed dump values within 10 mS
    // can be called whenever a configuration value has been changed
    if( sendButton->isEnabled() ) {
        receiveDump = false;
        sendCompleteDump = false;
        sendChangedDump = true;
        dumpSent = false;
        loadButton->setEnabled(false);
        saveButton->setEnabled(false);
        sendButton->setEnabled(false);
        receiveButton->setEnabled(false);
        startTimer(10);
    }
}

//==============================================================================
void MbhpMfToolControl::timerCallback()
{
    stopTimer(); // will be restarted if required

    bool transferFinished = false;

    if( receiveDump ) {
        if( dumpRequested ) {
            if( !dumpReceived ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("No response from core."),
                                            T("Check:\n- MIDI In/Out connections\n- Device ID\n- that MBHP_MF firmware has been uploaded"),
                                            String::empty);
            } else if( checksumError ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Detected checksum error!"),
                                            T("Check:\n- MIDI In/Out connections\n- your MIDI interface"),
                                            String::empty);
            } else {
                mbhpMfToolConfig->setDump(currentSyxDump);
                transferFinished = true;
            }
        } else {
            dumpReceived = false;
            checksumError = false;
            dumpRequested = true;
            Array<uint8> data = SysexHelper::createMbhpMfReadPatch((uint8)deviceIdSlider->getValue(),
                                                                 (int)patchSlider->getValue()-1);
            MidiMessage message = SysexHelper::createMidiMessage(data);
            miosStudio->sendMidiMessage(message);

            progress = 1.0;
            startTimer(1000);
        }
    } else if( sendCompleteDump || sendChangedDump ) {
        if( dumpSent ) {
            transferFinished = true;
            sendCompleteDump = false;
            sendChangedDump = false;
        } else {
            bool anyValueChanged = false;

            if( sendCompleteDump ) {
                mbhpMfToolConfig->getDump(currentSyxDump);
                anyValueChanged = true;
                Array<uint8> data = SysexHelper::createMbhpMfWritePatch((uint8)deviceIdSlider->getValue(),
                                                                        (int)patchSlider->getValue()-1,
                                                                        &currentSyxDump.getReference(0));
                MidiMessage message = SysexHelper::createMidiMessage(data);
                miosStudio->sendMidiMessage(message);
            } else if( sendChangedDump ) {
                Array<uint8> lastSyxDump(currentSyxDump);
                mbhpMfToolConfig->getDump(currentSyxDump);

                // send only changes via direct write
                for(int addr=0; addr<lastSyxDump.size(); ++addr) {
                    if( currentSyxDump[addr] != lastSyxDump[addr] ) {
                        anyValueChanged = true;

                        uint8 value = currentSyxDump[addr];
                        Array<uint8> data = SysexHelper::createMbhpMfWriteDirect((uint8)deviceIdSlider->getValue(),
                                                                                 addr,
                                                                                 &value,
                                                                                 1);
                        MidiMessage message = SysexHelper::createMidiMessage(data);
                        miosStudio->sendMidiMessage(message);
                    }
                }
            }

            if( anyValueChanged ) {
                dumpSent = true;
                progress = 1.0;
                startTimer(1000);
            } else {
                transferFinished = true;
                sendCompleteDump = false;
                sendChangedDump = false;
            }
        }
    }

    if( transferFinished ) {
        progress = 0;
        loadButton->setEnabled(true);
        saveButton->setEnabled(true);
        sendButton->setEnabled(true);
        receiveButton->setEnabled(true);
    }
}

//==============================================================================
void MbhpMfToolControl::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();

    if( SysexHelper::isValidMbhpMfTraceDump(data, size, (int)deviceIdSlider->getValue()) ) {
        Array<uint16> newTrace;

        for(int pos=7; pos<(size-1) && data[pos] != 0xf7; pos+=2)
            newTrace.add(data[pos+0] | (data[pos+1] << 7));

        mbhpMfToolConfig->calibration->calibrationCurve->setTrace(newTrace);
        return;
    }

    if( receiveDump ) {
        if( dumpRequested &&
            SysexHelper::isValidMbhpMfWritePatch(data, size, (int)deviceIdSlider->getValue()) &&
            data[7] == ((int)patchSlider->getValue()-1) ) {
            dumpReceived = true;

            if( size != 522 ) {
                checksumError = true;
            } else {
                uint8 checksum = 0x00;
                int pos;
                for(pos=8; pos<7+1+512; ++pos)
                    checksum += data[pos];
                if( data[pos] != (-(int)checksum & 0x7f) )
                    checksumError = true;
                else {
                    for(pos=8; pos<7+1+512; pos+=2) {
                        uint8 b = (data[pos+0] & 0x0f) | ((data[pos+1] << 4) & 0xf0);
                        currentSyxDump.add(b);
                    }
                }
            }

            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    } else if( isTimerRunning() ) {
        if( SysexHelper::isValidMbhpMfAcknowledge(data, size, (int)deviceIdSlider->getValue()) ) {
            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    }
}


//==============================================================================
bool MbhpMfToolControl::loadSyx(File &syxFile)
{
    FileInputStream *inFileStream = syxFile.createInputStream();

    if( !inFileStream || inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("The file ") + syxFile.getFileName(),
                                    T("doesn't exist!"),
                                    String::empty);
        return false;
    } else if( inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("The file ") + syxFile.getFileName(),
                                    T("is empty!"),
                                    String::empty);
        return false;
    }

    int64 size = inFileStream->getTotalLength();
    uint8 *buffer = (uint8 *)juce_malloc(size);
    int64 readNumBytes = inFileStream->read(buffer, size);
    int numValidPatches = 0;
    Array<uint8> syxDump;
    String errorMessage;

    if( readNumBytes != size ) {
        errorMessage = String(T("cannot be read completely!"));
    } else {
        for(int pos=0; pos<size; ++pos) {
            if( SysexHelper::isValidMbhpMfWritePatch(buffer+pos, size - pos, -1) ) {
                pos += 7;

                if( pos < size ) {
                    int patch = buffer[pos++]; // not relevant, will be ignored
                    int patchPos = 0;
                    uint8 checksum = 0x00;
                    while( pos < size && buffer[pos] != 0xf7 && patchPos < 256 ) {
                        uint8 bl = buffer[pos++] & 0x0f;
                        checksum += bl;
                        uint8 bh = buffer[pos++] & 0x0f;
                        checksum += bh;

                        syxDump.set(patchPos, (bh << 4) | bl);
                        ++patchPos;
                    }

                    if( patchPos != 256 ) {
                        errorMessage = String::formatted(T("contains invalid data: patch %d not complete (only %d bytes)"), patch, patchPos);
                    } else {
                        checksum = (-(int)checksum) & 0x7f;
                        if( buffer[pos] != checksum ) {
                            errorMessage = String::formatted(T("contains invalid checksum in patch %d (expected %02X but read %02X)!"), patch, checksum, buffer[pos]);
                        } else {
                            ++pos;
                            ++numValidPatches;
                        }
                    }
                }
            }
        }
    }

    juce_free(buffer);

    if( errorMessage == String::empty ) {
        if( !numValidPatches )
            errorMessage = String(T("doesn't contain a valid patch!"));
    }

    if( errorMessage != String::empty ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("The file ") + syxFile.getFileName(),
                                    errorMessage,
                                    String::empty);
        return false;
    }

    mbhpMfToolConfig->setDump(syxDump);

    return true;
}

bool MbhpMfToolControl::saveSyx(File &syxFile)
{
    syxFile.deleteFile();
    FileOutputStream *outFileStream = syxFile.createOutputStream();
            
    if( !outFileStream || outFileStream->failedToOpen() ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    String::empty,
                                    T("File cannot be created!"),
                                    String::empty);
        return false;
    }

    Array<uint8> syxDump;
    mbhpMfToolConfig->getDump(syxDump);

    Array<uint8> data = SysexHelper::createMbhpMfWritePatch((uint8)deviceIdSlider->getValue(),
                                                          (int)patchSlider->getValue()-1,
                                                          &syxDump.getReference(0));
    outFileStream->write(&data.getReference(0), data.size());

    delete outFileStream;

    return true;
}


//==============================================================================
//==============================================================================
//==============================================================================
MbhpMfTool::MbhpMfTool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , mbhpMfToolControl(NULL)
{
    addAndMakeVisible(mbhpMfToolConfig = new MbhpMfToolConfig(this));
    addAndMakeVisible(mbhpMfToolControl = new MbhpMfToolControl(miosStudio, mbhpMfToolConfig));

    // get initial dump
    mbhpMfToolControl->sysexUpdateRequest();

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(700, 630);
}

MbhpMfTool::~MbhpMfTool()
{
}

//==============================================================================
void MbhpMfTool::paint (Graphics& g)
{
    //g.fillAll(Colour(0xffc1d0ff));
    g.fillAll(Colours::white);
}

void MbhpMfTool::resized()
{
    mbhpMfToolControl->setBounds(0, 0, getWidth(), 40);
    mbhpMfToolConfig->setBounds(0, 40, getWidth(), getHeight()-40);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}
