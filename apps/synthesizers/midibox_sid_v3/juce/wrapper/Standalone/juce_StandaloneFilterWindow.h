/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
 * MIDIbox SID Standalone Main Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef __JUCE_STANDALONEFILTERWINDOW_JUCEHEADER__
#define __JUCE_STANDALONEFILTERWINDOW_JUCEHEADER__

#include "juce_AudioFilterStreamer.h"


//==============================================================================
/**
    A class that can be used to run a simple standalone application containing your filter.

    Just create one of these objects in your JUCEApplication::initialise() method, and
    let it do its work. It will create your filter object using the same createFilter() function
    that the other plugin wrappers use.
*/
class StandaloneFilterWindow    : public DocumentWindow,
                                  public ButtonListener
{
public:
    //==============================================================================
    StandaloneFilterWindow (const String& title,
                            const Colour& backgroundColour);

    ~StandaloneFilterWindow();

    //==============================================================================
    /** Deletes and re-creates the filter and its UI. */
    void resetFilter();

    /** Pops up a dialog letting the user save the filter's state to a file. */
    void saveState();

    /** Pops up a dialog letting the user re-load the filter's state from a file. */
    void loadState();

    /** Shows the audio properties dialog box modally. */
    virtual void showAudioSettingsDialog();

    /** Returns the property set to use for storing the app's last state.

        This will be used to store the audio set-up and the filter's last state.
    */
    virtual PropertySet* getGlobalSettings();

    //==============================================================================
    /** @internal */
    void closeButtonPressed();
    /** @internal */
    void buttonClicked (Button*);
    /** @internal */
    void resized();

    juce_UseDebuggingNewOperator

private:
    AudioProcessor* filter;
    AudioFilterStreamingDeviceManager* deviceManager;
    Button* optionsButton;

    void deleteFilter();

    StandaloneFilterWindow (const StandaloneFilterWindow&);
    const StandaloneFilterWindow& operator= (const StandaloneFilterWindow&);
};

#endif   // __JUCE_STANDALONEFILTERWINDOW_JUCEHEADER__
