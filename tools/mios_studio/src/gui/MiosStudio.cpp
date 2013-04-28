/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDI Monitor Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MiosStudio.h"
#include "../version.h"

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#endif

//==============================================================================
MiosStudio::MiosStudio()
    : batchMode(false)
    , uploadWindow(0)
    , midiInMonitor(0)
    , midiOutMonitor(0)
    , miosTerminal(0)
    , midiKeyboard(0)
    , initialMidiScanCounter(1) // start step-wise MIDI port scan
    , batchWaitCounter(0)
    , initialGuiX(-1) // centered
    , initialGuiY(-1) // centered
{
    bool hideMonitors = false;
    bool hideUpload = false;
    bool hideTerminal = false;
    bool hideKeyboard = false;
    int  guiWidth = 800;
    int  guiHeight = 650;
    int  firstDeviceId = -1;

    // parse the command line
    {
        int numErrors = 0;
        bool quitIfBatch = false;
        StringArray commandLineArray = JUCEApplication::getCommandLineParameterArray();

        // first search for --batch option
        for(int i=0; i<commandLineArray.size(); ++i) {
            if( commandLineArray[i].compare("--batch") == 0 ) {
                batchMode = true;
                redirectIOToConsole();
            }
        }

        // now for the remaining options
        for(int i=0; i<commandLineArray.size(); ++i) {
            if( commandLineArray[i].compare("--help") == 0 ) {
                commandLineInfoMessages += "Command Line Parameters:\n";
                commandLineInfoMessages += "--help                  this page\n";
                commandLineInfoMessages += "--version               shows version number\n";
                commandLineInfoMessages += "--batch                 don't open GUI\n";
                commandLineInfoMessages += "--in=<port>             optional search string for MIDI IN port\n";
                commandLineInfoMessages += "--out=<port>            optional search string for MIDI OUT port\n";
                commandLineInfoMessages += "--device_id=<id>        sets the device id, should be done before upload if necessary\n";
                commandLineInfoMessages += "--query                 queries the selected core\n";
                commandLineInfoMessages += "--upload_hex=<file>     upload specified .hex file to core. Multiple --upload_hex allowed!\n";
                commandLineInfoMessages += "--upload_file=<file>    upload specified file to SD Card. Multiple --upload_file allowed!\n";
                commandLineInfoMessages += "--send_syx=<file>       send specified .syx file to core. Multiple --send_syx allowed!\n";
                commandLineInfoMessages += "--terminal=<command>    send a MIOS terminal command. Multiple --terminal allowed!\n";
                commandLineInfoMessages += "--wait=<seconds>        Waits for the given seconds.\n";
                commandLineInfoMessages += "--gui_x=<x>             specifies the initial window X position\n";
                commandLineInfoMessages += "--gui_y=<y>             specifies the initial window Y position\n";
                commandLineInfoMessages += "--gui_width=<width>     specifies the initial window width\n";
                commandLineInfoMessages += "--gui_height=<height>   specifies the initial window height\n";
                commandLineInfoMessages += "--gui_title=<name>      changes the name of the application in the title bar\n";
                commandLineInfoMessages += "--gui_hide_monitors     disables the MIDI IN/OUT monitor when the GUI is started\n";
                commandLineInfoMessages += "--gui_hide_upload       disables the upload panel when the GUI is started\n";
                commandLineInfoMessages += "--gui_hide_terminal     disables the terminal panel when the GUI is started\n";
                commandLineInfoMessages += "--gui_hide_keyboard     disables the virtual keyboard panel when the GUI is started\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "Usage Examples:\n";
                commandLineInfoMessages += "  MIOS_Studio --in=MIOS32 --out=MIOS32\n";
                commandLineInfoMessages += "    starts MIOS Studio with MIDI IN/OUT port matching with 'MIOS32'\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "  MIOS_Studio --upload_hex=project.hex\n";
                commandLineInfoMessages += "    starts MIOS Studio and uploads the project.hex file immediately\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "  MIOS_Studio --batch --upload_hex=project.hex\n";
                commandLineInfoMessages += "    starts MIOS Studio without GUI and uploads the project.hex file\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "  MIOS_Studio --batch --upload_file=default.ngc --upload_file=default.ngl\n";
                commandLineInfoMessages += "    starts MIOS Studio without GUI and uploads two files to SD Card (MIOS32 only)\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "  MIOS_Studio --batch --terminal=\"help\" --wait=1\n";
                commandLineInfoMessages += "    starts MIOS Studio, executes the \"help\" command in MIOS Terminal and waits 1 second for response\n";
                commandLineInfoMessages += "\n";
                commandLineInfoMessages += "NOTE: most parameters can be combined to a sequence of operations.\n";
                commandLineInfoMessages += "      E.g. upload a .hex file, upload files to SD Card, execute a terminal command and wait some seconds before exit.\n";
                quitIfBatch = true;
            } else if( commandLineArray[i].compare("--version") == 0 ) {
                commandLineInfoMessages += String("MIOS Studio ") + String(MIOS_STUDIO_VERSION) + String("\n");
            } else if( commandLineArray[i].compare("--batch") == 0 ) {
                // already handled above
            } else if( commandLineArray[i].startsWith("--in=") ) {
                inPortFromCommandLine = commandLineArray[i].substring(5);
                inPortFromCommandLine.trimCharactersAtStart(" \t\"'");
                inPortFromCommandLine.trimCharactersAtEnd(" \t\"'");
                std::cout << "Preselected MIDI IN Port: " << inPortFromCommandLine << std::endl;
            } else if( commandLineArray[i].startsWith("--out=") ) {
                outPortFromCommandLine = commandLineArray[i].substring(6);
                outPortFromCommandLine.trimCharactersAtStart(" \t\"'");
                outPortFromCommandLine.trimCharactersAtEnd(" \t\"'");
                std::cout << "Preselected MIDI OUT Port: " << outPortFromCommandLine << std::endl;
            } else if( commandLineArray[i].startsWith("--device_id") ) {
                String id = commandLineArray[i].substring(12);
                id.trimCharactersAtStart(" \t\"'");
                id.trimCharactersAtEnd(" \t\"'");
                int idValue = id.getIntValue();
                if( idValue < 0 || idValue > 127 ) {
                    commandLineErrorMessages += String("ERROR: device ID should be within 0..127!\n");
                    ++numErrors;
                } else {
                    if( firstDeviceId < 0 ) {
                        firstDeviceId = idValue;
                    } else {
                        batchJobs.add(String("device_id ") + id);
                    }
                }
            } else if( commandLineArray[i].startsWith("--query") ) {
                batchJobs.add(String("query"));
            } else if( commandLineArray[i].startsWith("--upload_hex") ) {
                String file = commandLineArray[i].substring(13);
                file.trimCharactersAtStart(" \t\"'");
                file.trimCharactersAtEnd(" \t\"'");
                batchJobs.add(String("upload_hex ") + file);
            } else if( commandLineArray[i].startsWith("--upload_file") ) {
                String file = commandLineArray[i].substring(14);
                file.trimCharactersAtStart(" \t\"'");
                file.trimCharactersAtEnd(" \t\"'");
                batchJobs.add(String("upload_file ") + file);
            } else if( commandLineArray[i].startsWith("--send_syx") ) {
                String file = commandLineArray[i].substring(11);
                file.trimCharactersAtStart(" \t\"'");
                file.trimCharactersAtEnd(" \t\"'");
                batchJobs.add(String("send_syx ") + file);
            } else if( commandLineArray[i].startsWith("--terminal") ) {
                String command = commandLineArray[i].substring(11);
                command.trimCharactersAtStart(" \t\"'");
                command.trimCharactersAtEnd(" \t\"'");
                batchJobs.add(String("terminal ") + command);
            } else if( commandLineArray[i].startsWith("--wait") ) {
                String command = commandLineArray[i].substring(7);
                command.trimCharactersAtStart(" \t\"'");
                command.trimCharactersAtEnd(" \t\"'");
                batchJobs.add(String("wait ") + command);
            } else if( commandLineArray[i].startsWith("--gui_x") ) {
                int value = commandLineArray[i].substring(8).getIntValue();
                if( value >= 0 )
                    initialGuiX = value;
            } else if( commandLineArray[i].startsWith("--gui_y") ) {
                int value = commandLineArray[i].substring(8).getIntValue();
                if( value >= 0 )
                    initialGuiY = value;
            } else if( commandLineArray[i].startsWith("--gui_width") ) {
                int value = commandLineArray[i].substring(12).getIntValue();
                if( value > 0 )
                    guiWidth = value;
            } else if( commandLineArray[i].startsWith("--gui_height") ) {
                int value = commandLineArray[i].substring(13).getIntValue();
                if( value > 0 )
                    guiHeight = value;
            } else if( commandLineArray[i].startsWith("--gui_title") ) {
                initialGuiTitle = commandLineArray[i].substring(12);
                initialGuiTitle.trimCharactersAtStart(" \t\"'");
                initialGuiTitle.trimCharactersAtEnd(" \t\"'");
            } else if( commandLineArray[i].startsWith("--gui_hide_monitors") ) {
                hideMonitors = true;
            } else if( commandLineArray[i].startsWith("--gui_hide_upload") ) {
                hideUpload = true;
            } else if( commandLineArray[i].startsWith("--gui_hide_terminal") ) {
                hideTerminal = true;
            } else if( commandLineArray[i].startsWith("--gui_hide_keyboard") ) {
                hideKeyboard = true;
            } else if( commandLineArray[i].startsWith("-psn") ) {
                // ignore for MacOS
            } else {
                commandLineErrorMessages += String("ERROR: unknown command line parameter: ") + commandLineArray[i] + String("\n");
                commandLineErrorMessages += String("Enter '--help' to get a list of all available options!\n");
                ++numErrors;
            }
        }

        std::cout << commandLineInfoMessages;

        if( numErrors ) {
            quitIfBatch = true;

            if( runningInBatchMode() ) {
                std::cerr << commandLineErrorMessages;
            } else {
                // AlertWindow will be shown from timerCallback() once MIOS Studio is running
            }
        }

        if( runningInBatchMode() && quitIfBatch ) {
#ifdef _WIN32
            std::cout << "Press <enter> to quit console." << std::endl;
            while (GetAsyncKeyState(VK_RETURN) & 0x8000) {}
            while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {}
#endif
            JUCEApplication::getInstance()->setApplicationReturnValue(1); // error
            JUCEApplication::quit();
        }

        if( runningInBatchMode() ) {
            batchJobs.add("quit");
        }
    }

    // instantiate components
    uploadHandler = new UploadHandler(this);
    sysexPatchDb = new SysexPatchDb();

    addAndMakeVisible(uploadWindow = new UploadWindow(this));
    addAndMakeVisible(midiInMonitor = new MidiMonitor(this, true));
    addAndMakeVisible(midiOutMonitor = new MidiMonitor(this, false));
    addAndMakeVisible(miosTerminal = new MiosTerminal(this));
    addAndMakeVisible(midiKeyboard = new MidiKeyboard(this));

    // tools are created and made visible via tools button in Upload Window
    sysexToolWindow = 0;
    oscToolWindow = 0;
    midio128ToolWindow = 0;
    mbCvToolWindow = 0;
    mbhpMfToolWindow = 0;
    sysexLibrarianWindow = 0;
    miosFileBrowserWindow = 0;

    addAndMakeVisible(horizontalDividerBar1 = new StretchableLayoutResizerBar(&horizontalLayout, 1, false));
    addAndMakeVisible(horizontalDividerBar2 = new StretchableLayoutResizerBar(&horizontalLayout, 3, false));
    addAndMakeVisible(horizontalDividerBar3 = new StretchableLayoutResizerBar(&horizontalLayout, 5, false));
    addAndMakeVisible(verticalDividerBarMonitors = new StretchableLayoutResizerBar(&verticalLayoutMonitors, 1, true));
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));
    resizeLimits.setSizeLimits(200, 100, 2048, 2048);

    commandManager = new ApplicationCommandManager();
    commandManager->registerAllCommandsForTarget(this);
    commandManager->registerAllCommandsForTarget(JUCEApplication::getInstance());
    addKeyListener(commandManager->getKeyMappings());
    setApplicationCommandManagerToWatch(commandManager);

    if( hideMonitors ) {
        midiInMonitor->setVisible(false);
        verticalDividerBarMonitors->setVisible(false);
        midiOutMonitor->setVisible(false);
    }
    if( hideUpload ) {
        horizontalDividerBar1->setVisible(false);
        uploadWindow->setVisible(false);
    }
    if( hideTerminal ) {
        horizontalDividerBar2->setVisible(false);
        miosTerminal->setVisible(false);
    }
    if( hideKeyboard ) {
        horizontalDividerBar3->setVisible(false);
        midiKeyboard->setVisible(false);
    }

    updateLayout();

    if( firstDeviceId >= 0 ) {
        std::cout << "Setting Device ID=" << firstDeviceId << std::endl;
        uploadWindow->setDeviceId(firstDeviceId);
    }

    Timer::startTimer(1);

    setSize(guiWidth, guiHeight);
}

MiosStudio::~MiosStudio()
{
    if( uploadHandler )
        deleteAndZero(uploadHandler);
    if( sysexToolWindow )
        deleteAndZero(sysexToolWindow);
    if( oscToolWindow )
        deleteAndZero(oscToolWindow);
    if( midio128ToolWindow )
        deleteAndZero(midio128ToolWindow);
    if( mbCvToolWindow )
        deleteAndZero(mbCvToolWindow);
    if( mbhpMfToolWindow )
        deleteAndZero(mbhpMfToolWindow);
    if( sysexLibrarianWindow )
        deleteAndZero(sysexLibrarianWindow);
    if( miosFileBrowserWindow )
        deleteAndZero(miosFileBrowserWindow);

    // try: avoid crash under Windows by disabling all MIDI INs/OUTs
    closeMidiPorts();
}

//==============================================================================
#ifdef _WIN32
// see http://www.rawmaterialsoftware.com/viewtopic.php?f=2&t=9868

// Code taken from here: http://dslweb.nwnexus.com/~ast/dload/guicon.htm
// Modified to support attaching to an owner console.
void MiosStudio::redirectIOToConsole()
{
    int hConHandle;
    long lStdHandle;
    FILE *fp;
    if (1) // TK: crashes the application: AttachConsole(ATTACH_PARENT_PROCESS) == 0)
    {
      // We couldn't obtain a parent console.  Probably application was launched 
      // from inside Explorer (E.G. the run prompt, or a shortcut).
      // We'll spawn a new console window instead then!
      CONSOLE_SCREEN_BUFFER_INFO coninfo;
      AllocConsole();
      GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
      coninfo.dwSize.Y = 500;
      SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
    }
    // redirect unbuffered STDOUT to the console
    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );
    // redirect unbuffered STDIN to the console
    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );
    // redirect unbuffered STDERR to the console
    lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );
    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    std::ios::sync_with_stdio();
}

#else

// Empty function avoid ifdefs elsewhere.
void MiosStudio::redirectIOToConsole() {}

#endif


//==============================================================================
void MiosStudio::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void MiosStudio::resized()
{
    horizontalLayout.layOutComponents(layoutHComps.getRawDataPointer(), layoutHComps.size(),
                                       4, 4,
                                       getWidth() - 8, getHeight() - 8,
                                       true,  // lay out above each other
                                       true); // resize the components' heights as well as widths

    if( layoutVComps.size() ) {
        verticalLayoutMonitors.layOutComponents(layoutVComps.getRawDataPointer(), layoutVComps.size(),
                                                4,
                                                4 + horizontalLayout.getItemCurrentPosition(0),
                                                getWidth() - 8,
                                                horizontalLayout.getItemCurrentAbsoluteSize(0),
                                                false, // lay out side-by-side
                                                true); // resize the components' heights as well as widths
    }

    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}


//==============================================================================
bool MiosStudio::runningInBatchMode(void)
{
    return batchMode;
}

//==============================================================================
void MiosStudio::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    uint8 *data = (uint8 *)message.getRawData();
    uint32 size = message.getRawDataSize();

    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    

    // ugly fix for reduced buffer size under windows...
    if( size > 2 && data[0] == 0xf0 && data[size-1] != 0xf7 ) {
        // first message without F7 at the end
        sysexReceiveBuffer.clear();
        for(int pos=0; pos<size; ++pos)
            sysexReceiveBuffer.add(data[pos]);
        return; // hopefully we will receive F7 with the next call

    } else if( sysexReceiveBuffer.size() && !(data[0] & 0x80) && data[size-1] != 0xf7 ) {
        // continued message without F7 at the end
        for(int pos=0; pos<size; ++pos)
            sysexReceiveBuffer.add(data[pos]);
        return; // hopefully we will receive F7 with the next call

    } else if( sysexReceiveBuffer.size() && data[size-1] == 0xf7 ) {
        // finally we received F7
        for(int pos=0; pos<size; ++pos)
            sysexReceiveBuffer.add(data[pos]);

        // propagate combined message
        uint8 *bufferedData = &sysexReceiveBuffer.getReference(0);        
        MidiMessage combinedMessage(bufferedData, sysexReceiveBuffer.size());
        sysexReceiveBuffer.clear();

        const ScopedLock sl(midiInQueueLock); // lock will be released at end of function
        midiInQueue.push(combinedMessage);

        // propagate to upload handler
        uploadHandler->handleIncomingMidiMessage(source, combinedMessage);

    } else {
        sysexReceiveBuffer.clear();

        const ScopedLock sl(midiInQueueLock); // lock will be released at end of function
        midiInQueue.push(message);

        // propagate to upload handler
        uploadHandler->handleIncomingMidiMessage(source, message);
    }
}


//==============================================================================
void MiosStudio::sendMidiMessage(MidiMessage &message)
{
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();

    // if timestamp isn't set, to this now to ensure a plausible MIDI Out monitor output
    if( message.getTimeStamp() == 0 )
        message.setTimeStamp((double)Time::getMillisecondCounter() / 1000.0);

    if( out )
        out->sendMessageNow(message);

    const ScopedLock sl(midiOutQueueLock); // lock will be released at end of function
    midiOutQueue.push(message);
}


//==============================================================================
void MiosStudio::closeMidiPorts(void)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;)
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], false);

    audioDeviceManager.setDefaultMidiOutput(String::empty);
}


//==============================================================================
void MiosStudio::timerCallback()
{
    // step-wise MIDI port scan after startup
    if( initialMidiScanCounter ) {
        switch( initialMidiScanCounter ) {
        case 1:
            Timer::stopTimer();

            midiInMonitor->scanMidiDevices(inPortFromCommandLine);
            ++initialMidiScanCounter;

            Timer::startTimer(1);
            break;

        case 2:
            Timer::stopTimer();

            midiOutMonitor->scanMidiDevices(outPortFromCommandLine);
            ++initialMidiScanCounter;

            Timer::startTimer(1);
            break;

        case 3:
            Timer::stopTimer();

            if( !runningInBatchMode() ) {
                // and check for infos
                if( commandLineInfoMessages.length() ) {
                    AlertWindow::showMessageBox(AlertWindow::InfoIcon,
                                                T("Info"),
                                                commandLineInfoMessages,
                                                String::empty);
                    commandLineInfoMessages = String::empty;
                }

                // now also check for command line errors
                if( commandLineErrorMessages.length() ) {
                    AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                                T("Command Line Error"),
                                                commandLineErrorMessages,
                                                String::empty);
                    commandLineErrorMessages = String::empty;
                }
            }

            // try to query selected core
            audioDeviceManager.addMidiInputCallback(String::empty, this);

            if( getMidiOutput() != String::empty )
                uploadWindow->queryCore();

            initialMidiScanCounter = 0; // stop scan

            Timer::startTimer(1);
            break;
        }
    } else {
        // important: only broadcast 1..5 messages per timer tick to avoid GUI hangups when
        // a large bulk of data is received

        for(int checkLoop=0; checkLoop<5; ++checkLoop) {
            if( !midiInQueue.empty() ) {
                const ScopedLock sl(midiInQueueLock); // lock will be released at end of this scope

                MidiMessage &message = midiInQueue.front();

                uint8 *data = (uint8 *)message.getRawData();
                if( data[0] >= 0x80 && data[0] < 0xf8 )
                    runningStatus = data[0];

                // propagate incoming event to MIDI components
                midiInMonitor->handleIncomingMidiMessage(message, runningStatus);

                // filter runtime events for following components to improve performance
                if( data[0] < 0xf8 ) {
                    if( sysexToolWindow )
                        sysexToolWindow->handleIncomingMidiMessage(message, runningStatus);
                    if( midio128ToolWindow )
                        midio128ToolWindow->handleIncomingMidiMessage(message, runningStatus);
                    if( mbCvToolWindow )
                        mbCvToolWindow->handleIncomingMidiMessage(message, runningStatus);
                    if( mbhpMfToolWindow )
                        mbhpMfToolWindow->handleIncomingMidiMessage(message, runningStatus);
                    if( sysexLibrarianWindow )
                        sysexLibrarianWindow->handleIncomingMidiMessage(message, runningStatus);
                    if( miosFileBrowserWindow ) {
                        miosFileBrowserWindow->handleIncomingMidiMessage(message, runningStatus);
                    }
                    miosTerminal->handleIncomingMidiMessage(message, runningStatus);
                    midiKeyboard->handleIncomingMidiMessage(message, runningStatus);
                }

                midiInQueue.pop();
            }

            if( !midiOutQueue.empty() ) {
                const ScopedLock sl(midiOutQueueLock); // lock will be released at end of this scope

                MidiMessage &message = midiOutQueue.front();

                midiOutMonitor->handleIncomingMidiMessage(message, message.getRawData()[0]);

                midiOutQueue.pop();
            }
        }

        if( batchJobs.size() ) {
            if( batchWaitCounter ) {
                --batchWaitCounter;
            } else if( uploadWindow->uploadInProgress() ||
                       (sysexToolWindow && sysexToolWindow->sendSyxInProgress()) ||
                       (miosFileBrowserWindow && miosFileBrowserWindow->uploadFileInProgress()) ) {
                // wait...
            } else {
                String job(batchJobs[0]);
                batchJobs.remove(0);

                if( job.startsWithIgnoreCase("device_id ") ) {
                    int id = job.substring(10).getIntValue();
                    if( id < 0 || id > 127 ) {
                        std::cerr << "ERROR: device ID should be within 0..127!" << std::endl;
                    } else {
                        std::cout << "Setting Device ID=" << id << std::endl;
                        uploadWindow->setDeviceId(id);
                    }
                } else if( job.startsWithIgnoreCase("query") ) {
                    std::cout << "Query Core..." << std::endl;
                    uploadWindow->queryFromExternal();
                } else if( job.startsWithIgnoreCase("upload_hex ") ) {
                    String filename = job.substring(11);

                    std::cout << "Uploading " << filename << "..." << std::endl;
                    uploadWindow->uploadFileFromExternal(filename);
                } else if( job.startsWithIgnoreCase("upload_file ") ) {
                    String filename = job.substring(12);

                    std::cout << "Uploading " << filename << "..." << std::endl;
                    if( !miosFileBrowserWindow ) {
                        miosFileBrowserWindow = new MiosFileBrowserWindow(this);
                        if( !runningInBatchMode() ) {
                            miosFileBrowserWindow->setVisible(true);
                        }
                    }
                    miosFileBrowserWindow->uploadFileFromExternal(filename);
                } else if( job.startsWithIgnoreCase("send_syx ") ) {
                    String filename = job.substring(9);

                    std::cout << "Sending SysEx " << filename << "..." << std::endl;
                    if( !sysexToolWindow ) {
                        sysexToolWindow = new SysexToolWindow(this);
                        if( !runningInBatchMode() ) {
                            sysexToolWindow->setVisible(true);
                        }
                    }
                    sysexToolWindow->sendSyxFile(filename);
                } else if( job.startsWithIgnoreCase("terminal ") ) {
                    String command = job.substring(9);

                    std::cout << "MIOS Terminal command: " << command << std::endl;
                    miosTerminal->execCommand(command);
                } else if( job.startsWithIgnoreCase("wait ") ) {
                    int counter = job.substring(5).getIntValue();
                    if( counter < 0 ) {
                        counter = 0;
                    }
                    std::cout << "Waiting for " << counter << " second" << ((counter == 1) ? "" : "s") << "..." << std::endl;
                    batchWaitCounter = counter*1000;
                } else if( job.startsWithIgnoreCase("quit") ) {
                    if( runningInBatchMode() ) {
#ifdef _WIN32
                        std::cout << "Press <enter> to quit console." << std::endl;
                        while (GetAsyncKeyState(VK_RETURN) & 0x8000) {}
                        while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {}
#endif
                        JUCEApplication::getInstance()->setApplicationReturnValue(0); // no error
                        JUCEApplication::quit();
                    } else {
                        AlertWindow::showMessageBox(AlertWindow::InfoIcon,
                                                    T("Info"),
                                                    T("All batch jobs executed."),
                                                    String::empty);
                    }
                } else {
                    std::cerr << "ERROR: unknown batch job: '" << job << "'!" << std::endl;
                }
            }
        }
    }
}


//==============================================================================
void MiosStudio::setMidiInput(const String &port)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        bool enabled = allMidiIns[i] == port;
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
    }

    // propagate port change
    if( uploadWindow && initialMidiScanCounter == 0 && port != String::empty )
        uploadWindow->midiPortChanged();

    // store setting if MIDI input selected
    if( port != String::empty ) {
        PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("midiIn"), port);
    }
}

String MiosStudio::getMidiInput(void)
{
    // restore setting
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
    return propertiesFile ? propertiesFile->getValue(T("midiIn"), String::empty) : String::empty;
}

void MiosStudio::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);

    // propagate port change
    if( uploadWindow && initialMidiScanCounter == 0 && port != String::empty )
        uploadWindow->midiPortChanged();

    // store setting if MIDI output selected
    if( port != String::empty ) {
        PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(T("midiOut"), port);
    }
}

String MiosStudio::getMidiOutput(void)
{
    // restore setting
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
    return propertiesFile ? propertiesFile->getValue(T("midiOut"), String::empty) : String::empty;
}




//==============================================================================
StringArray MiosStudio::getMenuBarNames()
{
    const char* const names[] = { "Application", "Tools", "Help", 0 };

    return StringArray ((const char**) names);
}

PopupMenu MiosStudio::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
    PopupMenu menu;

    if( topLevelMenuIndex == 0 ) {
        // "Application" menu
        menu.addCommandItem(commandManager, enableMonitors);
        menu.addCommandItem(commandManager, enableUpload);
        menu.addCommandItem(commandManager, enableTerminal);
        menu.addCommandItem(commandManager, enableKeyboard);
        menu.addSeparator();
        menu.addCommandItem(commandManager, rescanDevices);
        menu.addSeparator();
        menu.addCommandItem(commandManager, StandardApplicationCommandIDs::quit);
    } else if( topLevelMenuIndex == 1 ) {
        // "Tools" menu
        menu.addCommandItem(commandManager, showSysexTool);
        menu.addCommandItem(commandManager, showSysexLibrarian);
        menu.addCommandItem(commandManager, showOscTool);
        menu.addCommandItem(commandManager, showMidio128Tool);
        menu.addCommandItem(commandManager, showMbCvTool);
        menu.addCommandItem(commandManager, showMbhpMfTool);
        menu.addCommandItem(commandManager, showMiosFileBrowser);
    } else if( topLevelMenuIndex == 2 ) {
        // "Help" menu
        menu.addCommandItem(commandManager, showMiosStudioPage);
        menu.addCommandItem(commandManager, showTroubleshootingPage);
    }

    return menu;
}

void MiosStudio::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
}


//==============================================================================
// The following methods implement the ApplicationCommandTarget interface, allowing
// this window to publish a set of actions it can perform, and which can be mapped
// onto menus, keypresses, etc.

ApplicationCommandTarget* MiosStudio::getNextCommandTarget()
{
    // this will return the next parent component that is an ApplicationCommandTarget (in this
    // case, there probably isn't one, but it's best to use this method in your own apps).
    return findFirstTargetParentComponent();
}

void MiosStudio::getAllCommands(Array <CommandID>& commands)
{
    // this returns the set of all commands that this target can perform..
    const CommandID ids[] = { showSysexTool,
                              showOscTool,
                              showMidio128Tool,
                              showMbCvTool,
                              showMbhpMfTool,
                              showSysexLibrarian,
                              showMiosFileBrowser,
                              enableMonitors,
                              enableUpload,
                              enableTerminal,
                              enableKeyboard,
                              rescanDevices,
                              showMiosStudioPage,
                              showTroubleshootingPage
    };

    commands.addArray (ids, numElementsInArray (ids));
}

// This method is used when something needs to find out the details about one of the commands
// that this object can perform..
void MiosStudio::getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result)
{
    const String applicationCategory (T("Application"));
    const String toolsCategory(T("Tools"));
    const String helpCategory (T("Help"));

    switch( commandID ) {
    case enableMonitors:
        result.setInfo(T("Show MIDI Monitors"), T("Enables/disables the MIDI IN/OUT Monitors"), applicationCategory, 0);
        result.setTicked(verticalDividerBarMonitors->isVisible());
        result.addDefaultKeypress('M', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case enableUpload:
        result.setInfo(T("Show Upload Window"), T("Enables/disables the Upload Window"), applicationCategory, 0);
        result.setTicked(uploadWindow->isVisible());
        result.addDefaultKeypress('U', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case enableTerminal:
        result.setInfo(T("Show MIOS Terminal"), T("Enables/disables the MIOS Terminal"), applicationCategory, 0);
        result.setTicked(miosTerminal->isVisible());
        result.addDefaultKeypress('T', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case enableKeyboard:
        result.setInfo(T("Show Virtual Keyboard"), T("Enables/disables the virtual Keyboard"), applicationCategory, 0);
        result.setTicked(midiKeyboard->isVisible());
        result.addDefaultKeypress('K', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case rescanDevices:
        result.setInfo(T("Rescan MIDI Devices"), T("Updates the MIDI In/Out port lists"), applicationCategory, 0);
        result.addDefaultKeypress('R', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showSysexTool:
        result.setInfo(T("SysEx Tool"), T("Allows to send and receive SysEx dumps"), toolsCategory, 0);
        result.setTicked(sysexToolWindow && sysexToolWindow->isVisible());
        result.addDefaultKeypress('1', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showSysexLibrarian:
        result.setInfo(T("SysEx Librarian"), T("Allows to manage SysEx files"), toolsCategory, 0);
        result.setTicked(sysexLibrarianWindow && sysexLibrarianWindow->isVisible());
        result.addDefaultKeypress('2', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showOscTool:
        result.setInfo(T("OSC Tool"), T("Allows to send and receive OSC messages"), toolsCategory, 0);
        result.setTicked(oscToolWindow && oscToolWindow->isVisible());
        result.addDefaultKeypress('3', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMidio128Tool:
        result.setInfo(T("MIDIO128 V2 Tool"), T("Allows to configure a MIDIO128 V2"), toolsCategory, 0);
        result.setTicked(midio128ToolWindow && midio128ToolWindow->isVisible());
        result.addDefaultKeypress('4', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMbCvTool:
        result.setInfo(T("MIDIbox CV V1 Tool"), T("Allows to configure a MIDIbox CV V1"), toolsCategory, 0);
        result.setTicked(mbCvToolWindow && mbCvToolWindow->isVisible());
        result.addDefaultKeypress('5', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMbhpMfTool:
        result.setInfo(T("MBHP_MF_NG Tool"), T("Allows to configure the MBHP_MF_NG firmware"), toolsCategory, 0);
        result.setTicked(mbhpMfToolWindow && mbhpMfToolWindow->isVisible());
        result.addDefaultKeypress('6', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMiosFileBrowser:
        result.setInfo(T("MIOS32 File Browser"), T("Allows to send and receive files to/from MIOS32 applications"), toolsCategory, 0);
        result.setTicked(miosFileBrowserWindow && miosFileBrowserWindow->isVisible());
        result.addDefaultKeypress('7', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMiosStudioPage:
        result.setInfo(T("MIOS Studio Page (Web)"), T("Opens the MIOS Studio page on uCApps.de"), helpCategory, 0);
        result.addDefaultKeypress ('H', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showTroubleshootingPage:
        result.setInfo(T("MIDI Troubleshooting Page (Web)"), T("Opens the MIDI Troubleshooting page on uCApps.de"), helpCategory, 0);
        result.addDefaultKeypress ('I', ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;
    }
}

// this is the ApplicationCommandTarget method that is used to actually perform one of our commands..
bool MiosStudio::perform(const InvocationInfo& info)
{
    switch( info.commandID ) {
    case enableMonitors:
        if( verticalDividerBarMonitors->isVisible() ) {
            midiInMonitor->setVisible(false);
            verticalDividerBarMonitors->setVisible(false);
            midiOutMonitor->setVisible(false);
        } else {
            midiInMonitor->setVisible(true);
            verticalDividerBarMonitors->setVisible(true);
            midiOutMonitor->setVisible(true);
        }
        updateLayout();
        break;

    case enableUpload:        
        if( horizontalDividerBar1->isVisible() ) {
            horizontalDividerBar1->setVisible(false);
            uploadWindow->setVisible(false);
        } else {
            horizontalDividerBar1->setVisible(true);
            uploadWindow->setVisible(true);
        }
        updateLayout();
        break;

    case enableTerminal:
        if( horizontalDividerBar2->isVisible() ) {
            horizontalDividerBar2->setVisible(false);
            miosTerminal->setVisible(false);
        } else {
            horizontalDividerBar2->setVisible(true);
            miosTerminal->setVisible(true);
        }
        updateLayout();
        break;

    case enableKeyboard:
        if( horizontalDividerBar3->isVisible() ) {
            horizontalDividerBar3->setVisible(false);
            midiKeyboard->setVisible(false);
        } else {
            horizontalDividerBar3->setVisible(true);
            midiKeyboard->setVisible(true);
        }
        updateLayout();
        break;

    case rescanDevices:
        // TK: doesn't always work, therefore some warnings ;-)
        if( AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                         T("Rescan MIDI Devices"),
                                         T("Please note that the rescan function\nmostly doesn't work properly!\nIt's better to restart MIOS Studio!\n"),
                                         T("I've no idea what this means"),
                                         T("Understood")) ) {
            if( AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                             T("Rescan MIDI Devices"),
                                             T("This means that it's better to quit MIOS Studio now, and open it again!\n"),
                                             T("I've still no idea what this means"),
                                             T("Understood")) ) {
                AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                             T("Rescan MIDI Devices"),
                                             T("Switched to Newbie Mode:\n"),
                                             T("Quit"),
                                             T("Quit"));
                // ;-)
                JUCEApplication::quit();
            }
        }
        
        closeMidiPorts();
        initialMidiScanCounter = 1;
        break;

    case showSysexTool:
        if( !sysexToolWindow )
            sysexToolWindow = new SysexToolWindow(this);
        sysexToolWindow->setVisible(true);
        sysexToolWindow->toFront(true);
        break;

    case showOscTool:
        if( !oscToolWindow )
            oscToolWindow = new OscToolWindow(this);
        oscToolWindow->setVisible(true);
        oscToolWindow->toFront(true);
        break;

    case showMidio128Tool:
        if( !midio128ToolWindow )
            midio128ToolWindow = new Midio128ToolWindow(this);
        midio128ToolWindow->setVisible(true);
        midio128ToolWindow->toFront(true);
        break;

    case showMbCvTool:
        if( !mbCvToolWindow )
            mbCvToolWindow = new MbCvToolWindow(this);
        mbCvToolWindow->setVisible(true);
        mbCvToolWindow->toFront(true);
        break;

    case showMbhpMfTool:
        if( !mbhpMfToolWindow )
            mbhpMfToolWindow = new MbhpMfToolWindow(this);
        mbhpMfToolWindow->setVisible(true);
        mbhpMfToolWindow->toFront(true);
        break;

    case showSysexLibrarian:
        if( !sysexLibrarianWindow )
            sysexLibrarianWindow = new SysexLibrarianWindow(this);
        sysexLibrarianWindow->setVisible(true);
        sysexLibrarianWindow->toFront(true);
        break;

    case showMiosFileBrowser:
        if( !miosFileBrowserWindow )
            miosFileBrowserWindow = new MiosFileBrowserWindow(this);
        miosFileBrowserWindow->setVisible(true);
        miosFileBrowserWindow->toFront(true);
        break;

    case showMiosStudioPage: {
        URL webpage(T("http://www.uCApps.de/mios_studio.html"));
        webpage.launchInDefaultBrowser();
    }  break;

    case showTroubleshootingPage: {
        URL webpage(T("http://www.uCApps.de/howto_debug_midi.html"));
        webpage.launchInDefaultBrowser();
    } break;
    default:
        return false;
    }

    return true;
};


void MiosStudio::updateLayout(void)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    layoutHComps.clear();
    horizontalLayout.clearAllItems();

    int itemIx = 0;
    if( verticalDividerBarMonitors->isVisible() ) {
        horizontalLayout.setItemLayout(itemIx++,    50,   -1, -1); // MIDI In/Out Monitors
        layoutHComps.add(0);
    }

    if( uploadWindow->isVisible() ) {
        if( itemIx ) {
            horizontalLayout.setItemLayout(itemIx++,    8,      8,     8); // Resizer
            layoutHComps.add(horizontalDividerBar1);
        }

        horizontalLayout.setItemLayout(itemIx++,   186,    186,  186); // Upload Window
        layoutHComps.add(uploadWindow);
    }

    if( miosTerminal->isVisible() ) {
        if( itemIx ) {
            horizontalLayout.setItemLayout(itemIx++,    8,      8,     8); // Resizer
            layoutHComps.add(horizontalDividerBar2);
        }

        horizontalLayout.setItemLayout(itemIx++,   50,   -1, -1); // MIOS Terminal
        layoutHComps.add(miosTerminal);

    }

    if( midiKeyboard->isVisible() ) {
        if( itemIx ) {
            horizontalLayout.setItemLayout(itemIx++,    8,      8,     8); // Resizer
            layoutHComps.add(horizontalDividerBar3);
        }

        horizontalLayout.setItemLayout(itemIx++,   124,    124,  124); // MIDI Keyboard
        layoutHComps.add(midiKeyboard);
    }

    // dummy to ensure that MIDI keyboard or upload window is displayed with right size if all other components invisible
    horizontalLayout.setItemLayout(itemIx++,   0, 0, 0);
    layoutHComps.add(0);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    layoutVComps.clear();
    verticalLayoutMonitors.clearAllItems();
    if( verticalDividerBarMonitors->isVisible() ) {
        //                                   num  min   max   prefered  
        verticalLayoutMonitors.setItemLayout(0, -0.2, -0.8, -0.5); // MIDI In Monitor
        layoutVComps.add(midiInMonitor);

        verticalLayoutMonitors.setItemLayout(1,    8,    8,    8); // resizer
        layoutVComps.add(verticalDividerBarMonitors);

        verticalLayoutMonitors.setItemLayout(2, -0.2, -0.8, -0.5); // MIDI Out Monitor
        layoutVComps.add(midiOutMonitor);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    resized();
}

