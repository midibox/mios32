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

class MiosStudio; // forward declaration
class MiosFileBrowserItem;
class MiosFileBrowserFileItem;


//==============================================================================
//==============================================================================
//==============================================================================
class MiosFileBrowser
    : public Component
    , public ButtonListener
    , public FilenameComponentListener
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
    void buttonClicked (Button* buttonThatWasClicked);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

    //==============================================================================
    String getSelectedPath(void);

    //==============================================================================
    void disableFileButtons(void);
    void enableFileButtons(void);
    void enableDirButtons(void);

    //==============================================================================
    void requestUpdateTreeView(void);
    void updateTreeView(bool accessPossible);

    void treeItemClicked(MiosFileBrowserItem* item);
    void treeItemDoubleClicked(MiosFileBrowserItem* item);

    //==============================================================================
    bool storeDownloadedFile(void);
    bool uploadFile(void);

    //==============================================================================
    void timerCallback();

    //==============================================================================
    void sendCommand(const String& command);
    void receiveCommand(const String& command);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

protected:
    //==============================================================================
    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    Label*  statusLabel;
    Button* updateButton;
    Button* uploadButton;
    Button* downloadButton;
    Button* editTextButton;
    Button* editHexButton;
    Button* createDirButton;
    Button* removeButton;

    TreeView* treeView;
    TreeViewItem* rootItem;

    MiosFileBrowserFileItem* rootFileItem;

    String       currentDirPath;
    MiosFileBrowserFileItem*        currentDirItem;
    Array<MiosFileBrowserFileItem*> currentDirFetchItems;
    XmlElement*  currentDirOpenStates;

    bool         currentReadInProgress;
    bool         currentReadError;
    String       currentReadFile;
    unsigned     currentReadSize;
    Array<uint8> currentReadData;
    uint32       currentReadStartTime;

    bool         currentWriteInProgress;
    bool         currentWriteError;
    String       currentWriteFile;
    unsigned     currentWriteSize;
    Array<uint8> currentWriteData;
    unsigned     currentWriteFirstBlockOffset;
    unsigned     currentWriteBlockCtr;
    uint32       currentWriteStartTime;

    unsigned     writeBlockCtrDefault;
    unsigned     writeBlockSizeDefault;

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
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
    {
        if( isVisible() )
            miosFileBrowser->handleIncomingMidiMessage(message, runningStatus);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _MIOS_FILE_BROWSER_H */
