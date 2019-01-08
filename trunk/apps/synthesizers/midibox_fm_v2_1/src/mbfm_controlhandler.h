// $Id: mbfm_controlhandler.h $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 synth engine control handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBFM_CONTROLHANDLER_H
#define _MBFM_CONTROLHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MBFM_MAINMODE_FM = 0,
  MBFM_MAINMODE_MIDI = 1,
  MBFM_MAINMODE_SEQ = 2,
  MBFM_MAINMODE_WTED = 3,
} mbfm_mainmode_t;

typedef enum {
  MBFM_HOLD_NONE = 0,

  MBFM_HOLD_DUPL,
  MBFM_HOLD_LINK,
  
  MBFM_HOLD_OPPARAM,
  MBFM_HOLD_WAVE,
  MBFM_HOLD_ALG,
  MBFM_HOLD_DEST,
  
  MBFM_HOLD_ASGN,
  /*
  MBFM_HOLD_ASGN_EG,
  MBFM_HOLD_ASGN_LFO,
  MBFM_HOLD_ASGN_WT,
  MBFM_HOLD_ASGN_VEL,
  MBFM_HOLD_ASGN_MOD,
  MBFM_HOLD_ASGN_VARI,
  */
  MBFM_HOLD_LFOWAVE,
  
  MBFM_HOLD_MUTE,
  MBFM_HOLD_SOLO,
  MBFM_HOLD_SEC,
  
  MBFM_HOLD_SELECT,
} mbfm_holdmode_t;

typedef enum {
  MBFM_SCREENMODE_NONE,
  
  MBFM_SCREENMODE_FM_MOD,
  MBFM_SCREENOMDE_FM_AOUT,
  MBFM_SCREENMODE_FM_OPT,
  MBFM_SCREENMODE_FM_OPL3,
  MBFM_SCREENMODE_FM_TEMPER,
  
  MBFM_SCREENMODE_MIDI_LOAD,
  MBFM_SCREENMODE_MIDI_SAVE,
  MBFM_SCREENMODE_MIDI_NAME,
  MBFM_SCREENMODE_MIDI_AUTO,
  MBFM_SCREENMODE_MIDI_MIXER,
  MBFM_SCREENMODE_MIDI_IN,
  MBFM_SCREENMODE_MIDI_OUT,
  
} mbfm_screenmode_t;


extern mbfm_mainmode_t   mainmode;
extern mbfm_holdmode_t   holdmode;
extern u8                holdmodeidx;
extern mbfm_screenmode_t screenmode;
extern mbfm_screenmode_t screenmodereturn;
extern u8                screenmodeidx;
extern u8                zeromode;
extern u8                act_opl3;
extern u8                act_voice;
extern u8                act_midich;
extern u8                act_temper;
extern u8                act_modconn;
extern u8                ctlrefreshvolume;


extern void MBFM_SelectOPL3(u8 value);
extern void MBFM_SelectVoice(u8 voice, u8 value);
extern void MBFM_SelectMode(u8 mode, u8 value);
extern void MBFM_BtnSoftkey(u8 softkey, u8 value);
extern void MBFM_BtnSelect(u8 value);
extern void MBFM_BtnMenu(u8 value);
extern void MBFM_BtnDupl(u8 value);
extern void MBFM_BtnLink(u8 value);
extern void MBFM_BtnRetrig(u8 value);
extern void MBFM_BtnDlyScale(u8 value);
extern void MBFM_Btn4Op(u8 value);
extern void MBFM_BtnAlg(u8 value);
extern void MBFM_BtnDest(u8 voicehalf, u8 value);
extern void MBFM_BtnOpParam(u8 op, u8 value);
extern void MBFM_BtnOpWave(u8 op, u8 value);
extern void MBFM_BtnOpMute(u8 op, u8 value);
extern void MBFM_BtnEGAssign(u8 eg, u8 value);
extern void MBFM_BtnLFOAssign(u8 lfo, u8 value);
extern void MBFM_BtnLFOWave(u8 lfo, u8 value);
extern void MBFM_BtnWTAssign(u8 wt, u8 value);
extern void MBFM_BtnVelAssign(u8 value);
extern void MBFM_BtnModAssign(u8 value);
extern void MBFM_BtnVariAssign(u8 value);

//For CCs
extern void MBFM_SetDatawheel(u8 value);
extern void MBFM_SetVoiceTranspose(s16 value);
extern void MBFM_SetVoiceTune(s16 value);
extern void MBFM_SetVoicePorta(s16 value);
extern void MBFM_SetVoiceDlyTime(s16 value);
extern void MBFM_SetVoiceFB(s16 value);
extern void MBFM_SetOpFMult(u8 op, s16 value);
extern void MBFM_SetOpAtk(u8 op, s16 value);
extern void MBFM_SetOpDec(u8 op, s16 value);
extern void MBFM_SetOpSus(u8 op, s16 value);
extern void MBFM_SetOpRel(u8 op, s16 value);
extern void MBFM_SetOpVolume(u8 op, s16 value);
extern void MBFM_SetEGAtk(u8 eg, s16 value);
extern void MBFM_SetEGDec1(u8 eg, s16 value);
extern void MBFM_SetEGLevel(u8 eg, s16 value);
extern void MBFM_SetEGDec2(u8 eg, s16 value);
extern void MBFM_SetEGSus(u8 eg, s16 value);
extern void MBFM_SetEGRel(u8 eg, s16 value);
extern void MBFM_SetLFOFreq(u8 lfo, s16 value);
extern void MBFM_SetLFODelay(u8 lfo, s16 value);

//For encoders, expecting to receive data as 0x40 +/- <delta>
extern void MBFM_IncDatawheel(u8 inc);
extern void MBFM_IncVoiceTranspose(u8 inc);
extern void MBFM_IncVoiceTune(u8 inc);
extern void MBFM_IncVoicePorta(u8 inc);
extern void MBFM_IncVoiceDlyTime(u8 inc);
extern void MBFM_IncVoiceFB(u8 inc);
extern void MBFM_IncOpFMult(u8 op, u8 inc);
extern void MBFM_IncOpAtk(u8 op, u8 inc);
extern void MBFM_IncOpDec(u8 op, u8 inc);
extern void MBFM_IncOpSus(u8 op, u8 inc);
extern void MBFM_IncOpRel(u8 op, u8 inc);
extern void MBFM_IncOpVolume(u8 op, u8 inc);
extern void MBFM_IncEGAtk(u8 eg, u8 inc);
extern void MBFM_IncEGDec1(u8 eg, u8 inc);
extern void MBFM_IncEGLevel(u8 eg, u8 inc);
extern void MBFM_IncEGDec2(u8 eg, u8 inc);
extern void MBFM_IncEGSus(u8 eg, u8 inc);
extern void MBFM_IncEGRel(u8 eg, u8 inc);
extern void MBFM_IncLFOFreq(u8 lfo, u8 inc);
extern void MBFM_IncLFODelay(u8 lfo, u8 inc);

//Core parameter change handler
extern s16 MBFM_ControlHandler(u8 param, s8 assignsrc, u8 origvoice, s16 value, u8 inc);

//For control elements on your keyboard
extern void MBFM_ControlMod(u8 channel, u8 value);
extern void MBFM_ControlVari(u8 channel, u8 value);
extern void MBFM_ControlSus(u8 channel, u8 value);
extern void MBFM_ControlCC(u8 channel, u8 cc, u8 value);

extern void MBFM_DrawScreen();
extern void MBFM_SetUpLCDChars();

extern void MBFM_Control_Init();
extern void MBFM_Control_Tick(u32 time);

extern char* NoteName(u8 midinote);
extern char* MIDIPortName(u8 midiport);

#ifdef __cplusplus
}
#endif

#endif /* _MBFM_CONTROLHANDLER_H */
