/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - drum modules          *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _DRUM_H
#define _DRUM_H

/////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////

void DRUM_ReloadSampleBuffer(u32 state);

void DRUM_init(void);

void DRUM_noteOff(u8 note);
void DRUM_noteOn(u8 note, u8 vel, u8 steal);

void DRUM_setSineDrum_SineFreqInitial(u8 voice, u16 value); 
void DRUM_setSineDrum_SineFreqEnd(u8 voice, u16 value); 
void DRUM_setSineDrum_SineFreqDecay(u8 voice, u16 value);
void DRUM_setSineDrum_SineAttack(u8 voice, u16 value);
void DRUM_setSineDrum_SineDecay(u8 voice, u16 value); 

void DRUM_setSineDrum_NoiseAttack(u8 voice, u16 value);
void DRUM_setSineDrum_NoiseDecay(u8 voice, u16 value); 
void DRUM_setSineDrum_NoiseCutoff(u8 voice, u16 value);
void DRUM_setSineDrum_NoiseCutoffDecay(u8 voice, u16 value); 

void DRUM_setSineDrum_Mix(u8 voice, u16 value);
void DRUM_setSineDrum_Volume(u8 voice, u16 value);
void DRUM_setSineDrum_Overdrive(u8 voice, u16 value);
void DRUM_setSineDrum_Waveform(u8 index, u8 value); 
void DRUM_setSineDrum_FilterType(u8 index, u8 value); 

#endif
