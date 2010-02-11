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

#include "UploadHandler.h"
#include "MiosStudio.h"

#include <list>


//==============================================================================
UploadHandler::UploadHandler(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , uploadHandlerThread(0)
    , currentBlock(0)
    , currentErrorCode(-1)
    , totalBlocks(0)
    , ignoredBlocks(0)
    , runningStatus(0x00)
    , deviceId(0x00)
{
    clearCoreInfo();
}

UploadHandler::~UploadHandler()
{
}


//==============================================================================
void UploadHandler::clearCoreInfo(void)
{
    coreOperatingSystem = "";
    coreBoard = "";
    coreFamily = "";
    coreChipId = "";
    coreSerialNumber = "";
    coreFlashSize = "";
    coreRamSize = "";
    coreAppHeader1 = "";
    coreAppHeader2 = "";
}


//==============================================================================

//==============================================================================
bool UploadHandler::busy(void)
{
    return uploadHandlerThread && uploadHandlerThread->isThreadRunning();
}


//==============================================================================
bool UploadHandler::startQuery(void)
{
    if( busy() )
        return false;

    clearCoreInfo();
    uploadHandlerThread = new UploadHandlerThread(miosStudio, this, true); // queryOnly

    return true;
}


//==============================================================================
bool UploadHandler::startUpload(void)
{
    if( busy() )
        return false;

    clearCoreInfo();
    uploadHandlerThread = new UploadHandlerThread(miosStudio, this, false); // !queryOnly

    return true;
}


//==============================================================================
// returns error message or String::empty if thread passed
// must always be called before startQuery() or startUpload() is called again
String UploadHandler::finish(void)
{
    if( uploadHandlerThread ) {
        String errorStatusMessage = uploadHandlerThread->errorStatusMessage;
        deleteAndZero(uploadHandlerThread);
        return errorStatusMessage;
    }

    return String::empty;
}


//==============================================================================
void UploadHandler::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // exit if thread is not running
    if( uploadHandlerThread == 0 )
        return;

    // start parsing
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    if( data[0] >= 0x80 && data[0] < 0xf8 )
        runningStatus = data[0];

    // only parse for SysEx messages >= 7 bytes
    if( runningStatus != 0xf0 || size < 7 )
        return;


    if( uploadHandlerThread->queryRequest && SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
        String *out = 0;
 
        switch( uploadHandlerThread->queryRequest ) {
        case 0x01: out = &coreOperatingSystem; break;
        case 0x02: out = &coreBoard; break;
        case 0x03: out = &coreFamily; break;
        case 0x04: out = &coreChipId; break;
        case 0x05: out = &coreSerialNumber; break;
        case 0x06: out = &coreFlashSize; break;
        case 0x07: out = &coreRamSize; break;
        case 0x08: out = &coreAppHeader1; break;
        case 0x09: out = &coreAppHeader2; break;
        }
 
        if( out ) {
            for(int i=7; i<size; ++i)
                if( data[i] != 0xf7 && data[i] != '\n' || size < (i+1) )
                    *out += String::formatted(T("%c"), data[i] & 0x7f);
        }

        uploadHandlerThread->queryRequest = 0;
        uploadHandlerThread->notify(); // wakeup run() thread

    } else if( uploadHandlerThread->mios32RebootRequest && SysexHelper::isValidMios32UploadRequest(data, size, deviceId) ) {
        uploadHandlerThread->mios32RebootRequest = 0;


    } else if( uploadHandlerThread->uploadRequest ) {
        if( SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
            uploadHandlerThread->uploadRequest = 0;
            uploadHandlerThread->notify(); // wakeup run() thread
        } else if( SysexHelper::isValidMios32Error(data, size, deviceId) ) {
            uploadHandlerThread->uploadRequest = 0;
            uploadHandlerThread->uploadErrorCode = data[7]; // data[7] contains error code
            uploadHandlerThread->notify(); // wakeup run() thread
        }
    }
}


//==============================================================================
//==============================================================================
//==============================================================================
UploadHandlerThread::UploadHandlerThread(MiosStudio *_miosStudio, UploadHandler *_uploadHandler, bool _queryOnly)
    : Thread("UploadHandlerThread")
    , miosStudio(_miosStudio)
    , uploadHandler(_uploadHandler)
    , queryOnly(_queryOnly)
    , queryRequest(0)
    , mios32RebootRequest(0)
    , uploadRequest(0)
    , uploadErrorCode(0)
{
    startThread(8); // start thread with pretty high priority (1..10)
}


UploadHandlerThread::~UploadHandlerThread()
{
    stopThread(2000); // give it a chance for 2 seconds
}


void UploadHandlerThread::sendRebootCore(void)
{
    Array<uint8> dataArray = SysexHelper::createMios32Query(uploadHandler->deviceId);
    dataArray.add(0x7f); // enter BSL mode
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


void UploadHandlerThread::sendQuery(uint8 query)
{
    Array<uint8> dataArray = SysexHelper::createMios32Query(uploadHandler->deviceId);
    dataArray.add(query);
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


void UploadHandlerThread::run()
{
    // update status variables of caller
    uploadHandler->ignoredBlocks = 0;
    uploadHandler->totalBlocks = uploadHandler->hexFileLoader.hexDumpAddressBlocks.size();
    uploadHandler->currentBlock = 0;

    // clear error message
    errorStatusMessage = String::empty;

    // query core parameters 1..9
    // TODO: for MIOS8 we have to find out some core parameters on a different way
    for(int query=1; query<=9; ++query) {

        if( threadShouldExit() )
            return;

        // send query request
        queryRequest = query;
        sendQuery(query);
        
        // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
        wait(1000);

        // check we got a reply (request cleared)
        if( queryRequest ) {
            errorStatusMessage = "Timeout on query request.";
            return;
        }
    }

    // exit if only a query was requested
    if( queryOnly )
        return;

    if( threadShouldExit() )
        return;

    bool forMios32 = uploadHandler->coreOperatingSystem == T("MIOS32");

    // MIOS32: always reboot the core to enter BSL mode
    if( forMios32 ) {
        mios32RebootRequest = 1;
        sendRebootCore();

        // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
        wait(1000);

        if( mios32RebootRequest ) {
            errorStatusMessage = "Bootloader Mode cannot be entered - try again?";
            return;
        }
    }

    // upload code blocks
    for(int block=0; block<uploadHandler->totalBlocks; ++block) {
        uploadHandler->currentBlock = block;

        if( threadShouldExit() )
            return;

        uint32 blockAddress;
        if( forMios32 ) {
            // TODO: check for STM32
            if( (blockAddress=uploadHandler->hexFileLoader.hexDumpAddressBlocks[block]) >= 0x08000000 &&
                   blockAddress < 0x08004000 ) {
                ++uploadHandler->ignoredBlocks;
                continue; // skip bootloader range
            }
        }

        int maxRetries = 16;
        int retry = 0;
        do {
            uploadRequest = 1;
            miosStudio->sendMidiMessage(uploadHandler->hexFileLoader.createMidiMessageForBlock(uploadHandler->deviceId, blockAddress, forMios32));

            // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
            wait(1000);

        } while( (uploadRequest || uploadErrorCode) && ++retry < maxRetries );

        // got error acknowledge? (note: up to 16 retries on error acknowledge)
        if( uploadErrorCode ) {
            errorStatusMessage += "Upload aborted due to error #" + String(uploadErrorCode) + ": ";
            errorStatusMessage += SysexHelper::decodeMiosErrorCode(uploadErrorCode);
        }

        // and/or timeout? Add this to message (note: up to 16 retries on timeouts)
        if( uploadRequest ) {
            errorStatusMessage += "No response from core after " + String(maxRetries) + " retries!";
        }

        if( errorStatusMessage != String::empty )
            return;
    }


    // MIOS32: reboot again, don't check for success
    if( forMios32 ) {
        mios32RebootRequest = 1;
        sendRebootCore();

        // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
        wait(1000);

        if( mios32RebootRequest ) {
            errorStatusMessage = "No response from core after reboot";
            return;
        }
    }

}
