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
// These values define the size of the available display buffer
// It's configured for two 2x40 CLCDs by default (connected to J15A/B of the core module)
// If more than 2 CLCDs are configured in the MIOS32 Bootloader, the size depends
// on the specified layout parameters (num_x, num_y, width, height)\n
// GLCDs are supported as well, we handle them as text displays\n
// All details: see implementation in BUFLCD_Init()
#ifndef BUFLCD_BUFFER_SIZE
# define BUFLCD_BUFFER_SIZE         (2*2*40)
#endif

// enable this flag if half of the buffer should be used to store the font
// Only required if the text message is print with *different* fonts.
// The BUFLCD_BUFFER_SIZE has be doubled in this case
#ifndef BUFLCD_SUPPORT_GLCD_FONTS
# define BUFLCD_SUPPORT_GLCD_FONTS   0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BUFLCD_Init(u32 mode);

extern s32 BUFLCD_MaxBufferGet(void);
extern s32 BUFLCD_DeviceNumXSet(u8 num_x);
extern s32 BUFLCD_DeviceNumXGet(void);
extern s32 BUFLCD_DeviceNumYSet(u8 num_y);
extern s32 BUFLCD_DeviceNumYGet(void);
extern s32 BUFLCD_DeviceWidthSet(u8 width);
extern s32 BUFLCD_DeviceWidthGet(void);
extern s32 BUFLCD_DeviceHeightSet(u8 height);
extern s32 BUFLCD_DeviceHeightGet(void);
extern s32 BUFLCD_OffsetXSet(u8 offset);
extern s32 BUFLCD_OffsetXGet(void);
extern s32 BUFLCD_OffsetYSet(u8 offset);
extern s32 BUFLCD_OffsetYGet(void);

extern s32 BUFLCD_BufferGet(char *str, u8 line, u8 len);

extern s32 BUFLCD_Clear(void);
extern s32 BUFLCD_FontInit(u8 *font);
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
