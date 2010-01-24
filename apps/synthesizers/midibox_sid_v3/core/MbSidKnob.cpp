/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Knob Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidKnob.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidKnob::MbSidKnob()
{
    knobNum = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidKnob::~MbSidKnob()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
void MbSidKnob::set(u8 value)
{
    // store new value into patch
    mbsid_knob_t *knob = (mbsid_knob_t *)&mbSidSePtr->mbSidPatch.body.knob[knobNum];
    knob->value = value;

    // copy it also into shadow buffer
    mbsid_knob_t *knob_shadow = (mbsid_knob_t *)&mbSidSePtr->mbSidPatch.bodyShadow.knob[knobNum];
    knob_shadow->value = value;

    // get scaled value between min/max boundaries
    s32 factor = ((knob->max > knob->min) ? (knob->max - knob->min) : (knob->min - knob->max)) + 1;
    s32 scaled_value16 = knob->min + (value * factor); // 16bit

    // copy as signed value into modulation source array
    mbSidSePtr->modSrc[SID_SE_MOD_SRC_KNOB1 + knobNum] = (value << 8) - 0x8000;

    if( knobNum == SID_KNOB_1 ) {
        // copy knob1 to MDW source
        // in distance to KNOB1, this one goes only into positive direction
        mbSidSePtr->modSrc[SID_SE_MOD_SRC_MDW] = value << 7;
    }

    // determine sidl/r and instrument selection depending on engine
    u8 sidlr;
    u8 ins;
    sid_se_engine_t engine = (sid_se_engine_t)mbSidSePtr->mbSidPatch.body.engine;
    switch( engine ) {
    case SID_SE_LEAD:
        sidlr = 3; // always modify both SIDs
        ins = 0; // instrument n/a
        break;

    case SID_SE_BASSLINE:
        sidlr = 3; // always modify both SIDs
        ins = 3; // TODO: for "current" use this selection! - value has to be taken from CS later
        break;

    case SID_SE_DRUM:
        sidlr = 3; // always modify both SIDs
        ins = 0; // TODO: for "current" use this selection! - value has to be taken from CS later
        break;

    case SID_SE_MULTI:
        sidlr = 3; // always modify both SIDs
        ins = 0; // TODO: for "current" use this selection! - value has to be taken from CS later
        break;

    default:
        // unsupported engine, use default values
        sidlr = 3;
        ins = 0;
    }

    // forward to parameter handler
    // interrupts should be disabled to ensure atomic access
    MIOS32_IRQ_Disable();
    mbSidParPtr->parSet16(knob->assign1, scaled_value16, sidlr, ins);
    mbSidParPtr->parSet16(knob->assign2, scaled_value16, sidlr, ins);
    MIOS32_IRQ_Enable();
}
