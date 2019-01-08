// $Id$
/*
 * LCD access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_LCD_H
#define _MBNG_LCD_H

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

extern s32 MBNG_LCD_Init(u32 mode);

extern s32 MBNG_LCD_FontInit(char font_name);
extern u8 *MBNG_LCD_FontGet(void);

extern s32 MBNG_LCD_Clear(void);
extern s32 MBNG_LCD_FontInit(char font_name);
extern s32 MBNG_LCD_CursorSet(u8 lcd, u16 x, u16 y);
extern s32 MBNG_LCD_PrintChar(char c);
extern s32 MBNG_LCD_PrintString(char *str);
extern s32 MBNG_LCD_PrintFormattedString(char *format, ...);
extern s32 MBNG_LCD_PrintSpaces(int num);

extern s32 MBNG_LCD_ClearScreenOnNextMessage(void);

extern s32 MBNG_LCD_PrintItemLabel(mbng_event_item_t *item, char *out_buffer, u32 max_buffer_len);

extern s32 MBNG_LCD_SpecialCharsInit(u8 charset, u8 force);
extern s32 MBNG_LCD_SpecialCharsReInit(void);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_LCD_H */
