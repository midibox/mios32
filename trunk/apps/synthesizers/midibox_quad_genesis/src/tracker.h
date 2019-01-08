/*
 * MIDIbox Quad Genesis: Tracker system header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _TRACKER_H
#define _TRACKER_H

#include <genesis.h>
#include <vgm.h>


extern void Tracker_EncToMIDI(u8 encoder, s32 incrementer, u8 selvoice, u8 selop, u8 midiport, u8 midichan);
extern void Tracker_BtnToMIDI(u8 button, u8 value, u8 selvoice, u8 selop, u8 midiport, u8 midichan);


#endif /* _TRACKER_H */
