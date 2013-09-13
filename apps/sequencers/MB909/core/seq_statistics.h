// $Id: seq_statistics.h 944 2010-03-06 00:57:46Z tk $
/*
 * Header file of MBSEQ Statistics functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_STATISTICS_H
#define _SEQ_STATISTICS_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_STATISTICS_Init(u32 mode);
extern s32 SEQ_STATISTICS_Reset(void);

extern s32 SEQ_STATISTICS_Idle(void);
extern u8 SEQ_STATISTICS_CurrentCPULoad(void);

extern s32 SEQ_STATISTICS_StopwatchInit(void);
extern s32 SEQ_STATISTICS_StopwatchReset(void);
extern s32 SEQ_STATISTICS_StopwatchCapture(void);
extern u32 SEQ_STATISTICS_StopwatchGetValue(void);
extern u32 SEQ_STATISTICS_StopwatchGetValueMax(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_STATISTICS_H */
