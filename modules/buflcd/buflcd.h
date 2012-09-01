// $Id$
/*
 * Header file for Buffered LCD functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BUFLCD_H
#define _BUFLCD_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// can be overruled in mios32_config.h
// These values only define the maximum numbers, they can be reduced during runtime
#ifndef BUFLCD_NUM_DEVICES
# define BUFLCD_NUM_DEVICES          2
#endif

#ifndef BUFLCD_COLUMNS_PER_DEVICE
# define BUFLCD_COLUMNS_PER_DEVICE  40
#endif

#ifndef BUFLCD_MAX_LINES
# define BUFLCD_MAX_LINES            2
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BUFLCD_Init(u32 mode);

extern s32 BUFLCD_NumDevicesSet(u8 devices);
extern s32 BUFLCD_NumDevicesGet(void);
extern s32 BUFLCD_ColumnsPerDeviceSet(u8 columns);
extern s32 BUFLCD_ColumnsPerDeviceGet(void);
extern s32 BUFLCD_NumLinesSet(u8 lines);
extern s32 BUFLCD_NumLinesGet(void);
extern s32 BUFLCD_OffsetXSet(u8 offset);
extern s32 BUFLCD_OffsetXGet(void);
extern s32 BUFLCD_OffsetYSet(u8 offset);
extern s32 BUFLCD_OffsetYGet(void);

extern s32 BUFLCD_Clear(void);
extern s32 BUFLCD_PrintChar(char c);
extern s32 BUFLCD_CursorSet(u16 column, u16 line);
extern s32 BUFLCD_Update(u8 force);

extern s32 BUFLCD_PrintString(char *str);
extern s32 BUFLCD_PrintFormattedString(char *format, ...);

extern s32 BUFLCD_PrintSpaces(int num);
extern s32 BUFLCD_PrintStringPadded(char *str, u32 width);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _BUFLCD_H */
