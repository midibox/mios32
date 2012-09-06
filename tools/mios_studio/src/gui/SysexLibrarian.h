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

#ifndef _SYSEX_LIBRARIAN_H
#define _SYSEX_LIBRARIAN_H

#include "../includes.h"
#include "ConfigTableComponents.h"

class MiosStudio; // forward declaration
class SysexLibrarian;
class SysexPatchDb;


//==============================================================================
//==============================================================================
//==============================================================================
class SysexLibrarianBank
    : public Component
    , public TableListBoxModel
    , public ConfigTableController
    , public ButtonListener
{
public:
    //==============================================================================
    SysexLibrarianBank(MiosStudio *_miosStudio, const String& bankName);
    ~SysexLibrarianBank();

    int getNumRows();
    void setNumRows(const int& rows);

    void initBank(const unsigned& patchSpec);
    unsigned getPatchSpec(void);

    unsigned getSelectedPatch(void);
    bool isSelectedPatch(const unsigned& patch);
    void selectPatch(const unsigned& patch);

    void paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected);
    void paintCell(Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected);
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate);

    void resized();

    //==============================================================================
    int getTableValue(const int rowNumber, const int columnId);
    void setTableValue(const int rowNumber, const int columnId, const int newValue);
    String getTableString(const int rowNumber, const int columnId);
    void setTableString(const int rowNumber, const int columnId, const String newString);

    //==============================================================================
    Array<uint8>* getPatch(const uint8& patch);
    void setPatch(const uint8& patch, const Array<uint8> &payload);

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);

protected:
    //==============================================================================
    Label* bankHeaderLabel;
    Button* moveDownButton;
    Button* moveUpButton;
    Button* insertButton;
    Button* deleteButton;
    Button* clearButton;
    TableListBox* table;

    unsigned patchSpec;
    StringArray patchName;
    OwnedArray<Array<uint8> > patchStorage;

    Font font;

    int numRows;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexLibrarianControl
    : public Component
    , public ButtonListener
    , public SliderListener
    , public ComboBoxListener
    , public Timer
{
public:
    //==============================================================================
    SysexLibrarianControl(MiosStudio *_miosStudio, SysexLibrarian *_sysexLibrarian);
    ~SysexLibrarianControl();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged(Slider* slider);
    void comboBoxChanged(ComboBox*);

    //==============================================================================
    void setSpec(const unsigned& spec);

    //==============================================================================
    void stopTransfer(void);
    void timerCallback();

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

    //==============================================================================
    bool loadSyx(File &syxFile, const bool& loadBank);
    bool saveSyx(File &syxFile, const bool& saveBank);

protected:
    //==============================================================================
    Label*      deviceTypeLabel;
    ComboBox*   deviceTypeSelector;

    Label*      deviceIdLabel;
    Slider*     deviceIdSlider;

    Label*      bankSelectLabel;
    Slider*     bankSelectSlider;

    Button* loadBankButton;
    Button* saveBankButton;
    Button* sendBankButton;
    Button* receiveBankButton;

    Button* loadPatchButton;
    Button* savePatchButton;
    Button* sendPatchButton;
    Button* receivePatchButton;

    Label*      bufferLabel;
    Slider*     bufferSlider;

    Button* sendBufferButton;
    Button* receiveBufferButton;

    Button* stopButton;
    ProgressBar* progressBar;

    //==============================================================================
    SysexLibrarian *sysexLibrarian;

    //==============================================================================
    File syxFile;
    int currentPatch;
    bool handleSinglePatch;
    bool bufferTransfer;
    bool receiveDump;
    bool dumpReceived;
    bool checksumError;
    bool errorResponse;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    double progress;
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexLibrarian
    : public Component
    , public ButtonListener
{
public:
    //==============================================================================
    SysexLibrarian(MiosStudio *_miosStudio);
    ~SysexLibrarian();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    SysexLibrarianControl* sysexLibrarianControl;
    SysexLibrarianBank* sysexLibrarianBank;
    SysexLibrarianBank* sysexLibrarianAssemblingBank;

    //==============================================================================
    void buttonClicked (Button* buttonThatWasClicked);

protected:
    //==============================================================================
    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    TextButton *transferBankRButton;
    TextButton *transferPatchRButton;
    TextButton *transferPatchLButton;
    TextButton *transferBankLButton;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class SysexLibrarianWindow
    : public DocumentWindow
{
public:
    SysexLibrarianWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("SysEx Librarian"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        sysexLibrarian = new SysexLibrarian(_miosStudio);
        setContentComponent(sysexLibrarian, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~SysexLibrarianWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    SysexLibrarian *sysexLibrarian;

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            sysexLibrarian->sysexLibrarianControl->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _SYSEX_LIBRARIAN_H */
