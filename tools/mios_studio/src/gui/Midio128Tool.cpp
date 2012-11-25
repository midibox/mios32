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


Midio128ToolConfigGlobals::Midio128ToolConfigGlobals()
{
    addAndMakeVisible(mergerLabel = new Label(String::empty, T("MIDI Merger:")));
    mergerLabel->setJustificationType(Justification::right);
    addAndMakeVisible(mergerComboBox = new ComboBox(String::empty));
    mergerComboBox->setWantsKeyboardFocus(true);
    mergerComboBox->addItem(T("Disabled"), 1);
    mergerComboBox->addItem(T("Enabled (received MIDI events forwarded to MIDI Out)"), 2);
    mergerComboBox->addItem(T("MIDIbox Link Endpoint (last core in the MIDIbox chain)"), 3);
    mergerComboBox->addItem(T("MIDIbox Link Forwarding Point (core in a MIDIbox chain)"), 4);
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
    inverseInputsComboBox->addItem(T("Inverted (0V = Active/Pressed, 5V = Open/Depressed"), 1);
    inverseInputsComboBox->addItem(T("Normal (0V = Open/Depressed, 5V = Active/Pressed"), 2);
    inverseInputsComboBox->setSelectedId(2);

    addAndMakeVisible(inverseOutputsLabel = new Label(String::empty, T("DOUT Output Polarity:")));
    inverseOutputsLabel->setJustificationType(Justification::right);
    addAndMakeVisible(inverseOutputsComboBox = new ComboBox(String::empty));
    inverseOutputsComboBox->setWantsKeyboardFocus(true);
    inverseOutputsComboBox->addItem(T("Normal (5V = Active/On, 0V = Not Active/Off"), 1);
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
void Midio128ToolConfigGlobals::getDump(Array<uint8> &syxDump)
{
    int merger = mergerComboBox->getSelectedId()-1;
    int debounce = debounceSlider->getValue();
    int altPrgChanges = altPrgChangesComboBox->getSelectedId()-1;
    int forwardInToOut = forwardInToOutComboBox->getSelectedId()-1;
    int inverseInputs = inverseInputsComboBox->getSelectedId()-1;
    int inverseOutputs = inverseOutputsComboBox->getSelectedId()-1;
    int touchsensorSensitivity = touchsensorSensitivitySlider->getValue();
    int allNotesOffChannel = allNotesOffChannelComboBox->getSelectedId()-1;

    syxDump.set(0x400, merger);
    syxDump.set(0x402, debounce & 0x0f);
    syxDump.set(0x403, (debounce>>4) & 0x0f);
    syxDump.set(0x404, altPrgChanges);
    syxDump.set(0x406, forwardInToOut);
    syxDump.set(0x408, inverseInputs);
    syxDump.set(0x40a, inverseOutputs);
    syxDump.set(0x40c, touchsensorSensitivity & 0x0f);
    syxDump.set(0x40d, (touchsensorSensitivity>>4) & 0x0f);
    syxDump.set(0x40e, 0x00); // reserved for global channel
    syxDump.set(0x410, allNotesOffChannel);
}

void Midio128ToolConfigGlobals::setDump(const Array<uint8> &syxDump)
{
    int merger = syxDump[0x400];
    int debounce = (syxDump[0x402] & 0x0f) | ((syxDump[0x403] << 4) & 0xf0);
    int altPrgChanges = syxDump[0x404];
    int forwardInToOut = syxDump[0x406];
    int inverseInputs = syxDump[0x408];
    int inverseOutputs = syxDump[0x40a];
    int touchsensorSensitivity = (syxDump[0x40c] & 0x0f) | ((syxDump[0x40d] << 4) & 0xf0);
    int allNotesOffChannel = syxDump[0x410];

    mergerComboBox->setSelectedId(merger+1);
    debounceSlider->setValue(debounce);
    altPrgChangesComboBox->setSelectedId(altPrgChanges+1);
    forwardInToOutComboBox->setSelectedId(forwardInToOut+1);
    inverseInputsComboBox->setSelectedId(inverseInputs+1);
    inverseOutputsComboBox->setSelectedId(inverseOutputs+1);
    allNotesOffChannelComboBox->setSelectedId(allNotesOffChannel+1);
    touchsensorSensitivitySlider->setValue(touchsensorSensitivity);
}


//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfigDout::Midio128ToolConfigDout()
    : font(14.0f)
    , numRows(128)
{
    for(int dout=0; dout<numRows; ++dout) {
        doutChannel.add(1); // Default MIDI Channel

        if( dout < 64 ) {
            doutEvent.add(3); // Note On
            doutParameter.add(0x30 + dout); // Note Number
        } else {
            doutEvent.add(5); // CC
            doutParameter.add(0x10 + dout - 64); // CC Number
        }
    }

    addAndMakeVisible(table = new TableListBox(T("DOUT Button Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    table->getHeader().addColumn(T("DOUT"), 1, 50);
    table->getHeader().addColumn(T("SR/Pin"), 2, 50);
    table->getHeader().addColumn(T("Channel"), 3, 75);
    table->getHeader().addColumn(T("Event"), 4, 125);
    table->getHeader().addColumn(T("Parameter"), 5, 75);

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
            comboBox->addItem(T("Disabled"), 1);
            comboBox->addItem(T("Note Off"), 2);
            comboBox->addItem(T("Note On"), 3);
            comboBox->addItem(T("Poly Aftertouch"), 4);
            comboBox->addItem(T("Controller (CC)"), 5);
            comboBox->addItem(T("Program Change"), 6);
            comboBox->addItem(T("Channel Aftertouch"), 7);
            comboBox->addItem(T("Pitch Bender"), 8);
            comboBox->addItem(T("Meta Event"), 9);
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
    case 3: doutChannel.set(rowNumber, newValue); break;
    case 4: doutEvent.set(rowNumber, newValue); break;
    case 5: doutParameter.set(rowNumber, newValue); break;
    }
}


//==============================================================================
void Midio128ToolConfigDout::getDump(Array<uint8> &syxDump)
{
    for(int dout=0; dout<numRows; ++dout) {
        int channel = (doutChannel[dout] - 1) & 0x0f;
        int event = (doutEvent[dout] - 2);
        int parameter = doutParameter[dout] & 0x7f;

        if( event < 0 ) {
            syxDump.set(0x000 + 2*dout + 0, 0x7f);
            syxDump.set(0x000 + 2*dout + 1, 0x7f);
        } else {
            syxDump.set(0x000 + 2*dout + 0, (event << 4) | channel);
            syxDump.set(0x000 + 2*dout + 1, parameter);
        }
    }
}

void Midio128ToolConfigDout::setDump(const Array<uint8> &syxDump)
{
    for(int dout=0; dout<numRows; ++dout) {
        int channel = syxDump[0x000 + 2*dout + 0] & 0x0f;
        int event = (syxDump[0x000 + 2*dout + 0] >> 4) & 0x7;
        int parameter = syxDump[0x000 + 2*dout + 1] & 0x7f;

        if( channel == 0xf && event == 0x7 && parameter == 0x7f ) {
            doutChannel.set(dout, 1);
            doutEvent.set(dout, 1);
            doutParameter.set(dout, 0);
        } else {
            doutChannel.set(dout, channel + 1);
            doutEvent.set(dout, event ? (event+2) : 1);
            doutParameter.set(dout, parameter);
        }
    }
    table->updateContent();
}



//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfigDin::Midio128ToolConfigDin()
    : font(14.0f)
    , numRows(128)
{
    for(int din=0; din<numRows; ++din) {
        dinChannel.add(1); // Default MIDI Channel
        dinMode.add(1); // OnOff

        if( din < 64 ) {
            dinOnEvent.add(3); // Note On
            dinOnParameter1.add(0x30 + din); // Note Number
            dinOnParameter2.add(0x7f); // Max Velocity
            dinOffEvent.add(3); // Note On
            dinOffParameter1.add(0x30 + din); // Note Number
            dinOffParameter2.add(0x00); // 0 Velocity (-> Note Off)
        } else {
            dinOnEvent.add(5); // CC
            dinOnParameter1.add(0x10 + din - 64); // CC Number
            dinOnParameter2.add(0x7f); // Max value
            dinOffEvent.add(5); // CC
            dinOffParameter1.add(0x10 + din - 64); // CC Number
            dinOffParameter2.add(0x00); // 0
        }
    }

    addAndMakeVisible(table = new TableListBox(T("DIN Button Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    table->getHeader().addColumn(T("DIN"), 1, 50);
    table->getHeader().addColumn(T("SR/Pin"), 2, 50);
    table->getHeader().addColumn(T("Channel"), 3, 75);
    table->getHeader().addColumn(T("On Event"), 4, 125);
    table->getHeader().addColumn(T("On Par.1"), 5, 75);
    table->getHeader().addColumn(T("On Par.2"), 6, 75);
    table->getHeader().addColumn(T("Off Event"), 7, 125);
    table->getHeader().addColumn(T("Off Par.1"), 8, 75);
    table->getHeader().addColumn(T("Off Par.2"), 9, 75);
    table->getHeader().addColumn(T("Mode"), 10, 75);

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
            comboBox->addItem(T("Disabled"), 1);
            comboBox->addItem(T("Note Off"), 2);
            comboBox->addItem(T("Note On"), 3);
            comboBox->addItem(T("Poly Aftertouch"), 4);
            comboBox->addItem(T("Controller (CC)"), 5);
            comboBox->addItem(T("Program Change"), 6);
            comboBox->addItem(T("Channel Aftertouch"), 7);
            comboBox->addItem(T("Pitch Bender"), 8);
            comboBox->addItem(T("Meta Event"), 9);
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
    case 3: dinChannel.set(rowNumber, newValue); break;
    case 4: dinOnEvent.set(rowNumber, newValue); break;
    case 5: dinOnParameter1.set(rowNumber, newValue); break;
    case 6: dinOnParameter2.set(rowNumber, newValue); break;
    case 7: dinOffEvent.set(rowNumber, newValue); break;
    case 8: dinOffParameter1.set(rowNumber, newValue); break;
    case 9: dinOffParameter2.set(rowNumber, newValue); break;
    case 10: dinMode.set(rowNumber, newValue); break;
    }
}


//==============================================================================
void Midio128ToolConfigDin::getDump(Array<uint8> &syxDump)
{
    for(int din=0; din<numRows; ++din) {
        int onChannel = (dinChannel[din] - 1) & 0x0f;
        int onEvent = (dinOnEvent[din] - 2);
        int onParameter1 = dinOnParameter1[din] & 0x7f;
        int onParameter2 = dinOnParameter2[din] & 0x7f;
        int offChannel = (dinChannel[din] - 1) & 0x0f;
        int offEvent = (dinOffEvent[din] - 2);
        int offParameter1 = dinOffParameter1[din] & 0x7f;
        int offParameter2 = dinOffParameter2[din] & 0x7f;
        int mode = (dinMode[din]-1) & 0x07;

        if( onEvent < 0 ) {
            syxDump.set(0x100 + 6*din + 0, 0x7f);
            syxDump.set(0x100 + 6*din + 1, 0x7f);
            syxDump.set(0x100 + 6*din + 2, 0x7f);
        } else {
            syxDump.set(0x100 + 6*din + 0, (onEvent << 4) | onChannel);
            syxDump.set(0x100 + 6*din + 1, onParameter1);
            syxDump.set(0x100 + 6*din + 2, onParameter2);
        }

        if( offEvent < 0 ) {
            syxDump.set(0x100 + 6*din + 3, 0x7f);
            syxDump.set(0x100 + 6*din + 4, 0x7f);
            syxDump.set(0x100 + 6*din + 5, 0x7f);
        } else {
            syxDump.set(0x100 + 6*din + 3, (offEvent << 4) | offChannel);
            syxDump.set(0x100 + 6*din + 4, offParameter1);
            syxDump.set(0x100 + 6*din + 5, offParameter2);
        }

        int modeAddr = 0x420 + (din/2);
        if( din & 1 )
            syxDump.set(modeAddr, (syxDump[modeAddr] & 0x0f) | (mode << 4));
        else
            syxDump.set(modeAddr, (syxDump[modeAddr] & 0xf0) | mode);
    }
}

void Midio128ToolConfigDin::setDump(const Array<uint8> &syxDump)
{
    for(int din=0; din<numRows; ++din) {
        int onChannel = syxDump[0x100 + 6*din + 0] & 0x0f;
        int onEvent = (syxDump[0x100 + 6*din + 0] >> 4) & 0x7;
        int onParameter1 = syxDump[0x100 + 6*din + 1] & 0x7f;
        int onParameter2 = syxDump[0x100 + 6*din + 2] & 0x7f;
        int offChannel = syxDump[0x100 + 6*din + 3] & 0x0f;
        int offEvent = (syxDump[0x100 + 6*din + 3] >> 4) & 0x7;
        int offParameter1 = syxDump[0x100 + 6*din + 4] & 0x7f;
        int offParameter2 = syxDump[0x100 + 6*din + 5] & 0x7f;
        int mode = syxDump[0x420 + (din/2)];
        mode = ((din & 1) ? (mode >> 4) : mode) & 0x07;

        if( onChannel == 0xf && onEvent == 0x7 && onParameter1 == 0x7f && onParameter2 == 0x7f ) {
            onChannel = -1;
            dinOnEvent.set(din, 1);
            dinOnParameter1.set(din, 0);
            dinOnParameter2.set(din, 0);
        } else {
            dinOnEvent.set(din, onEvent ? (onEvent+2) : 1);
            dinOnParameter1.set(din, onParameter1);
            dinOnParameter2.set(din, onParameter2);
        }

        if( offChannel == 0xf && offEvent == 0x7 && offParameter1 == 0x7f && offParameter2 == 0x7f ) {
            offChannel = -1;
            dinOffEvent.set(din, 1);
            dinOffParameter1.set(din, 0);
            dinOffParameter2.set(din, 0);
        } else {
            dinOffEvent.set(din, offEvent ? (offEvent+2) : 1);
            dinOffParameter1.set(din, offParameter1);
            dinOffParameter2.set(din, offParameter2);
        }

        // we only allow to configure a single channel for both events
        if( onChannel >= 0 )
            dinChannel.set(din, onChannel+1);
        else if( offChannel >= 0 )
            dinChannel.set(din, offChannel+1);
        else
            dinChannel.set(din, 1);

        // DIN mode
        dinMode.set(din, mode+1);
    }
    table->updateContent();
}

//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolConfig::Midio128ToolConfig(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , TabbedComponent(TabbedButtonBar::TabsAtTop)
{
    addTab(T("DINs"),   Colour(0xfff0f0d0), configDin = new Midio128ToolConfigDin(), true);
    addTab(T("DOUTs"),  Colour(0xfff0f0c0), configDout = new Midio128ToolConfigDout(), true);
    addTab(T("Global"), Colour(0xfff0f0e0), configGlobals = new Midio128ToolConfigGlobals(), true);
    setSize(800, 200);
}

Midio128ToolConfig::~Midio128ToolConfig()
{
}

//==============================================================================
void Midio128ToolConfig::getDump(Array<uint8> &syxDump)
{
    for(int addr=0x000; addr<0x460; ++addr)
        syxDump.set(addr, 0x00);
    for(int addr=0x460; addr<0x600; ++addr)
        syxDump.set(addr, 0x7f);

    configGlobals->getDump(syxDump);
    configDin->getDump(syxDump);
    configDout->getDump(syxDump);
}

void Midio128ToolConfig::setDump(const Array<uint8> &syxDump)
{
    configGlobals->setDump(syxDump);
    configDin->setDump(syxDump);
    configDout->setDump(syxDump);
}



//==============================================================================
//==============================================================================
//==============================================================================
Midio128ToolControl::Midio128ToolControl(MiosStudio *_miosStudio, Midio128ToolConfig *_midio128ToolConfig)
    : miosStudio(_miosStudio)
    , midio128ToolConfig(_midio128ToolConfig)
    , progress(0)
    , receiveDump(false)
    , dumpReceived(false)
    , checksumError(false)
{
    addAndMakeVisible(loadButton = new TextButton(T("Load")));
    loadButton->addListener(this);

    addAndMakeVisible(saveButton = new TextButton(T("Save")));
    saveButton->addListener(this);

    addAndMakeVisible(deviceIdLabel = new Label(T("Device ID"), T("Device ID:")));
    deviceIdLabel->setJustificationType(Justification::right);
    
    addAndMakeVisible(deviceIdSlider = new Slider(T("Device ID")));
    deviceIdSlider->setRange(0, 7, 1);
    deviceIdSlider->setSliderStyle(Slider::IncDecButtons);
    deviceIdSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    deviceIdSlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(receiveButton = new TextButton(T("Receive")));
    receiveButton->addListener(this);

    addAndMakeVisible(sendButton = new TextButton(T("Send")));
    sendButton->addListener(this);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        deviceIdSlider->setValue(propertiesFile->getIntValue(T("midio128DeviceId"), 0) & 7);
        String syxFileName(propertiesFile->getValue(T("midio128SyxFile"), String::empty));
        if( syxFileName != String::empty )
            syxFile = File(syxFileName);
    }

    setSize(800, 200);
}

Midio128ToolControl::~Midio128ToolControl()
{
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

    receiveButton->setBounds(buttonX0 + 40+4*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    sendButton->setBounds(buttonX0 + 40+5*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    int progressBarX = sendButton->getX() + buttonWidth + 20;
    progressBar->setBounds(progressBarX, buttonY, getWidth()-progressBarX-buttonX0, buttonHeight);
}

//==============================================================================
void Midio128ToolControl::buttonClicked(Button* buttonThatWasClicked)
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
                    propertiesFile->setValue(T("midio128SyxFile"), syxFile.getFullPathName());
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
                    propertiesFile->setValue(T("midio128SyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == sendButton || buttonThatWasClicked == receiveButton ) {
        loadButton->setEnabled(false);
        saveButton->setEnabled(false);
        sendButton->setEnabled(false);
        receiveButton->setEnabled(false);
        syxBlock = 0;
        progress = 0;
        checksumError = false;
        if( buttonThatWasClicked == receiveButton ) {
            receiveDump = true;
            currentSyxDump.clear();
        } else {
            receiveDump = false;
            midio128ToolConfig->getDump(currentSyxDump);
        }
        startTimer(1);
    }
}


//==============================================================================
void Midio128ToolControl::sliderValueChanged(Slider* slider)
{
    if( slider == deviceIdSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("midio128DeviceId"), (int)slider->getValue());
    }
}


//==============================================================================
void Midio128ToolControl::timerCallback()
{
    stopTimer(); // will be restarted if required

    bool transferFinished = false;

    if( receiveDump ) {
        if( syxBlock > 0 ) {
            if( !dumpReceived ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("No response from core."),
                                            T("Check:\n- MIDI In/Out connections\n- Device ID\n- that MIDIO128 firmware has been uploaded"),
                                            String::empty);
            } else if( checksumError ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Detected checksum error!"),
                                            T("Check:\n- MIDI In/Out connections\n- your MIDI interface"),
                                            String::empty);
            }
        }

        if( !transferFinished ) {
            if( syxBlock >= 6 ) {
                transferFinished = true;

                if( currentSyxDump.size() != 6*256 ) {
                    AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                                T("Unexpected MIDI handling error!"),
                                                T("Please inform TK how this happened."),
                                                T("will do"));
                } else {
                    midio128ToolConfig->setDump(currentSyxDump);
                }
            } else {
                dumpReceived = false;
                checksumError = false;
                Array<uint8> data = SysexHelper::createMidio128ReadBlock((uint8)deviceIdSlider->getValue(),
                                                                         syxBlock);
                MidiMessage message = SysexHelper::createMidiMessage(data);
                miosStudio->sendMidiMessage(message);

                progress = (double)++syxBlock / 6.0;
                startTimer(1000);
            }
        }
    } else {
        if( syxBlock >= 6 ) {
            transferFinished = true;
        } else {
            Array<uint8> data = SysexHelper::createMidio128WriteBlock((uint8)deviceIdSlider->getValue(),
                                                                      syxBlock,
                                                                      &currentSyxDump.getReference(syxBlock*256));
            MidiMessage message = SysexHelper::createMidiMessage(data);
            miosStudio->sendMidiMessage(message);

            progress = (double)++syxBlock / 6.0;
            startTimer(1000);
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
void Midio128ToolControl::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();

    if( receiveDump ) {
        int receivedBlock = syxBlock-1;
        if( SysexHelper::isValidMidio128WriteBlock(data, size, (int)deviceIdSlider->getValue()) &&
            data[6] == receivedBlock ) {
            dumpReceived = true;

            if( size != 265 ) {
                checksumError = true;
            } else {
                uint8 checksum = 0x00;
                int pos;
                for(pos=6; pos<6+1+256; ++pos)
                    checksum += data[pos];
                if( data[pos] != (-(int)checksum & 0x7f) )
                    checksumError = true;
                else {
                    for(pos=7; pos<6+1+256; ++pos)
                        currentSyxDump.add(data[pos]);
                }
            }

            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    } else if( isTimerRunning() ) {
        if( SysexHelper::isValidMidio128Acknowledge(data, size, (int)deviceIdSlider->getValue()) ) {
            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    }
}


//==============================================================================
bool Midio128ToolControl::loadSyx(File &syxFile)
{
    FileInputStream *inFileStream = syxFile.createInputStream();

    if( !inFileStream ) {
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
    int numValidBlocks = 0;
    Array<uint8> syxDump;
    String errorMessage;

    if( readNumBytes != size ) {
        errorMessage = String(T("cannot be read completely!"));
    } else {
        for(int pos=0; pos<size; ++pos) {
            if( SysexHelper::isValidMidio128WriteBlock(buffer+pos, size - pos, -1) ) {
                pos += 6;

                if( pos < size ) {
                    int block = buffer[pos++];
                    if( block < 6 ) {
                        int blockPos = 0;
                        uint8 checksum = block;
                        while( pos < size && buffer[pos] != 0xf7 && blockPos < 256 ) {
                            uint8 b = buffer[pos++];
                            syxDump.set(block*256 + blockPos, b);
                            checksum += b;
                            ++blockPos;
                        }

                        if( blockPos != 256 ) {
                            errorMessage = String::formatted(T("contains invalid data: block %d not complete (only %d bytes)"), block, blockPos);
                        } else {
                            checksum = (-(int)checksum) & 0x7f;
                            if( buffer[pos] != checksum ) {
                                errorMessage = String::formatted(T("contains invalid checksum in block %d (expected %02X but read %02X)!"), block, checksum, buffer[pos]);
                            } else {
                                ++pos;
                                ++numValidBlocks;
                            }
                        }
                    } else {
                        errorMessage = String::formatted(T("contains invalid block: %d"), block);
                    }
                }
            }
        }
    }

    juce_free(buffer);

    if( errorMessage == String::empty ) {
        if( !numValidBlocks )
            errorMessage = String(T("doesn't contain valid MIDIO128 data!"));
        else if( numValidBlocks < 6 )
            errorMessage = String::formatted(T("is not complete (expect 6 blocks, only found %d blocks"), numValidBlocks);
    }

    if( errorMessage != String::empty ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("The file ") + syxFile.getFileName(),
                                    errorMessage,
                                    String::empty);
        return false;
    }

    midio128ToolConfig->setDump(syxDump);

    return true;
}

bool Midio128ToolControl::saveSyx(File &syxFile)
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
    midio128ToolConfig->getDump(syxDump);

    for(int block=0; block<6; ++block) {
        Array<uint8> data = SysexHelper::createMidio128WriteBlock((uint8)deviceIdSlider->getValue(),
                                                                  block,
                                                                  &syxDump.getReference(block*256));
        outFileStream->write(&data.getReference(0), data.size());
    }

    delete outFileStream;

    return true;
}


//==============================================================================
//==============================================================================
//==============================================================================
Midio128Tool::Midio128Tool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(midio128ToolConfig = new Midio128ToolConfig(miosStudio));
    addAndMakeVisible(midio128ToolControl = new Midio128ToolControl(miosStudio, midio128ToolConfig));

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(860, 500);
}

Midio128Tool::~Midio128Tool()
{
}

//==============================================================================
void Midio128Tool::paint (Graphics& g)
{
    //g.fillAll(Colour(0xffc1d0ff));
    g.fillAll(Colours::white);
}

void Midio128Tool::resized()
{
    midio128ToolControl->setBounds(0, 0, getWidth(), 40);
    midio128ToolConfig->setBounds(0, 40, getWidth(), getHeight()-40);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}
