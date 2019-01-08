// $Id: mbng_cv.h 1985 2014-05-03 19:44:11Z tk $
/*
 * CV access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_CV_H
#define _MBNG_CV_H

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_CV_Init(u32 mode);

extern s32 MBNG_CV_PitchRangeSet(u8 cv, u8 value);
extern s32 MBNG_CV_PitchRangeGet(u8 cv);
extern s32 MBNG_CV_PitchSet(u8 cv, s16 value);
extern s32 MBNG_CV_PitchGet(u8 cv);
extern s32 MBNG_CV_TransposeOctaveSet(u8 cv, s8 value);
extern s32 MBNG_CV_TransposeOctaveGet(u8 cv);
extern s32 MBNG_CV_TransposeSemitonesSet(u8 cv, s8 value);
extern s32 MBNG_CV_TransposeSemitonesGet(u8 cv);

extern s32 MBNG_CV_Update(void);

extern s32 MBNG_CV_NotifyReceivedValue(mbng_event_item_t *item);

extern s32 MBNG_CV_ResetAllChannels(void);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_CV_H */
