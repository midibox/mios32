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
    , busy(false)
    , currentBlock(0)
    , currentErrorCode(-1)
    , totalBlocks(0)
    , ignoredBlocks(0)
    , runningStatus(0x00)
    , ongoingQueryMessage(0)
    , ongoingRebootRequest(0)
    , ongoingUpload(0)
    , deviceId(0x00)
    , retryCounter(0)
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
bool UploadHandler::rebootCore(bool afterTransfer)
{
    if( coreOperatingSystem == T("MIOS32") ) {
        ongoingRebootRequest = 1;

        Array<uint8> dataArray = SysexHelper::createMios32Query(deviceId);
        dataArray.add(0x7f); // enter BSL mode
        dataArray.add(0xf7);
        MidiMessage message = SysexHelper::createMidiMessage(dataArray);
        miosStudio->sendMidiMessage(message);
        return true;
    }

    return false;
}


//==============================================================================
bool UploadHandler::startQuery(void)
{
    return requestCoreInfo(1);
}


//==============================================================================
bool UploadHandler::startUpload(void)
{
    if( busy )
        return false;

    currentBlock = 0;
    ignoredBlocks = 0;
    totalBlocks = hexFileLoader.hexDumpAddressBlocks.size();

    busy = 1;
    currentErrorCode = -1;
    ongoingRebootRequest = 1;
    rebootCore(false);

    return true;
}


//==============================================================================
bool UploadHandler::stop(void)
{
    ongoingQueryMessage = 0;
    ongoingRebootRequest = 0;
    ongoingUpload = 0;
    busy = 0;
    return true;
}


//==============================================================================
bool UploadHandler::requestCoreInfo(uint8 queryRequest)
{
    // start with request 1
    if( queryRequest <= 1 ) {
        if( busy )
            return false;

        busy = 1;

        queryRequest = 1;
        clearCoreInfo();
    }

    ongoingQueryMessage = queryRequest;

    Array<uint8> dataArray = SysexHelper::createMios32Query(deviceId);
    dataArray.add(queryRequest);
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);

    return true;
}


//==============================================================================
void UploadHandler::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    if( data[0] >= 0x80 && data[0] < 0xf8 )
        runningStatus = data[0];

    // only parse for SysEx messages >= 7 bytes
    if( runningStatus != 0xf0 || size < 7 )
        return;


    if( ongoingQueryMessage && SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
        String *out = 0;

        switch( ongoingQueryMessage ) {
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

            // request next query
            if( ongoingQueryMessage < 0x09 )
                requestCoreInfo(ongoingQueryMessage + 1);
            else {
                ongoingQueryMessage = 0;
                busy = 0;
            }
        }
    }


    if( ongoingRebootRequest && SysexHelper::isValidMios32UploadRequest(data, size, deviceId) ) {
        ongoingRebootRequest = 0;

        //uploadStatus->insertTextAtCursor(T("Got upload request from core!\n"));
        retryCounter = 0;
        ongoingUpload = 1;
        sendBlock(0);
    }


    if( ongoingUpload ) {
        if( SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
            retryCounter = 0;
            sendBlock(currentBlock+1);
        } else if( SysexHelper::isValidMios32Error(data, size, deviceId) ) {
            currentErrorCode = data[7]; // data[7] contains error code
            ++retryCounter;
            sendBlock(currentBlock);
        }
    }
}


//==============================================================================
void UploadHandler::sendBlock(const uint32 &block)
{
    currentBlock = block;    
    if( block == 0 ) {
        ignoredBlocks = 0;
        totalBlocks = hexFileLoader.hexDumpAddressBlocks.size();
    }


    if( retryCounter >= 16 ) {
        stop();
    } else if( currentBlock >= totalBlocks ) {
        // upload finished - reboot core again
        rebootCore(true);
        stop();
    } else {
        // upload block
        bool forMios32 = coreOperatingSystem == T("MIOS32");
        uint32 blockAddress;

        if( forMios32 ) {
            while( (blockAddress=hexFileLoader.hexDumpAddressBlocks[currentBlock]) >= 0x08000000 &&
                   blockAddress < 0x08004000 ) {
                ++currentBlock; // skip bootloader range
                ++ignoredBlocks;
            }
        }

        if( currentBlock >= hexFileLoader.hexDumpAddressBlocks.size() ) {
            // no valid block - upload finished
            stop();
        } else {
            miosStudio->sendMidiMessage(hexFileLoader.createMidiMessageForBlock(deviceId, blockAddress, forMios32));
        }
    }
}
