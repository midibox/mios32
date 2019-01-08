/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * External Controller Handling
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _EXT_CTRL_H
#define _EXT_CTRL_H

#include "JuceHeader.h"

#define EXT_CTRL_MAX_DEVICES 4

class MainComponent; // forward declaration
class ExtCtrl; // forward declaration

class ExtCtrlDevice
    : public Component
    , public ComboBoxListener
    , public MidiInputCallback
{
public:

    typedef enum {
        Rotation_0 = 0,
        Rotation_90,
        Rotation_180,
        Rotation_270,
    } RotationT;

    //==============================================================================
    ExtCtrlDevice(ExtCtrl *_extCtrl, const unsigned& _deviceId);
    ~ExtCtrlDevice();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

    //==============================================================================
    unsigned getDeviceId(void) { return deviceId; }
    RotationT getRotation(void) { return (RotationT)rotationSelection->getSelectedItemIndex(); }

    //==============================================================================
    void scanMidiInDevices();
    void scanMidiOutDevices();

    void sendMidiMessage(const MidiMessage& message);

protected:
    unsigned deviceId;

    ComboBox* midiInput;
    ComboBox* midiOutput;
    ComboBox* rotationSelection;
    Label* midiInLabel;
    Label* midiOutLabel;
    Label* rotationLabel;

    MidiInput  *enabledMidiInput;
    MidiOutput *enabledMidiOutput;

	void setMidiOutput(const String &port);
	void setMidiInput(const String &port);

    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);

    //==============================================================================
    ExtCtrl *extCtrl;
};


//==============================================================================
//==============================================================================
//==============================================================================
class ExtCtrl
    : public Component
    , public ComboBoxListener
{
public:

    typedef enum {
        NovationLaunchPad = 1,
    } ControllerTypeT;

    //==============================================================================
    ExtCtrl(MainComponent *_mainComponent);
    ~ExtCtrl();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

    void scanMidiInDevices();
    void scanMidiOutDevices();

    void handleIncomingMidiMessage(const unsigned& deviceId, const MidiMessage& message);
    void sendMidiMessage(const unsigned& deviceId, const MidiMessage& message);

    void sendGridLed(const unsigned& x, const unsigned& y, const unsigned& ledState);
    void sendExtraColumnLed(const unsigned& x, const unsigned& y, const unsigned& ledState);
    void sendExtraRowLed(const unsigned& x, const unsigned& y, const unsigned& ledState);
    void sendShiftLed(const unsigned& ledState);

    void sendAllLedsOff(void);

    //==============================================================================
    MainComponent *mainComponent;

protected:

    ComboBox* controllerSelection;
    Label*    controllerLabel;

    ExtCtrlDevice *extCtrlDevice[EXT_CTRL_MAX_DEVICES];

};


//==============================================================================
//==============================================================================
//==============================================================================
class ExtCtrlWindow
    : public DocumentWindow
{
public:
    ExtCtrlWindow(MainComponent *_mainComponent)
        : DocumentWindow("External Controller", Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        extCtrl = new ExtCtrl(_mainComponent);
        setContentComponent(extCtrl, true, true);
        setUsingNativeTitleBar(true);
        centreWithSize(getWidth(), getHeight());
        setVisible(false); // will be made visible by clicking on the tools button
    }

    ~ExtCtrlWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    ExtCtrl *extCtrl;

    //==============================================================================
    void closeButtonPressed()
    {
        setVisible(false);
    }

};

#endif /* _EXT_CTRL_H */
