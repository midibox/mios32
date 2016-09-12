// $Id: seq_led.h 1864 2013-11-23 19:54:46Z tk $
/*
 * Header file for SEQ LED handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LED_H
#define _SEQ_LED_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of SRs: 23 supported by MIOS32_DOUT
#define SEQ_LED_NUM_SR 23
// (8 additional SRs are mapped to BLM8x8, and configured with SR M1..M8)


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LED_Init(u32 mode);

extern s32 SEQ_LED_PinSet(u32 pin, u32 value);
extern s32 SEQ_LED_SRSet(u32 sr, u8 value);
extern s32 SEQ_LED_SRGet(u32 sr);

extern s32 SEQ_LED_DigitPatternGet(u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LED_H */
