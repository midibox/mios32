// $Id$
/*
 * Header file for MIDIbox CV V2 button functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_BUTTON_H
#define _MBCV_BUTTON_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBCV_BUTTON_MATRIX_COLUMNS 8
#if MBCV_BUTTON_MATRIX_COLUMNS != 8
#error "MBCV_BUTTON_MATRIX_COLUMNS must be 8 - no other option considered!"
#endif

#define MBCV_BUTTON_MATRIX_ROWS 16
#if (MBCV_BUTTON_MATRIX_ROWS % 8) != 0
#error "MBCV_BUTTON_MATRIX_ROWS must be dividable by 8 (one DIN SR per 8 rows)!"
#endif


// for the keyboard keys
#define MBCV_BUTTON_KEYBOARD_KEYS_NUM 17


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBCV_BUTTON_Init(u32 mode);

extern s32 MBCV_BUTTON_Handler(u32 pin, u8 depressed);

extern s32 MBCV_BUTTON_MATRIX_PrepareCol(void);
extern s32 MBCV_BUTTON_MATRIX_GetRow(void);
extern s32 MBCV_BUTTON_MATRIX_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBCV_BUTTON_H */
