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


//==============================================================================
UploadWindow::UploadWindow(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , ongoingMidiMessage(0)
    , gotFirstMessage(0)
    , ongoingQueryMessage(0)
    , deviceId(0x00)
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

    addAndMakeVisible(stopButton = new TextButton(T("Stop Button")));
    stopButton->setButtonText(T("Stop"));
    stopButton->addButtonListener(this);

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
    }
    else if( buttonThatWasClicked == stopButton ) {
    }
    else if( buttonThatWasClicked == queryButton ) {
    }
}

void UploadWindow::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
}


//==============================================================================
void UploadWindow::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    const int mios32SysExId = 0x32;

    uint8 *data = message.getRawData();
    int messageOffset = 0;

    if( runningStatus == 0xf0 && !ongoingMidiMessage ) {
        if( data[1] == 0x00 &&
            data[2] == 0x00 &&
            data[3] == 0x7e &&
            data[4] == mios32SysExId &&
            data[5] == deviceId &&
            data[6] == 0x0f ) { // acknowledge message
            ongoingMidiMessage = 1;
            messageOffset = 7;
        }
    } else
        ongoingMidiMessage = 0;

    if( ongoingMidiMessage ) {
        String out = gotFirstMessage ? "\n" : "Got reply from MIOS32 core:\n";

        if( ongoingQueryMessage ) {

            switch( ongoingQueryMessage ) {
            case 0x01: out += "Operating System: "; break;
            case 0x02: out += "Board: "; break;
            case 0x03: out += "Core Family: "; break;
            case 0x04: out += "Chip ID: 0x"; break;
            case 0x05: out += "Serial Number: #"; break;
            case 0x06: out += "Flash Memory Size: "; break;
            case 0x07: out += "RAM Size: "; break;
            case 0x08: break; // Application first line
            case 0x09: break; // Application second line
            default: {
                String decString = String(ongoingQueryMessage);
                out += "Query #" + decString + ": ";
            }
            }

            int size = message.getRawDataSize();

            for(int i=messageOffset; i<size; ++i) {
                if( data[i] == 0xf7 ) {
                    ongoingMidiMessage = 0;
                } else {
                    if( data[i] != '\n' || size < (i+1) )
                        out += String::formatted(T("%c"), data[i] & 0x7f);
                }
            }

            // request next query
            if( ongoingQueryMessage < 0x09 )
                queryCore(ongoingQueryMessage + 1);
        } else {
            // TODO
            out = T("<unsupported yet>");
        }

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
    const int mios32SysExId = 0x32;

    ongoingQueryMessage = queryRequest;

    const uint8 queryMios32[9] = { 0xf0, 0x00, 0x00, 0x7e, mios32SysExId, deviceId, 0x00, queryRequest, 0xf7 };
    MidiMessage message = MidiMessage(queryMios32, 9);
    miosStudio->sendMidiMessage(message);
}
