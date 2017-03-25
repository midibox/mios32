/*
 * MIDIbox Quad Genesis: Synth engine header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYENG_H
#define _SYENG_H

#include <genesis.h>
#include <vgm.h>


typedef union {
    u16 ALL;
    struct {
        u8 pi_using;
        u8 use:2; //3 for tracker mode, 2 for in use, 1 for standby, 0 for none
        u8 lfo:1;
        u8 beingcleared:1;
        u8 dummy:4;
    };
} syngenesis_usage_t;

typedef struct {
    union{
        u8 optionbits;
        struct{
            u8 lfomode:2; //0: untouched | 1: set to constant lfofixedspeed | 2 or 3: messed with
            u8 lfofixedspeed:3;
            u8 noisefreqsq3:1;
            u8 dummy:2;
        };
    };
    u8 dummy2;
    u16 dummy3;
    syngenesis_usage_t channels[12];
} syngenesis_t;

extern syngenesis_t syngenesis[GENESIS_COUNT];

typedef struct {
    VgmUsageBits usage;
    VgmSource* initsource;
    VgmSource* noteonsource;
    VgmSource* noteoffsource;
    char name[13];
    u8 rootnote;
    u16 dummy2;
} synprogram_t;


typedef struct {
    u8 valid:1;
    u8 isstatic:1;
    u8 playing:1;
    u8 playinginit:1;
    u8 waitingforclear:1;
    u8 needsnewinit:1;
    u8 dummy:2;
    u8 sourcechannel;
    u8 note;
    u8 dummy2;
    u32 recency;
    VgmHead_Channel mapping[12];
    VgmHead* head;
} synproginstance_t;

#ifndef MBQG_NUM_PROGINSTANCES
#define MBQG_NUM_PROGINSTANCES 10*GENESIS_COUNT
#endif

extern synproginstance_t proginstances[MBQG_NUM_PROGINSTANCES];


typedef struct {
    u8 trackermode:1;
    u8 trackervoice:7;
    u8 dummy1;
    u16 dummy2;
    synprogram_t* program;
} synchannel_t;

#ifndef MBQG_NUM_PORTS
#define MBQG_NUM_PORTS 4
#endif

extern synchannel_t channels[16*MBQG_NUM_PORTS];

////////////////////////////////////////////////////////////////////////////////

extern u8 voiceclearfull;

extern VgmSource** SelSource(synprogram_t* prog, u8 num);

extern void SyEng_Init();
extern void SyEng_Tick();

extern void SyEng_Note_On(mios32_midi_package_t pkg);
extern void SyEng_Note_Off(mios32_midi_package_t pkg);

extern void SyEng_ClearVoice(u8 g, u8 v);
extern void SyEng_HardFlushProgram(synprogram_t* prog);
extern void SyEng_SoftFlushProgram(synprogram_t* prog);
extern void SyEng_RecalcSourceAndProgramUsage(synprogram_t* prog, VgmSource* srcchanged);

extern void SyEng_DeleteSource(VgmSource* src);
extern void SyEng_DeleteProgram(u8 chan);

extern u8 SyEng_GetStaticPI(VgmUsageBits usage);
extern void SyEng_ReleaseStaticPI(u8 piindex);
extern void SyEng_PlayVGMOnPI(synproginstance_t* pi, VgmSource* source, u8 rootnote, u8 startplaying);
extern void SyEng_SilencePI(synproginstance_t* pi);

extern void SyEng_PrintEngineDebugInfo();

#endif /* _SYENG_H */
