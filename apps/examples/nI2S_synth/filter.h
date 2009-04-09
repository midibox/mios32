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

// tables for Antti's Moog Filter 
/*
static const u16 tab_vcf[] = {
	0x0101,	0x0101,	0x0201,	0x0202, 0x0202,	0x0202,	0x0202,	0x0402,	0x0404,	0x0505,
    0x0606, 0x0806,	0x0908,	0x0A0A,	0x0C0C,	0x100C,	0x1211,	0x1614,	0x1A18,	0x201C,
	0x2422,	0x2C28,	0x3430,	0x4038,	0x4C44,	0x5852,	0x6861,	0x8070,	0x968A,	0xB0A4,
	0xD8C4,	0xFFE8
};
*/

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

u16 FILTER_filter(u16 in, u16 cutoff);

void FILTER_setCutoff(u16 c);
void FILTER_setResonance(u16 r);
void FILTER_setFilter(u8 f);

s16 FILTER_bandpass(s16 input);

#endif
