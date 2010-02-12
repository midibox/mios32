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
    , excludedBlocks(0)
    , runningStatus(0x00)
    , deviceId(0x00)
    , recoveredErrorsCounter(0)
    , timeUploadBegin(0)
    , timeUploadEnd(0)
{
    clearCoreInfo();
}

UploadHandler::~UploadHandler()
{
}


//==============================================================================
void UploadHandler::clearCoreInfo(void)
{
    coreOperatingSystem = String::empty;
    coreBoard = String::empty;
    coreFamily = String::empty;
    coreChipId = String::empty;
    coreSerialNumber = String::empty;
    coreFlashSize = String::empty;
    coreRamSize = String::empty;
    coreAppHeader1 = String::empty;
    coreAppHeader2 = String::empty;
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
    timeUploadBegin = 0;
    timeUploadEnd = 0;
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
bool UploadHandler::checkAndDisplayRanges(LogBox* logbox)
{
    bool checksOk = true;

    uint32 currentBlockAddress = hexFileLoader.hexDumpAddressBlocks[0];
    uint32 currentBlockStartAddress = currentBlockAddress;
    for(int i=1; i<miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks.size(); ++i) {
        uint32 blockAddress = miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks[i];

        if( blockAddress != (currentBlockAddress + 0x100) ) {
            if( !checkAndDisplaySingleRange(logbox, currentBlockStartAddress, currentBlockAddress+0xff) )
                checksOk = false;
            currentBlockStartAddress = blockAddress;
        }
        currentBlockAddress = blockAddress;
    }

    if( !checkAndDisplaySingleRange(logbox, currentBlockStartAddress, currentBlockAddress+0xff) )
        checksOk = false;

    return checksOk;
}


bool UploadHandler::checkAndDisplaySingleRange(LogBox* logbox, uint32 startAddress, uint32 endAddress)
{
    bool checkOk = false;
    String rangeName;

    if( startAddress <= hexFileLoader.HEX_RANGE_MIOS8_BL_END ) {
        rangeName = T("PIC Bootloader (ERROR!)");
        checkOk = false;
    } else if( startAddress >= hexFileLoader.HEX_RANGE_MIOS8_FLASH_START &&
        endAddress <= hexFileLoader.HEX_RANGE_MIOS8_FLASH_END ) {

        if( startAddress <= hexFileLoader.HEX_RANGE_MIOS8_OS_END ) {
            logbox->addEntry(String::formatted(T("Range 0x%08x-0x%08x (%u bytes) - MIOS8 area"),
                                               hexFileLoader.HEX_RANGE_MIOS8_OS_START,
                                               hexFileLoader.HEX_RANGE_MIOS8_OS_END,
                                               hexFileLoader.HEX_RANGE_MIOS8_OS_END-hexFileLoader.HEX_RANGE_MIOS8_OS_START+1));
            startAddress = hexFileLoader.HEX_RANGE_MIOS8_OS_END + 1;
        }

        rangeName = T("PIC Flash");
        checkOk = true;
    } else if( startAddress >= hexFileLoader.HEX_RANGE_MIOS8_EEPROM_START &&
               endAddress <= hexFileLoader.HEX_RANGE_MIOS8_EEPROM_END ) {
        rangeName = T("PIC EEPROM");
        checkOk = true;
    } else if( startAddress >= hexFileLoader.HEX_RANGE_MIOS8_BANKSTICK_START &&
               endAddress <= hexFileLoader.HEX_RANGE_MIOS8_BANKSTICK_END ) {
        rangeName = T("PIC BankStick");
        checkOk = true;
    } else if( startAddress >= hexFileLoader.HEX_RANGE_MIOS32_STM32_FLASH_START &&
               endAddress <= hexFileLoader.HEX_RANGE_MIOS32_STM32_FLASH_END ) {

        if( startAddress <= hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_END ) {
            logbox->addEntry(String::formatted(T("Range 0x%08x-0x%08x (%u bytes) - BL excluded"),
                                               hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_START,
                                               hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_END,
                                               hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_END-hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_START+1));
            startAddress = hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_END + 1;
        }

        rangeName = T("STM32 Flash");
        checkOk = true;
    } else {
        rangeName = T("(unknown)");
        checkOk = false;
    }

    logbox->addEntry(String::formatted(T("Range 0x%08x-0x%08x (%u bytes) - "),
                                       startAddress,
                                       endAddress,
                                       endAddress-startAddress + 1) + rangeName);

    return checkOk;
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

    // upload request is always detected
    if( SysexHelper::isValidMios32UploadRequest(data, size, deviceId) ) {
        uploadHandlerThread->mios32RebootRequest = 0;
        uploadHandlerThread->detectedMios32UploadRequest = 1;
    }

    if( SysexHelper::isValidMios8UploadRequest(data, size, deviceId) ) {
        uploadHandlerThread->detectedMios8UploadRequest = 1;
    }



    // query request?
    if( uploadHandlerThread->mios8QueryRequest || uploadHandlerThread->mios32QueryRequest ) {

        if( SysexHelper::isValidMios8DebugMessage(data, size, deviceId) ) {
            uploadHandlerThread->detectedMios8FeedbackLoop = 1;
            printf("Mios8 Feedback\n");
        } else if( uploadHandlerThread->mios8QueryRequest && SysexHelper::isValidMios8Acknowledge(data, size, deviceId) ) {
            uploadHandlerThread->mios8QueryRequest = 0;
        } else if( SysexHelper::isValidMios32Query(data, size, deviceId) ) {
            uploadHandlerThread->detectedMios32FeedbackLoop = 1;
            printf("Mios32 Feedback\n");
        } else if( uploadHandlerThread->mios32QueryRequest && SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
            String *out = 0;

            switch( uploadHandlerThread->mios32QueryRequest ) {
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
                *out = String::empty;
                for(int i=7; i<size; ++i)
                    if( data[i] != 0xf7 && data[i] != '\n' || size < (i+1) )
                        *out += String::formatted(T("%c"), data[i] & 0x7f);
            }

            uploadHandlerThread->mios32QueryRequest = 0;
        }

        uploadHandlerThread->notify(); // wakeup run() thread

    }

    // acknowledge on write block initiated by MIOS Studio?
    if( uploadHandlerThread->mios32UploadRequest ) {
        if( SysexHelper::isValidMios32Acknowledge(data, size, deviceId) ) {
            uploadHandlerThread->mios32UploadRequest = 0;
            uploadHandlerThread->detectedMios32UploadRequest = 1;
            uploadHandlerThread->notify(); // wakeup run() thread
        } else if( SysexHelper::isValidMios32Error(data, size, deviceId) ) {
            uploadHandlerThread->mios32UploadRequest = 0;
            uploadHandlerThread->detectedMios32UploadRequest = 0;
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
    , detectedMios8FeedbackLoop(0)
    , detectedMios32FeedbackLoop(0)
    , detectedMios8UploadRequest(0)
    , detectedMios32UploadRequest(0)
    , mios8QueryRequest(0)
    , mios32QueryRequest(0)
    , mios32RebootRequest(0)
    , uploadErrorCode(-1)
{
    // update status variables of caller
    uploadHandler->excludedBlocks = 0;
    uploadHandler->totalBlocks = uploadHandler->hexFileLoader.hexDumpAddressBlocks.size();
    uploadHandler->currentBlock = 0;
    uploadHandler->recoveredErrorsCounter = 0;

    startThread(8); // start thread with pretty high priority (1..10)
}


UploadHandlerThread::~UploadHandlerThread()
{
    stopThread(2000); // give it a chance for 2 seconds
}


void UploadHandlerThread::sendMios32RebootCore()
{
    Array<uint8> dataArray = SysexHelper::createMios32Query(uploadHandler->deviceId);
    dataArray.add(0x7f); // enter BL mode
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


void UploadHandlerThread::sendMios8Query(void)
{
    // initiate a RAM read to query for MIOS8 core
    Array<uint8> dataArray = SysexHelper::createMios8DebugMessage(uploadHandler->deviceId);
    dataArray.add(0x02); // Read RAM
    dataArray.add(0x00); // AU (RAM address = 0x000)
    dataArray.add(0x00); // AH ..
    dataArray.add(0x00); // AL ..
    dataArray.add(0x00); // WH  (Counter = 0x0001)
    dataArray.add(0x00); // WL  ..
    dataArray.add(0x00); // M1H ..
    dataArray.add(0x01); // M1L ..
    dataArray.add(0x00); // M2H
    dataArray.add(0x00); // M2L
    dataArray.add(0x00); // M3H
    dataArray.add(0x00); // M3L
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


void UploadHandlerThread::sendMios8InvalidBlock(void)
{
    // try to upload some bytes to invalid address and with invalid checksum
    uint32 address = 0x00000000;
    uint32 size = 8;
    uint8 checksum = 0x00;
    Array<uint8> dataArray = SysexHelper::createMios8WriteBlock(uploadHandler->deviceId, address, size, checksum);
    // note: the number of bytes is important!
    for(int i=0; i<11; ++i)
        dataArray.add(0x10 + i);
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}

void UploadHandlerThread::sendMios32Query(uint8 query)
{
    Array<uint8> dataArray = SysexHelper::createMios32Query(uploadHandler->deviceId);
    dataArray.add(query);
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


void UploadHandlerThread::run()
{
    // Core Detection Procedure
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    //
    // MIOS32: send query requests and check for incoming replies which contain useful informations
    //         this is the perfect solution (based on MIOS8 experiences and thanks to the bigger Flash size :)
    //
    // MIOS8: unfortunately the 1st level bootloader is very primitive, it doesn't provide a proper
    // way to request a reply without causing a core reboot - which would hurt if the status is
    // requested on a running application. MIOS8 also doesn't provide query strings to request
    // properties like PIC derivative, Flash and RAM size...
    //
    // Therefore we are using following approach to detect the cores:
    // 1) send a MIOS32 query request and a MIOS8 debug RAM read and check for an incoming reply
    // 1a) if exactly the same commands are received back, we can savely assume a hardware loopback,
    //     abort the transfer, and notify the user.
    // 1b) if either a MIOS32 or MIOS8 message feedback loop is detected, assume a software
    //     loopback (e.g. MIDI merger enabled) - ignore and continue
    // 1c) if a MIOS32 acknowledge is received, select this core for the whole thread
    // 1d) if a MIOS8 acknowledge is received, select this core for the whole thread
    // 1e) if a MIOS8 as well as a MIOS32 acknowledge is received, notify the user about this
    //     issue (could happen if PIC is connected to MIOS32 MIDI In/Out - provide a selection
    //     method in the GUI later)
    // 2) if no response is received after 100 mS, send an invalid MIOS8 upload block request
    //    (block with 9 bytes, wrong checksum to prohibited address 0x0000)
    // 2a) if a MIOS8 upload request is received again, select MIOS8 for the whole thread


    // variables changed during detection procedure
    bool forMios32 = false; // (false: MIOS8, true: MIOS32)
    bool viaBootloader = false; // (MIOS8: will be detected, MIOS32: always via bootloader)
    bool tryMios8Bootloader = false; // if step 1) fails


    //////////////////////////////////////////////////////////////////////////////////////
    // Step 1) first try - see comments above
    //////////////////////////////////////////////////////////////////////////////////////
    sendMios32Query(mios32QueryRequest = 1);
    mios8QueryRequest = 1;
    sendMios8Query();

    // wait for wakeup from handleIncomingMidiMessage() - timeout after 100 mS
    // we wait for up to 20*10 mS (for the case that multiple replies are received)
    for(int i=0; i<20; ++i)
        wait(10);

    if( detectedMios32FeedbackLoop && detectedMios8FeedbackLoop ) {
        errorStatusMessage = "Detected a feedback loop!";
        return;
    } else if( mios8QueryRequest == 0 && mios32QueryRequest == 0 ) {
        errorStatusMessage = "Detected MIOS8 and MIOS32 response - selection not supported yet!";
        return;
    } else if( mios32QueryRequest == 0 ) {
        forMios32 = true;
        viaBootloader = true;
    } else if( mios8QueryRequest == 0 ) {
        forMios32 = false;
        viaBootloader = false;
    } else {
        // errorStatusMessage = "No response from MIOS8 or MIOS32 core!";
        // return;
        tryMios8Bootloader = true;
    }


    //////////////////////////////////////////////////////////////////////////////////////
    // Step 2) check for MIOS8 bootloader
    //////////////////////////////////////////////////////////////////////////////////////
    if( tryMios8Bootloader ) {
        detectedMios8UploadRequest = 0;
        sendMios8InvalidBlock();
        
        // wait for wakeup from handleIncomingMidiMessage() - timeout after 100 mS
        // we wait for up to 20*10 mS (for the case that multiple replies are received)
        for(int i=0; i<20; ++i)
            wait(10);

        if( detectedMios32FeedbackLoop && detectedMios8FeedbackLoop ) {
            errorStatusMessage = "Detected a feedback loop!";
            return;
        } else if( detectedMios8UploadRequest ) {
            forMios32 = false;
            viaBootloader = true;
        } else {
            errorStatusMessage = "No response from MIOS8 or MIOS32 core!";
            return;
        }
    }


    //////////////////////////////////////////////////////////////////////////////////////
    // MIOS32: query core parameters 1..9
    // PIC: best guess... ;)
    //////////////////////////////////////////////////////////////////////////////////////
    if( forMios32 ) {
        for(int query=1; query<=9; ++query) {

            if( threadShouldExit() )
                return;

            // send query request
            sendMios32Query(mios32QueryRequest = query);
        
            // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
            wait(1000);

            // check we got a reply (request cleared)
            if( mios32QueryRequest ) {
                errorStatusMessage = "Timeout on query request.";
                return;
            }
        }
    } else {
        uploadHandler->coreOperatingSystem = "MIOS8";
        uploadHandler->coreBoard = "MBHP_CORE or similar";
        uploadHandler->coreFamily = "PIC18F";
        uploadHandler->coreChipId = String::empty;
        uploadHandler->coreSerialNumber = String::empty;
        uploadHandler->coreFlashSize = String::empty;
        uploadHandler->coreRamSize = String::empty;
        if( viaBootloader )
            uploadHandler->coreAppHeader1 = "Bootloader is up & running!";
        else
            uploadHandler->coreAppHeader1 = "Application is up & running!";
        uploadHandler->coreAppHeader2 = String::empty;
    }


    //////////////////////////////////////////////////////////////////////////////////////
    // exit if only a query was requested
    //////////////////////////////////////////////////////////////////////////////////////
    if( queryOnly )
        return;

    if( forMios32 && uploadHandler->coreOperatingSystem != T("MIOS32") ) {
        errorStatusMessage = "Got MIOS32 response, but operating system has a different name?";
        return;
    }

    if( threadShouldExit() )
        return;


    // no query requests anymore...
    mios8QueryRequest = 0;
    mios32QueryRequest = 0;


    //////////////////////////////////////////////////////////////////////////////////////
    // Check if hex file is qualified for detected core
    //////////////////////////////////////////////////////////////////////////////////////
    if( forMios32 ) {
        // TODO: check for flash size

        if( uploadHandler->hexFileLoader.disqualifiedForMios32_STM32 ) {
            errorStatusMessage = "Hex file contains invalid ranges for MIOS32!";
            return;
        }

        if( !uploadHandler->hexFileLoader.qualifiedForMios32_STM32 ) {
            errorStatusMessage = "Hex file doesn't contain a valid MIOS32 range!";
            return;
        }
    } else {
        if( uploadHandler->hexFileLoader.disqualifiedForMios8 ) {
            errorStatusMessage = "Hex file contains invalid ranges for MIOS8!";
            return;
        }

        if( !uploadHandler->hexFileLoader.qualifiedForMios8 ) {
            errorStatusMessage = "Hex file doesn't contain a valid MIOS8 range!";
            return;
        }
    }


    // exit if MIOS8... not supported yet
    if( !forMios32 ) {
        errorStatusMessage = "MIOS8 upload not supported yet!";
        return;
    }
        
    //////////////////////////////////////////////////////////////////////////////////////
    // MIOS32: always reboot the core to enter BL mode
    //////////////////////////////////////////////////////////////////////////////////////
    if( forMios32 ) {
        mios32RebootRequest = 1;
        sendMios32RebootCore();

        // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
        wait(1000);

        if( mios32RebootRequest ) {
            errorStatusMessage = "Bootloader Mode cannot be entered - try again?";
            return;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // upload code blocks
    //////////////////////////////////////////////////////////////////////////////////////
    uploadHandler->timeUploadBegin = Time::getCurrentTime().toMilliseconds();

    for(int block=0; block<uploadHandler->totalBlocks; ++block) {
        uploadHandler->currentBlock = block;

        if( threadShouldExit() )
            return;

        uint32 blockAddress = uploadHandler->hexFileLoader.hexDumpAddressBlocks[block];
        if( forMios32 ) {
            // TODO: check for STM32
            if( blockAddress >= uploadHandler->hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_START &&
                blockAddress <= uploadHandler->hexFileLoader.HEX_RANGE_MIOS32_STM32_BL_END ) {
                ++uploadHandler->excludedBlocks;
                continue; // skip bootloader range
            }
        }

        int maxRetries = 16;
        int retry = 0;        
        do {
            uploadErrorCode = -1;
            mios32UploadRequest = 1;
            miosStudio->sendMidiMessage(uploadHandler->hexFileLoader.createMidiMessageForBlock(uploadHandler->deviceId, blockAddress, forMios32));

            // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
            wait(1000);

            if( uploadErrorCode >= 0 )
                ++uploadHandler->recoveredErrorsCounter; // counter is only relevant if the procedure passes

        } while( (mios32UploadRequest || uploadErrorCode >= 0) && ++retry < maxRetries );

        // got error acknowledge? (note: up to 16 retries on error acknowledge)
        if( uploadErrorCode >= 0 ) {
            errorStatusMessage += "Upload aborted due to error #" + String(uploadErrorCode) + ": ";
            errorStatusMessage += SysexHelper::decodeMiosErrorCode(uploadErrorCode);
        }

        // and/or timeout? Add this to message (note: up to 16 retries on timeouts)
        if( mios32UploadRequest ) {
            errorStatusMessage += "No response from core after " + String(maxRetries) + " retries!";
        }

        if( errorStatusMessage != String::empty )
            return;
    }

	// take over last block (for progress bar - it will flicker now)
	uploadHandler->currentBlock = uploadHandler->totalBlocks;

    // for statistics
    uploadHandler->timeUploadEnd = Time::getCurrentTime().toMilliseconds();
	

    //////////////////////////////////////////////////////////////////////////////////////
    // MIOS32: reboot again
    //////////////////////////////////////////////////////////////////////////////////////
    if( forMios32 ) {
        mios32RebootRequest = 1;
        sendMios32RebootCore();

        // wait for wakeup from handleIncomingMidiMessage() - timeout after 1 second
        wait(1000);

        if( mios32RebootRequest ) {
            errorStatusMessage = "No response from core after reboot";
            return;
        }
    }

}
