/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Voice Handlers for Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbSidVoiceDrum.h"
#include "MbSidMidiVoice.h"
#include "MbSidTables.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoiceDrum::MbSidVoiceDrum()
{
    MbSidVoice::init(0, 0, NULL);
    midiVoicePtr = NULL; // assigned by MbSid!
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoiceDrum::~MbSidVoiceDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::init(void)
{
    MbSidVoice::init();

    drumWaveform = 0;
    drumGatelength = 0;
    drumModel = 0;
}
