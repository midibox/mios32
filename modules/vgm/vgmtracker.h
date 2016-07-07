/*
 * VGM Data and Playback Driver: Tracker system header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMTRACKER_H
#define _VGMTRACKER_H

#include <mios32.h>

#include "vgmhead.h"

#ifdef GENMDM_ALIGN_MSB
//Take the most significant bits of value
#define GENMDM_ENCODE(value,bits) (((value) & ((1<<(bits))-1)) << (7-(bits)))
#define GENMDM_DECODE(value,bits) ((value) >> (7-(bits)))
#else
//Take the least significant bits of value
#define GENMDM_ENCODE(value,bits) ((value) | ((u8)(bits) >> 9)) //Just value, but otherwise warning bits is unused
#define GENMDM_DECODE(value,bits) ((value) & ((1<<(bits))-1))
#endif


extern void VGM_Tracker_Init();
extern void VGM_Tracker_Enqueue(VgmChipWriteCmd cmd, u8 fixfreq);

extern void VGM_MidiToGenesis(mios32_midi_package_t midi_package, u8 g, u8 v, u8 ch3_op, u8 keyonmask);


#endif /* _VGMTRACKER_H */
