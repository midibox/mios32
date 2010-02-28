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

#include "MbCvTool.h"
#include "MiosStudio.h"


MbCvToolConfigGlobals::MbCvToolConfigGlobals()
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

    addAndMakeVisible(clockDividerLabel = new Label(String::empty, T("Output Clock Divider:")));
    clockDividerLabel->setJustificationType(Justification::right);
    addAndMakeVisible(clockDividerComboBox = new ComboBox(String::empty));
    clockDividerComboBox->setWantsKeyboardFocus(true);
    clockDividerComboBox->addItem(T("96 ppqn"), 1);
    clockDividerComboBox->addItem(T("48 ppqn"), 2);
    clockDividerComboBox->addItem(T("32 ppqn"), 3);
    clockDividerComboBox->addItem(T("24 ppqn"), 4);
    for(int i=2; i<=16; ++i)
        clockDividerComboBox->addItem(T("24 ppqn / " + String(i)), 5+i-2);
    clockDividerComboBox->setSelectedId(4, true);

    addAndMakeVisible(nameLabel = new Label(String::empty, T("Patch Name:")));
    nameLabel->setJustificationType(Justification::right);
    addAndMakeVisible(nameEditor = new TextEditor(String::empty));
    nameEditor->setTextToShowWhenEmpty(T("<No Name>"), Colours::grey);
    nameEditor->setInputRestrictions(16);
}


MbCvToolConfigGlobals::~MbCvToolConfigGlobals()
{
    deleteAllChildren();
}

//==============================================================================
void MbCvToolConfigGlobals::resized()
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

    clockDividerLabel->setBounds(labelX, labelY0 + 1*labelYOffset, labelWidth, labelHeight);
    clockDividerComboBox->setBounds(controlX, controlY0 + 1*controlYOffset, controlWidth, controlHeight);

    nameLabel->setBounds(labelX, labelY0 + 2*labelYOffset, labelWidth, labelHeight);
    nameEditor->setBounds(controlX, controlY0 + 2*controlYOffset, controlWidth, controlHeight);
}


//==============================================================================
void MbCvToolConfigGlobals::getDump(Array<uint8> &syxDump)
{
    int merger = mergerComboBox->getSelectedId()-1;
    int clockDivider = clockDividerComboBox->getSelectedId()-1;

    syxDump.set(0x00, merger);
    syxDump.set(0x03, clockDivider & 0x7f);

    int i;
    String txt = nameEditor->getText();
    for(i=0; i<16 && i<txt.length(); ++i)
        syxDump.set(0xf0 + i, txt[i]);
    while( i < 16 ) {
        syxDump.set(0xf0 + i, 0);
        i++;
    }
}

void MbCvToolConfigGlobals::setDump(const Array<uint8> &syxDump)
{
    int merger = syxDump[0x00];
    int clockDivider = syxDump[0x03];

    mergerComboBox->setSelectedId(merger+1);
    clockDividerComboBox->setSelectedId(clockDivider+1);

    String txt;
    for(int i=0; i<16 && syxDump[0xf0+i]; ++i) {
        char dummy = syxDump[0xf0 + i];
        txt += String(&dummy, 1);
    }
    nameEditor->setText(txt);
}


//==============================================================================
//==============================================================================
//==============================================================================
MbCvToolConfigChannels::MbCvToolConfigChannels()
    : font(14.0f)
    , numRows(8)
{
    for(int cv=0; cv<numRows; ++cv) {
        cvChannel.add(1+cv); // Default MIDI Channel
        cvEvent.add(1); // Note
        cvLegato.add(1); // enabled
        cvPoly.add(0); // disabled
        cvGateInverted.add(0); // disabled
        cvPitchrange.add(2);
        cvSplitLower.add(0x00+1);
        cvSplitUpper.add(0x7f+1);
        cvTransposeOctave.add(0);
        cvTransposeSemitones.add(0);
        cvCcNumber.add(0x10 + cv);
        cvCurve.add(1);
        cvSlewRate.add(1);
    }

    addAndMakeVisible(table = new TableListBox(T("CV Channels Table"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);

    TableHeaderComponent *tableHeader = table->getHeader();
    tableHeader->addColumn(T("CV"), 1, 25);
    tableHeader->addColumn(T("Channel"), 2, 70);
    tableHeader->addColumn(T("Event"), 3, 100);
    tableHeader->addColumn(T("Legato"), 4, 40);
    tableHeader->addColumn(T("Poly"), 5, 30);
    tableHeader->addColumn(T("Inv.Gate"), 6, 50);
    tableHeader->addColumn(T("Pitchrange"), 7, 65);
    tableHeader->addColumn(T("Lower"), 8, 50);
    tableHeader->addColumn(T("Upper"), 9, 50);
    tableHeader->addColumn(T("Tr.Octave"), 10, 70);
    tableHeader->addColumn(T("Tr.Semitones"), 11, 70);
    tableHeader->addColumn(T("CC Number"), 12, 70);
    tableHeader->addColumn(T("Curve"), 13, 80);
    tableHeader->addColumn(T("Slew Rate"), 14, 70);

    setSize(800, 200);
}

MbCvToolConfigChannels::~MbCvToolConfigChannels()
{
}

//==============================================================================
int MbCvToolConfigChannels::getNumRows()
{
    return numRows;
}

void MbCvToolConfigChannels::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);
}

void MbCvToolConfigChannels::paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
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


Component* MbCvToolConfigChannels::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    switch( columnId ) {
    case 2: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(1, 16);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 3: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("Note"), 1);
            comboBox->addItem(T("Velocity"), 2);
            comboBox->addItem(T("Aftertouch"), 3);
            comboBox->addItem(T("Controller (CC)"), 4);
            comboBox->addItem(T("NRPN"), 5);
            comboBox->addItem(T("Pitch Bender"), 6);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;

    case 4:
    case 5:
    case 6: {
        ConfigTableToggleButton *toggleButton = (ConfigTableToggleButton *)existingComponentToUpdate;

        if( toggleButton == 0 )
            toggleButton = new ConfigTableToggleButton(*this);

        toggleButton->setRowAndColumn(rowNumber, columnId);
        return toggleButton;
    } break;

    case 7: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 24);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 8:
    case 9: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            const char noteTab[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("---"), 1);
            for(int i=1; i<128; ++i) {
                comboBox->addItem(String(noteTab[i%12]) + String((i/12)-2), i+1);
            }
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;

    case 10:
    case 11: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(-8, 7);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 12: {
        ConfigTableSlider *slider = (ConfigTableSlider *)existingComponentToUpdate;

        if( slider == 0 ) {
            slider = new ConfigTableSlider(*this);
            slider->setRange(0, 127);
        }

        slider->setRowAndColumn(rowNumber, columnId);
        return slider;
    } break;

    case 13: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("V/Octave"), 1);
            comboBox->addItem(T("Hz/V"), 2);
            comboBox->addItem(T("Inverted"), 3);
            comboBox->addItem(T("Exponential"), 4);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;

    case 14: {
        ConfigTableComboBox *comboBox = (ConfigTableComboBox *)existingComponentToUpdate;

        if( comboBox == 0 ) {
            comboBox = new ConfigTableComboBox(*this);
            comboBox->addItem(T("off"), 1);
            comboBox->addItem(T("1 mS"), 2);
            comboBox->addItem(T("2 mS"), 3);
            comboBox->addItem(T("4 mS"), 4);
            comboBox->addItem(T("8 mS"), 5);
            comboBox->addItem(T("16 mS"), 6);
            comboBox->addItem(T("32 mS"), 7);
            comboBox->addItem(T("64 mS"), 8);
        }

        comboBox->setRowAndColumn(rowNumber, columnId);
        return comboBox;
    } break;
    }

    return 0;
}


//==============================================================================
void MbCvToolConfigChannels::resized()
{
    // position our table with a gap around its edge
    table->setBoundsInset(BorderSize(8));
}


//==============================================================================
int MbCvToolConfigChannels::getTableValue(const int rowNumber, const int columnId)
{
    switch( columnId ) {
    case 2: return cvChannel[rowNumber];
    case 3: return cvEvent[rowNumber];
    case 4: return cvLegato[rowNumber];
    case 5: return cvPoly[rowNumber];
    case 6: return cvGateInverted[rowNumber];
    case 7: return cvPitchrange[rowNumber];
    case 8: return cvSplitLower[rowNumber];
    case 9: return cvSplitUpper[rowNumber];
    case 10: return cvTransposeOctave[rowNumber];
    case 11: return cvTransposeSemitones[rowNumber];
    case 12: return cvCcNumber[rowNumber];
    case 13: return cvCurve[rowNumber];
    case 14: return cvSlewRate[rowNumber];
    }
    return 0;
}

void MbCvToolConfigChannels::setTableValue(const int rowNumber, const int columnId, const int newValue)
{
    switch( columnId ) {
    case 2: cvChannel.set(rowNumber, newValue); break;
    case 3: cvEvent.set(rowNumber, newValue); break;
    case 4: cvLegato.set(rowNumber, newValue); break;
    case 5: cvPoly.set(rowNumber, newValue); break;
    case 6: cvGateInverted.set(rowNumber, newValue); break;
    case 7: cvPitchrange.set(rowNumber, newValue); break;
    case 8: cvSplitLower.set(rowNumber, newValue); break;
    case 9: cvSplitUpper.set(rowNumber, newValue); break;
    case 10: cvTransposeOctave.set(rowNumber, newValue); break;
    case 11: cvTransposeSemitones.set(rowNumber, newValue); break;
    case 12: cvCcNumber.set(rowNumber, newValue); break;
    case 13: cvCurve.set(rowNumber, newValue); break;
    case 14: cvSlewRate.set(rowNumber, newValue); break;
    }
}


//==============================================================================
void MbCvToolConfigChannels::getDump(Array<uint8> &syxDump)
{
    for(int cv=0; cv<numRows; ++cv) {
        syxDump.set(1*8+cv, (cvChannel[cv]-1) & 0xf);
        syxDump.set(2*8+cv, ((cvEvent[cv]-1) & 0xf) | (cvLegato[cv] << 4) | (cvPoly[cv] << 5));
        syxDump.set(3*8+cv, cvPitchrange[cv]);
        syxDump.set(4*8+cv, cvSplitLower[cv]-1);
        syxDump.set(5*8+cv, cvSplitUpper[cv]-1);
        syxDump.set(6*8+cv, (cvTransposeOctave[cv] >= 0) ? cvTransposeOctave[cv] : (cvTransposeOctave[cv]+16));
        syxDump.set(7*8+cv, (cvTransposeSemitones[cv] >= 0) ? cvTransposeSemitones[cv] : (cvTransposeSemitones[cv]+16));
        syxDump.set(8*8+cv, cvCcNumber[cv]);
        syxDump.set(9*8+cv, (cvCurve[cv]-1));
        syxDump.set(10*8+cv, (cvSlewRate[cv]-1));

        uint8 andMask = ~(1 << cv);
        uint8 orMask = cvGateInverted[cv] ? (1 << cv) : 0;
        syxDump.set(0x02, (syxDump[0x02] & andMask) | orMask);
    }
}

void MbCvToolConfigChannels::setDump(const Array<uint8> &syxDump)
{
    for(int cv=0; cv<numRows; ++cv) {
        cvChannel.set(cv, (syxDump[1*8+cv] & 0x0f)+1);
        cvEvent.set(cv, (syxDump[2*8+cv] & 0x0f)+1);
        cvLegato.set(cv, (syxDump[2*8+cv] & 0x10) ? 1 : 0);
        cvPoly.set(cv, (syxDump[2*8+cv] & 0x20) ? 1 : 0);
        cvPitchrange.set(cv, syxDump[3*8+cv]);
        cvSplitLower.set(cv, syxDump[4*8+cv]+1);
        cvSplitUpper.set(cv, syxDump[5*8+cv]+1);
        cvTransposeOctave.set(cv, (syxDump[6*8+cv] < 8) ? syxDump[6*8+cv] : syxDump[6*8+cv] - 16);
        cvTransposeSemitones.set(cv, (syxDump[7*8+cv] < 8) ? syxDump[7*8+cv] : syxDump[7*8+cv] - 16);
        cvCcNumber.set(cv, syxDump[8*8+cv]);
        cvCurve.set(cv, syxDump[9*8+cv]+1);
        cvSlewRate.set(cv, syxDump[10*8+cv]+1);
        cvGateInverted.set(cv, (syxDump[0x02] & (1 << cv)) ? 1 : 0);
    }
    table->updateContent();
}

//==============================================================================
//==============================================================================
//==============================================================================
MbCvToolConfig::MbCvToolConfig(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , TabbedComponent(TabbedButtonBar::TabsAtTop)
{
    addTab(T("Channels"), Colour(0xfff0f0d0), configChannels = new MbCvToolConfigChannels(), true);
    addTab(T("Global"),   Colour(0xfff0f0e0), configGlobals = new MbCvToolConfigGlobals(), true);
    setSize(800, 200);
}

MbCvToolConfig::~MbCvToolConfig()
{
}

//==============================================================================
void MbCvToolConfig::getDump(Array<uint8> &syxDump)
{
    for(int addr=0x000; addr<0x460; ++addr)
        syxDump.set(addr, 0x00);
    for(int addr=0x460; addr<0x600; ++addr)
        syxDump.set(addr, 0x7f);

    configGlobals->getDump(syxDump);
    configChannels->getDump(syxDump);
}

void MbCvToolConfig::setDump(const Array<uint8> &syxDump)
{
    configGlobals->setDump(syxDump);
    configChannels->setDump(syxDump);
}



//==============================================================================
//==============================================================================
//==============================================================================
MbCvToolControl::MbCvToolControl(MiosStudio *_miosStudio, MbCvToolConfig *_mbCvToolConfig)
    : miosStudio(_miosStudio)
    , mbCvToolConfig(_mbCvToolConfig)
    , progress(0)
    , receiveDump(false)
    , dumpRequested(false)
    , dumpReceived(false)
    , checksumError(false)
    , dumpSent(false)
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

    addAndMakeVisible(patchLabel = new Label(T("Patch"), T("Patch:")));
    patchLabel->setJustificationType(Justification::right);
    
    addAndMakeVisible(patchSlider = new Slider(T("Patch")));
    patchSlider->setRange(1, 128, 1);
    patchSlider->setSliderStyle(Slider::IncDecButtons);
    patchSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    patchSlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(receiveButton = new TextButton(T("Receive Button")));
    receiveButton->setButtonText(T("Receive"));
    receiveButton->addButtonListener(this);

    addAndMakeVisible(sendButton = new TextButton(T("Send Button")));
    sendButton->setButtonText(T("Send"));
    sendButton->addButtonListener(this);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        deviceIdSlider->setValue(propertiesFile->getIntValue(T("mbCvDeviceId"), 0) & 0x7f);
        patchSlider->setValue(propertiesFile->getIntValue(T("mbCvPatch"), 0) & 0x7f);
        String syxFileName(propertiesFile->getValue(T("mbCvSyxFile"), String::empty));
        if( syxFileName != String::empty )
            syxFile = File(syxFileName);
    }

    setSize(800, 200);
}

MbCvToolControl::~MbCvToolControl()
{
    deleteAllChildren();
}

//==============================================================================
void MbCvToolControl::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MbCvToolControl::resized()
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

    patchLabel->setBounds(buttonX0 + 20+4*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    patchSlider->setBounds(buttonX0 + 20+5*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    receiveButton->setBounds(buttonX0 + 40+6*buttonXOffset, buttonY, buttonWidth, buttonHeight);
    sendButton->setBounds(buttonX0 + 40+7*buttonXOffset, buttonY, buttonWidth, buttonHeight);

    int progressBarX = sendButton->getX() + buttonWidth + 20;
    progressBar->setBounds(progressBarX, buttonY, getWidth()-progressBarX-buttonX0, buttonHeight);
}

//==============================================================================
void MbCvToolControl::buttonClicked(Button* buttonThatWasClicked)
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
                    propertiesFile->setValue(T("mbCvSyxFile"), syxFile.getFullPathName());
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
                    propertiesFile->setValue(T("mbCvSyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == sendButton || buttonThatWasClicked == receiveButton ) {
        loadButton->setEnabled(false);
        saveButton->setEnabled(false);
        sendButton->setEnabled(false);
        receiveButton->setEnabled(false);
        progress = 0;
        if( buttonThatWasClicked == receiveButton ) {
            dumpRequested = false;
            receiveDump = true;
            currentSyxDump.clear();
        } else {
            receiveDump = false;
            dumpSent = false;
            mbCvToolConfig->getDump(currentSyxDump);
        }
        startTimer(1);
    }
}


//==============================================================================
void MbCvToolControl::sliderValueChanged(Slider* slider)
{
    if( slider == deviceIdSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("mbCvDeviceId"), (int)slider->getValue());
    } else if( slider == patchSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("mbCvPatch"), (int)slider->getValue());
    }
}


//==============================================================================
void MbCvToolControl::timerCallback()
{
    stopTimer(); // will be restarted if required

    bool transferFinished = false;

    if( receiveDump ) {
        if( dumpRequested ) {
            if( !dumpReceived ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("No response from core."),
                                            T("Check:\n- MIDI In/Out connections\n- Device ID\n- that MIDIbox CV firmware has been uploaded"),
                                            String::empty);
            } else if( checksumError ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Detected checksum error!"),
                                            T("Check:\n- MIDI In/Out connections\n- your MIDI interface"),
                                            String::empty);
            } else {
                mbCvToolConfig->setDump(currentSyxDump);
                transferFinished = true;
            }
        } else {
            dumpReceived = false;
            checksumError = false;
            dumpRequested = true;
            Array<uint8> data = SysexHelper::createMbCvReadPatch((uint8)deviceIdSlider->getValue(),
                                                                 (int)patchSlider->getValue()-1);
            MidiMessage message = SysexHelper::createMidiMessage(data);
            miosStudio->sendMidiMessage(message);

            progress = 1.0;
            startTimer(1000);
        }
    } else {
        if( dumpSent ) {
            transferFinished = true;
        } else {
            Array<uint8> data = SysexHelper::createMbCvWritePatch((uint8)deviceIdSlider->getValue(),
                                                                  (int)patchSlider->getValue()-1,
                                                                  &currentSyxDump.getReference(0));
            MidiMessage message = SysexHelper::createMidiMessage(data);
            miosStudio->sendMidiMessage(message);

            dumpSent = true;
            progress = 1.0;
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
void MbCvToolControl::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();

    if( receiveDump ) {
        if( dumpRequested &&
            SysexHelper::isValidMbCvWritePatch(data, size, (int)deviceIdSlider->getValue()) &&
            data[7] == ((int)patchSlider->getValue()-1) ) {
            dumpReceived = true;

            if( size != 266 ) {
                checksumError = true;
            } else {
                uint8 checksum = 0x00;
                int pos;
                for(pos=8; pos<7+1+256; ++pos)
                    checksum += data[pos];
                if( data[pos] != (-(int)checksum & 0x7f) )
                    checksumError = true;
                else {
                    for(pos=8; pos<7+1+256; ++pos)
                        currentSyxDump.add(data[pos]);
                }
            }

            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    } else if( isTimerRunning() ) {
        if( SysexHelper::isValidMbCvAcknowledge(data, size, (int)deviceIdSlider->getValue()) ) {
            // trigger timer immediately
            stopTimer();
            startTimer(1);
        }
    }
}


//==============================================================================
bool MbCvToolControl::loadSyx(File &syxFile)
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
            if( SysexHelper::isValidMbCvWritePatch(buffer+pos, size - pos, -1) ) {
                pos += 7;

                if( pos < size ) {
                    int patch = buffer[pos++]; // not relevant, will be ignored
                    int patchPos = 0;
                    uint8 checksum = 0x00;
                    while( pos < size && buffer[pos] != 0xf7 && patchPos < 256 ) {
                        uint8 b = buffer[pos++];
                        syxDump.set(patchPos, b);
                        checksum += b;
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

    mbCvToolConfig->setDump(syxDump);

    return true;
}

bool MbCvToolControl::saveSyx(File &syxFile)
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
    mbCvToolConfig->getDump(syxDump);

    Array<uint8> data = SysexHelper::createMbCvWritePatch((uint8)deviceIdSlider->getValue(),
                                                          (int)patchSlider->getValue()-1,
                                                          &syxDump.getReference(0));
    outFileStream->write(&data.getReference(0), data.size());

    delete outFileStream;

    return true;
}


//==============================================================================
//==============================================================================
//==============================================================================
MbCvTool::MbCvTool(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(mbCvToolConfig = new MbCvToolConfig(miosStudio));
    addAndMakeVisible(mbCvToolControl = new MbCvToolControl(miosStudio, mbCvToolConfig));

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(860, 500);
}

MbCvTool::~MbCvTool()
{
    deleteAllChildren();
}

//==============================================================================
void MbCvTool::paint (Graphics& g)
{
    //g.fillAll(Colour(0xffc1d0ff));
    g.fillAll(Colours::white);
}

void MbCvTool::resized()
{
    mbCvToolControl->setBounds(0, 0, getWidth(), 40);
    mbCvToolConfig->setBounds(0, 40, getWidth(), getHeight()-40);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}
