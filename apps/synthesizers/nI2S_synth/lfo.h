/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - lfo module            *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _LFO_H
#define _LFO_H

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void LFO_setFreq(u8 lfo, u16 frq);
void LFO_setPW(u8 lfo, u16 pw);
void LFO_setDepth(u8 lfo, u16 d);
void LFO_setWaveform(u8 lfo, u8 wav);
void LFO_tick(void);

#endif