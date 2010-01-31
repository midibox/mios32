/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Bassline Sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SEQ_BASSLINE_H
#define _MB_SID_SEQ_BASSLINE_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidSeq.h"
#include "MbSidVoice.h"

class MbSidSe; // forward declaration

class MbSidSeqBassline : public MbSidSeq
{
public:
    // Constructor
    MbSidSeqBassline();

    // Destructor
    ~MbSidSeqBassline();

    // sequencer init function
    void init(void);

    // sequencer handler
    void tick(MbSidVoice *v, MbSidSe *se);

    // input parameters: see also MbSidSeq.h

    // additional parameters for bassline sequencer

#if 0
    // seqStopReq = mbSidClockPtr->eventStop
    // seqRestartReq = mbSidClockPtr->eventStart
    // seqClockReq = mbSidClockPtr->eventClock
    // seqEnabled = WTO flags in v_flags
    // seqSynchToMeasure = speed_par.SYNCH_TO_MEASURE
    // seqClockDivider = speed_par.CLKDIV
    // exit if arpeggiator is enabled
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voicePatch->arp_mode;
    if( arp_mode.ENABLE )
        return;

    // seqPatternNum = (s->seq_patch->num & 0x7)
    // seqPatternMemory = mbSidPatch.body.B.seq_memory
    // seqParameterAssign = s->seq_patch->assign
#endif
};

#endif /* _MB_SID_SEQ_BASSLINE_H */
