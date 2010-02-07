/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Upload Window Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _UPLOAD_WINDOW_H
#define _UPLOAD_WINDOW_H

#include "../includes.h"
#include "../HexFileLoader.h"
#include "../SysexHelper.h"

class MiosStudio; // forward declaration

class UploadWindow
    : public Component
    , public ButtonListener
    , public FilenameComponentListener
    , public MultiTimer
{
public:
    //==============================================================================
    UploadWindow(MiosStudio *_miosStudio);
    ~UploadWindow();

    //==============================================================================
    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);

    //==============================================================================
    void handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus);

    void midiPortChanged(void);
    void queryCore(int queryRequest);
    void rebootCore(void);

    //==============================================================================
    void uploadStart(void);
    void uploadStartNow(void);
    void uploadStop(void);
    void uploadNext(void);
    void uploadRetry(int errorAcknowledge);

    //==============================================================================
    void timerCallback(const int timerId);


protected:
    //==============================================================================
    FilenameComponent* fileChooser;
    TextEditor* uploadStatus;
    TextButton* startButton;
    TextButton* stopButton;
    TextButton* queryButton;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    HexFileLoader hexFileLoader;

    //==============================================================================
    uint8 deviceId;
    bool gotFirstMessage;
    int ongoingQueryMessage;
    bool ongoingUpload;
    bool ongoingUploadRequest;
    bool ongoingRebootRequest;

    int currentCoreType;
    uint32 currentUploadBlock;
    uint32 retryCounter;

    int64 timeUploadBegin;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    UploadWindow (const UploadWindow&);
    const UploadWindow& operator= (const UploadWindow&);
};

#endif /* _UPLOAD_WINDOW_H */
