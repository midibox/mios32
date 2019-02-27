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

#ifndef _OSC_MONITOR_H
#define _OSC_MONITOR_H

#include "../includes.h"
#include "../OscHelper.h"
#include "LogBox.h"

class MiosStudio; // forward declaration

class OscMonitor
    : public Component
    , public ComboBox::Listener
    , public OscListener
{
public:
    //==============================================================================
    OscMonitor(MiosStudio *_miosStudio);
    ~OscMonitor();

    //==============================================================================
    void paint(Graphics& g);
    void resized();

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);

    //==============================================================================
    void parsedOscPacket(const OscHelper::OscArgsT& oscArgs, const unsigned& methodArg);

    //==============================================================================
    void handleIncomingOscMessage(const unsigned char *message, unsigned size);

protected:
    //==============================================================================
    Label* displayOptionsLabel;
    ComboBox* displayOptionsComboBox;
    LogBox* monitorLogBox;

    String oscString;

    //==============================================================================
    MiosStudio *miosStudio;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    OscMonitor (const OscMonitor&);
    const OscMonitor& operator= (const OscMonitor&);
};

#endif /* _OSC_MONITOR_H */
