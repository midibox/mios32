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


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_LCD_CHARSET_None,
  SEQ_LCD_CHARSET_VBars,
  SEQ_LCD_CHARSET_HBars
} seq_lcd_charset_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LCD_Clear(void);

extern s32 SEQ_LCD_InitSpecialChars(seq_lcd_charset_t charset);

extern s32 SEQ_LCD_PrintSpaces(u8 num);
extern s32 SEQ_LCD_PrintVBar(u8 value);
extern s32 SEQ_LCD_PrintHBar(u8 value);
extern s32 SEQ_LCD_PrintNote(u8 note);
extern s32 SEQ_LCD_PrintArp(u8 note);
extern s32 SEQ_LCD_PrintGatelength(u8 len);
extern s32 SEQ_LCD_PrintGxTy(u8 group, u8 track);
extern s32 SEQ_LCD_PrintMIDIPort(mios32_midi_port_t port);
extern s32 SEQ_LCD_PrintStepView(u8 step_view);
extern s32 SEQ_LCD_PrintSelectedStep(u8 step_sel, u8 step_max);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LCD_H */
