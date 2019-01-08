// $Id: seq_tpd.h 1865 2013-11-23 21:53:50Z tk $
/*
 * Header file for relative track position matrix LED display (by hawkeye)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_TPD_H
#define _SEQ_TPD_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_TPD_Mode_PosAndTrack = 0,
  SEQ_TPD_Mode_RotatedPosAndTrack,
  SEQ_TPD_Mode_MeterAndPos,
  SEQ_TPD_Mode_RotatedMeterAndPos,
  SEQ_TPD_Mode_DotMeterAndPos,
  SEQ_TPD_Mode_RotatedDotMeterAndPos,
} seq_tpd_mode_t;

#define SEQ_TPD_NUM_MODES 6 // for seq_ui_opt.c


/////////////////////////////////////////////////////////////////////////////
// Global functions
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_TPD_Init(u32 mode);

extern s32 SEQ_TPD_ModeSet(seq_tpd_mode_t mode);
extern seq_tpd_mode_t SEQ_TPD_ModeGet(void);

extern s32 SEQ_TPD_PrintString(char *str);

extern s32 SEQ_TPD_LED_Update(void);

extern s32 SEQ_TPD_Handler(void);

#endif 
