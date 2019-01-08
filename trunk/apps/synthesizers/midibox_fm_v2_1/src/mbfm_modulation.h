// $Id: mbfm_modulation.h $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 synth engine parameter modulation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "mbfm.h"

#ifndef _MBFM_MODULATION_H
#define _MBFM_MODULATION_H

#ifdef __cplusplus
extern "C" {
#endif

//Number of modulation connections allowed per voice, doubled for 4-op voice
#ifndef MBFM_MODL_NUMCONN
#define MBFM_MODL_NUMCONN 16
#endif

#ifndef MBFM_DLY_STEPS
#define MBFM_DLY_STEPS 128
#endif

#if MBFM_DLY_STEPS > 256
#error "MBFM_DLY_STEPS has max size of 256 (even if you can fit more in RAM for some reason!)"
#endif

#ifndef MBFM_WT_LEN
#define MBFM_WT_LEN 32
#endif

typedef union {
  u8 data[8];
  struct{
    u32 data1;
    u32 data2;
  };
  struct{
    u8 wave;
    u8 fmult;
    u8 atk;
    u8 dec;
    u8 sus;
    u8 rel;
    u8 vol;
    union{
      u8 bits;
      struct{
        u8 frqLFO:1;
        u8 ampLFO:1;
        u8 ampKSCL:2;
        u8 rateKSCL:1;
        u8 dosus:1;
        u8 mute:1;
        u8 unused:1;
      };
    };
  };
} mbfm_opparams_t;

typedef union {
  u8 data[6];
  struct{
    u16 data1;
    u16 data2;
    u16 data3;
  };
  struct{
    s8 tp;
    s8 tune;
    u8 porta;
    u8 dlytime;
    u8 feedback;
    union{
      u8 bits;
      struct{
        u8 alg:2;
        u8 retrig:1;
        u8 dlyscale:1;
        u8 dest:4;
      };
    };
  };
} mbfm_voiceparams_t;

typedef union {
  u8 data[13];
  struct{
    u8 EGatk;
    u8 EGdec1;
    u8 EGlvl;
    u8 EGdec2;
    u8 EGsus;
    u8 EGrel;
    u8 LFOfrq[2];
    u8 LFOdly[2];
    u8 LFOwave[2]; //Highest bit is FreeRunning
    u8 WTfrq;
  };
} mbfm_modlparams_t;

typedef struct {
  u8 src;   //11 sources available
  u8 dest;
  s8 depth;
  s8 pre_depth;
} mbfm_modlentry_t;


//Synth parameters, before modulation
extern mbfm_opparams_t pre_opparams[36*OPL3_COUNT];
extern mbfm_voiceparams_t pre_voiceparams[18*OPL3_COUNT];
extern mbfm_modlparams_t pre_modlparams[18*OPL3_COUNT];

//Parameters including modulation
extern mbfm_voiceparams_t voiceparams[18*OPL3_COUNT];
extern mbfm_modlparams_t modlparams[18*OPL3_COUNT];

//Modulation connections
extern mbfm_modlentry_t modllists[18*OPL3_COUNT][MBFM_MODL_NUMCONN];
extern u8 modl_lastsrc;
extern u8 modl_lastdest;

//Delay line
extern notevel_t delaytape[18*OPL3_COUNT][MBFM_DLY_STEPS];
//TODO this is a function of delay scale
extern u8 msecperdelaystep;
extern u8 delayrechead[18*OPL3_COUNT];

//Wavetable
extern s8 wavetable[18*OPL3_COUNT][MBFM_WT_LEN];

//Passive modulators
extern u8 modl_mods[18*OPL3_COUNT];
extern u8 modl_varis[18*OPL3_COUNT];
extern s8 pitchbends[18*OPL3_COUNT];
extern u8 pitchbendrange; //in half-steps, each way
extern u8 tuningrange; //Number of half-steps a full turn in one direction of tune is

//Portamento
//extern u8 portamode; //1 is constspeed, 0 consttime 
//Currently set to consttime mode, i.e. glide takes same time for long or short gap
extern u8 portastartnote[18*OPL3_COUNT];
extern u32 portastarttime[18*OPL3_COUNT];

//EG and LFO
extern u32 egstarttime[18*OPL3_COUNT];
extern u8  egmode[18*OPL3_COUNT]; //0 ADLDS, 1 R
extern s8  egrelstartval[18*OPL3_COUNT];
extern u32 lfostarttime[18*OPL3_COUNT]; //Used for both LFOs and WT
extern s8  lforandval[18*OPL3_COUNT][2];
extern u8  lforandstate[18*OPL3_COUNT];

extern u32 last_time;
extern u32 dly_last_time;

//

extern void MBFM_Modulation_Init();
extern void MBFM_Modulation_Tick(u32 time);

extern u8 MBFM_GetNumModulationConnections(u8 voice);
extern s8 MBFM_GetModDepth(u8 voice, u8 src, u8 dest);
extern u8 MBFM_FindLastModDest(u8 voice, u8 src);
extern s8 MBFM_TrySetModulation(u8 voice, u8 src, u8 dest, s8 depth); //Returns -1 if failed, 0 if modified, 1 if added or removed
extern void StartModulatorsNow(u8 voice, u32 time);
extern void MBFM_InitVoiceValues(u8 voice);
extern void MBFM_InitPercValues(u8 voice);
extern void MBFM_ModValuesToOPL3(u8 voice);
extern void MBFM_SetVoiceOutputVolume(u8 voice, u8 midivalue);

extern const char* MBFM_GetModSourceName(u8 source);
extern const char* MBFM_GetParamName(u8 param);
extern s16 MBFM_GetParamValue(u8 voice, u8 param);
extern s16 MBFM_SetParamValue(u8 voice, u8 param, s16 value); //Returns the actual value set to


#ifdef __cplusplus
}
#endif

#endif /* _MBFM_MODULATION_H */
