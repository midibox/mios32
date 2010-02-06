/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIOS Terminal Component
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS_TERMINAL_H
#define _MIOS_TERMINAL_H

#include "../includes.h"

class MiosTerminal
    : public Component
{
public:
    //==============================================================================
    MiosTerminal(AudioDeviceManager &_audioDeviceManager);
    ~MiosTerminal();

    //==============================================================================
    void paint(Graphics& g);
    void resized();


private:
    //==============================================================================
    TextEditor* terminalWindow;

    //==============================================================================
    AudioDeviceManager *audioDeviceManager;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MiosTerminal (const MiosTerminal&);
    const MiosTerminal& operator= (const MiosTerminal&);
};

#endif /* _MIOS_TERMINAL_H */
