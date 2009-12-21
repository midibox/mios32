// $Id$
/*
 * Header file for random generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_RANDOM_H
#define _SID_RANDOM_H


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern u32 SID_RANDOM_Gen(u32 seed);
extern u32 SID_RANDOM_Gen_Range(u32 min, u32 max);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SID_RANDOM_H */
