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

#include "Midio128Tool.h"
#include "MiosStudio.h"


//==============================================================================
Midio128ToolControl::Midio128ToolControl(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , progress(0)
{
    addAndMakeVisible(loadButton = new TextButton(T("Load Button")));
    loadButton->setButtonText(T("Load"));
    loadButton->addButtonListener(this);

    addAndMakeVisible(saveButton = new TextButton(T("Save Button")));
    saveButton->setButtonText(T("Save"));
    saveButton->addButtonListener(this);

    addAndMakeVisible(deviceIdLabel = new Label(T("Device ID"), T("Device ID:")));
    deviceIdLabel->setJustificationType(Justification::right);
    
    addAndMakeVisible(deviceIdSlider = new Slider(T("Device ID")));
    deviceIdSlider->setRange(0, 127, 1);
    deviceIdSlider->setSliderStyle(Slider::IncDecButtons);
    deviceIdSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    deviceIdSlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(sendButton = new TextButton(T("Send Button")));
    sendButton->setButtonText(T("Send"));
    sendButton->addButtonListener(this);

    addAndMakeVisible(receiveButton = new TextButton(T("Receive Button")));
    receiveButton->setButtonText(T("Receive"));
    receiveButton->addButtonListener(this);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        //sendDelaySlider->setValue(propertiesFile->getIntValue(T("sysexSendDelay"), 750));
    }

    setSize(800, 200);
}

Midio128ToolControl::~Midio128ToolControl()
{
    deleteAllChildren();
}

//==============================================================================
void Midio128ToolControl::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void Midio128ToolControl::resized()
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

    sendButton->setBounds(buttonX0 + 40+4*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    receiveButton->setBounds(buttonX0 + 40+5*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    int progressBarX = receiveButton->getX() + buttonWidth + 20;
    progressBar->setBounds(progressBarX, buttonY, getWidth()-progressBarX-buttonX0, buttonHeight);
}

//==============================================================================
void Midio128ToolControl::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == loadButton ) {
    } else if( buttonThatWasClicked == saveButton ) {
    } else if( buttonThatWasClicked == sendButton ) {
    } else if( buttonThatWasClicked == receiveButton ) {
    }
}


//==============================================================================
void Midio128ToolControl::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
}


//==============================================================================
void Midio128ToolControl::timerCallback()
{
    stopTimer(); // will be restarted if required
}

//==============================================================================
void Midio128ToolControl::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    //receiveBox->addBinary(data, size);
}


//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfigGlobals::Midio128ToolConfigGlobals()
{
    addAndMakeVisible(mergerLabel = new Label(String::empty, T("MIDI Merger:")));
    mergerLabel->setJustificationType(Justification::right);
    addAndMakeVisible(mergerComboBox = new ComboBox(String::empty));
    mergerComboBox->setWantsKeyboardFocus(true);
    mergerComboBox->addItem(T("Disabled"), 1);
    mergerComboBox->addItem(T("Enabled (received MIDI events forwarded to MIDI Out)"), 2);
    mergerComboBox->addItem(T("MIDIbox Link Forwarding Point (core in a MIDIbox chain)"), 3);
    mergerComboBox->addItem(T("MIDIbox Link Endpoint (last core in the MIDIbox chain)"), 4);
    mergerComboBox->setSelectedId(1, true);

    addAndMakeVisible(debounceLabel = new Label(String::empty, T("DIN Debouncing (mS):")));
    debounceLabel->setJustificationType(Justification::right);
    addAndMakeVisible(debounceSlider = new Slider(String::empty));
    debounceSlider->setWantsKeyboardFocus(true);
    debounceSlider->setSliderStyle(Slider::LinearHorizontal);
    debounceSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    debounceSlider->setRange(0, 255, 1);
    debounceSlider->setValue(20);

    addAndMakeVisible(altPrgChangesLabel = new Label(String::empty, T("Program Change Mode:")));
    altPrgChangesLabel->setJustificationType(Justification::right);
    addAndMakeVisible(altPrgChangesComboBox = new ComboBox(String::empty));
    altPrgChangesComboBox->setWantsKeyboardFocus(true);
    altPrgChangesComboBox->addItem(T("DOUT pin will toggle between 0 and 1 whenever PC event is received"), 1);
    altPrgChangesComboBox->addItem(T("Exclusive: assigned DOUT pin will be activated, all others deactivated (channel-wise)"), 2);
    altPrgChangesComboBox->setSelectedId(1, true);

    addAndMakeVisible(forwardInToOutLabel = new Label(String::empty, T("DIN->DOUT Forwarding Mode:")));
    forwardInToOutLabel->setJustificationType(Justification::right);
    addAndMakeVisible(forwardInToOutComboBox = new ComboBox(String::empty));
    forwardInToOutComboBox->setWantsKeyboardFocus(true);
    forwardInToOutComboBox->addItem(T("Disabled"), 1);
    forwardInToOutComboBox->addItem(T("DIN pin changes will be forwarded to DOUT pins with same number"), 2);
    forwardInToOutComboBox->setSelectedId(2);

    addAndMakeVisible(inverseInputsLabel = new Label(String::empty, T("DIN Input Polarity:")));
    inverseInputsLabel->setJustificationType(Justification::right);
    addAndMakeVisible(inverseInputsComboBox = new ComboBox(String::empty));
    inverseInputsComboBox->setWantsKeyboardFocus(true);
    inverseInputsComboBox->addItem(T("Normal (0V = Active/Pressed, 5V = Open/Depressed"), 1);
    inverseInputsComboBox->addItem(T("Inverted (0V = Open/Depressed, 5V = Active/Pressed"), 2);
    inverseInputsComboBox->setSelectedId(1);

    addAndMakeVisible(inverseOutputsLabel = new Label(String::empty, T("DOUT Output Polarity:")));
    inverseOutputsLabel->setJustificationType(Justification::right);
    addAndMakeVisible(inverseOutputsComboBox = new ComboBox(String::empty));
    inverseOutputsComboBox->setWantsKeyboardFocus(true);
    inverseOutputsComboBox->addItem(T("Normal (5V = Active/On, 5V = Not Active/Off"), 1);
    inverseOutputsComboBox->addItem(T("Inverted (0V = Not Active/Off, 5V = Active/On"), 2);
    inverseOutputsComboBox->setSelectedId(1);

    addAndMakeVisible(allNotesOffChannelLabel = new Label(String::empty, T("MIDI Channel for \"All Notes Off\":")));
    allNotesOffChannelLabel->setJustificationType(Justification::right);
    addAndMakeVisible(allNotesOffChannelComboBox = new ComboBox(String::empty));
    allNotesOffChannelComboBox->setWantsKeyboardFocus(true);
    for(int i=1; i<=17; ++i) {
        if( i == 1 )
            allNotesOffChannelComboBox->addItem(T("none (ignored)"), i);
        else
            allNotesOffChannelComboBox->addItem(String(i-1), i);
    }
    allNotesOffChannelComboBox->setSelectedId(1);

    addAndMakeVisible(touchsensorSensitivityLabel = new Label(String::empty, T("Touch Sensor Sensitivity:")));
    touchsensorSensitivityLabel->setJustificationType(Justification::right);
    addAndMakeVisible(touchsensorSensitivitySlider = new Slider(String::empty));
    touchsensorSensitivitySlider->setWantsKeyboardFocus(true);
    touchsensorSensitivitySlider->setSliderStyle(Slider::LinearHorizontal);
    touchsensorSensitivitySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    touchsensorSensitivitySlider->setRange(0, 32, 1);
    touchsensorSensitivitySlider->setValue(3);
}


Midio128ToolConfigGlobals::~Midio128ToolConfigGlobals()
{
    deleteAllChildren();
}

//==============================================================================
void Midio128ToolConfigGlobals::resized()
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

    mergerLabel->setBounds(labelX, labelY0 + 0*labelYOffset, labelWidth, labelHeight);
    mergerComboBox->setBounds(controlX, controlY0 + 0*controlYOffset, controlWidth, controlHeight);

    debounceLabel->setBounds(labelX, labelY0 + 1*labelYOffset, labelWidth, labelHeight);
    debounceSlider->setBounds(controlX, controlY0 + 1*controlYOffset, controlWidth, controlHeight);

    altPrgChangesLabel->setBounds(labelX, labelY0 + 2*labelYOffset, labelWidth, labelHeight);
    altPrgChangesComboBox->setBounds(controlX, controlY0 + 2*controlYOffset, controlWidth, controlHeight);

    forwardInToOutLabel->setBounds(labelX, labelY0 + 3*labelYOffset, labelWidth, labelHeight);
    forwardInToOutComboBox->setBounds(controlX, controlY0 + 3*controlYOffset, controlWidth, controlHeight);

    inverseInputsLabel->setBounds(labelX, labelY0 + 4*labelYOffset, labelWidth, labelHeight);
    inverseInputsComboBox->setBounds(controlX, controlY0 + 4*controlYOffset, controlWidth, controlHeight);

    inverseOutputsLabel->setBounds(labelX, labelY0 + 5*labelYOffset, labelWidth, labelHeight);
    inverseOutputsComboBox->setBounds(controlX, controlY0 + 5*controlYOffset, controlWidth, controlHeight);

    allNotesOffChannelLabel->setBounds(labelX, labelY0 + 6*labelYOffset, labelWidth, labelHeight);
    allNotesOffChannelComboBox->setBounds(controlX, controlY0 + 6*controlYOffset, controlWidth, controlHeight);

    touchsensorSensitivityLabel->setBounds(labelX, labelY0 + 7*labelYOffset, labelWidth, labelHeight);
    touchsensorSensitivitySlider->setBounds(controlX, controlY0 + 7*controlYOffset, controlWidth, controlHeight);
}



//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfigDout::Midio128ToolConfigDout()
    : font(14.0f)
    , numRows(128)
{
    for(int dout=0; dout<numRows; ++dout) {
        doutChannel.push_back(1); // Default MIDI Channel

        if( dout < 64 ) {
            doutEvent.push_back(1); // Note On
            doutParameter.push_back(0x30 + dout); // Note Number
        } else {
            doutEvent.push_back(3); // CC
            doutParameter.push_back(0x10 + dout - 64); // CC Number
        }
    }

    addAndMakeVisible(table = new TableListBox(T("DOUT Button Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    TableHeaderComponent *tableHeader = table->getHeader();
    tableHeader->addColumn(T("DOUT"), 1, 50);
    tableHeader->addColumn(T("SR/Pin"), 2, 50);
    tableHeader->addColumn(T("Channel"), 3, 75);
    tableHeader->addColumn(T("Event"), 4, 125);
    tableHeader->addColumn(T("Parameter"), 5, 75);

    setSize(800, 200);
}

Midio128ToolConfigDout::~Midio128ToolConfigDout()
{
}

//==============================================================================
int Midio128ToolConfigDout::getNumRows()
{
    return numRows;
}

void Midio128ToolConfigDout::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);
}

void Midio128ToolConfigDout::paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(Colours::black);
    g.setFont(font);

    switch( columnId ) {
    case 1: {
        const String text(rowNumber);
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;

    case 2: {
        const String text = String(rowNumber/8 + 1) + T("/D") + String(7 - (rowNumber%8));
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;
    }

    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}


Component* Midio128ToolConfigDout::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    switch( columnId ) {
    case 3: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(1, 16);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 4: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("Note On"), 1);
            comboBox->addItem(T("Poly Aftertouch"), 2);
            comboBox->addItem(T("Controller"), 3);
            comboBox->addItem(T("Program Change"), 4);
            comboBox->addItem(T("Channel Aftertouch"), 5);
            comboBox->addItem(T("Pitch Bender"), 6);
            comboBox->addItem(T("Meta Event"), 7);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;

    case 5: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 127);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;
    }

    return 0;
}


//==============================================================================
void Midio128ToolConfigDout::resized()
{
    // position our table with a gap around its edge
    table->setBoundsInset(BorderSize(8));
}


//==============================================================================
int Midio128ToolConfigDout::getTableValue(const int rowNumber, const int columnId)
{
    switch( columnId ) {
    case 3: return doutChannel[rowNumber];
    case 4: return doutEvent[rowNumber];
    case 5: return doutParameter[rowNumber];
    }
    return 0;
}

void Midio128ToolConfigDout::setTableValue(const int rowNumber, const int columnId, const int newValue)
{
    switch( columnId ) {
    case 3: doutChannel[rowNumber] = newValue; break;
    case 4: doutEvent[rowNumber] = newValue; break;
    case 5: doutParameter[rowNumber] = newValue; break;
    }
}



//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfigDin::Midio128ToolConfigDin()
    : font(14.0f)
    , numRows(128)
{
    for(int din=0; din<numRows; ++din) {
        dinChannel.push_back(1); // Default MIDI Channel
        dinMode.push_back(1); // OnOff

        if( din < 64 ) {
            dinOnEvent.push_back(1); // Note On
            dinOnParameter1.push_back(0x30 + din); // Note Number
            dinOnParameter2.push_back(0x7f); // Max Velocity
            dinOffEvent.push_back(1); // Note On
            dinOffParameter1.push_back(0x30 + din); // Note Number
            dinOffParameter2.push_back(0x7f); // 0 Velocity (-> Note Off)
        } else {
            dinOnEvent.push_back(3); // CC
            dinOnParameter1.push_back(0x10 + din - 64); // CC Number
            dinOnParameter2.push_back(0x7f); // Max value
            dinOffEvent.push_back(3); // CC
            dinOffParameter1.push_back(0x10 + din - 64); // CC Number
            dinOffParameter2.push_back(0x7f); // 0
        }
    }

    addAndMakeVisible(table = new TableListBox(T("DIN Button Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    TableHeaderComponent *tableHeader = table->getHeader();
    tableHeader->addColumn(T("DIN"), 1, 50);
    tableHeader->addColumn(T("SR/Pin"), 2, 50);
    tableHeader->addColumn(T("Channel"), 3, 75);
    tableHeader->addColumn(T("On Event"), 4, 125);
    tableHeader->addColumn(T("On Par.1"), 5, 75);
    tableHeader->addColumn(T("On Par.2"), 6, 75);
    tableHeader->addColumn(T("Off Event"), 7, 125);
    tableHeader->addColumn(T("Off Par.1"), 8, 75);
    tableHeader->addColumn(T("Off Par.2"), 9, 75);
    tableHeader->addColumn(T("Mode"), 10, 75);

    setSize(800, 200);
}

Midio128ToolConfigDin::~Midio128ToolConfigDin()
{
}

//==============================================================================
int Midio128ToolConfigDin::getNumRows()
{
    return numRows;
}

void Midio128ToolConfigDin::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);
}

void Midio128ToolConfigDin::paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(Colours::black);
    g.setFont(font);

    switch( columnId ) {
    case 1: {
        const String text(rowNumber);
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;

    case 2: {
        const String text = String(rowNumber/8 + 1) + T("/D") + String(rowNumber%8);
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;
    }

    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}


Component* Midio128ToolConfigDin::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    switch( columnId ) {
    case 3: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(1, 16);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 4:
    case 7: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("Note On"), 1);
            comboBox->addItem(T("Poly Aftertouch"), 2);
            comboBox->addItem(T("Controller"), 3);
            comboBox->addItem(T("Program Change"), 4);
            comboBox->addItem(T("Channel Aftertouch"), 5);
            comboBox->addItem(T("Pitch Bender"), 6);
            comboBox->addItem(T("Meta Event"), 7);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;

    case 5:
    case 6:
    case 8:
    case 9: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 127);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 10: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("On/Off"), 1);
            comboBox->addItem(T("On Only"), 2);
            comboBox->addItem(T("Toggle"), 3);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;
    }

    return 0;
}


//==============================================================================
void Midio128ToolConfigDin::resized()
{
    // position our table with a gap around its edge
    table->setBoundsInset(BorderSize(8));
}


//==============================================================================
int Midio128ToolConfigDin::getTableValue(const int rowNumber, const int columnId)
{
    switch( columnId ) {
    case 3: return dinChannel[rowNumber];
    case 4: return dinOnEvent[rowNumber];
    case 5: return dinOnParameter1[rowNumber];
    case 6: return dinOnParameter2[rowNumber];
    case 7: return dinOffEvent[rowNumber];
    case 8: return dinOffParameter1[rowNumber];
    case 9: return dinOffParameter2[rowNumber];
    case 10: return dinMode[rowNumber];
    }
    return 0;
}

void Midio128ToolConfigDin::setTableValue(const int rowNumber, const int columnId, const int newValue)
{
    switch( columnId ) {
    case 3: dinChannel[rowNumber] = newValue; break;
    case 4: dinOnEvent[rowNumber] = newValue; break;
    case 5: dinOnParameter1[rowNumber] = newValue; break;
    case 6: dinOnParameter2[rowNumber] = newValue; break;
    case 7: dinOffEvent[rowNumber] = newValue; break;
    case 8: dinOffParameter1[rowNumber] = newValue; break;
    case 9: dinOffParameter2[rowNumber] = newValue; break;
    case 10: dinMode[rowNumber] = newValue; break;
    }
}


//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfig::Midio128ToolConfig(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , TabbedComponent(TabbedButtonBar::TabsAtTop)
{
    addTab(T("Global"), Colour(0xfff0f0e0), new Midio128ToolConfigGlobals(), true);
    addTab(T("DINs"),   Colour(0xfff0f0d0), new Midio128ToolConfigDin(), true);
    addTab(T("DOUTs"),  Colour(0xfff0f0c0), new Midio128ToolConfigDout(), true);
    setSize(800, 200);
}

Midio128ToolConfig::~Midio128ToolConfig()
{
}

//==============================================================================
void Midio128ToolConfig::buttonClicked(Button* buttonThatWasClicked)
{
}


//==============================================================================
void Midio128ToolConfig::sliderValueChanged(Slider* slider)
{
    //if( slider == sendDelaySlider ) {
    //}
}



//==============================================================================
//==============================================================================
//==============================================================================
Midio128Tool::Midio128Tool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(midio128ToolControl = new Midio128ToolControl(miosStudio));
    addAndMakeVisible(midio128ToolConfig = new Midio128ToolConfig(miosStudio));

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(860, 500);
}

Midio128Tool::~Midio128Tool()
{
    deleteAllChildren();
}

//==============================================================================
void Midio128Tool::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void Midio128Tool::resized()
{
    midio128ToolControl->setBounds(0, 0, getWidth(), 40);
    midio128ToolConfig->setBounds(0, 40, getWidth(), getHeight()-40);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}
