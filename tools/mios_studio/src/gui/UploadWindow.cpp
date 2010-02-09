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
#define TIMER_DELAYED_PROGRESS_OFF     4
#define TIMER_QUERY_CORE               5


//==============================================================================
UploadWindow::UploadWindow(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , queryRetryCounter(0)
    , timeUploadBegin(0)
    , progress(0)
{
	addAndMakeVisible(fileChooser = new FilenameComponent (T("hexfile"),
                                                           File::nonexistent,
                                                           true, false, false,
                                                           "*.hex",
                                                           String::empty,
                                                           T("(choose a .hex file to upload)")));
	fileChooser->addListener(this);
	fileChooser->setBrowseButtonText(T("Browse"));

    addAndMakeVisible(queryButton = new TextButton(T("Query Button")));
    queryButton->setButtonText(T("Query"));
    queryButton->addButtonListener(this);

    addAndMakeVisible(deviceIdSlider = new Slider (T("Device ID")));
    deviceIdSlider->setRange(0, 127, 1);
    deviceIdSlider->setSliderStyle(Slider::IncDecButtons);
    deviceIdSlider->setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    deviceIdSlider->addListener (this);


    addAndMakeVisible(uploadQuery = new TextEditor(T("Upload Query")));
    uploadQuery->setMultiLine(true);
    uploadQuery->setReturnKeyStartsNewLine(true);
    uploadQuery->setReadOnly(true);
    uploadQuery->setScrollbarsShown(true);
    uploadQuery->setCaretVisible(true);
    uploadQuery->setPopupMenuEnabled(false);
    uploadQuery->setText(T("Query Status ready."));

    addAndMakeVisible(uploadStatus = new TextEditor(T("Upload Status")));
    uploadStatus->setMultiLine(true);
    uploadStatus->setReturnKeyStartsNewLine(true);
    uploadStatus->setReadOnly(true);
    uploadStatus->setScrollbarsShown(true);
    uploadStatus->setCaretVisible(true);
    uploadStatus->setPopupMenuEnabled(false);
    uploadStatus->setText(T("Upload Status ready."));


    addAndMakeVisible(startButton = new TextButton(T("Start Button")));
    startButton->setButtonText(T("Start"));
    startButton->addButtonListener(this);
    startButton->setEnabled(false);

    addAndMakeVisible(stopButton = new TextButton(T("Stop Button")));
    stopButton->setButtonText(T("Stop"));
    stopButton->addButtonListener(this);
    stopButton->setEnabled(false);


    addAndMakeVisible(progressBar = new ProgressBar(progress));

    setSize(400, 200);
}

UploadWindow::~UploadWindow()
{
    deleteAndZero(fileChooser);
    deleteAndZero(queryButton);
    deleteAndZero(deviceIdSlider);
    deleteAndZero(uploadStatus);
    deleteAndZero(uploadQuery);
    deleteAndZero(startButton);
    deleteAndZero(stopButton);
}

//==============================================================================
void UploadWindow::paint (Graphics& g)
{
    g.fillAll (Colours::white);
}

void UploadWindow::resized()
{
    fileChooser->setBounds(4, 4, getWidth()-8, 24);

    int buttonY = 4+40;
    int buttonWidth = 72;
    deviceIdSlider->setBounds(4, buttonY+0*36, buttonWidth, 40);
    queryButton->setBounds(4, buttonY+1*36+10, buttonWidth, 24);

    int uploadQueryX = 4+buttonWidth+4;
    int uploadQueryWidth = getWidth()/2 - (buttonWidth-4-4-10-10) - 50;
    uploadQuery->setBounds(uploadQueryX, 4+30, uploadQueryWidth, getHeight()-(4+24+4+10));

    int uploadStatusX = uploadQueryX + uploadQueryWidth + 10;
    int uploadStatusY = 4+30;
    int uploadStatusWidth = getWidth() - uploadStatusX - 4 - buttonWidth - 10;
    int uploadStatusHeight = getHeight()-(4+24+4+10+24+4);
    uploadStatus->setBounds(uploadStatusX, uploadStatusY, uploadStatusWidth, uploadStatusHeight);
    progressBar->setBounds(uploadStatusX, uploadStatusY+4+uploadStatusHeight, uploadStatusWidth, 24);

    int startStopButtonX = uploadStatusX + uploadStatusWidth + 10;
    startButton->setBounds(startStopButtonX, buttonY+0*36, buttonWidth, 24);
    stopButton->setBounds (startStopButtonX, buttonY+1*36, buttonWidth, 24);
}

void UploadWindow::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == startButton ) {
        File inFile = fileChooser->getCurrentFile();
        uploadStatus->clear();
        uploadStatus->insertTextAtCursor(T("Reading ") + inFile.getFileName() + T("\n"));
        uploadStart();
    }
    else if( buttonThatWasClicked == stopButton ) {
        uploadStop();
        uploadStatus->insertTextAtCursor(T("Upload has been stopped by user!\n"));
    }
    else if( buttonThatWasClicked == queryButton ) {
        queryCore();
    }
}

void UploadWindow::sliderValueChanged(Slider* slider)
{
    if( slider == deviceIdSlider ) {
        uploadStop();
        miosStudio->uploadHandler->deviceId = (uint8)slider->getValue();
    }
}

void UploadWindow::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
    if( fileComponentThatHasChanged == fileChooser ) {
        File inFile = fileChooser->getCurrentFile();

        uploadStatus->clear();
        uploadStatus->insertTextAtCursor(T("Reading ") + inFile.getFileName() + T("\n"));
        MultiTimer::startTimer(TIMER_LOAD_HEXFILE, 1);
        startButton->setEnabled(false); // will be enabled if file is valid
    }
}


//==============================================================================
void UploadWindow::midiPortChanged(void)
{
    miosStudio->uploadHandler->stop();
    stopButton->setEnabled(false);
    queryButton->setEnabled(true);
    deviceIdSlider->setEnabled(true);

    uploadQuery->clear();
    uploadStatus->clear();
    uploadStatus->setText(T("No response from a core yet... check MIDI IN/OUT  connections!"));
    queryCore();
}

//==============================================================================
void UploadWindow::queryCore(void)
{
    if( !miosStudio->uploadHandler->startQuery() ) {
        uploadQuery->setText(T("Please try again later - ongoing transactions!\n"));
    } else {
        queryButton->setEnabled(false);
        deviceIdSlider->setEnabled(false);
        queryRetryCounter = 0;
        MultiTimer::startTimer(TIMER_QUERY_CORE, 10);
    }
}


//==============================================================================
void UploadWindow::uploadStart(void)
{
    progress = 0;

    if( miosStudio->uploadHandler->coreOperatingSystem != T("MIOS32") ) {
        uploadStatus->insertTextAtCursor(T("Code upload currently only supported for MIOS32!\n"));
    } else {
        startButton->setEnabled(false); // will be enabled again if file is valid
        queryButton->setEnabled(false);
        deviceIdSlider->setEnabled(false);
        uploadStatus->insertTextAtCursor(T("Sending reboot request to enter Bootloader Mode.\n"));
        MultiTimer::startTimer(TIMER_LOAD_HEXFILE_AND_UPLOAD, 1);
    }
}


void UploadWindow::uploadStop(void)
{
    miosStudio->uploadHandler->stop();
    startButton->setEnabled(miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks.size() >= 1);
    stopButton->setEnabled(false);
    queryButton->setEnabled(true);
    deviceIdSlider->setEnabled(true);
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
        if( miosStudio->uploadHandler->hexFileLoader.loadFile(inFile, statusMessage) ) {
            uploadStatus->insertTextAtCursor(statusMessage + T("\n"));

            if( miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks.size() < 1 ) {
                uploadStatus->insertTextAtCursor(T("ERROR: no blocks found\n"));
            } else {
                // display and check ranges
                uint32 currentBlockAddress = miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks[0];
                uint32 currentBlockStartAddress = currentBlockAddress;
                for(int i=1; i<miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks.size(); ++i) {
                    uint32 blockAddress = miosStudio->uploadHandler->hexFileLoader.hexDumpAddressBlocks[i];

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

                if( timerId == TIMER_LOAD_HEXFILE_AND_UPLOAD ) {
                    // start upload of first block
                    stopButton->setEnabled(true);
                    queryButton->setEnabled(false);
                    deviceIdSlider->setEnabled(false);
                    miosStudio->uploadHandler->startUpload();
                    MultiTimer::startTimer(TIMER_UPLOAD, 10);

                    Time currentTime = Time::getCurrentTime();
                    timeUploadBegin = currentTime.toMilliseconds();
                } else {
                    uploadStatus->insertTextAtCursor(T("Press start button to begin upload.\n"));
                    startButton->setEnabled(true);
                }
            }
        } else {
            uploadStatus->insertTextAtCursor(T("ERROR: ") + statusMessage + T("\n"));
        }
    } else if( timerId == TIMER_UPLOAD ) {
        progress = (double)miosStudio->uploadHandler->currentBlock / (double)miosStudio->uploadHandler->totalBlocks;
        if( miosStudio->uploadHandler->busy ) {
            MultiTimer::startTimer(TIMER_UPLOAD, 10);
        } else {
            uint32 totalBlocks = miosStudio->uploadHandler->totalBlocks - miosStudio->uploadHandler->ignoredBlocks;
            Time currentTime = Time::getCurrentTime();
            int64 timeUploadEnd = currentTime.toMilliseconds();
            float timeUpload = (float) (timeUploadEnd - timeUploadBegin) / 1000;
            float transferRateKb = ((totalBlocks * 256) / timeUpload) / 1024;
            uploadStatus->insertTextAtCursor(String::formatted(T("Upload of %d bytes completed after %3.2fs (%3.2f kb/s)\n"),
                                                               totalBlocks*256,
                                                               timeUpload,
                                                               transferRateKb));
            uploadStop();
            MultiTimer::startTimer(TIMER_DELAYED_PROGRESS_OFF, 2000);
        }
    } else if( timerId == TIMER_DELAYED_PROGRESS_OFF ) {
        if( !miosStudio->uploadHandler->busy ) // only if loader still unbusy
            progress = 0;
    } else if( timerId == TIMER_QUERY_CORE ) {
        if( miosStudio->uploadHandler->busy ) {
            if( ++queryRetryCounter >= 100 ) {
                uploadStop();
                uploadQuery->clear();
                uploadQuery->insertTextAtCursor(T("No reply from core on queries!\n"));
            } else {
                // 100*10 = 1 second should be sufficient to retrieve all queries
                MultiTimer::startTimer(TIMER_QUERY_CORE, 10);
            }
        } else {
            uploadStop();
            String str;
            uploadStatus->clear();
            uploadQuery->clear();
            if( !(str=miosStudio->uploadHandler->coreOperatingSystem).isEmpty() )
                uploadQuery->insertTextAtCursor("\nOperating Systen: " + str);
            if( !(str=miosStudio->uploadHandler->coreBoard).isEmpty() )
                uploadQuery->insertTextAtCursor("\nBoard: " + str);
            if( !(str=miosStudio->uploadHandler->coreFamily).isEmpty() )
                uploadQuery->insertTextAtCursor("\nCore Family: " + str);
            if( !(str=miosStudio->uploadHandler->coreChipId).isEmpty() )
                uploadQuery->insertTextAtCursor("\nChip ID: 0x" + str);
            if( !(str=miosStudio->uploadHandler->coreSerialNumber).isEmpty() )
                uploadQuery->insertTextAtCursor("\nSerial Number: #" + str);
            if( !(str=miosStudio->uploadHandler->coreFlashSize).isEmpty() )
                uploadQuery->insertTextAtCursor("\nFlash Memory Size: " + str);
            if( !(str=miosStudio->uploadHandler->coreRamSize).isEmpty() )
                uploadQuery->insertTextAtCursor("\nRAM Size: " + str);
            if( !(str=miosStudio->uploadHandler->coreAppHeader1).isEmpty() )
                uploadQuery->insertTextAtCursor("\n"+str);
            if( !(str=miosStudio->uploadHandler->coreAppHeader2).isEmpty() )
                uploadQuery->insertTextAtCursor("\n"+str);
        }
    }
}
