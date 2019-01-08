// $Id$
/*
 * Sequencer functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_SEQ_H
#define _MBNG_SEQ_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_SEQ_Init(u32 mode);

extern s32 MBNG_SEQ_Reset(void);
extern s32 MBNG_SEQ_Handler(void);

extern s32 MBNG_SEQ_PauseEnabled(void);
extern s32 MBNG_SEQ_SetPauseMode(u8 enable);

extern s32 MBNG_SEQ_PlayButton(void);
extern s32 MBNG_SEQ_StopButton(void);
extern s32 MBNG_SEQ_PauseButton(void);
extern s32 MBNG_SEQ_PlayStopButton(void);

extern s32 MBNG_SEQ_ClockDividerGet();
extern s32 MBNG_SEQ_ClockDividerSet(u8 divider);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_SEQ_H */
