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


class MiosStudio; // forward declaration

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
    bool startQuery(void);
    bool startUpload(void);
    bool stop(void);

    //==============================================================================
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);

    //==============================================================================
    HexFileLoader hexFileLoader;

    uint8 deviceId;

    bool busy;
    uint32 currentBlock;
    uint32 totalBlocks;
    uint32 ignoredBlocks;
    int currentErrorCode;

protected:
    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    bool requestCoreInfo(uint8 queryRequest);
    bool rebootCore(bool afterTransfer);

    void sendBlock(const uint32 &block);

    //==============================================================================
    uint8 runningStatus;

    uint8 ongoingQueryMessage;
    bool ongoingUpload;
    bool ongoingRebootRequest;

    uint32 retryCounter;

};

#endif /* _UPLOAD_HANDLER_H */
