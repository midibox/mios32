// $Id$
/*
 * Header file for WS2812 functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2015 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _WS2812_H
#define _WS2812_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Maximum number of LEDs connected to the WS2812 chain
// Each LED will consume 48 bytes!
#ifndef WS2812_NUM_LEDS
#define WS2812_NUM_LEDS 64
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 WS2812_Init(u32 mode);

extern s32 WS2812_LED_SetRGB(u16 led, u8 colour, u8 value);
extern s32 WS2812_LED_GetRGB(u16 led, u8 colour);

extern s32 WS2812_LED_SetHSV(u16 led, float h, float s, float v);
extern s32 WS2812_LED_GetHSV(u16 led, float *h, float *s, float *v);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _WS2812_H */
