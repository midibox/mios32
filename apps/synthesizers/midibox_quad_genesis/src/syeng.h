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
    u32 all;
    struct {
        //Don't change the order of these bits, they're accessed by bit mask operations
        u8 fm1:1;
        u8 fm2:1;
        u8 fm3:1;
        u8 fm4:1;
        u8 fm5:1;
        u8 fm6:1;
        
        u8 fm1_lfo:1;
        u8 fm2_lfo:1;
        u8 fm3_lfo:1;
        u8 fm4_lfo:1;
        u8 fm5_lfo:1;
        u8 fm6_lfo:1;
        
        u8 dac:1;
        u8 fm3_special:1;
        u8 opn2_globals:1;
        u8 lfofixed:1;
        
        u8 lfofixedspeed:3;
        u8 dummy:5;
        
        u8 sq1:1;
        u8 sq2:1;
        u8 sq3:1;
        u8 noise:1;
        u8 noisefreqsq3:1;
        u8 dummy2:3;
    };
} usage_bits_t;

typedef union {
    u8 ALL;
    struct {
        u8 pi_using:6;
        u8 use:2; //3 for tracker mode, 2 for in use, 1 for standby, 0 for none
    };
} syngenesis_usage_t;

typedef struct {
    union{
        u8 lfobits;
        struct{
            u8 fm1_lfo:1;
            u8 fm2_lfo:1;
            u8 fm3_lfo:1;
            u8 fm4_lfo:1;
            u8 fm5_lfo:1;
            u8 fm6_lfo:1;
            u8 dummy:2;
        };
    };
    union{
        u8 optionbits;
        struct{
            u8 lfovaries:1;
            u8 lfofixed:1;
            u8 lfofixedspeed:3;
            u8 noisefreqsq3:1;
            u8 dummy2:2;
        };
    };
    u16 dummy3;
    syngenesis_usage_t channels[12];
} syngenesis_t;

extern syngenesis_t syngenesis[GENESIS_COUNT];

typedef struct {
    usage_bits_t usage;
    VgmSource* initsource;
    VgmSource* noteonsource;
    VgmSource* noteoffsource;
    char name[13];
    u8 rootnote;
    u16 dummy2;
} synprogram_t;


typedef struct {
    u8 valid:1;
    u8 playing:1;
    u8 playinginit:1;
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

#if (MBQG_NUM_PROGINSTANCES > 64)
#error "Too many proginstances (6 bit index)!"
#endif

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

extern void SyEng_Init();
extern void SyEng_Tick();

extern void SyEng_Note_On(mios32_midi_package_t pkg);
extern void SyEng_Note_Off(mios32_midi_package_t pkg);

#endif /* _SYENG_H */
