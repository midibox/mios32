/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * SysEx Librarian Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "SysexLibrarian.h"
#include "MiosStudio.h"

SysexLibrarianBank::SysexLibrarianBank(MiosStudio *_miosStudio, const String& bankName)
    : miosStudio(_miosStudio)
    , font(14.0f)
{
    addAndMakeVisible(bankHeaderLabel = new Label(bankName, bankName));
    bankHeaderLabel->setJustificationType(Justification::centred);

    addAndMakeVisible(moveDownButton = new TextButton(T("Down")));
    moveDownButton->addListener(this);

    addAndMakeVisible(moveUpButton = new TextButton(T("Up")));
    moveUpButton->addListener(this);

    addAndMakeVisible(insertButton = new TextButton(T("Insert")));
    insertButton->addListener(this);

    addAndMakeVisible(deleteButton = new TextButton(T("Delete")));
    deleteButton->addListener(this);

    addAndMakeVisible(clearButton = new TextButton(T("Clear")));
    clearButton->addListener(this);

    addAndMakeVisible(table = new TableListBox(T("Bank"), this));
    table->setColour(ListBox::outlineColourId, Colours::grey);
    table->setOutlineThickness(1);
    table->setMultipleSelectionEnabled(true);

    table->getHeader().addColumn(T("Patch"), 1, 50);
    table->getHeader().addColumn(T("Name"), 2, 140);

    initBank(0);

    setSize(225, 200);
}

SysexLibrarianBank::~SysexLibrarianBank()
{
}

//==============================================================================
int SysexLibrarianBank::getNumRows()
{
    return numRows;
}

void SysexLibrarianBank::setNumRows(const int& rows)
{
    numRows = rows;

    patchStorage.clear();
    patchName.clear();
    for(int i=0; i<numRows; ++i) {
        patchName.set(i, T(""));
    }
}


void SysexLibrarianBank::initBank(const unsigned& _patchSpec)
{
    if( _patchSpec < miosStudio->sysexPatchDb->getNumSpecs() ) {
        patchSpec = _patchSpec;

        // this workaround ensures that the table view will be updated
        numRows = 10001;
        table->selectRow(10000);

        setNumRows(miosStudio->sysexPatchDb->getNumPatchesPerBank(patchSpec));
        table->selectRow(0);
    }
}

unsigned SysexLibrarianBank::getPatchSpec(void)
{
    return patchSpec;
}

unsigned SysexLibrarianBank::getSelectedPatch(void)
{
    return table->getSelectedRow();
}

bool SysexLibrarianBank::isSelectedPatch(const unsigned& patch)
{
    return table->isRowSelected(patch);
}

void SysexLibrarianBank::selectPatch(const unsigned& patch)
{
    table->selectRow(patch);
}

bool SysexLibrarianBank::isSingleSelection(void)
{
    int numSelected = 0;

    for(int patch=0; patch<numRows; ++patch)
        if( isSelectedPatch(patch) )
            ++numSelected;

    return numSelected == 1;
}

void SysexLibrarianBank::incPatchIfSingleSelection(void)
{
    if( !isSelectedPatch(numRows-1) && isSingleSelection() )
        selectPatch(getSelectedPatch()+1);
}


void SysexLibrarianBank::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if( rowIsSelected )
        g.fillAll(Colours::lightblue);
}

void SysexLibrarianBank::paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(Colours::black);
    g.setFont(font);

    switch( columnId ) {
    case 1: {
        char buffer[20];
        sprintf(buffer, "%03d", rowNumber+1);
        const String text(buffer);
        g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);
    } break;
    }

    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}


Component* SysexLibrarianBank::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    switch( columnId ) {
    case 2: {
        ConfigTableLabel *label = (ConfigTableLabel *)existingComponentToUpdate;

        if( label == 0 ) {
            label = new ConfigTableLabel(*this);
        }

        label->setRowAndColumn(rowNumber, columnId);

        return label;
    } break;
    }

    return 0;
}


//==============================================================================
void SysexLibrarianBank::resized()
{
    bankHeaderLabel->setBounds(0, 0, getWidth(), 20);

    //unsigned buttonWidth = getWidth() / 5;
    unsigned buttonWidth = 200 / 5;

    moveDownButton->setBounds (0*buttonWidth+8, 20, buttonWidth-8, 16);
    moveUpButton->setBounds   (1*buttonWidth+8, 20, buttonWidth-8, 16);
    insertButton->setBounds   (2*buttonWidth+8, 20, buttonWidth-8, 16);
    deleteButton->setBounds   (3*buttonWidth+8, 20, buttonWidth-8, 16);
    clearButton->setBounds    (4*buttonWidth+8, 20, buttonWidth-8, 16);

    table->setBounds          (8, 40, getWidth()-16-16, getHeight()-40-8);
}


//==============================================================================
int SysexLibrarianBank::getTableValue(const int rowNumber, const int columnId)
{
    return 0;
}

void SysexLibrarianBank::setTableValue(const int rowNumber, const int columnId, const int newValue)
{
}


//==============================================================================
String SysexLibrarianBank::getTableString(const int rowNumber, const int columnId)
{
    switch( columnId ) {
    case 2: return patchName[rowNumber];
    }
    return String(T("???"));
}

void SysexLibrarianBank::setTableString(const int rowNumber, const int columnId, const String newString)
{
    switch( columnId ) {
    case 2: {
        Array<uint8>* p = patchStorage[rowNumber];
        if( p != NULL && p->size() ) {
            // replace patchname
            miosStudio->sysexPatchDb->replacePatchNameInPayload(patchSpec, *p, newString);

            // and back from payload (to doublecheck that it has been taken over)
            patchName.set(rowNumber, miosStudio->sysexPatchDb->getPatchNameFromPayload(patchSpec, *p));
        }
    } break;
    }
}


//==============================================================================
Array<uint8>* SysexLibrarianBank::getPatch(const uint8& patch)
{
    return patchStorage[patch];
}

void SysexLibrarianBank::setPatch(const uint8& patch, const Array<uint8> &payload)
{
    if( patchStorage[patch] != NULL ) {
        //juce_free(patchStorage[patch]);
    }
    patchStorage.set(patch, new Array<uint8>(payload));
    if( payload.size() ) {
        patchName.set(patch, miosStudio->sysexPatchDb->getPatchNameFromPayload(patchSpec, payload));
    } else {
        patchName.set(patch, String::empty);
    }

    table->resized(); // will do the trick, repaint() doesn't cause update
}


//==============================================================================
void SysexLibrarianBank::buttonClicked(Button* buttonThatWasClicked)
{
    Array<uint8> emptyArray;
    int maxPatches=getNumRows();

    if( buttonThatWasClicked == clearButton ) {
        initBank(patchSpec);
    } else if( buttonThatWasClicked == moveDownButton ) {
        for(int patch=maxPatches-2; patch >= 0; --patch) { // last patch won't be moved
            if( isSelectedPatch(patch) ) {
                Array<uint8>* p = getPatch(patch);
                Array<uint8> storage((p != NULL) ? *p : emptyArray);
                Array<uint8>* p2 = getPatch(patch+1);
                setPatch(patch, (p2 != NULL) ? *p2 : emptyArray);
                setPatch(patch+1, storage);
                table->deselectRow(patch);
                table->selectRow(patch+1, false, false);
            }
        }
    } else if( buttonThatWasClicked == moveUpButton ) {
        for(int patch=1; patch <maxPatches; ++patch) { // first patch won't be moved
            if( isSelectedPatch(patch) ) {
                Array<uint8>* p = getPatch(patch-1);
                Array<uint8> storage((p != NULL) ? *p : emptyArray);
                Array<uint8>* p2 = getPatch(patch);
                setPatch(patch-1, (p2 != NULL) ? *p2 : emptyArray);
                setPatch(patch, storage);
                table->deselectRow(patch);
                table->selectRow(patch-1, false, false);
            }
        }
    } else if( buttonThatWasClicked == insertButton ) {
        for(int patch=0; patch<maxPatches; ++patch) {
            if( isSelectedPatch(patch) ) {
                for(int i=maxPatches-2; i >= patch; --i) {
                    Array<uint8>* p = getPatch(i);
                    setPatch(i+1, (p != NULL) ? *p : emptyArray);
                }
                setPatch(patch, emptyArray);
                table->selectRow(patch, false, false); // to ensure table update
            }
        }
    } else if( buttonThatWasClicked == deleteButton ) {
        for(int patch=0; patch<maxPatches; ++patch) {
            if( isSelectedPatch(patch) ) {
                for(int i=patch; i < (maxPatches-1); ++i) {
                    Array<uint8>* p = getPatch(i+1);
                    setPatch(i, (p != NULL) ? *p : emptyArray);
                }
                setPatch(maxPatches-1, emptyArray);
                table->selectRow(patch, false, false); // to ensure table update
            }
        }
    }
}


//==============================================================================
//==============================================================================
//==============================================================================
SysexLibrarianControl::SysexLibrarianControl(MiosStudio *_miosStudio, SysexLibrarian *_sysexLibrarian)
    : miosStudio(_miosStudio)
    , sysexLibrarian(_sysexLibrarian)
    , currentPatch(0)
    , timerRestartDelay(10)
    , retryCtr(0)
    , progress(0)
    , receiveDump(false)
    , handleSinglePatch(false)
    , bufferTransfer(false)
    , dumpReceived(false)
    , checksumError(false)
    , errorResponse(false)
{
    addAndMakeVisible(deviceTypeLabel = new Label(T("MIDI Device:"), T("MIDI Device:")));
    deviceTypeLabel->setJustificationType(Justification::right);

	addAndMakeVisible(deviceTypeSelector = new ComboBox(String::empty));
	deviceTypeSelector->addListener(this);
	deviceTypeSelector->clear();
    for(int i=0; i<miosStudio->sysexPatchDb->getNumSpecs(); ++i) {
        deviceTypeSelector->addItem(miosStudio->sysexPatchDb->getSpecName(i), i+1);
    }
    deviceTypeSelector->setSelectedId(1, true);
    deviceTypeSelector->setEnabled(true);

    addAndMakeVisible(deviceIdLabel = new Label(T("Device ID"), T("Device ID:")));
    deviceIdLabel->setJustificationType(Justification::right);

    addAndMakeVisible(deviceIdSlider = new Slider(T("Device ID")));
    deviceIdSlider->setRange(0, 7, 1);
    deviceIdSlider->setSliderStyle(Slider::IncDecButtons);
    deviceIdSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    deviceIdSlider->setDoubleClickReturnValue(true, 0);
    
    addAndMakeVisible(bankSelectLabel = new Label(T("Bank"), T("Bank:")));
    bankSelectLabel->setJustificationType(Justification::right);

    addAndMakeVisible(bankSelectSlider = new Slider(T("Bank")));
    bankSelectSlider->setRange(1, 8, 1);
    bankSelectSlider->setSliderStyle(Slider::IncDecButtons);
    bankSelectSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    bankSelectSlider->setDoubleClickReturnValue(true, 0);
    
    addAndMakeVisible(loadBankButton = new TextButton(T("Load Bank")));
    loadBankButton->addListener(this);
    addAndMakeVisible(saveBankButton = new TextButton(T("Save Bank")));
    saveBankButton->addListener(this);
    addAndMakeVisible(receiveBankButton = new TextButton(T("Receive Bank")));
    receiveBankButton->addListener(this);
    addAndMakeVisible(sendBankButton = new TextButton(T("Send Bank")));
    sendBankButton->addListener(this);

    addAndMakeVisible(loadPatchButton = new TextButton(T("Load Patch")));
    loadPatchButton->addListener(this);
    addAndMakeVisible(savePatchButton = new TextButton(T("Save Patch")));
    savePatchButton->addListener(this);
    addAndMakeVisible(receivePatchButton = new TextButton(T("Receive Patch")));
    receivePatchButton->addListener(this);
    addAndMakeVisible(sendPatchButton = new TextButton(T("Send Patch")));
    sendPatchButton->addListener(this);

    addAndMakeVisible(bufferLabel = new Label(T("Buffer"), T("SID:")));
    bufferLabel->setJustificationType(Justification::right);

    addAndMakeVisible(bufferSlider = new Slider(T("Buffer")));
    bufferSlider->setRange(1, 4, 1);
    bufferSlider->setSliderStyle(Slider::IncDecButtons);
    bufferSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 30, 20);
    bufferSlider->setDoubleClickReturnValue(true, 0);

    addAndMakeVisible(receiveBufferButton = new TextButton(T("Receive Buffer")));
    receiveBufferButton->addListener(this);
    addAndMakeVisible(sendBufferButton = new TextButton(T("Send Buffer")));
    sendBufferButton->addListener(this);

    addAndMakeVisible(stopButton = new TextButton(T("Stop")));
    stopButton->addListener(this);
    stopButton->setEnabled(false);

    addAndMakeVisible(progressBar = new ProgressBar(progress));

    // restore settings
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        deviceIdSlider->setValue(propertiesFile->getIntValue(T("sysexLibrarianDeviceId"), 0));
        String syxFileName(propertiesFile->getValue(T("sysexLibrarianSyxFile"), String::empty));
        if( syxFileName != String::empty )
            syxFile = File(syxFileName);
        deviceTypeSelector->setSelectedId(propertiesFile->getIntValue(T("sysexLibrarianDevice"), 1), true);
        setSpec(deviceTypeSelector->getSelectedId()-1);
    }

    setSize(300, 200);
}

SysexLibrarianControl::~SysexLibrarianControl()
{
}

//==============================================================================
void SysexLibrarianControl::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void SysexLibrarianControl::resized()
{
    int buttonX0 = 15;
    int buttonXOffset = 100;
    int buttonY = 8;
    int buttonYOffset = 32;
    int buttonWidth = 90;
    int buttonHeight = 24;

#if 0
    deviceTypeLabel->setBounds   (buttonX0 + 0*buttonXOffset, buttonY + 0*buttonYOffset, buttonWidth, buttonHeight);
    deviceTypeSelector->setBounds(buttonX0 + 1*buttonXOffset, buttonY + 0*buttonYOffset, buttonWidth, buttonHeight);
#else
    deviceTypeSelector->setBounds(buttonX0 + 0*buttonXOffset, buttonY + 0*buttonYOffset, 2*buttonWidth+10, buttonHeight);
#endif

    deviceIdLabel->setBounds (buttonX0 + 0*buttonXOffset, buttonY + 1*buttonYOffset, buttonWidth, buttonHeight);
    deviceIdSlider->setBounds(buttonX0 + 1*buttonXOffset, buttonY + 1*buttonYOffset+4, buttonWidth, buttonHeight-8);

    bankSelectLabel->setBounds (buttonX0 + 0*buttonXOffset, buttonY + 3*buttonYOffset, buttonWidth, buttonHeight);
    bankSelectSlider->setBounds(buttonX0 + 1*buttonXOffset, buttonY + 3*buttonYOffset+4, buttonWidth, buttonHeight-8);

    loadBankButton->setBounds   (buttonX0 + 0*buttonXOffset, buttonY + 4*buttonYOffset, buttonWidth, buttonHeight);
    saveBankButton->setBounds   (buttonX0 + 1*buttonXOffset, buttonY + 4*buttonYOffset, buttonWidth, buttonHeight);
    receiveBankButton->setBounds(buttonX0 + 0*buttonXOffset, buttonY + 5*buttonYOffset, buttonWidth, buttonHeight);
    sendBankButton->setBounds   (buttonX0 + 1*buttonXOffset, buttonY + 5*buttonYOffset, buttonWidth, buttonHeight);

    loadPatchButton->setBounds   (buttonX0 + 0*buttonXOffset, buttonY + 7*buttonYOffset, buttonWidth, buttonHeight);
    savePatchButton->setBounds   (buttonX0 + 1*buttonXOffset, buttonY + 7*buttonYOffset, buttonWidth, buttonHeight);
    receivePatchButton->setBounds(buttonX0 + 0*buttonXOffset, buttonY + 8*buttonYOffset, buttonWidth, buttonHeight);
    sendPatchButton->setBounds   (buttonX0 + 1*buttonXOffset, buttonY + 8*buttonYOffset, buttonWidth, buttonHeight);

    bufferLabel->setBounds (buttonX0 + 0*buttonXOffset, buttonY + 10*buttonYOffset, buttonWidth, buttonHeight);
    bufferSlider->setBounds(buttonX0 + 1*buttonXOffset, buttonY + 10*buttonYOffset+4, buttonWidth, buttonHeight-8);

    receiveBufferButton->setBounds(buttonX0 + 0*buttonXOffset, buttonY + 11*buttonYOffset, buttonWidth, buttonHeight);
    sendBufferButton->setBounds   (buttonX0 + 1*buttonXOffset, buttonY + 11*buttonYOffset, buttonWidth, buttonHeight);

    progressBar->setBounds(buttonX0, getHeight()-buttonY-buttonHeight, 2*buttonWidth-45, buttonHeight);
    stopButton->setBounds(buttonX0 + 2*buttonWidth-45+10, getHeight()-buttonY-buttonHeight, 45, buttonHeight);
}

//==============================================================================
void SysexLibrarianControl::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == stopButton ) {
        stopTransfer();
    } else if( buttonThatWasClicked == loadBankButton ||
               buttonThatWasClicked == loadPatchButton ) {
        FileChooser fc(T("Choose a .syx file that you want to open..."),
                       syxFile,
                       T("*.syx"));

        if( fc.browseForFileToOpen() ) {
            syxFile = fc.getResult();
            if( loadSyx(syxFile, buttonThatWasClicked == loadBankButton) ) {
                PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
                if( propertiesFile )
                    propertiesFile->setValue(T("sysexLibrarianSyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == saveBankButton ||
               buttonThatWasClicked == savePatchButton ) {
        FileChooser fc(T("Choose a .syx file that you want to save..."),
                       syxFile,
                       T("*.syx"));
        if( fc.browseForFileToSave(true) ) {
            syxFile = fc.getResult();
            if( saveSyx(syxFile, buttonThatWasClicked == saveBankButton) ) {
                PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
                if( propertiesFile )
                    propertiesFile->setValue(T("sysexLibrarianSyxFile"), syxFile.getFullPathName());
            }
        }
    } else if( buttonThatWasClicked == sendBankButton ||
               buttonThatWasClicked == receiveBankButton ||
               buttonThatWasClicked == sendPatchButton ||
               buttonThatWasClicked == receivePatchButton ||
               buttonThatWasClicked == sendBufferButton ||
               buttonThatWasClicked == receiveBufferButton ) {
        deviceIdSlider->setEnabled(false);
        bankSelectSlider->setEnabled(false);
        loadBankButton->setEnabled(false);
        saveBankButton->setEnabled(false);
        sendBankButton->setEnabled(false);
        receiveBankButton->setEnabled(false);
        loadPatchButton->setEnabled(false);
        savePatchButton->setEnabled(false);
        sendPatchButton->setEnabled(false);
        receivePatchButton->setEnabled(false);
        bufferSlider->setEnabled(false);
        sendBufferButton->setEnabled(false);
        receiveBufferButton->setEnabled(false);
        stopButton->setEnabled(true);

        currentPatch = (buttonThatWasClicked == sendBankButton || buttonThatWasClicked == receiveBankButton) ? 0 : sysexLibrarian->sysexLibrarianBank->getSelectedPatch();
        progress = 0;
        dumpRequested = false;
        errorResponse = false;
        checksumError = false;

        retryCtr = 0; // no retry yet

        handleSinglePatch =
            buttonThatWasClicked == sendPatchButton ||
            buttonThatWasClicked == receivePatchButton ||
            buttonThatWasClicked == sendBufferButton ||
            buttonThatWasClicked == receiveBufferButton;

        bufferTransfer =
            buttonThatWasClicked == sendBufferButton ||
            buttonThatWasClicked == receiveBufferButton;

        receiveDump =
            buttonThatWasClicked == receiveBankButton ||
            buttonThatWasClicked == receivePatchButton ||
            buttonThatWasClicked == receiveBufferButton;

        startTimer(1);
    }
}


//==============================================================================
void SysexLibrarianControl::sliderValueChanged(Slider* slider)
{
    if( slider == deviceIdSlider ) {
        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("sysexLibrarianDeviceId"), (int)slider->getValue());
    }
}


//==============================================================================
void SysexLibrarianControl::comboBoxChanged(ComboBox* comboBox)
{
    if( comboBox == deviceTypeSelector ) {
        setSpec(comboBox->getSelectedId()-1);

        // store setting
        PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("sysexLibrarianDevice"), (int)comboBox->getSelectedId());
    }
}

//==============================================================================
void SysexLibrarianControl::setSpec(const unsigned& spec)
{
    if( spec < miosStudio->sysexPatchDb->getNumSpecs() ) {
        sysexLibrarian->sysexLibrarianBank->initBank(spec);
        sysexLibrarian->sysexLibrarianAssemblyBank->initBank(spec);
        bankSelectSlider->setRange(1, miosStudio->sysexPatchDb->getNumBanks(spec), 1);

        String bufferName(miosStudio->sysexPatchDb->getBufferName(spec));
        if( bufferName.isEmpty() )
            bufferLabel->setText(bufferName, true); // empty...
        else
            bufferLabel->setText(bufferName + String(T(":")), true); // empty...

        int numBuffers = miosStudio->sysexPatchDb->getNumBuffers(spec);
        if( numBuffers ) {
            bufferSlider->setRange(1, numBuffers, 1);
            bufferSlider->setEnabled(true);
        } else {
            bufferSlider->setRange(1, 1, 1);
            bufferSlider->setEnabled(false);
        }

        // starting delay - will be increased if synth needs more time
        timerRestartDelay = 10;
    }
}

//==============================================================================

void SysexLibrarianControl::stopTransfer(void)
{
    stopTimer();

    progress = 0;
    deviceIdSlider->setEnabled(true);
    bankSelectSlider->setEnabled(true);
    loadBankButton->setEnabled(true);
    saveBankButton->setEnabled(true);
    sendBankButton->setEnabled(true);
    receiveBankButton->setEnabled(true);
    loadPatchButton->setEnabled(true);
    savePatchButton->setEnabled(true);
    sendPatchButton->setEnabled(true);
    receivePatchButton->setEnabled(true);
    bufferSlider->setEnabled(true);
    sendBufferButton->setEnabled(true);
    receiveBufferButton->setEnabled(true);
    stopButton->setEnabled(false);
}

//==============================================================================
void SysexLibrarianControl::timerCallback()
{
    stopTimer(); // will be restarted if required

    // transfer has been stopped?
    if( !stopButton->isEnabled() )
        return;

    bool transferFinished = false;

    if( receiveDump ) {
        if( dumpRequested ) {
            if( checksumError ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Detected checksum error!"),
                                            T("Check:\n- MIDI In/Out connections\n- your MIDI interface"),
                                            String::empty);
            } else if( !dumpReceived ) {
                if( ++retryCtr < 16 ) {
                    --currentPatch;
                    timerRestartDelay = 100*retryCtr; // delay increases with each retry
                } else {
                    transferFinished = true;
                    AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                                T("No response from device."),
                                                T("Check:\n- MIDI In/Out connections\n- Device ID\n- that MIDIbox firmware has been uploaded"),
                                                String::empty);
                }
            }
        }

        if( !transferFinished ) {
            int spec = deviceTypeSelector->getSelectedId()-1;
            if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() ) {
                transferFinished = true;
            } else if( (handleSinglePatch && !sysexLibrarian->sysexLibrarianBank->isSelectedPatch(currentPatch)) ||
                       (currentPatch >= miosStudio->sysexPatchDb->getNumPatchesPerBank(spec)) ) {
                transferFinished = true;
            } else {
                dumpReceived = false;
                checksumError = false;
                errorResponse = false;
                int spec = deviceTypeSelector->getSelectedId()-1;
                if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() ) {
                    dumpReceived = true;
                } else {
                    Array<uint8> data;

                    if( bufferTransfer ) {
                        uint8 bufferNum = (uint8)bufferSlider->getValue() - 1;
                        Array<uint8> selectBuffer = miosStudio->sysexPatchDb->createSelectBuffer(spec,
                                                                                                 (uint8)deviceIdSlider->getValue(),
                                                                                                 bufferNum);
                        if( selectBuffer.size() ) {
                            Array<uint8> data(selectBuffer);
                            MidiMessage message = SysexHelper::createMidiMessage(data);
                            miosStudio->sendMidiMessage(message);
                            bufferNum = 0; // don't transfer offset in this case
                        }

                        data = miosStudio->sysexPatchDb->createReadBuffer(spec,
                                                                          (uint8)deviceIdSlider->getValue(),
                                                                          bufferNum,
                                                                          (uint8)bankSelectSlider->getValue()-1,
                                                                          currentPatch);
                    } else {
                        data = miosStudio->sysexPatchDb->createReadPatch(spec,
                                                                         (uint8)deviceIdSlider->getValue(),
                                                                         0,
                                                                         (uint8)bankSelectSlider->getValue()-1,
                                                                         currentPatch);
                    }
                    MidiMessage message = SysexHelper::createMidiMessage(data);
                    miosStudio->sendMidiMessage(message);

                    dumpRequested = true;
                    ++currentPatch;

                    if( handleSinglePatch )
                        progress = 1;
                    else
                        progress = (double)currentPatch / (double)miosStudio->sysexPatchDb->getNumPatchesPerBank(spec);
                    startTimer(miosStudio->sysexPatchDb->getDelayBetweenReads(spec));
                }
            }
        }
    } else {
        int spec = deviceTypeSelector->getSelectedId()-1;
        if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() ) {
            transferFinished = true;
        } else {
            if( errorResponse ) {
                transferFinished = true;
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            T("Got Error response!"),
                                            T("Check:\n- if a valid patch has been uploaded\n- error code in MIDI IN monitor"),
                                            String::empty);
            } else {
                if( (handleSinglePatch && currentPatch != sysexLibrarian->sysexLibrarianBank->getSelectedPatch()) ||
                    currentPatch >= miosStudio->sysexPatchDb->getNumPatchesPerBank(spec) ) {
                    transferFinished = true;
                } else {
                    Array<uint8>* p = NULL;
                    while( currentPatch < sysexLibrarian->sysexLibrarianBank->getNumRows() &&
                           (p=sysexLibrarian->sysexLibrarianBank->getPatch(currentPatch)) == NULL )
                        ++currentPatch;

                    if( p == NULL ) {
                        transferFinished = true;
                    } else {
                        errorResponse = false;

                        Array<uint8> data;

                        if( bufferTransfer ) {
                            uint8 bufferNum = (uint8)bufferSlider->getValue() - 1;
                            Array<uint8> selectBuffer = miosStudio->sysexPatchDb->createSelectBuffer(spec,
                                                                                                     (uint8)deviceIdSlider->getValue(),
                                                                                                     bufferNum);
                            if( selectBuffer.size() ) {
                                Array<uint8> data(selectBuffer);
                                MidiMessage message = SysexHelper::createMidiMessage(data);
                                miosStudio->sendMidiMessage(message);
                                bufferNum = 0; // don't transfer offset in this case
                            }

                            data = miosStudio->sysexPatchDb->createWriteBuffer(spec,
                                                                              (uint8)deviceIdSlider->getValue(),
                                                                              bufferNum,
                                                                              (uint8)bankSelectSlider->getValue()-1,
                                                                              currentPatch,
                                                                              &p->getReference(0),
                                                                              p->size());
                        } else {
                            data = miosStudio->sysexPatchDb->createWritePatch(spec,
                                                                              (uint8)deviceIdSlider->getValue(),
                                                                              0,
                                                                              (uint8)bankSelectSlider->getValue()-1,
                                                                              currentPatch,
                                                                              &p->getReference(0),
                                                                              p->size());
                        }

                        MidiMessage message = SysexHelper::createMidiMessage(data);
                        miosStudio->sendMidiMessage(message);

                        ++currentPatch;

                        if( handleSinglePatch )
                            progress = 1;
                        else {
                            progress = (double)currentPatch / (double)miosStudio->sysexPatchDb->getNumPatchesPerBank(spec);
                            sysexLibrarian->sysexLibrarianBank->selectPatch(currentPatch-1);
                        }

                        startTimer(miosStudio->sysexPatchDb->getDelayBetweenWrites(spec));
                    }
                }
            }
        }
    }

    if( transferFinished ) {
        stopTransfer();
    }
}

//==============================================================================
void SysexLibrarianControl::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();

    int spec = deviceTypeSelector->getSelectedId()-1;
    if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() )
        return;

    if( receiveDump ) {
        int receivedPatch = currentPatch-1;
        uint8 bufferNum = (uint8)bufferSlider->getValue() - 1;
        Array<uint8> selectBuffer = miosStudio->sysexPatchDb->createSelectBuffer(spec,
                                                                                 (uint8)deviceIdSlider->getValue(),
                                                                                 bufferNum);
        if( selectBuffer.size() ) {
            bufferNum = 0; // offset not transfered in this case
        }

        if( (bufferTransfer && miosStudio->sysexPatchDb->isValidWriteBuffer(spec,
                                                                            data, size,
                                                                            (int)deviceIdSlider->getValue(),
                                                                            bufferNum,
                                                                            -1,
                                                                            -1))
            ||
            (!bufferTransfer && miosStudio->sysexPatchDb->isValidWritePatch(spec,
                                                                            data, size,
                                                                            (int)deviceIdSlider->getValue(),
                                                                            0,
                                                                            -1,
                                                                            -1))
            ) {

            dumpReceived = true;

            if( size != miosStudio->sysexPatchDb->getPatchSize(spec) ||
                !miosStudio->sysexPatchDb->hasValidChecksum(spec, data, size) ) {
                checksumError = true;
            } else {
                Array<uint8> payload(miosStudio->sysexPatchDb->getPayload(spec, data, size));
                sysexLibrarian->sysexLibrarianBank->setPatch(receivedPatch, payload);
                sysexLibrarian->sysexLibrarianBank->selectPatch(receivedPatch);
                // leads to endless download if a single patch is selected and received, since the handler checks for the selection...
                //sysexLibrarian->sysexLibrarianBank->incPatchIfSingleSelection();
            }

            // trigger timer immediately with variable delay (will be increased if timeouts have been notified)
            stopTimer();
            startTimer(timerRestartDelay);
        }
    } else if( isTimerRunning() ) {
        if( miosStudio->sysexPatchDb->isValidErrorAcknowledge(spec, data, size, (int)deviceIdSlider->getValue()) ) {
            // trigger timer immediately
            errorResponse = true;
            stopTimer();
            startTimer(100);
        } else if( miosStudio->sysexPatchDb->isValidAcknowledge(spec, data, size, (int)deviceIdSlider->getValue()) ) {
            // trigger timer immediately
            stopTimer();
            startTimer(100);
        }
    }
}


//==============================================================================
bool SysexLibrarianControl::loadSyx(File &syxFile, const bool& loadBank)
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
    String errorMessage;
    int spec = deviceTypeSelector->getSelectedId()-1;

    if( readNumBytes != size ) {
        errorMessage = String(T("cannot be read completely!"));
    } else if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() ) {
        errorMessage = String(T("invalid patch type selected!"));
    } else {
        int patchSize = miosStudio->sysexPatchDb->getPatchSize(spec);
        int maxPatches = miosStudio->sysexPatchDb->getNumPatchesPerBank(spec);

        if( loadBank ) {
            sysexLibrarian->sysexLibrarianBank->initBank(spec);
        }

        unsigned patch = 0;
        unsigned numPatches = 0;
        int pos = 0;
        while( errorMessage.isEmpty() &&
               (pos+patchSize) <= size ) {
            uint8* patchPtr = &buffer[pos];
            if( miosStudio->sysexPatchDb->isValidWritePatch(spec, patchPtr, patchSize, -1, -1, -1, -1) ||
                miosStudio->sysexPatchDb->isValidWriteBuffer(spec, patchPtr, patchSize, -1, -1, -1, -1) ) {

                if( !loadBank ) {
                    while( patch < maxPatches && !sysexLibrarian->sysexLibrarianBank->isSelectedPatch(patch) )
                        ++patch;
                }
                if( patch >= maxPatches )
                    break;

                Array<uint8> payload(miosStudio->sysexPatchDb->getPayload(spec, patchPtr, patchSize));
                sysexLibrarian->sysexLibrarianBank->setPatch(patch, payload);
                if( loadBank ) {
                    sysexLibrarian->sysexLibrarianBank->selectPatch(patch);
                }
                sysexLibrarian->sysexLibrarianBank->incPatchIfSingleSelection();

                ++numPatches;
                ++patch;
                pos += patchSize;
            } else {
                // search for next F0
                while( ((++pos+patchSize) <= size) && (buffer[pos] != 0xf0) );
            }
        }

        if( numPatches == 0 ) {
            errorMessage = String(T("doesn't contain any valid SysEx dump for a ") + miosStudio->sysexPatchDb->getSpecName(spec));
        } else {
            if( loadBank ) {
                sysexLibrarian->sysexLibrarianBank->selectPatch(0); // select the first patch in bank
            }
        }
    }

    juce_free(buffer);

    if( !errorMessage.isEmpty() ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("The file ") + syxFile.getFileName(),
                                    errorMessage,
                                    String::empty);
        return false;
    }

    return true;
}

bool SysexLibrarianControl::saveSyx(File &syxFile, const bool& saveBank)
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


    int spec = deviceTypeSelector->getSelectedId()-1;
    if( spec < 0 || spec >= miosStudio->sysexPatchDb->getNumSpecs() ) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    String::empty,
                                    T("Invalid patch type selected!"),
                                    String::empty);
    } else {
        int maxPatches = sysexLibrarian->sysexLibrarianBank->getNumRows();
        for(int patch=0; patch<maxPatches; ++patch) {
            if( saveBank || sysexLibrarian->sysexLibrarianBank->isSelectedPatch(patch) ) {
                Array<uint8>* p = NULL;
                if( ((p=sysexLibrarian->sysexLibrarianBank->getPatch(patch)) != NULL) && p->size() ) {
                    Array<uint8> syxDump(miosStudio->sysexPatchDb->createWritePatch(spec,
                                                                                    0,
                                                                                    0,
                                                                                    (uint8)bankSelectSlider->getValue()-1,
                                                                                    patch,
                                                                                    &p->getReference(0),
                                                                                    p->size()));
                    outFileStream->write(&syxDump.getReference(0), syxDump.size());
                }
            }
        }
    }

    delete outFileStream;

    return true;
}


//==============================================================================
//==============================================================================
//==============================================================================
SysexLibrarian::SysexLibrarian(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(sysexLibrarianAssemblyBank = new SysexLibrarianBank(miosStudio, String(T("Assembly Bank"))));
    addAndMakeVisible(sysexLibrarianBank = new SysexLibrarianBank(miosStudio, String(T("Transaction Bank"))));
    addAndMakeVisible(sysexLibrarianControl = new SysexLibrarianControl(miosStudio, this));

    addAndMakeVisible(transferBankRButton = new TextButton(T("==>")));
    transferBankRButton->addListener(this);
    transferBankRButton->setTooltip(T("Move all patches from Transaction to Assembly Bank"));

    addAndMakeVisible(transferPatchRButton = new TextButton(T("-->")));
    transferPatchRButton->addListener(this);
    transferPatchRButton->setTooltip(T("Move selected patches from Transaction to Assembly Bank"));

    addAndMakeVisible(transferPatchLButton = new TextButton(T("<--")));
    transferPatchLButton->addListener(this);
    transferPatchLButton->setTooltip(T("Move selected patches from Assembly to Transaction Bank"));

    addAndMakeVisible(transferBankLButton = new TextButton(T("<==")));
    transferBankLButton->addListener(this);
    transferBankLButton->setTooltip(T("Move all patches from Assembly to Transaction Bank"));

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(725, 500);
}

SysexLibrarian::~SysexLibrarian()
{
}

//==============================================================================
void SysexLibrarian::paint (Graphics& g)
{
    //g.fillAll(Colour(0xffc1d0ff));
    g.fillAll(Colours::white);
}

void SysexLibrarian::resized()
{
    sysexLibrarianControl->setBounds       (0, 0, 225, getHeight());
    sysexLibrarianBank->setBounds          (220, 0, 225, getHeight());
    sysexLibrarianAssemblyBank->setBounds(500, 0, 225, getHeight());

    unsigned buttonYOffset = (getHeight()-4*24)/2 - 4;

    transferBankRButton->setBounds         (445, buttonYOffset + 0*24 + 4, 45, 16);
    transferPatchRButton->setBounds        (445, buttonYOffset + 1*24 + 4, 45, 16);
    transferPatchLButton->setBounds        (445, buttonYOffset + 2*24 + 4, 45, 16);
    transferBankLButton->setBounds         (445, buttonYOffset + 3*24 + 4, 45, 16);

    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}

//==============================================================================
void SysexLibrarian::buttonClicked (Button* buttonThatWasClicked)
{
    bool transfer =
        (buttonThatWasClicked == transferBankRButton) ||
        (buttonThatWasClicked == transferPatchRButton) ||
        (buttonThatWasClicked == transferPatchLButton) ||
        (buttonThatWasClicked == transferBankLButton);

    bool transferRight =
        (buttonThatWasClicked == transferBankRButton) ||
        (buttonThatWasClicked == transferPatchRButton);

    bool transferBank =
        (buttonThatWasClicked == transferBankRButton) ||
        (buttonThatWasClicked == transferBankLButton);

    if( transfer ) {
        SysexLibrarianBank* srcBank = transferRight ? sysexLibrarianBank : sysexLibrarianAssemblyBank;
        SysexLibrarianBank* dstBank = transferRight ? sysexLibrarianAssemblyBank : sysexLibrarianBank;

        if( transferBank )
            dstBank->initBank(srcBank->getPatchSpec());

        int dstPatch = -1;
        int lastDstPatch = -1;
        int maxPatches = srcBank->getNumRows();
        for(int srcPatch=0; srcPatch<maxPatches; ++srcPatch) {
            if( transferBank || srcBank->isSelectedPatch(srcPatch) ) {
                if( transferBank ) {
                    dstPatch = srcPatch;
                } else {
                    while( ++dstPatch < maxPatches && !dstBank->isSelectedPatch(dstPatch) );
                    if( dstPatch >= maxPatches )
                        dstPatch = lastDstPatch + 1;
                }

                if( dstPatch >= maxPatches )
                    break;

                Array<uint8>* p = srcBank->getPatch(srcPatch);
                if( p != NULL ) {
                    dstBank->setPatch(dstPatch, *p);
                }
                lastDstPatch = dstPatch;
            }
        }

        if( !transferBank ) {
            srcBank->incPatchIfSingleSelection();
            dstBank->incPatchIfSingleSelection();
        }
    }
}

