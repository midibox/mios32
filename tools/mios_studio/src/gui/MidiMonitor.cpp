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

#include "MidiMonitor.h"
#include "MiosStudio.h"


//==============================================================================
MidiMonitor::MidiMonitor(MiosStudio *_miosStudio, const bool _inPort)
    : miosStudio(_miosStudio)
    , inPort(_inPort)
    , filterMidiClock(1)
    , filterActiveSense(1)
    , filterMiosTerminalMessage(1)
    , cutLongMessages(1)
{
	addAndMakeVisible(midiPortSelector = new ComboBox(String::empty));
	midiPortSelector->addListener(this);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< device scan running >>"), -1);
    midiPortSelector->setSelectedId(-1, true);
    midiPortSelector->setEnabled(false);
	midiPortLabel = new Label("", inPort ? T("MIDI IN: ") : T("MIDI OUT: "));
	midiPortLabel->attachToComponent(midiPortSelector, true);

    addAndMakeVisible(monitorLogBox = new LogBox(T("Midi Monitor")));
    monitorLogBox->addEntry(Colours::red, T("Connecting to MIDI driver - be patient!"));

    setSize(400, 200);
}

MidiMonitor::~MidiMonitor()
{
}

//==============================================================================
// Should be called after startup once window is visible
void MidiMonitor::scanMidiDevices()
{
    monitorLogBox->clear();

    midiPortSelector->setEnabled(false);
	midiPortSelector->clear();
    midiPortSelector->addItem (TRANS("<< none >>"), -1);
    midiPortSelector->addSeparator();

    // restore settings
    String selectedPort;
    StringArray midiPorts;
    if( inPort ) {
        monitorLogBox->addEntry(Colours::red, T("Scanning for MIDI Inputs..."));
        selectedPort = miosStudio->getMidiInput();
        midiPorts = MidiInput::getDevices();
    } else {
        monitorLogBox->addEntry(Colours::red, T("Scanning for MIDI Outputs..."));
        selectedPort = miosStudio->getMidiOutput();
        midiPorts = MidiOutput::getDevices();
    }

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiPortSelector->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled )
            current = i + 1;

        monitorLogBox->addEntry(Colours::blue, "[" + String(i+1) + "] " + midiPorts[i] + (enabled ? " (*)" : ""));

        if( inPort )
            miosStudio->audioDeviceManager.setMidiInputEnabled(midiPorts[i], enabled);
        else if( enabled )
            miosStudio->audioDeviceManager.setDefaultMidiOutput(midiPorts[i]);
    }
    midiPortSelector->setSelectedId(current, true);
    midiPortSelector->setEnabled(true);

    if( current == -1 ) {
        if( inPort ) {
            miosStudio->setMidiInput(String::empty);
        } else {
            miosStudio->setMidiOutput(String::empty);
        }
    }

    monitorLogBox->addEntry(Colours::grey, T("MIDI Monitor ready."));
}

//==============================================================================
void MidiMonitor::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void MidiMonitor::resized()
{
    midiPortLabel->setBounds(4, 4, 60, 22);
	midiPortSelector->setBounds(4+60, 4, getWidth()-8-60, 22);
    monitorLogBox->setBounds(4, 4+22+4, getWidth()-8, getHeight()-(4+22+4+4));
}


//==============================================================================
void MidiMonitor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiPortSelector ) {
        String portName = midiPortSelector->getText();
        if( portName == T("<< none >>") )
            portName = String::empty;

        if( inPort )
            miosStudio->setMidiInput(portName);
        else
            miosStudio->setMidiOutput(portName);
    }
}


//==============================================================================
String MidiMonitor::getNoteString(uint8 note)
{
    String strBuffer;
    const char note_tab[12][3] = { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };
    
    // determine octave, note contains semitone number thereafter
    uint8 octave = note / 12;
    note %= 12;
        
    // octave
    char octaveChr;
    switch( octave ) {
        case 0:  octaveChr = '2'; break; // -2
        case 1:  octaveChr = '1'; break; // -1
        default: octaveChr = '0' + (octave-2); // 0..7
    }

    // semitone (capital letter if octave >= 2)
    strBuffer = String::formatted(T("%c%c%c"), octave >= 2 ? (note_tab[note][0] + 'A'-'a') : note_tab[note][0],
                                  note_tab[note][1],
                                  octaveChr);
    
    return strBuffer;
}

//==============================================================================
void MidiMonitor::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint32 size = message.getRawDataSize();
    uint8 *data = (uint8 *)message.getRawData();

    bool isMidiClock = data[0] == 0xf8;
    bool isActiveSense = data[0] == 0xfe;
    bool isMiosTerminalMessage = SysexHelper::isValidMios32DebugMessage(data, size, -1);

    if( !(isMidiClock && filterMidiClock) &&
        !(isActiveSense && filterActiveSense) &&
        !(isMiosTerminalMessage && filterMiosTerminalMessage) ) {

        double timeStamp = message.getTimeStamp() ? message.getTimeStamp() : ((double)Time::getMillisecondCounter() / 1000.0);
        String timeStampStr = (timeStamp > 0)
            ? String::formatted(T("%8.3f"), timeStamp)
            : T("now");

        String hexStr = String::toHexString(data, size);

        String descStr;
        switch( data[0] & 0xf0 ) {
            case 0x80:
                descStr = String::formatted(T("   Chn#%2d  Note Off "), (data[0] & 0x0f) + 1);
                descStr += getNoteString(data[1]);
                descStr += String::formatted(T("  Vel:%d"), data[2]);
                break;

            case 0x90:
                if( data[2] == 0 ) {
                    descStr = String::formatted(T("   Chn#%2d  Note Off "), (data[0] & 0x0f) + 1);
                    descStr += getNoteString(data[1]) + " (optimized)";
                } else {
                    descStr = String::formatted(T("   Chn#%2d  Note On  "), (data[0] & 0x0f) + 1);
                    descStr += getNoteString(data[1]);
                    descStr += String::formatted(T("  Vel:%d"), data[2]);
                }
                break;
                
            case 0xa0:
                descStr = String::formatted(T("   Chn#%2d  Aftertouch "), (data[0] & 0x0f) + 1);
                descStr += getNoteString(data[1]);
                descStr += String::formatted(T(" %d"), data[2]);
                break;
                
            case 0xb0:
                descStr = String::formatted(T("   Chn#%2d  CC#%3d = %d"),
                                            (data[0] & 0x0f) + 1,
                                            data[1],
                                            data[2]);
                break;
                
            case 0xc0:
                descStr = String::formatted(T("   Chn#%2d  Program Change %d"),
                                            (data[0] & 0x0f) + 1,
                                            data[1]);
                break;
                
            case 0xd0:
                descStr = String::formatted(T("   Chn#%2d  Aftertouch "), (data[0] & 0x0f) + 1);
                descStr += getNoteString(data[1]);
                break;
                
            case 0xe0:
                descStr = String::formatted(T("   Chn#%2d  Pitchbend %d"),
                                            (data[0] & 0x0f) + 1,
                                            (int)((data[1] & 0x7f) | ((data[2] & 0x7f) << 7)) - 8192);
                break;
                
            default:
                descStr = String::formatted(T("")); // nothing to add here
        }
        
        monitorLogBox->addEntry(Colours::black, "[" + timeStampStr + "] " + hexStr + descStr);
    }
}
