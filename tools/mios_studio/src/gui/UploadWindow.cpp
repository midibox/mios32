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

//==============================================================================
UploadWindow::UploadWindow(AudioDeviceManager &_audioDeviceManager)
    : audioDeviceManager(&_audioDeviceManager)
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
    uploadStatus->setMultiLine(false);
    uploadStatus->setReturnKeyStartsNewLine(false);
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

