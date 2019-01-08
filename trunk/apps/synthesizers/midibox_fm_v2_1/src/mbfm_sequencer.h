// $Id: mbfm_sequencer.h $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 fractal drum sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBFM_SEQUENCER_H
#define _MBFM_SEQUENCER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MBFM_SEQSHOW_ON = 0,
  MBFM_SEQSHOW_VEL = 1,
  MBFM_SEQSHOW_VARI = 2,
  MBFM_SEQSHOW_GATE = 3,
} mbfm_seqshow_t;

typedef union{
  u8 data;
  struct{
    u8 on:1;
    u8 vel:1;
    u8 gate:2;
    u8 vari:1;
    u8 dummy:3;
  };
} mbfm_drumstep_t;

typedef union{
  mbfm_drumstep_t drums[6];
} mbfm_step_t;

typedef union{
  u8 data;
  struct{
    u8 state:1;
    u8 update:2;
    u8 muted:1;
    u8 soloed:1;
    u8 dummy:3;
  };
} mbfm_drumstate_t;


extern mbfm_step_t sequence[OPL3_COUNT*17*4];
extern u8 submeas_enabled[4];
#define TICKS_PER_STEP_SHIFT 4 //If you change this, update table tps_gate_lengths in mbfm_sequencer.c!
#define TICKS_PER_STEP (1 << TICKS_PER_STEP_SHIFT)
extern u32 seq_tempo; //In 256ths of a ms per tick, with TICKS_PER_STEP
//Thus BPM = 60000 * 256 / (seq_tempo * TICKS_PER_STEP * seq_beatlen)
extern u8 seq_swing; //1 to 99
extern u32 seq_starttime;
extern u16 seq_masterpos;
extern u8 seq_tickinstep;
extern u8 seq_mastermeas;
extern u8 seq_measstep;
extern u8 seq_submeas;
extern u8 seq_lastsubmeas;

extern u8 seq_measurelen;
extern u8 seq_beatlen;

extern u8 seq_recording;
extern u8 seq_playing;

extern mbfm_seqshow_t seq_show;
extern u8 seq_selsubmeas;
extern u8 seq_seldrum;

extern mbfm_drumstate_t drumstates[6*OPL3_COUNT];
extern u8 drums_soloed;

//Send velocity == 0 for note off
extern void MBFM_Drum_ReceiveMIDITrigger(u8 drum, u8 velocity);
extern void MBFM_Drum_ReceivePanelTrigger(u8 drum, u8 state);
extern u8   MBFM_Drum_GetChannel(u8 drum);
extern void MBFM_Drum_RecordNow(u8 drum, u8 state);

extern const char *MBFM_Drum_GetName(u8 drum);
extern const char *MBFM_CondensedDrum_GetName(u8 drum);

extern void MBFM_SEQ_Init();
extern void MBFM_SEQ_Tick(u32 time);
extern void MBFM_SEQ_PostTick(u32 time);

extern void MBFM_BtnSeqRec(u8 value);
extern void MBFM_BtnSeqPlay(u8 value);
extern void MBFM_BtnSeqBack(u8 value);
extern void MBFM_BtnSeqMute(u8 value);
extern void MBFM_BtnSeqSolo(u8 value);
extern void MBFM_BtnSeqVel(u8 value);
extern void MBFM_BtnSeqVari(u8 value);
extern void MBFM_BtnSeqGate(u8 value);
extern void MBFM_BtnSeqTrigger(u8 instrument, u8 value);
extern void MBFM_BtnSeqMeasure(u8 measure, u8 value);
extern void MBFM_BtnSeqSection(u8 value);
extern void MBFM_SEQ_TrackButton(u8 button, u8 value);
extern void MBFM_SEQ_Softkey(u8 button, u8 value);
extern void MBFM_SEQ_Datawheel(s16 realinc);

extern void MBFM_SEQ_RedrawTrack();
extern void MBFM_SEQ_RedrawAll();

extern void MBFM_Drum_InitPercussionMode(u8 chip);


#ifdef __cplusplus
}
#endif

#endif /* _MBFM_SEQUENCER_H */
