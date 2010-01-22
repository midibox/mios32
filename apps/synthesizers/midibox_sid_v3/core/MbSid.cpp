/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#include "MbSid.h"

MbSid::MbSid()
{
}

MbSid::~MbSid()
{
}

void MbSid::sendAlive(void)
{
    MIOS32_MIDI_SendDebugMessage("Ok, I'm %d!\n", sidNum);
}

