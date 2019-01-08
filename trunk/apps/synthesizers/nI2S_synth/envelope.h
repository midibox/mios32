/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - envelope module       *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _ENVELOPE_H
#define _ENVELOPE_H

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void ENV_tick(void);
void ENV_setAttack(u8 env, u16 v);
void ENV_setDecay(u8 env, u16 v);
void ENV_setSustain(u8 env, u16 v);
void ENV_setRelease(u8 env, u16 v);

#endif