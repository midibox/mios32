/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Arpeggiator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_ARP_H
#define _MB_SID_ARP_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidRandomGen.h"
#include "MbSidVoice.h"
//#include "MbSidSe.h"
class MbSidSe; // forward declaration, see also http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.11


class MbSidArp
{
public:
    // Constructor
    MbSidArp();

    // Destructor
    ~MbSidArp();

    // arp init function
    void init(void);

    // arpeggiator handler
    void tick(MbSidVoice *v, MbSidSe *mbSidSe);

    // requests a restart and next clock (divided by Arp)
    bool restartReq;
    bool clockReq;

    // random generator
    MbSidRandomGen randomGen;

protected:
    // internal variables
    bool arpActive;
    bool arpUp;
    bool arpHoldSaved;

    u8 arpDivCtr;
    u8 arpGlCtr;
    u8 arpNoteCtr;
    u8 arpOctCtr;

};

#endif /* _MB_SID_ARP_H */
