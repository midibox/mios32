/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Standalone Main
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "includes.h"
#include "juce_StandaloneFilterWindow.h"


//==============================================================================
/** This is the application object that is started up when Juce starts. It handles
    the initialisation and shutdown of the whole application.
*/
class JUCEMidiboxSidApplication : public JUCEApplication
{
    /* Important! NEVER embed objects directly inside your JUCEApplication class! Use
       ONLY pointers to objects, which you should create during the initialise() method
       (NOT in the constructor!) and delete in the shutdown() method (NOT in the
       destructor!)

       This is because the application object gets created before Juce has been properly
       initialised, so any embedded objects would also get constructed too soon.
   */
    StandaloneFilterWindow* standaloneFilterWindow;

public:
    //==============================================================================
    JUCEMidiboxSidApplication()
        : standaloneFilterWindow(0)
    {
        // NEVER do anything in here that could involve any Juce function being called
        // - leave all your startup tasks until the initialise() method.
    }

    ~JUCEMidiboxSidApplication()
    {
        // Your shutdown() method should already have done all the things necessary to
        // clean up this app object, so you should never need to put anything in
        // the destructor.

        // Making any Juce calls in here could be very dangerous...
    }

    //==============================================================================
    void initialise (const String& commandLine)
    {
        // create the main window...
        standaloneFilterWindow = new StandaloneFilterWindow(T("MIDIbox SID"), Colours::lightgrey);
        standaloneFilterWindow->setUsingNativeTitleBar(true);

        /*  ..and now return, which will fall into to the main event
            dispatch loop, and this will run until something calls
            JUCEAppliction::quit().

            In this case, JUCEAppliction::quit() will be called by the
            hello world window being clicked.
        */
    }

    void shutdown()
    {
        ApplicationProperties::getInstance()->closeFiles();

        // clear up..
        if( standaloneFilterWindow != 0 )
            delete standaloneFilterWindow;
    }


    //==============================================================================
    const String getApplicationName()
    {
        return T(JucePlugin_Desc);
    }

    const String getApplicationVersion()
    {
        return T(JucePlugin_VersionString);
    }

    bool moreThanOneInstanceAllowed()
    {
        return true;
    }

    void anotherInstanceStarted (const String& commandLine)
    {
    }
};


//==============================================================================
// This macro creates the application's main() function..
START_JUCE_APPLICATION (JUCEMidiboxSidApplication)
