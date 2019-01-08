// $Id$
/*
 * Header File for Digital IO Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LC_DIO_H
#define _LC_DIO_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_DIO_ButtonHandler(u8 button, u8 value);
extern s32 LC_DIO_SFBHandler(u8 button_id, u8 pin_value);
extern s32 LC_DIO_SFBLEDUpdate(void);

extern s32 LC_DIO_LEDSet(u8 lc_id, u8 led_on);
extern s32 LC_DIO_LED_CheckUpdate(void);

#endif /* _LC_DIO_H */
