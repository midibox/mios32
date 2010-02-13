/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Upload Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _UPLOAD_HANDLER_H
#define _UPLOAD_HANDLER_H

#include "includes.h"
#include "HexFileLoader.h"
#include "SysexHelper.h"
#include "LogBox.h"


class MiosStudio; // forward declaration
class UploadHandler; // forward declaration


class UploadHandlerThread
    : public Thread
{
public:
    UploadHandlerThread(MiosStudio *_miosStudio, UploadHandler *_uploadHandler, bool _queryOnly);
    ~UploadHandlerThread();

    void run();


    MiosStudio *miosStudio;
    UploadHandler *uploadHandler;

    bool queryOnly;

    // error status message from run() thread
    String errorStatusMessage;

    uint8 deviceId; // taken over from upload handler to ensure that upload always accesses the firstly selected one

    volatile bool detectedMios8FeedbackLoop;
    volatile bool detectedMios32FeedbackLoop;

    volatile bool detectedMios8UploadRequest;
    volatile bool detectedMios32UploadRequest;

    volatile bool mios8QueryRequest;
    volatile int mios32QueryRequest;

    volatile bool mios8RebootRequest;
    volatile bool mios32RebootRequest;

    volatile bool mios8UploadRequest;
    volatile bool mios32UploadRequest;

    volatile int uploadErrorCode;

protected:
    void sendMios8Query(void);
    void sendMios32Query(uint8 query);
    void sendMios8InvalidBlock(void);
    void sendMios8RebootCore(void);
    void sendMios32RebootCore(void);

};


class UploadHandler
    : public MidiInputCallback
{
public:
    UploadHandler(MiosStudio *_miosStudio);
    ~UploadHandler();

    //==============================================================================
    String coreOperatingSystem;
    String coreBoard;
    String coreFamily;
    String coreChipId;
    String coreSerialNumber;
    String coreFlashSize;
    String coreRamSize;
    String coreAppHeader1;
    String coreAppHeader2;

    void clearCoreInfo(void);

    //==============================================================================
    bool busy(void);
    bool startQuery(void);
    bool startUpload(void);

    // returns error message or String::empty if thread passed
    // must always be called before startQuery() or startUpload() is called again
    String finish(void);

    bool checkAndDisplayRanges(LogBox* logbox);
    bool checkAndDisplaySingleRange(LogBox* logbox, uint32 startAddress, uint32 endAddress);

    //==============================================================================
    uint8 getDeviceId();
    void setDeviceId(uint8 id);

    //==============================================================================
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);

    //==============================================================================
    HexFileLoader hexFileLoader;

    uint32 currentBlock;
    uint32 totalBlocks;
    uint32 excludedBlocks;
    int currentErrorCode;
    int recoveredErrorsCounter;

    float timeUpload;

protected:
    //==============================================================================
    MiosStudio *miosStudio;

    UploadHandlerThread *uploadHandlerThread;

    uint8 deviceId;

    //==============================================================================
    uint8 runningStatus;

};

#endif /* _UPLOAD_HANDLER_H */
