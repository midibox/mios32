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

// note: the order has to be kept in sync with MBSEQ V3 for MBSEQ Remote Mode!
typedef enum {
  SEQ_LCD_CHARSET_None,
  SEQ_LCD_CHARSET_Menu,
  SEQ_LCD_CHARSET_VBars,
  SEQ_LCD_CHARSET_HBars,
  SEQ_LCD_CHARSET_DrumSymbolsBig,
  SEQ_LCD_CHARSET_DrumSymbolsMedium,
  SEQ_LCD_CHARSET_DrumSymbolsSmall
} seq_lcd_charset_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LCD_Init(u32 mode);

extern s32 SEQ_LCD_Clear(void);
extern s32 SEQ_LCD_PrintChar(char c);
extern s32 SEQ_LCD_CursorSet(u16 column, u16 line);
extern s32 SEQ_LCD_Update(u8 force);

extern s32 SEQ_LCD_InitSpecialChars(seq_lcd_charset_t charset);
extern s32 SEQ_LCD_PrintString(char *str);
extern s32 SEQ_LCD_PrintFormattedString(char *format, ...);

extern s32 SEQ_LCD_PrintSpaces(int num);
extern s32 SEQ_LCD_PrintStringPadded(char *str, u32 width);
extern s32 SEQ_LCD_PrintVBar(u8 value);
extern s32 SEQ_LCD_PrintHBar(u8 value);
extern s32 SEQ_LCD_PrintLongHBar(u8 value);
extern s32 SEQ_LCD_PrintNote(u8 note);
extern s32 SEQ_LCD_PrintArp(u8 note);
extern s32 SEQ_LCD_PrintGatelength(u8 len);
extern s32 SEQ_LCD_PrintProbability(u8 probability);
extern s32 SEQ_LCD_PrintStepDelay(s32 delay);
extern s32 SEQ_LCD_PrintRollMode(u8 roll_mode);
extern s32 SEQ_LCD_PrintRoll2Mode(u8 roll2_mode);
extern s32 SEQ_LCD_PrintEvent(mios32_midi_package_t package, u8 num_chars);
extern s32 SEQ_LCD_PrintLayerEvent(u8 track, u8 step, u8 par_layer, u8 instrument, u8 step_view, int print_edit_value);
extern s32 SEQ_LCD_PrintGxTy(u8 group, u16 selected_tracks);
extern s32 SEQ_LCD_PrintPattern(seq_pattern_t pattern);
extern s32 SEQ_LCD_PrintPatternCategory(seq_pattern_t pattern, char *pattern_name);
extern s32 SEQ_LCD_PrintPatternLabel(seq_pattern_t pattern, char *pattern_name);
extern s32 SEQ_LCD_PrintTrackLabel(u8 track, char *track_name);
extern s32 SEQ_LCD_PrintTrackCategory(u8 track, char *track_name);
extern s32 SEQ_LCD_PrintTrackDrum(u8 track, u8 drum, char *track_name);
extern s32 SEQ_LCD_PrintMIDIInPort(mios32_midi_port_t port);
extern s32 SEQ_LCD_PrintMIDIOutPort(mios32_midi_port_t port);
extern s32 SEQ_LCD_PrintStepView(u8 step_view);

extern s32 SEQ_LCD_PrintList(char *list, u8 item_width, u8 num_items, u8 max_items_on_screen, u8 selected_item_on_screen, u8 view_offset);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LCD_H */
