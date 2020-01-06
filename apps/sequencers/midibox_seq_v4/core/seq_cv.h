// $Id$
/*
 * Header file for CV functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_CV_H
#define _SEQ_CV_H

#include <mios32.h>
#if !defined(MIOS32_DONT_USE_AOUT)
#include <aout.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

//#define SEQ_CV_NUM_IF AOUT_NUM_IF
// INTDAC clashes with SD Card port, therefore it's not used (it's always located at last position)
#define SEQ_CV_NUM_IF (AOUT_NUM_IF-1)

// number of CV channels (max)
#define SEQ_CV_NUM AOUT_NUM_CHANNELS

// selects V/Oct, Hz/V, Inv.
#define SEQ_CV_NUM_CURVES 3

// selects calibration mode
#if AOUT_NUM_CALI_POINTS_X > 0
# define SEQ_CV_NUM_CALI_MODES (AOUT_NUM_CALI_MODES+AOUT_NUM_CALI_POINTS_X)
#else
# define SEQ_CV_NUM_CALI_MODES AOUT_NUM_CALI_MODES
#endif

// size of notestack(s)
#define SEQ_CV_NOTESTACK_SIZE 10

// number of clock outputs
#define SEQ_CV_NUM_CLKOUT 8

// maximum DOUT trigger width (each mS will create a pipeline stage)
#define SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX 10


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CV_Init(u32 mode);

extern s32 SEQ_CV_IfSet(aout_if_t if_type);
extern aout_if_t SEQ_CV_IfGet(void);
extern const char* SEQ_CV_IfNameGet(aout_if_t if_type);

extern u8 SEQ_CV_NumChnGet(void);

extern s32 SEQ_CV_CurveSet(u8 cv, u8 curve);
extern u8 SEQ_CV_CurveGet(u8 cv);
extern const char* SEQ_CV_CurveNameGet(u8 cv);

extern s32 SEQ_CV_CaliModeSet(u8 cv, aout_cali_mode_t mode);
extern aout_cali_mode_t SEQ_CV_CaliModeGet(void);
extern s32 SEQ_CV_CaliNameGet(char *str, u8 cv, u8 display_bipolar);

extern s32 SEQ_CV_SlewRateSet(u8 cv, u8 value);
extern s32 SEQ_CV_SlewRateGet(u8 cv);

extern s32 SEQ_CV_PitchRangeSet(u8 cv, u8 range);
extern u8 SEQ_CV_PitchRangeGet(u8 cv);

extern u16* SEQ_CV_CaliPointsPtrGet(u8 cv);

extern s32 SEQ_CV_ClkPulseWidthSet(u8 clkout, u8 width);
extern u8 SEQ_CV_ClkPulseWidthGet(u8 clkout);

extern s32 SEQ_CV_ClkDividerSet(u8 clkout, u16 div);
extern u16 SEQ_CV_ClkDividerGet(u8 clkout);

extern s32 SEQ_CV_Clk_Trigger(u8 clkout);

extern s32 SEQ_CV_DOUT_GateSet(u8 dout, u8 value);

extern s32 SEQ_CV_GateInversionSet(u8 gate, u8 inverted);
extern u8 SEQ_CV_GateInversionGet(u8 gate);
extern s32 SEQ_CV_GateInversionAllSet(u32 mask);
extern u32 SEQ_CV_GateInversionAllGet(void);

extern s32 SEQ_CV_SusKeySet(u8 gate, u8 sus_key);
extern u8 SEQ_CV_SusKeyGet(u8 gate);
extern s32 SEQ_CV_SusKeyAllSet(u32 mask);
extern u32 SEQ_CV_SusKeyAllGet(void);

extern s32 SEQ_CV_DOUT_TriggerWidthSet(u8 width_ms);
extern u8 SEQ_CV_DOUT_TriggerWidthGet(void);

extern s32 SEQ_CV_Update(void);

extern s32 SEQ_CV_SRIO_Prepare(void);
extern s32 SEQ_CV_SRIO_Finish(void);

extern s32 SEQ_CV_DOUT_TriggerUpdate(void);

extern s32 SEQ_CV_SendPackage(u8 cv_port, mios32_midi_package_t package);

extern s32 SEQ_CV_ResetAllChannels(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

u16 seq_cv_clkout_divider[SEQ_CV_NUM_CLKOUT];
u8  seq_cv_clkout_pulsewidth[SEQ_CV_NUM_CLKOUT];

#endif
#endif /* _SEQ_CV_H */
