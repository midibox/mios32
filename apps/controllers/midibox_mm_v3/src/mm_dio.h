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

#ifndef _MM_DIO_H
#define _MM_DIO_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_DIO_ButtonHandler(u8 button, u8 value);
extern s32 MM_DIO_SFBHandler(u8 button_id, u8 pin_value);
extern s32 MM_DIO_SFBLEDUpdate(void);

extern s32 MM_DIO_LEDHandler(u8 led_id_l, u8 led_id_h);
extern s32 MM_DIO_LEDSet(u8 mm_id, u8 led_on);
extern s32 MM_DIO_LED_CheckUpdate(void);

#endif /* _MM_DIO_H */
