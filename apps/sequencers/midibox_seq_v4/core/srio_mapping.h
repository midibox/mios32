// $Id$
/*
 * Temporary file which describes the DIN/DOUT mapping (later part of setup_* file)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SRIO_MAPPING_H
#define _SRIO_MAPPING_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// temporary differ between two setups here
// 0: for virtual MBSEQ
// 1: for Wilba's frontpanel
#ifndef SRIO_MAPPING
#define SRIO_MAPPING 0
#endif


#if SRIO_MAPPING == 0
#include "srio_mapping_v4.h"
#elif SRIO_MAPPING == 1
#include "srio_mapping_wilba.h"
#endif

#endif /* _SRIO_MAPPING_H */
