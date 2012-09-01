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

#ifndef _LC_LCD_H
#define _LC_LCD_H

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

extern s32 LC_LCD_Init(u32 mode);
extern s32 LC_LCD_Update(u8 force);

extern u8 LC_LCD_DisplayPageGet(void);
extern s32 LC_LCD_DisplayPageSet(u8 page);

extern s32 LC_LCD_Msg_CursorSet(u8 cursor_pos);
extern u8 LC_LCD_Msg_CursorGet(void);
extern s32 LC_LCD_Msg_PrintHost(char c);

extern s32 LC_LCD_SpecialCharsInit(u8 charset);

extern s32 LC_LCD_Update_HostMsg(void);
extern s32 LC_LCD_Update_Meter(u8 meter);
extern s32 LC_LCD_Update_Knob(u8 knob);
extern s32 LC_LCD_Update_MTC(void);
extern s32 LC_LCD_Update_SMPTE_Beats(void);
extern s32 LC_LCD_Update_Status(void);
extern s32 LC_LCD_Update_RSM(void);

extern s32 LC_LCD_LEDStatusUpdate(u8 evnt1, u8 evnt2);

#endif /* _LC_LCD_H */
