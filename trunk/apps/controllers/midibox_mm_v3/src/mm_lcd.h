// $Id$
/*
 * Header file for LCD Access Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MM_LCD_H
#define _MM_LCD_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u8 ALL;
  };

  struct {
    unsigned PAGE:2;
    unsigned LARGE_SCREEN:1;
    unsigned SECOND_LINE:1;
    unsigned SMPTE_VIEW:1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
  };
} display_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_LCD_Init(u32 mode);
extern s32 MM_LCD_Update(u8 force);

extern s32 MM_LCD_Msg_CursorSet(u8 cursor_pos);
extern u8 MM_LCD_Msg_CursorGet(void);
extern s32 MM_LCD_Msg_PrintChar(char c);
extern s32 MM_LCD_Msg_PrintHost(char c);

extern s32 MM_LCD_PointerInit(u8 type);
extern s32 MM_LCD_PointerSet(u8 pointer, u8 value);
extern s32 MM_LCD_PointerPrint(u8 pointer);

extern s32 MM_LCD_SpecialCharsInit(u8 charset);

#endif /* _MM_LCD_H */
