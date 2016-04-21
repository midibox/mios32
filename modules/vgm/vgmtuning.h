/*
 * VGM Data and Playback Driver: Tuning header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMTUNING_H
#define _VGMTUNING_H

#include <mios32.h>
#include "vgmhead.h"

extern void VGM_fixOPN2Frequency(VgmChipWriteCmd* writecmd, u32 opn2mult);
extern void VGM_fixPSGFrequency(VgmChipWriteCmd* writecmd, u32 psgmult, u8 freq0to1);

//Cents is a pitch bend value between -100 and 99
//These commands just return the frequency portions of the VgmChipWriteCmd,
//not any of the control/channel portions
extern VgmChipWriteCmd VGM_getOPN2Frequency(u8 midinote, s8 cents, u32 opn2clock);
extern VgmChipWriteCmd VGM_getPSGFrequency(u8 midinote, s8 cents, u32 psgclock);


#endif /* _VGMTUNING_H */
