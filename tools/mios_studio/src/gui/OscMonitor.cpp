/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * OSC Monitor Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@oscbox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "OscMonitor.h"
#include "MiosStudio.h"

//==============================================================================
OscMonitor::OscMonitor(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
{
    addAndMakeVisible(displayOptionsLabel = new Label(String::empty, T("OSC Display Options:")));
    displayOptionsLabel->setJustificationType(Justification::right);
    addAndMakeVisible(displayOptionsComboBox = new ComboBox(String::empty));
    displayOptionsComboBox->setWantsKeyboardFocus(true);
    displayOptionsComboBox->addItem(T("Decoded Text only"), 1);
    displayOptionsComboBox->addItem(T("Hex dump only"), 2);
    displayOptionsComboBox->addItem(T("Decoded Text and Hex Dump"), 3);
    displayOptionsComboBox->setSelectedId(1, true);
    displayOptionsComboBox->addListener(this);

    addAndMakeVisible(monitorLogBox = new LogBox(T("Osc Monitor")));
    monitorLogBox->addEntry(Colours::grey, T("OSC Monitor ready."));

    // restore settings
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        displayOptionsComboBox->setSelectedId(propertiesFile->getIntValue(T("oscDisplayOption")), true);
    }

    setSize(400, 200);
}

OscMonitor::~OscMonitor()
{
}

//==============================================================================
void OscMonitor::paint (Graphics& g)
{
    g.fillAll(Colours::white);
}

void OscMonitor::resized()
{
    displayOptionsLabel->setBounds(4, 4, 150, 24);
    displayOptionsComboBox->setBounds(4+150+10, 4, 250, 24);

    monitorLogBox->setBounds(4, 4+24+4, getWidth()-8, getHeight()-(4+24+4+4));
}


//==============================================================================
void OscMonitor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if( comboBoxThatHasChanged == displayOptionsComboBox ) {
        PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile ) {
            propertiesFile->setValue(T("oscDisplayOption"), String(displayOptionsComboBox->getSelectedId()));
        }
    }
}

//==============================================================================
void OscMonitor::parsedOscPacket(const OscHelper::OscArgsT& oscArgs, const unsigned& methodArg)
{
    if( oscString == String::empty )
        oscString = T("@") + String::formatted(T("%d.%d "), oscArgs.timetag.seconds, oscArgs.timetag.fraction);
    else
        oscString += " ";

    oscString += OscHelper::element2String(oscArgs);
}


//==============================================================================
void OscMonitor::handleIncomingOscMessage(const unsigned char *message, unsigned size)
{
    double timeStamp = ((double)Time::getMillisecondCounter() / 1000.0);
    String timeStampStr = String::formatted(T("%8.3f"), timeStamp);

    unsigned displayOption = displayOptionsComboBox->getSelectedId();

    if( displayOption >= 2 ) {
        String hexStr = String::toHexString(message, size);
        monitorLogBox->addEntry(Colours::black, "[" + timeStampStr + "] " + hexStr);
    }

    if( displayOption == 1 || displayOption == 3 ) {
        oscString = String::empty;
        OscHelper::OscSearchTreeT searchTree[] = {
            //{ "midi", NULL, this, 0x00000000 },
            { NULL, NULL, this, 0 } // terminator - will receive all messages that haven't been parsed
        };

        int status = OscHelper::parsePacket((unsigned char *)message, size, searchTree);
        if( status == -1 )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] received packet with invalid format!");
        else if( status == -2 )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] received packet with invalid element format!");
        else if( status == -3 )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] received packet with unsupported format!");
        else if( status == -4 )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] MIOS32_OSC_MAX_PATH_PARTS has been exceeded!");
        else if( status == -5 )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] received erroneous packet with status " + String(status));
        else if( oscString == String::empty )
            monitorLogBox->addEntry(Colours::red, "[" + timeStampStr + "] received empty OSC packet (check hex view!)");
        else
            monitorLogBox->addEntry(Colours::blue, "[" + timeStampStr + "] " + oscString);
    }
}
