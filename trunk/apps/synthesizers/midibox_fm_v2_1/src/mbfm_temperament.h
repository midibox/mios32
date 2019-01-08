// $Id: mbfm_temperament.h $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 OPL3 temperament systems
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBFM_TEMPERAMENT_H
#define _MBFM_TEMPERAMENT_H

#include <mios32.h>

extern u8 GetOPL3Block(u8 note);

extern u16 GetOPL3Frequency(u8 note);
extern u16 GetOPL3NextFrequency(u8 note); //Frequency tables are set up so
//portamento works, that is, so that you can, just using frequency, get to the
//note a half step above each note. This returns the frequency of the note
//a half step above the given note, guaranteed to be in the same block as it.

extern u8 GetOPL3Temperament(); //0 if equal, 1 if custom
extern void SetOPL3Temperament(u8 temper);

extern void Temperament_Init();

//Frequencies are stored as u16 of ten times their value
//e.g. 440 Hz would be (u16)4400
extern u8 GetOPL3BaseNote();
extern u16 GetOPL3BaseFreq();
extern void SetOPL3BaseNote(u8 midinote, u16 freq);
extern u16 GetEqualTemperBaseFreq(u8 midinote);

//The numbers here are one lower than they act
//e.g. a zero here will be calculated as a 1, and a 255 as a 256
extern u8 GetTemperNumerator(u8 notenum);
extern u8 GetTemperDenominator(u8 notenum);
extern u16 GetTemperFrequency(u8 notenum);
extern void SetTemperNumerator(u8 notenum, u8 numerator);
extern void SetTemperDenominator(u8 notenum, u8 denominator);




#endif /* _MBFM_TEMPERAMENT_H */
