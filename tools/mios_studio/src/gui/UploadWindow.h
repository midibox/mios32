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

class UploadWindow
    : public Component
    , public ButtonListener
    , public FilenameComponentListener
                     
{
public:
    //==============================================================================
    UploadWindow(AudioDeviceManager &_audioDeviceManager);
    ~UploadWindow();

    //==============================================================================
    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged);


private:
    //==============================================================================
    FilenameComponent* fileChooser;
    TextEditor* uploadStatus;
    TextButton* startButton;
    TextButton* stopButton;
    TextButton* queryButton;

    //==============================================================================
    AudioDeviceManager *audioDeviceManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    UploadWindow (const UploadWindow&);
    const UploadWindow& operator= (const UploadWindow&);
};

#endif /* _UPLOAD_WINDOW_H */
