// $Id$
/*
 * Header file for logo functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LCD_LOGO_H
#define _SEQ_LCD_LOGO_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LCD_LOGO_Init(u32 mode);
extern s32 SEQ_LCD_LOGO_Print(u8 logo_offset, u8 lcd_pos);

extern s32 SEQ_LCD_LOGO_ScreenSaver_Period1S(void);
extern s32 SEQ_LCD_LOGO_ScreenSaver_Enable(void);
extern s32 SEQ_LCD_LOGO_ScreenSaver_Disable(void);
extern s32 SEQ_LCD_LOGO_ScreenSaver_IsActive(void);
extern s32 SEQ_LCD_LOGO_ScreenSaver_Print(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_lcd_logo_screensaver_delay; // in minutes


#endif /* _SEQ_LCD_LOGO_H */
