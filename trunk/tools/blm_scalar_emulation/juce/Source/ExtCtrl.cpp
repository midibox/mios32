/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * External Controller Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "ExtCtrl.h"
#include "MainComponent.h"


ExtCtrlDevice::ExtCtrlDevice(ExtCtrl *_extCtrl, const unsigned& _deviceId)
    : extCtrl(_extCtrl)
    , deviceId(_deviceId)
    , enabledMidiInput(NULL)
    , enabledMidiOutput(NULL)
{
    addAndMakeVisible(midiInput = new ComboBox ("MIDI Inputs"));
    midiInput->setEditableText(false);
    midiInput->setJustificationType(Justification::centredLeft);
    midiInput->setTextWhenNothingSelected(String::empty);
    midiInput->setTextWhenNoChoicesAvailable("(no choices)");
    midiInput->addItem(TRANS("<< device scan running >>"), -1);
    midiInput->addListener(this);

    addAndMakeVisible(midiInLabel = new Label("", String("MIDI IN") + String(deviceId+1) + String(": ")));
    midiInLabel->attachToComponent(midiInput, true);

    addAndMakeVisible(midiOutput = new ComboBox ("MIDI Output"));
    midiOutput->setEditableText(false);
    midiOutput->setJustificationType(Justification::centredLeft);
    midiOutput->setTextWhenNothingSelected(String::empty);
    midiOutput->setTextWhenNoChoicesAvailable("(no choices)");
    midiOutput->addItem(TRANS("<< device scan running >>"), -1);
    midiOutput->addListener(this);

    addAndMakeVisible(midiOutLabel = new Label("", String("MIDI OUT") + String(deviceId+1) + String(": ")));
    midiOutLabel->attachToComponent(midiOutput, true);

    addAndMakeVisible(rotationSelection = new ComboBox ("Rotation"));
    rotationSelection->setEditableText(false);
    rotationSelection->setJustificationType(Justification::centredLeft);
    rotationSelection->setTextWhenNothingSelected(String::empty);
    rotationSelection->setTextWhenNoChoicesAvailable("(no choices)");
    rotationSelection->addItem(TRANS("0 Degree"), 1);
    rotationSelection->addItem(TRANS("90 Degree"), 2);
    rotationSelection->addItem(TRANS("180 Degree"), 3);
    rotationSelection->addItem(TRANS("270 Degree"), 4);
    rotationSelection->setSelectedId(1, false);
    rotationSelection->addListener(this);

    addAndMakeVisible(rotationLabel = new Label("", "Rotation: "));
    rotationLabel->attachToComponent(rotationSelection, true);

    // restore settings
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        String rotation = propertiesFile->getValue("rotation" + String(deviceId+1), String::empty);
        for(int i=0; i<rotationSelection->getNumItems(); ++i) {
            if( rotationSelection->getItemText(i) == rotation ) {
                rotationSelection->setSelectedItemIndex(i, false);
                break;
            }
        }
    }

    setSize(800, 24);
}

ExtCtrlDevice::~ExtCtrlDevice()
{
    if( enabledMidiInput )
        deleteAndZero(enabledMidiInput);
    if( enabledMidiOutput )
        deleteAndZero(enabledMidiOutput);
}

//==============================================================================
void ExtCtrlDevice::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void ExtCtrlDevice::resized()
{
    int part = getWidth() / 3;
    midiInput->setBounds(80, 0, 1*part-10-80, 24);
    midiOutput->setBounds(1*part+80, 0, 1*part-10-80, 24);
    rotationSelection->setBounds(2*part+80, 0, part-10-80, 24);
}

//==============================================================================
void ExtCtrlDevice::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == midiInput ) {
        setMidiInput(midiInput->getText());
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(String("extCtrlMidiIn") + String(deviceId+1), midiInput->getText());
        extCtrl->mainComponent->blmClass->sendBLMLayout();
    } else if( comboBoxThatHasChanged == midiOutput ) {
        setMidiOutput(midiOutput->getText());
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(String("extCtrlMidiOut") + String(deviceId+1), midiOutput->getText());
        extCtrl->mainComponent->blmClass->sendBLMLayout();
    } else if( comboBoxThatHasChanged == rotationSelection ) {
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue(String("rotation") + String(deviceId+1), rotationSelection->getText());
        extCtrl->mainComponent->blmClass->sendBLMLayout();
    }
}


//==============================================================================
void ExtCtrlDevice::scanMidiInDevices()
{
    midiInput->setEnabled(false);
    midiInput->clear();
    midiInput->addItem (TRANS("<< none >>"), -1);
    midiInput->addSeparator();

    // restore MIDI input port from .xml file
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue(String("extCtrlMidiIn") + String(deviceId+1), String::empty) : String::empty;

    StringArray midiPorts = MidiInput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiInput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled )
            current = i + 1;

        if( enabled ) {
            setMidiInput(midiPorts[i]);
        }
    }
    midiInput->setSelectedId(current, true);
    midiInput->setEnabled(true);
}

void ExtCtrlDevice::scanMidiOutDevices()
{
    midiOutput->setEnabled(false);
    midiOutput->clear();
    midiOutput->addItem (TRANS("<< none >>"), -1);
    midiOutput->addSeparator();

    // restore MIDI output port from .xml file
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue(String("extCtrlMidiOut") + String(deviceId+1), String::empty) : String::empty;

    StringArray midiPorts = MidiOutput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiOutput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled ) {
            current = i + 1;
            setMidiOutput(midiPorts[i]);
        }
    }
    midiOutput->setSelectedId(current, true);
    midiOutput->setEnabled(true);
}

void ExtCtrlDevice::setMidiOutput(const String &port)
{
    if( enabledMidiOutput != NULL ) {
        deleteAndZero(enabledMidiOutput);
    }

    const int index = MidiOutput::getDevices().indexOf(port);
    if( index >= 0 ) {
        enabledMidiOutput = MidiOutput::openDevice(index);
    }
}

void ExtCtrlDevice::setMidiInput(const String &port)
{
    if( enabledMidiInput != NULL ) {
        deleteAndZero(enabledMidiInput);
    }

    const int index = MidiInput::getDevices().indexOf(port);
    if( index >= 0 ) {
        if( enabledMidiInput = MidiInput::openDevice(index, this) ) {
            enabledMidiInput->start();
        }
    }
}


//==============================================================================
void ExtCtrlDevice::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    extCtrl->handleIncomingMidiMessage(deviceId, message);
}

void ExtCtrlDevice::sendMidiMessage(const MidiMessage& message)
{
    if( enabledMidiOutput ) {
        enabledMidiOutput->sendMessageNow(message);
    }
}



//==============================================================================
//==============================================================================
//==============================================================================
ExtCtrl::ExtCtrl(MainComponent *_mainComponent)
    : mainComponent(_mainComponent)
{
    addAndMakeVisible(controllerSelection = new ComboBox ("Controller"));
    controllerSelection->setEditableText(false);
    controllerSelection->setJustificationType(Justification::centredLeft);
    controllerSelection->setTextWhenNothingSelected(String::empty);
    controllerSelection->setTextWhenNoChoicesAvailable("(no choices)");
    controllerSelection->addItem(TRANS("<< none >>"), -1);
    controllerSelection->addItem(TRANS("Novation Launchpad"), 1);
    controllerSelection->setSelectedId(-1, false);
    controllerSelection->addListener(this);

    addAndMakeVisible(controllerLabel = new Label("", "Controller: "));
    controllerLabel->attachToComponent(controllerSelection, true);

    for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
        addAndMakeVisible(extCtrlDevice[deviceId] = new ExtCtrlDevice(this, deviceId));
    }

    // done in MainComponent::timerCallback()
    //scanMidiInDevices();
    //scanMidiOutDevices();

    // restore settings
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        String layout = propertiesFile->getValue("controller", String::empty);
        for(int i=0; i<controllerSelection->getNumItems(); ++i) {
            if( controllerSelection->getItemText(i) == layout ) {
                controllerSelection->setSelectedItemIndex(i, false);
                break;
            }
        }
    }

    setSize(800, 110);
}

ExtCtrl::~ExtCtrl()
{
    for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
        deleteAndZero(extCtrlDevice[deviceId]);
    }
}

//==============================================================================
void ExtCtrl::paint (Graphics& g)
{
    g.fillAll(Colour(0xffc1d0ff));
}

void ExtCtrl::resized()
{
    controllerSelection->setBounds(80 + 5, 5, 150, 24);

    for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
        ExtCtrlDevice *d = extCtrlDevice[deviceId];
        d->setBounds(5, 5 + deviceId*d->getHeight() + 30, d->getWidth(), d->getHeight());
    }
}

//==============================================================================
void ExtCtrl::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == controllerSelection ) {
        int selectedIndex = controllerSelection->getSelectedItemIndex();
        if( selectedIndex >= 0 ) {
            String controller(controllerSelection->getItemText(selectedIndex));

            PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
            if( propertiesFile )
                propertiesFile->setValue("controller", controller);
        }
    }
}


//==============================================================================
void ExtCtrl::scanMidiInDevices()
{
    for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
        extCtrlDevice[deviceId]->scanMidiInDevices();
    }
}

void ExtCtrl::scanMidiOutDevices()
{
    for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
        extCtrlDevice[deviceId]->scanMidiOutDevices();
    }
}

//==============================================================================
void ExtCtrl::handleIncomingMidiMessage(const unsigned& deviceId, const MidiMessage& message)
{
	uint8 *data = (uint8 *)message.getRawData();
	uint8 event_type = data[0] >> 4;
	uint8 chn = data[0] & 0xf;
	uint8 evnt1 = data[1];
	uint8 evnt2 = data[2];

#if 0
	fprintf(stderr,"[%d] Chn:0x%02x, event_type: 0x%02x, evnt1: 0x%02x, evnt2: 0x%02x\n",ix,chn,event_type,evnt1,evnt2);
    fflush(stderr);
#endif

    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        if( event_type == 0x09 && chn == 0 ) {
            if( (evnt1 & 0x0f) == 8 ) {
                // Button A..H
                int b = evnt1 >> 4;
                //printf("Button %c\n", 'A' + b);

                switch( extCtrlDevice[deviceId]->getRotation() ) {
                case ExtCtrlDevice::Rotation_0: {
                    unsigned mappedX = 0x40 + 0x10*(deviceId % 2);
                    unsigned mappedY = b + 8*(deviceId / 2);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_90: {
                    unsigned mappedX = 0x60 + 8*(deviceId % 2) + (7-b) + 8*(deviceId / 2);
                    unsigned mappedY = 0;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_180: {
                    unsigned mappedX = 0x40 + 0x10*(deviceId % 2);
                    unsigned mappedY = (7-b) + 8*(deviceId / 2);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_270: {
                    unsigned mappedX = 0x60 + 8*(deviceId % 2) + b + 8*(deviceId / 2);
                    unsigned mappedY = 0;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                }
            } else if( (evnt1 & 0x0f) <= 7 ) {
                // grid button
                int x = evnt1 & 0x07;
                int y = evnt1 >> 4;
                //printf("Button %d.%d\n", x+1, y+1);
                
                switch( extCtrlDevice[deviceId]->getRotation() ) {
                case ExtCtrlDevice::Rotation_0: {
                    unsigned mappedX = 8*(deviceId % 2) + x;
                    unsigned mappedY = 8*(deviceId / 2) + y;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_90: {
                    unsigned mappedX = 8*(deviceId / 2) + (7-y);
                    unsigned mappedY = 8*(deviceId % 2) + x;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_180: {
                    unsigned mappedX = 8*(deviceId % 2) + (7-x);
                    unsigned mappedY = 8*(deviceId / 2) + (7-y);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_270: {
                    unsigned mappedX = 8*(deviceId / 2) + y;
                    unsigned mappedY = 8*(deviceId % 2) + (7-x);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                }
            }
        } else if( event_type == 0x0b && chn == 0 ) {
            if( evnt1 >= 0x68 && evnt1 <= 0x6f ) {
                // Button 1..8
                int b = evnt1 & 0x07;
                //printf("Button %d\n", b+1);

                switch( extCtrlDevice[deviceId]->getRotation() ) {
                case ExtCtrlDevice::Rotation_0: {
                    unsigned mappedX = 0x60 + 8*(deviceId % 2) + b + 8*(deviceId / 2);
                    unsigned mappedY = 0;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_90: {
                    unsigned mappedX = 0x40 + 0x10*(deviceId % 2);
                    unsigned mappedY = b + 8*(deviceId / 2);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_180: {
                    unsigned mappedX = 0x60 + 8*(deviceId % 2) + (7-b) + 8*(deviceId / 2);
                    unsigned mappedY = 0;
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                case ExtCtrlDevice::Rotation_270: {
                    unsigned mappedX = 0x40 + 0x10*(deviceId % 2);
                    unsigned mappedY = (7-b) + 8*(deviceId / 2);
                    mainComponent->blmClass->sendNoteEvent(mappedY, mappedX, evnt2);
                } break;
                }
            }
        }
    } break;
    }
}


void ExtCtrl::sendMidiMessage(const unsigned& deviceId, const MidiMessage& message)
{
    if( deviceId < EXT_CTRL_MAX_DEVICES )
        extCtrlDevice[deviceId]->sendMidiMessage(message);
}

//==============================================================================
static unsigned convertLedState(const unsigned& ledState)
{
    switch( ledState ) {
    case 1: return 0x3c;
    case 2: return 0x0f;
    case 3: return 0x3f;
    }
    return 0x00;
}


void ExtCtrl::sendGridLed(const unsigned& x, const unsigned& y, const unsigned& ledState)
{
    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        unsigned mappedCol = x & 7;
        unsigned mappedRow = y & 7;
        unsigned mappedLedState = convertLedState(ledState);

        int mappedDeviceId = -1;
        if( x <= 7 && y <= 7 ) {
            mappedDeviceId = 0;
        } else if( x <= 15 && y <= 7 ) {
            mappedDeviceId = 1;
        } else if( x <= 7 && y <= 15 ) {
            mappedDeviceId = 2;
        } else if( x <= 15 && y <= 15 ) {
            mappedDeviceId = 3;
        }

        if( mappedDeviceId >= 0 ) {
            switch( extCtrlDevice[mappedDeviceId]->getRotation() ) {
            case ExtCtrlDevice::Rotation_0: {
                MidiMessage message(0x90, mappedCol + mappedRow*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_90: {
                MidiMessage message(0x90, mappedRow + (7-mappedCol)*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_180: {
                MidiMessage message(0x90, (7-mappedCol) + (7-mappedRow)*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_270: {
                MidiMessage message(0x90, (7-mappedRow) + mappedCol*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            }
        }
    } break;
    }
}

void ExtCtrl::sendExtraColumnLed(const unsigned& x, const unsigned& y, const unsigned& ledState)
{
    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        unsigned mappedLedState = convertLedState(ledState);
        unsigned mappedValue = y & 7;

        int mappedDeviceId = -1;
        if( x == 0 && y <= 7 ) {
            mappedDeviceId = 0;
        } else if( x == 1 && y <= 7 ) {
            mappedDeviceId = 1;
        } else if( x == 0 && y <= 15 ) {
            mappedDeviceId = 2;
        } else if( x == 1 && y <= 15 ) {
            mappedDeviceId = 3;
        }

        if( mappedDeviceId >= 0 ) {
            switch( extCtrlDevice[mappedDeviceId]->getRotation() ) {
            case ExtCtrlDevice::Rotation_0: {
                MidiMessage message(0x90, 0x08 + mappedValue*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_90: {
                MidiMessage message(0xb0, 0x68 + mappedValue, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_180: {
                MidiMessage message(0x90, 0x08 + (7-mappedValue)*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_270: {
                MidiMessage message(0xb0, 0x68 + (7 - mappedValue), mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            }
        }
    } break;
    }
}

void ExtCtrl::sendExtraRowLed(const unsigned& x, const unsigned& y, const unsigned& ledState)
{
    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        unsigned mappedValue = x & 7;
        unsigned mappedLedState = convertLedState(ledState);

        int mappedDeviceId = -1;
        if( x <= 7 && y == 0 ) {
            mappedDeviceId = 0;
        } else if( x <= 15 && y == 0 ) {
            mappedDeviceId = 1;
        } else if( x <= 7 && y == 1 ) {
            mappedDeviceId = 2;
        } else if( x <= 15 && y == 1 ) {
            mappedDeviceId = 3;
        }

        if( mappedDeviceId >= 0 ) {
            switch( extCtrlDevice[mappedDeviceId]->getRotation() ) {
            case ExtCtrlDevice::Rotation_0: {
                MidiMessage message(0xb0, 0x68 + mappedValue, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_90: {
                MidiMessage message(0x90, 0x08 + (7-mappedValue)*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_180: {
                MidiMessage message(0xb0, 0x68 + (7-mappedValue), mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            case ExtCtrlDevice::Rotation_270: {
                MidiMessage message(0x90, 0x08 + mappedValue*0x10, mappedLedState, 0);
                sendMidiMessage(mappedDeviceId, message);
            } break;
            }
        }
    } break;
    }
}

void ExtCtrl::sendShiftLed(const unsigned& ledState)
{
    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        // n/a
    } break;
    }
}

void ExtCtrl::sendAllLedsOff(void)
{
    ControllerTypeT controllerType = (ControllerTypeT)controllerSelection->getSelectedItemIndex();
    switch( controllerType ) {
    case NovationLaunchPad: {
        MidiMessage message(0xb0, 0x00, 0x00, 0);

        for(int deviceId=0; deviceId<EXT_CTRL_MAX_DEVICES; ++deviceId) {
            extCtrlDevice[deviceId]->sendMidiMessage(message);
        }
    } break;
    }
}

