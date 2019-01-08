/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - filter module         *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _FILTER_H
#define _FILTER_H
 
/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

s16 FILTER_filter(s16 in, u16 cutoff);

void FILTER_setCutoff(u16 c);
void FILTER_setResonance(u16 r);
void FILTER_setFilter(u8 f);

#endif
