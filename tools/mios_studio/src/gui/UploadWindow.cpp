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

#include "UploadWindow.h"
#include "MiosStudio.h"

#include <list>


#define TIMER_LOAD_HEXFILE             1
#define TIMER_LOAD_HEXFILE_AND_UPLOAD  2
#define TIMER_UPLOAD                   3
#define TIMER_UPLOAD_TIMEOUT           4

#define CORE_TYPE_INVALID     0
#define CORE_TYPE_MIOS8       1
#define CORE_TYPE_MIOS32      2


//==============================================================================
UploadWindow::UploadWindow(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , gotFirstMessage(0)
    , ongoingQueryMessage(0)
    , ongoingUpload(0)
    , ongoingUploadRequest(0)
    , ongoingRebootRequest(0)
    , deviceId(0x00)
    , currentCoreType(CORE_TYPE_INVALID)
    , currentUploadBlock(0)
    , retryCounter(0)
    , timeUploadBegin(0)
{
	addAndMakeVisible(fileChooser = new FilenameComponent (T("hexfile"),
                                                           File::nonexistent,
                                                           true, false, false,
                                                           "*.hex",
                                                           String::empty,
                                                           T("(choose a .hex file to upload)")));
	fileChooser->addListener(this);
	fileChooser->setBrowseButtonText(T("Browse"));

    addAndMakeVisible(startButton = new TextButton(T("Start Button")));
    startButton->setButtonText(T("Start"));
    startButton->addButtonListener(this);
    startButton->setEnabled(false);

    addAndMakeVisible(stopButton = new TextButton(T("Stop Button")));
    stopButton->setButtonText(T("Stop"));
    stopButton->addButtonListener(this);
    stopButton->setEnabled(false);

    addAndMakeVisible(queryButton = new TextButton(T("Query Button")));
    queryButton->setButtonText(T("Query"));
    queryButton->addButtonListener(this);

    addAndMakeVisible(uploadStatus = new TextEditor(T("Upload Status")));
    uploadStatus->setMultiLine(true);
    uploadStatus->setReturnKeyStartsNewLine(true);
    uploadStatus->setReadOnly(true);
    uploadStatus->setScrollbarsShown(true);
    uploadStatus->setCaretVisible(true);
    uploadStatus->setPopupMenuEnabled(false);
    uploadStatus->setText(T("Upload Status ready."));

    setSize(400, 200);
}

UploadWindow::~UploadWindow()
{
    deleteAndZero(fileChooser);
    deleteAndZero(uploadStatus);
    deleteAndZero(startButton);
    deleteAndZero(stopButton);
    deleteAndZero(queryButton);
}

//==============================================================================
void UploadWindow::paint (Graphics& g)
{
    g.fillAll (Colours::white);
}

void UploadWindow::resized()
{
    fileChooser->setBounds(4, 4, getWidth()-8, 24);
    startButton->setBounds(4, 4+30+0*36, 72, 24);
    stopButton->setBounds (4, 4+30+1*36, 72, 24);
    queryButton->setBounds(4, 4+30+2*36, 72, 24);
    uploadStatus->setBounds(4+72+4, 4+30, getWidth() - (4+72+4+4), getHeight()- (4+24+4+10));
}

void UploadWindow::buttonClicked (Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == startButton ) {
        File inFile = fileChooser->getCurrentFile();
        uploadStatus->clear();
        uploadStatus->insertTextAtCursor(String::formatted(T("Reading %s\n"), (const char *)inFile.getFileName()));
        MultiTimer::startTimer(TIMER_LOAD_HEXFILE_AND_UPLOAD, 1);
        startButton->setEnabled(false); // will be enabled again if file is valid
    }
    else if( buttonThatWasClicked == stopButton ) {
        uploadStop();
        uploadStatus->insertTextAtCursor(T("Upload has been stopped by user!\n"));
    }
    else if( buttonThatWasClicked == queryButton ) {
        queryCore(1);
    }
}

void UploadWindow::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
    if( fileComponentThatHasChanged == fileChooser ) {
        File inFile = fileChooser->getCurrentFile();

        uploadStatus->clear();
        uploadStatus->insertTextAtCursor(String::formatted(T("Reading %s\n"), (const char *)inFile.getFileName()));
        MultiTimer::startTimer(TIMER_LOAD_HEXFILE, 1);
        startButton->setEnabled(false); // will be enabled if file is valid
    }
}


//==============================================================================
void UploadWindow::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    int messageOffset = 0;

    bool isUploadRequest = false;
    bool isAcknowledge = false;
    bool isError = false;

    if( runningStatus == 0xf0 ) {
        isUploadRequest = SysexHelper::isValidMios32UploadRequest(data, size, deviceId);
        isAcknowledge = SysexHelper::isValidMios32Acknowledge(data, size, deviceId);
        isError = SysexHelper::isValidMios32Error(data, size, deviceId);
        if( isUploadRequest || isAcknowledge || isError ) {
            messageOffset = 7;
        }
    }

    if( ongoingRebootRequest && isUploadRequest ) {
        ongoingRebootRequest = 0;
        if( ongoingUploadRequest ) {
            uploadStatus->insertTextAtCursor(T("Got upload request from core!\n"));
            uploadStartNow();
        }
    }

    if( ongoingUpload ) {
        if( isAcknowledge )
            uploadNext();
        else if( isError )
            uploadRetry(data[7]); // data[7] contains error code
    }

    if( ongoingQueryMessage && isAcknowledge ) {
        String out = gotFirstMessage ? "\n" : "Got reply from MIOS32 core:\n";

        switch( ongoingQueryMessage ) {
        case 0x01: out += "Operating System: "; currentCoreType = CORE_TYPE_MIOS32; break; // TODO: check if really MIOS32
        case 0x02: out += "Board: "; break;
        case 0x03: out += "Core Family: "; break; // TODO: use for memory range checks
        case 0x04: out += "Chip ID: 0x"; break;
        case 0x05: out += "Serial Number: #"; break;
        case 0x06: out += "Flash Memory Size: "; break; // TODO: use for memory range checks
        case 0x07: out += "RAM Size: "; break;
        case 0x08: break; // Application first line
        case 0x09: break; // Application second line
        default: {
            String decString = String(ongoingQueryMessage);
            out += "Query #" + decString + ": ";
        }
        }

        int size = message.getRawDataSize();

        for(int i=messageOffset; i<size; ++i)
            if( data[i] != 0xf7 && data[i] != '\n' || size < (i+1) )
                out += String::formatted(T("%c"), data[i] & 0x7f);

        // request next query
        if( ongoingQueryMessage < 0x09 )
            queryCore(ongoingQueryMessage + 1);
        else
            ongoingQueryMessage = 0;

        if( !gotFirstMessage )
            uploadStatus->clear();
        uploadStatus->insertTextAtCursor(out);
        gotFirstMessage = 1;
    }
}


//==============================================================================
void UploadWindow::midiPortChanged(void)
{
    uploadStatus->clear();
    uploadStatus->setText(T("No response from a core yet... check MIDI IN/OUT  connections!"));
    queryCore(1);
}

//==============================================================================
void UploadWindow::queryCore(int queryRequest)
{
    if( queryRequest == 1 )
        gotFirstMessage = 0; // display will be cleared once first message received

    ongoingQueryMessage = queryRequest;

    Array<uint8> dataArray = SysexHelper::createMios32Query(deviceId);
    dataArray.add(queryRequest);
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
}


//==============================================================================
void UploadWindow::rebootCore(void)
{
    if( currentCoreType == CORE_TYPE_MIOS32 ) {
        ongoingRebootRequest = 1;

        Array<uint8> dataArray = SysexHelper::createMios32Query(deviceId);
        dataArray.add(0x7f); // enter BSL mode
        dataArray.add(0xf7);
        MidiMessage message = SysexHelper::createMidiMessage(dataArray);
        miosStudio->sendMidiMessage(message);
    }
}

//==============================================================================
void UploadWindow::uploadStart(void)
{
    if( currentCoreType != CORE_TYPE_MIOS32 ) {
        uploadStatus->insertTextAtCursor(T("Code upload currently only supported for MIOS32!\n"));
    } else {
        uploadStatus->insertTextAtCursor(T("Sending reboot request to enter Bootloader Mode.\n"));
        ongoingUploadRequest = 1;
        rebootCore();
    }
}

void UploadWindow::uploadStartNow(void)
{
    // called after reboot request
    // start upload of first block
    ongoingUploadRequest = 0;
    ongoingUpload = 1;
    currentUploadBlock = 0;
    retryCounter = 0;
    MultiTimer::startTimer(TIMER_UPLOAD, 1);
    stopButton->setEnabled(true);

    Time currentTime = Time::getCurrentTime();
    timeUploadBegin = currentTime.toMilliseconds();
}

void UploadWindow::uploadStop(void)
{
    ongoingUpload = 0;
    stopButton->setEnabled(false);
}

void UploadWindow::uploadNext(void)
{
    MultiTimer::stopTimer(TIMER_UPLOAD_TIMEOUT);
    ++currentUploadBlock;
    retryCounter = 0;
    MultiTimer::startTimer(TIMER_UPLOAD, 1);
}

void UploadWindow::uploadRetry(int errorAcknowledge)
{
    const int numMaxRetries = 16;

    if( errorAcknowledge < 0 ) { // was timeout
        if( ++retryCounter >= numMaxRetries ) {
            uploadStatus->insertTextAtCursor(String::formatted(T("No response from core after %d retries!\n"),
                                                               numMaxRetries));
            uploadStatus->insertTextAtCursor(T("Upload aborted!\n"));
            uploadStop();
        } else {
            uploadStatus->insertTextAtCursor(String::formatted(T("No response from core - retry #%d!\n"),
                                                               retryCounter));
            MultiTimer::startTimer(TIMER_UPLOAD, 1);
        }
    } else { // was Error
        MultiTimer::stopTimer(TIMER_UPLOAD_TIMEOUT);

        if( ++retryCounter >= numMaxRetries ) {
            uploadStatus->insertTextAtCursor(String::formatted(T("Got error acknowledge (0x%02x) after %d retries!\n"),
                                                               errorAcknowledge, numMaxRetries));
            uploadStatus->insertTextAtCursor(T("Upload aborted!\n"));
            uploadStop();
        } else {
            uploadStatus->insertTextAtCursor(String::formatted(T("Error acknowledge (0x%02x) from core - retry #%d!\n"),
                                                               errorAcknowledge, retryCounter));
            MultiTimer::startTimer(TIMER_UPLOAD, 1);
        }
    }
}


//==============================================================================
void UploadWindow::timerCallback(const int timerId)
{
    // always stop timer, it has to be restarted if required
    MultiTimer::stopTimer(timerId);

    // check for requested service
    if( timerId == TIMER_LOAD_HEXFILE || timerId == TIMER_LOAD_HEXFILE_AND_UPLOAD ) {
        File inFile = fileChooser->getCurrentFile();
        String statusMessage;
        if( hexFileLoader.loadFile(inFile, statusMessage) ) {
            uploadStatus->insertTextAtCursor(statusMessage + T("\n"));

            if( hexFileLoader.hexDumpAddressBlocks.size() < 1 ) {
                uploadStatus->insertTextAtCursor(T("ERROR: no blocks found\n"));
            } else {
                // display and check ranges
                uint32 currentBlockAddress = hexFileLoader.hexDumpAddressBlocks[0];
                uint32 currentBlockStartAddress = currentBlockAddress;
                for(int i=1; i<hexFileLoader.hexDumpAddressBlocks.size(); ++i) {
                    uint32 blockAddress = hexFileLoader.hexDumpAddressBlocks[i];

                    if( blockAddress != (currentBlockAddress + 0x100) ) {
                        uploadStatus->insertTextAtCursor(String::formatted(T("Range 0x%08x-0x%08x (%u bytes)\n"),
                                                                           currentBlockStartAddress,
                                                                           currentBlockAddress+0xff,
                                                                           (currentBlockAddress-currentBlockStartAddress)+0x100));
                        currentBlockStartAddress = blockAddress;
                    }
                    currentBlockAddress = blockAddress;
                }

                uploadStatus->insertTextAtCursor(String::formatted(T("Range 0x%08x-0x%08x (%u bytes)\n"),
                                                                   currentBlockStartAddress,
                                                                   currentBlockAddress+0xff,
                                                                   (currentBlockAddress-currentBlockStartAddress)+0x100));


                uploadStatus->insertTextAtCursor(T("TODO: check for invalid ranges!\n"));

                startButton->setEnabled(true);
                if( timerId == TIMER_LOAD_HEXFILE_AND_UPLOAD )
                    uploadStart();
                else
                    uploadStatus->insertTextAtCursor(T("Press start button to begin upload.\n"));
            }
        } else {
            uploadStatus->insertTextAtCursor(T("ERROR: ") + statusMessage + T("\n"));
        }
    } else if( timerId == TIMER_UPLOAD ) {
        uint32 numBlocks = hexFileLoader.hexDumpAddressBlocks.size();
        if( currentUploadBlock >= numBlocks ) {
            Time currentTime = Time::getCurrentTime();
            int64 timeUploadEnd = currentTime.toMilliseconds();
            float timeUpload = (float) (timeUploadEnd - timeUploadBegin) / 1000;
            float transferRateKb = ((numBlocks * 256) / timeUpload) / 1024;
            uploadStatus->insertTextAtCursor(String::formatted(T("Upload of %d bytes completed after %3.2fs (%3.2f kb/s)\n"),
                                                               numBlocks*256,
                                                               timeUpload,
                                                               transferRateKb));
            uploadStop();
            rebootCore();
        } else {
            uint32 blockAddress;

            if( currentCoreType == CORE_TYPE_MIOS32 ) {
                bool bslRangeSkipped = false;
                while( (blockAddress=hexFileLoader.hexDumpAddressBlocks[currentUploadBlock]) >= 0x08000000 &&
                       blockAddress < 0x08004000 ) {
                    ++currentUploadBlock; // skip bootloader range
                    bslRangeSkipped = true;
                }

                if( bslRangeSkipped ) {
                    uploadStatus->insertTextAtCursor(T("Skipped bootloader range 0x08000000..0x08003fff\n"));
                }
            }

            if( currentUploadBlock >= hexFileLoader.hexDumpAddressBlocks.size() ) {
                uploadStatus->insertTextAtCursor(String::formatted(T("No valid block - upload finished.\n")));
                uploadStop();
            } else {
                uploadStatus->insertTextAtCursor(String::formatted(T("Sending block #%d 0x%08x-0x%08x\n"),
                                                                   currentUploadBlock+1,
                                                                   blockAddress,
                                                                   blockAddress+0xff));

                bool forMios32 = currentCoreType == CORE_TYPE_MIOS32;
                miosStudio->sendMidiMessage(hexFileLoader.createMidiMessageForBlock(deviceId, blockAddress, forMios32));

                // timeout counter will be stopped by acknowledge handler
                // this handler will also request the next block upload
                MultiTimer::startTimer(TIMER_UPLOAD_TIMEOUT, 1000);
            }
        }
    } else if( timerId == TIMER_UPLOAD_TIMEOUT ) {
        if( ongoingUpload )
            uploadRetry(-1); // due to timeout...
    }
}
