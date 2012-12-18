// $Id$
/*
 * Scan Matrix access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_MATRIX_H
#define _MBNG_MATRIX_H

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBNG_MATRIX_DOUT_NUM_PATTERN_POS 17

/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_MATRIX_Init(u32 mode);

extern s32 MBNG_MATRIX_PatternSet(u8 num, u8 pos, u16 pattern);
extern u16 MBNG_MATRIX_PatternGet(u8 num, u8 pos);

extern s32 MBNG_MATRIX_DOUT_PinSet(u8 matrix, u8 color, u16 pin, u8 value);
extern s32 MBNG_MATRIX_DOUT_PatternSet(u8 matrix, u8 color, u16 row, u16 value, u16 range, u8 pattern);
extern s32 MBNG_MATRIX_DOUT_PatternSet_LC(u8 matrix, u8 color, u16 row, u16 value);

extern s32 MBNG_MATRIX_PrepareCol(void);
extern s32 MBNG_MATRIX_GetRow(void);
extern s32 MBNG_MATRIX_ButtonHandler(void);

extern s32 MBNG_MATRIX_DIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value);
extern s32 MBNG_MATRIX_DIN_GetCurrentValueFromId(mbng_event_item_id_t id);
extern s32 MBNG_MATRIX_DOUT_NotifyReceivedValue(mbng_event_item_t *item, u16 value);
extern s32 MBNG_MATRIX_DOUT_GetCurrentValueFromId(mbng_event_item_id_t id);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_MATRIX_H */
