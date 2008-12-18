// $Id$
/*
 * Header file for BPM generator routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_BPM_H
#define _SEQ_BPM_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// MIOS32 timer used by BPM generator (0..2)
#ifndef SEQ_BPM_MIOS32_TIMER_NUM
#define SEQ_BPM_MIOS32_TIMER_NUM 0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_BPM_MODE_Auto,
  SEQ_BPM_MODE_Master,
  SEQ_BPM_MODE_Slave
} seq_bpm_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_BPM_Init(u32 mode);

extern seq_bpm_mode_t SEQ_BPM_ModeGet(void);
extern s32 SEQ_BPM_ModeSet(seq_bpm_mode_t mode);

extern s32 SEQ_BPM_Get(void);
extern s32 SEQ_BPM_Set(u16 bpm);

extern s32 SEQ_BPM_PPQN_Get(void);
extern s32 SEQ_BPM_PPQN_Set(u16 ppqn);

extern u32 SEQ_BPM_TickGet(void);
extern s32 SEQ_BPM_TickSet(u32 tick);

extern s32 SEQ_BPM_IsRunning(void);
extern s32 SEQ_BPM_IsMaster(void);
extern s32 SEQ_BPM_CheckAutoMaster(void);

extern s32 SEQ_BPM_NotifyMIDIRx(u8 midi_byte);

extern s32 SEQ_BPM_TapTempo(void);

extern s32 SEQ_BPM_Start(void);
extern s32 SEQ_BPM_Cont(void);
extern s32 SEQ_BPM_Stop(void);

extern s32 SEQ_BPM_ChkReqStop(void);
extern s32 SEQ_BPM_ChkReqStart(void);
extern s32 SEQ_BPM_ChkReqCont(void);
extern s32 SEQ_BPM_ChkReqClk(u32 *bpm_tick_ptr);
extern s32 SEQ_BPM_ChkReqSongPos(u16 *song_pos);

extern u32 SEQ_BPM_TicksFor_mS(u16 time_ms);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_BPM_H */
