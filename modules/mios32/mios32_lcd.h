// $Id$
/*
 * Header file for LCD Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_LCD_H
#define _MIOS32_LCD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

s32 MIOS32_LCD_Init(u32 mode);
s32 MIOS32_LCD_DeviceSet(u8 device);
u8  MIOS32_LCD_DeviceGet(void);
s32 MIOS32_LCD_CursorSet(u16 line, u16 column);
s32 MIOS32_LCD_Clear(void);
s32 MIOS32_LCD_PrintChar(char c);
s32 MIOS32_LCD_PrintString(char *str);

s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8]);
s32 MIOS32_LCD_SpecialCharsInit(u8 table[64]);

s32 MIOS32_DOUT_PinSet(u32 pin, u32 value);
s32 MIOS32_DOUT_SRSet(u32 sr, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_LCD_H */
