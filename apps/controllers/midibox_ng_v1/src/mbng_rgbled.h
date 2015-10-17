// $Id$
/*
 * RGB LED access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_RGBLED_H
#define _MBNG_RGBLED_H

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_RGBLED_Init(u32 mode);

extern s32 MBNG_RGBLED_Periodic_1mS(void);

extern s32 MBNG_RGBLED_RainbowSpeedSet(u8 speed);
extern s32 MBNG_RGBLED_RainbowSpeedGet(void);
extern s32 MBNG_RGBLED_RainbowBrightnessSet(u8 brightness);
extern s32 MBNG_RGBLED_RainbowBrightnessGet(void);

extern s32 MBNG_RGBLED_NotifyReceivedValue(mbng_event_item_t *item);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_RGBLED_H */
