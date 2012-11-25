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

#ifndef _MIOS_STUDIO_H
#define _MIOS_STUDIO_H

#include "../includes.h"
#include <queue>

#include "UploadWindow.h"
#include "MidiMonitor.h"
#include "MiosTerminal.h"
#include "MidiKeyboard.h"
#include "SysexTool.h"
#include "OscTool.h"
#include "Midio128Tool.h"
#include "MbCvTool.h"
#include "MbhpMfTool.h"
#include "SysexLibrarian.h"
#include "MiosFileBrowser.h"
#include "../SysexPatchDb.h"
#include "../UploadHandler.h"

class MiosStudio
    : public Component
    , public MidiInputCallback
    , public MenuBarModel
    , public ApplicationCommandTarget
    , public Timer
{
public:
    enum CommandIDs {
        rescanDevices              = 0x1000,
        showSysexTool              = 0x2000,
        showOscTool                = 0x2001,
        showMidio128Tool           = 0x2002,
        showMbCvTool               = 0x2003,
        showMbhpMfTool             = 0x2004,
        showSysexLibrarian         = 0x2005,
        showMiosFileBrowser        = 0x2006,
        showMiosStudioPage         = 0x3000,
        showTroubleshootingPage    = 0x3001,
    };

    //==============================================================================
    MiosStudio();
    ~MiosStudio();

    //==============================================================================
    void paint (Graphics& g);
    void resized();

    //==============================================================================
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);

    void timerCallback();

    void sendMidiMessage(MidiMessage &message);
    void closeMidiPorts(void);

    void setMidiInput(const String &port);
    String getMidiInput(void);
    void setMidiOutput(const String &port);
    String getMidiOutput(void);

    const StringArray getMenuBarNames();
    const PopupMenu getMenuForIndex(int topLevelMenuIndex, const String& menuName);
    void menuItemSelected(int menuItemID, int topLevelMenuIndex);

    ApplicationCommandTarget* getNextCommandTarget();
    void getAllCommands(Array <CommandID>& commands);
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
    void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result);
#else
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result);
#endif
	bool perform(const InvocationInfo& info);

    AudioDeviceManager audioDeviceManager;

    UploadHandler *uploadHandler;

    // Windows opened by Tools button in Upload Window
    SysexToolWindow *sysexToolWindow;
    OscToolWindow *oscToolWindow;
    Midio128ToolWindow *midio128ToolWindow;
    MbCvToolWindow *mbCvToolWindow;
    MbhpMfToolWindow *mbhpMfToolWindow;
    SysexLibrarianWindow *sysexLibrarianWindow;
    MiosFileBrowserWindow *miosFileBrowserWindow;

    //==============================================================================
    SysexPatchDb *sysexPatchDb;

    //==============================================================================
	// This is needed by MSVC in debug mode (please #ifdef if it causes Mac problems)
    juce_UseDebuggingNewOperator
protected:
    //==============================================================================
    UploadWindow *uploadWindow;
    MidiMonitor *midiInMonitor;
    MidiMonitor *midiOutMonitor;
    MiosTerminal *miosTerminal;
    MidiKeyboard *midiKeyboard;

    StretchableLayoutManager horizontalLayout;
    StretchableLayoutResizerBar* horizontalDividerBar1;
    StretchableLayoutResizerBar* horizontalDividerBar2;
    StretchableLayoutResizerBar* horizontalDividerBar3;

    StretchableLayoutManager verticalLayoutMonitors;
    StretchableLayoutResizerBar* verticalDividerBarMonitors;

    ResizableCornerComponent *resizer;
    ComponentBoundsConstrainer resizeLimits;

    // TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiInQueue;
    CriticalSection midiInQueueLock;
    uint8 runningStatus;

    std::queue<MidiMessage> midiOutQueue;
    CriticalSection midiOutQueueLock;

    Array<uint8> sysexReceiveBuffer;

    int initialMidiScanCounter;

    // the command manager object used to dispatch command events
    ApplicationCommandManager* commandManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosStudio (const MiosStudio&);
    const MiosStudio& operator= (const MiosStudio&);
};

#endif /* _MIOS_STUDIO_H */
