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

//==============================================================================
MiosStudio::MiosStudio()
    : uploadWindow(0)
    , midiInMonitor(0)
    , midiOutMonitor(0)
    , miosTerminal(0)
    , midiKeyboard(0)
    , initialMidiScanCounter(1) // start step-wise MIDI port scan
{
    uploadHandler = new UploadHandler(this);

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

    commandManager = new ApplicationCommandManager();
    commandManager->registerAllCommandsForTarget(this);
    commandManager->registerAllCommandsForTarget(JUCEApplication::getInstance());
    addKeyListener(commandManager->getKeyMappings());
    setApplicationCommandManagerToWatch(commandManager);

    //                             num   min   max  prefered  
#if 0
    horizontalLayout.setItemLayout(0, -0.005, -0.9, -0.25); // MIDI In/Out Monitors
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2, -0.005, -0.9, -0.30); // Upload Window
    horizontalLayout.setItemLayout(3,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(4, -0.005, -0.9, -0.25); // MIOS Terminal
    horizontalLayout.setItemLayout(5,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(6, -0.005, -0.2, -0.20); // MIDI Keyboard
#else
    // new: fixed size of Upload window and MIDI keyboard window, so that MIDI IN/OUT and MIOS Terminal can be enlarged easier
    horizontalLayout.setItemLayout(0,    50, -0.9, -0.25); // MIDI In/Out Monitors
    horizontalLayout.setItemLayout(1,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(2,   186,    186,  186); // Upload Window
    horizontalLayout.setItemLayout(3,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(4,   50, -0.9, -0.25); // MIOS Terminal
    horizontalLayout.setItemLayout(5,    8,      8,     8); // Resizer
    horizontalLayout.setItemLayout(6,   124,    124,  124); // MIDI Keyboard
#endif

    horizontalDividerBar1 = new StretchableLayoutResizerBar(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalDividerBar1);
    horizontalDividerBar2 = new StretchableLayoutResizerBar(&horizontalLayout, 3, false);
    addAndMakeVisible(horizontalDividerBar2);
    horizontalDividerBar3 = new StretchableLayoutResizerBar(&horizontalLayout, 5, false);
    addAndMakeVisible(horizontalDividerBar3);

    //                           num  min   max   prefered  
    verticalLayoutMonitors.setItemLayout(0, -0.2, -0.8, -0.5); // MIDI In Monitor
    verticalLayoutMonitors.setItemLayout(1,    8,    8,    8); // resizer
    verticalLayoutMonitors.setItemLayout(2, -0.2, -0.8, -0.5); // MIDI Out Monitor

    verticalDividerBarMonitors = new StretchableLayoutResizerBar(&verticalLayoutMonitors, 1, true);
    addAndMakeVisible(verticalDividerBarMonitors);

    resizeLimits.setSizeLimits(200, 100, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    Timer::startTimer(1);

    setSize(800, 650);
}

MiosStudio::~MiosStudio()
{
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

    // try: avoid crash under Windows by disabling all MIDI INs/OUTs
    closeMidiPorts();
}

//==============================================================================
void MiosStudio::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void MiosStudio::resized()
{
    Component* hcomps[] = { 0,
                            horizontalDividerBar1,
                            uploadWindow,
                            horizontalDividerBar2,
                            miosTerminal,
                            horizontalDividerBar3,
                            midiKeyboard
    };

    horizontalLayout.layOutComponents(hcomps, 7,
                                       4, 4,
                                       getWidth() - 8, getHeight() - 8,
                                       true,  // lay out above each other
                                       true); // resize the components' heights as well as widths

    Component* vcomps[] = { midiInMonitor, verticalDividerBarMonitors, midiOutMonitor };

    verticalLayoutMonitors.layOutComponents(vcomps, 3,
                                            4,
                                            4 + horizontalLayout.getItemCurrentPosition(0),
                                            getWidth() - 8,
                                            horizontalLayout.getItemCurrentAbsoluteSize(0),
                                            false, // lay out side-by-side
                                            true); // resize the components' heights as well as widths

    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}


//==============================================================================
void MiosStudio::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    uint8 *data = message.getRawData();
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
            midiInMonitor->scanMidiDevices();
            ++initialMidiScanCounter;
            break;

        case 2:
            midiOutMonitor->scanMidiDevices();
            ++initialMidiScanCounter;
            break;

        case 3:
            audioDeviceManager.addMidiInputCallback(String::empty, this);
            if( getMidiOutput() != String::empty )
                uploadWindow->queryCore();

            initialMidiScanCounter = 0; // stop scan
            break;
        }
    } else {
        // important: only broadcast 1..5 messages per timer tick to avoid GUI hangups when
        // a large bulk of data is received

        for(int checkLoop=0; checkLoop<5; ++checkLoop) {
            if( !midiInQueue.empty() ) {
                const ScopedLock sl(midiInQueueLock); // lock will be released at end of this scope

                MidiMessage &message = midiInQueue.front();

                uint8 *data = message.getRawData();
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

    // store setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile )
        propertiesFile->setValue(T("midiIn"), port);
}

String MiosStudio::getMidiInput(void)
{
    // restore setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    return propertiesFile ? propertiesFile->getValue(T("midiIn"), String::empty) : String::empty;
}

void MiosStudio::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);

    // propagate port change
    if( uploadWindow && initialMidiScanCounter == 0 && port != String::empty )
        uploadWindow->midiPortChanged();

    // store setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile )
        propertiesFile->setValue(T("midiOut"), port);
}

String MiosStudio::getMidiOutput(void)
{
    // restore setting
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    return propertiesFile ? propertiesFile->getValue(T("midiOut"), String::empty) : String::empty;
}




//==============================================================================
const StringArray MiosStudio::getMenuBarNames()
{
    const tchar* const names[] = { T("Application"), T("Tools"), T("Help"), 0 };

    return StringArray ((const tchar**) names);
}

const PopupMenu MiosStudio::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
    PopupMenu menu;

    if( topLevelMenuIndex == 0 ) {
        // "Application" menu
        menu.addCommandItem(commandManager, rescanDevices);
        menu.addSeparator();
        menu.addCommandItem(commandManager, StandardApplicationCommandIDs::quit);
    } else if( topLevelMenuIndex == 1 ) {
        // "Tools" menu
        menu.addCommandItem(commandManager, showSysexTool);
        menu.addCommandItem(commandManager, showOscTool);
        menu.addCommandItem(commandManager, showMidio128Tool);
        menu.addCommandItem(commandManager, showMbCvTool);
        menu.addCommandItem(commandManager, showMbhpMfTool);
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
    case rescanDevices:
        result.setInfo(T("Rescan MIDI Devices"), T("Updates the MIDI In/Out port lists"), applicationCategory, 0);
        result.addDefaultKeypress(T('R'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showSysexTool:
        result.setInfo(T("SysEx Tool"), T("Allows to send and receive SysEx dumps"), toolsCategory, 0);
        result.setTicked(sysexToolWindow && sysexToolWindow->isVisible());
        result.addDefaultKeypress(T('1'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showOscTool:
        result.setInfo(T("OSC Tool"), T("Allows to send and receive OSC messages"), toolsCategory, 0);
        result.setTicked(oscToolWindow && oscToolWindow->isVisible());
        result.addDefaultKeypress(T('1'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMidio128Tool:
        result.setInfo(T("MIDIO128 V2 Tool"), T("Allows to configure a MIDIO128 V2"), toolsCategory, 0);
        result.setTicked(midio128ToolWindow && midio128ToolWindow->isVisible());
        result.addDefaultKeypress(T('2'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMbCvTool:
        result.setInfo(T("MIDIbox CV V1 Tool"), T("Allows to configure a MIDIbox CV V1"), toolsCategory, 0);
        result.setTicked(mbCvToolWindow && mbCvToolWindow->isVisible());
        result.addDefaultKeypress(T('3'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMbhpMfTool:
        result.setInfo(T("MBHP_MF_NG Tool"), T("Allows to configure the MBHP_MF_NG firmware"), toolsCategory, 0);
        result.setTicked(mbhpMfToolWindow && mbhpMfToolWindow->isVisible());
        result.addDefaultKeypress(T('3'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showMiosStudioPage:
        result.setInfo(T("MIOS Studio Page (Web)"), T("Opens the MIOS Studio page on uCApps.de"), helpCategory, 0);
        result.addDefaultKeypress (T('H'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;

    case showTroubleshootingPage:
        result.setInfo(T("MIDI Troubleshooting Page (Web)"), T("Opens the MIDI Troubleshooting page on uCApps.de"), helpCategory, 0);
        result.addDefaultKeypress (T('T'), ModifierKeys::commandModifier|ModifierKeys::shiftModifier);
        break;
    }
}

// this is the ApplicationCommandTarget method that is used to actually perform one of our commands..
bool MiosStudio::perform(const InvocationInfo& info)
{
    switch( info.commandID ) {
    case rescanDevices:
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
