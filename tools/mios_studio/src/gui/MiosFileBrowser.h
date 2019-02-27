/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * File browser for MIOS32 applications
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS_FILE_BROWSER_H
#define _MIOS_FILE_BROWSER_H

#include "../includes.h"
#include "../SysexHelper.h"
#include "HexTextEditor.h"

class MiosStudio; // forward declaration
class MiosFileBrowserItem;
class MiosFileBrowserFileItem;


//==============================================================================
//==============================================================================
//==============================================================================
class MiosFileBrowser
    : public Component
    , public Button::Listener
    , public TextEditor::Listener
    , public Timer
{
public:
    //==============================================================================
    MiosFileBrowser(MiosStudio *_miosStudio);
    ~MiosFileBrowser();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    //==============================================================================
    void setStatus(const String& str);

    //==============================================================================
    void buttonClicked(Button* buttonThatWasClicked);

    //==============================================================================
    void textEditorTextChanged(TextEditor &editor);
    void textEditorReturnKeyPressed(TextEditor &editor);
    void textEditorEscapeKeyPressed(TextEditor &editor);
    void textEditorFocusLost(TextEditor &editor);

    //==============================================================================
    String getSelectedPath(void);

    //==============================================================================
    void disableFileButtons(void);
    void enableFileButtons(void);
    void enableDirButtons(void);
    void enableEditorButtons(void);

    //==============================================================================
    String convertToString(const Array<uint8>& data, bool& hasBinaryData);
    bool updateEditors(const Array<uint8>& data);
    void openTextEditor(const Array<uint8>& data);
    void openHexEditor(const Array<uint8>& data);

    //==============================================================================
    void requestUpdateTreeView(void);
    void updateTreeView(bool accessPossible);

    void treeItemClicked(MiosFileBrowserItem* item);
    void treeItemDoubleClicked(MiosFileBrowserItem* item);

    //==============================================================================
    bool downloadFileSelection(unsigned selection);
    bool downloadFinished(void);

    //==============================================================================
    bool deleteFileSelection(unsigned selection);
    bool deleteFinished(void);

    //==============================================================================
    bool createDir(void);
    bool createDirFinished(void);

    //==============================================================================
    bool createFile(void);

    //==============================================================================
    bool uploadFile(String filename = String::empty);
    bool uploadBuffer(String filename, const Array<uint8>& buffer);
    bool uploadFinished(void);

    //==============================================================================
    void timerCallback();

    //==============================================================================
    void sendCommand(const String& command);
    void receiveCommand(const String& command);

    //==============================================================================
    bool uploadFileInProgress(void);
    bool uploadFileFromExternal(const String& filename);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    Label*  editLabel;
    Label*  statusLabel;
    Button* updateButton;
    Button* uploadButton;
    Button* downloadButton;
    Button* editTextButton;
    Button* editHexButton;
    Button* createDirButton;
    Button* createFileButton;
    Button* removeButton;

    Button* cancelButton;
    Button* saveButton;

    TreeView* treeView;
    TreeViewItem* rootItem;

    MiosFileBrowserFileItem* rootFileItem;

    String       currentDirPath;
    MiosFileBrowserFileItem*        currentDirItem;
    Array<MiosFileBrowserFileItem*> currentDirFetchItems;
    XmlElement*  currentDirOpenStates;

    unsigned     transferSelectionCtr;
    bool         openTextEditorAfterRead;
    bool         openHexEditorAfterRead;

    bool         currentReadInProgress;
    bool         currentReadError;
    TreeViewItem* currentReadFileBrowserItem;
    File         currentReadFile;
    FileOutputStream *currentReadFileStream;
    String       currentReadFileName;
    unsigned     currentReadSize;
    Array<uint8> currentReadData;
    uint32       currentReadStartTime;

    bool         currentWriteInProgress;
    bool         currentWriteError;
    String       currentWriteFileName;
    unsigned     currentWriteSize;
    Array<uint8> currentWriteData;
    unsigned     currentWriteFirstBlockOffset;
    unsigned     currentWriteBlockCtr;
    uint32       currentWriteStartTime;

    unsigned     writeBlockCtrDefault;
    unsigned     writeBlockSizeDefault;

    HexTextEditor* hexEditor;
    TextEditor*    textEditor;

    //==============================================================================
    MiosStudio *miosStudio;
};


//==============================================================================
//==============================================================================
//==============================================================================
class MiosFileBrowserWindow
    : public DocumentWindow
{
public:
    MiosFileBrowserWindow(MiosStudio *_miosStudio)
        : DocumentWindow(T("MIOS File Browser"), Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        miosFileBrowser = new MiosFileBrowser(_miosStudio);
        setContentComponent(miosFileBrowser, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~MiosFileBrowserWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    MiosFileBrowser *miosFileBrowser;

    //==============================================================================
    bool uploadFileInProgress(void)
    {
        return miosFileBrowser->uploadFileInProgress();
    }

    bool uploadFileFromExternal(const String& filename)
    {
        return miosFileBrowser->uploadFileFromExternal(filename);
    }


    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
#if 0
        if( isVisible() )
            miosFileBrowser->handleIncomingMidiMessage(message, runningStatus);
#else
        // (also in batch mode)
        miosFileBrowser->handleIncomingMidiMessage(message, runningStatus);
#endif
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _MIOS_FILE_BROWSER_H */
