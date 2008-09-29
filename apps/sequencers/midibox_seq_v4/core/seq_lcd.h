// $Id$
/*
 * Header file for LCD utility functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LCD_H
#define _SEQ_LCD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_LCD_CHARSET_NONE  0
#define SEQ_LCD_CHARSET_VBARS 1

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LCD_Clear(void);

extern s32 SEQ_LCD_InitSpecialChars(u8 charset);

extern s32 SEQ_LCD_PrintSpaces(u8 num);
extern s32 SEQ_LCD_PrintNote(u8 note);
extern s32 SEQ_LCD_PrintGatelength(u8 len);
extern s32 SEQ_LCD_PrintGxTy(u8 group, u8 track);
extern s32 SEQ_LCD_PrintParLayer(u8 layer);
extern s32 SEQ_LCD_PrintTrgLayer(u8 layer);
extern s32 SEQ_LCD_PrintMIDIPort(u8 port);
extern s32 SEQ_LCD_PrintStepView(u8 step_view);
extern s32 SEQ_LCD_PrintSelectedStep(u8 step_sel, u8 step_max);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LCD_H */
