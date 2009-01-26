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

#include "seq_pattern.h"


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
extern s32 SEQ_LCD_PrintChar(char c);
extern s32 SEQ_LCD_CursorSet(u16 column, u16 line);
extern s32 SEQ_LCD_Update(u8 force);

extern s32 SEQ_LCD_InitSpecialChars(seq_lcd_charset_t charset);

extern s32 SEQ_LCD_PrintSpaces(u8 num);
extern s32 SEQ_LCD_PrintStringPadded(char *str, u32 width);
extern s32 SEQ_LCD_PrintVBar(u8 value);
extern s32 SEQ_LCD_PrintHBar(u8 value);
extern s32 SEQ_LCD_PrintNote(u8 note);
extern s32 SEQ_LCD_PrintArp(u8 note);
extern s32 SEQ_LCD_PrintGatelength(u8 len);
extern s32 SEQ_LCD_PrintGxTy(u8 group, u8 track);
extern s32 SEQ_LCD_PrintPattern(seq_pattern_t pattern);
extern s32 SEQ_LCD_PrintPatternCategory(seq_pattern_t pattern, char *pattern_name);
extern s32 SEQ_LCD_PrintPatternLabel(seq_pattern_t pattern, char *pattern_name);
extern s32 SEQ_LCD_PrintTrackLabel(u8 track, char *track_name);
extern s32 SEQ_LCD_PrintTrackCategory(u8 track, char *track_name);
extern s32 SEQ_LCD_PrintTrackDrum(u8 track, u8 drum, char *track_name);
extern s32 SEQ_LCD_PrintMIDIPort(mios32_midi_port_t port);
extern s32 SEQ_LCD_PrintStepView(u8 step_view);
extern s32 SEQ_LCD_PrintSelectedStep(u8 step_sel, u8 step_max);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LCD_H */
