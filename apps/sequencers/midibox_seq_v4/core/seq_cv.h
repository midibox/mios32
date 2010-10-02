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

#include <aout.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

//#define SEQ_CV_NUM_IF AOUT_NUM_IF
// INTDAC clashes with SD Card port, therefore it's not used (it's always located at last position)
#define SEQ_CV_NUM_IF (AOUT_NUM_IF-1)

// number of CV channels (max)
#define SEQ_CV_NUM 8

// selects V/Oct, Hz/V, Inv.
#define SEQ_CV_NUM_CURVES 3

// selects calibration mode
#define SEQ_CV_NUM_CALI_MODES AOUT_NUM_CALI_MODES

// size of notestack(s)
#define SEQ_CV_NOTESTACK_SIZE 10


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CV_Init(u32 mode);

extern s32 SEQ_CV_IfSet(aout_if_t if_type);
extern aout_if_t SEQ_CV_IfGet(void);
extern const char* SEQ_CV_IfNameGet(void);

extern s32 SEQ_CV_CurveSet(u8 cv, u8 curve);
extern u8 SEQ_CV_CurveGet(u8 cv);
extern const char* SEQ_CV_CurveNameGet(u8 cv);

extern s32 SEQ_CV_CaliModeSet(u8 cv, aout_cali_mode_t mode);
extern aout_cali_mode_t SEQ_CV_CaliModeGet(void);
extern const char* SEQ_CV_CaliNameGet(void);

extern s32 SEQ_CV_SlewRateSet(u8 cv, u8 value);
extern s32 SEQ_CV_SlewRateGet(u8 cv);

extern s32 SEQ_CV_PitchRangeSet(u8 cv, u8 range);
extern u8 SEQ_CV_PitchRangeGet(u8 cv);

extern s32 SEQ_CV_ClkPulseWidthSet(u8 width);
extern u8 SEQ_CV_ClkPulseWidthGet(void);

extern s32 SEQ_CV_ClkDividerSet(u16 div);
extern u16 SEQ_CV_ClkDividerGet(void);

extern s32 SEQ_CV_GateInversionSet(u8 gate, u8 inverted);
extern u8 SEQ_CV_GateInversionGet(u8 gate);
extern s32 SEQ_CV_GateInversionAllSet(u8 mask);
extern u8 SEQ_CV_GateInversionAllGet(void);

extern s32 SEQ_CV_Update(void);

extern s32 SEQ_CV_SendPackage(u8 cv_port, mios32_midi_package_t package);

extern s32 SEQ_CV_ResetAllChannels(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_CV_H */
