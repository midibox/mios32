/*
 * MIDIbox Quad Genesis: Genesis State Drawing Functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "genesisstate.h"

#include <genesis.h>
#include <vgm.h>
#include "frontpanel.h"

typedef struct {
    u8 lastdac;
    u8 timer;
    u16 dacvu:9;
    u8 playing:1;
    u8 dummy:6;
} DAC_State_t;

DAC_State_t dacstates[GENESIS_COUNT];

void GenesisState_Tick(){
    u8 g, dac, de, playing;
    for(g=0; g<GENESIS_COUNT; ++g){
        dac = genesis[g].opn2.dac_high;
        de = genesis[g].opn2.dac_enable;
        playing = de && (dac != dacstates[g].lastdac);
        dacstates[g].playing = playing;
        if(playing){
            if(dac >= 0xC0 || dac < 0x40) dacstates[g].dacvu = 0x1FF;
            else if(dac >= 0xA0 || dac < 0x60) dacstates[g].dacvu = 0x0FF;
            else if(dac >= 0x90 || dac < 0x70) dacstates[g].dacvu = 0x07F;
            else dacstates[g].dacvu = 0x03F;
            dacstates[g].timer = 0;
        }else if(!de){
            dacstates[g].dacvu = 0;
        }else{
            ++dacstates[g].timer;
            if(dacstates[g].timer > 50){
                dacstates[g].dacvu >>= 1;
                dacstates[g].timer = 0;
            }
        }
        dacstates[g].lastdac = genesis[g].opn2.dac_high;
    }
}

void DrawGenesisActivity(u8 g){
    //FM voices
    if(g >= GENESIS_COUNT) return;
    u8 v, o, k;
    for(v=0; v<6; v++){
        k = 0;
        for(o=0; o<4; o++){
            k |= genesis[g].opn2.chan[v].op[0].kon;
        }
        FrontPanel_GenesisLEDSet(g, v+1, 0, k);
    }
    //DAC
    FrontPanel_GenesisLEDSet(g, 7, 0, dacstates[g].playing);
    //PSG voices
    for(v=0; v<4; v++){
        FrontPanel_GenesisLEDSet(g, v+8, 0, genesis[g].psg.voice[v].atten != 0xF);
    }
}

#define DETUNE_DISPLAY(d) (((d) >= 4) ? (7 - (d)) : (3 + (d)))
void DrawGenesisState_Op(u8 g, u8 chan, u8 op){
    FrontPanel_LEDRingSet(FP_LEDR_HARM, 1, genesis[g].opn2.chan[chan].op[op].fmult);
    FrontPanel_LEDRingSet(FP_LEDR_DETUNE, 0, DETUNE_DISPLAY(genesis[g].opn2.chan[chan].op[op].detune));
    FrontPanel_LEDRingSet(FP_LEDR_ATTACK, 1, genesis[g].opn2.chan[chan].op[op].attackrate >> 1);
    FrontPanel_LEDRingSet(FP_LEDR_DEC1R, 1, genesis[g].opn2.chan[chan].op[op].decay1rate >> 1);
    FrontPanel_LEDRingSet(FP_LEDR_DECLVL, 1, 15 - genesis[g].opn2.chan[chan].op[op].decaylevel);
    FrontPanel_LEDRingSet(FP_LEDR_DEC2R, 1, genesis[g].opn2.chan[chan].op[op].decay2rate >> 1);
    FrontPanel_LEDRingSet(FP_LEDR_RELRATE, 1, genesis[g].opn2.chan[chan].op[op].releaserate);
    FrontPanel_LEDSet(FP_LED_KSR, genesis[g].opn2.chan[chan].op[op].ratescale);
    FrontPanel_LEDSet(FP_LED_SSGON, genesis[g].opn2.chan[chan].op[op].ssg_enable);
    FrontPanel_LEDSet(FP_LED_SSGINIT, genesis[g].opn2.chan[chan].op[op].ssg_init);
    FrontPanel_LEDSet(FP_LED_SSGTGL, genesis[g].opn2.chan[chan].op[op].ssg_toggle);
    FrontPanel_LEDSet(FP_LED_SSGHOLD, genesis[g].opn2.chan[chan].op[op].ssg_hold);
    FrontPanel_LEDSet(FP_LED_LFOAM, genesis[g].opn2.chan[chan].op[op].amplfo);
    if(genesis[g].opn2.ch3_mode != 0 && chan == 2){
        if(op == 0){
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + genesis[g].opn2.ch3op1_block);
            FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)genesis[g].opn2.ch3op1_fnum_high << 8) | genesis[g].opn2.ch3op1_fnum_low);
        }else if(op == 1){
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + genesis[g].opn2.ch3op2_block);
            FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)genesis[g].opn2.ch3op2_fnum_high << 8) | genesis[g].opn2.ch3op2_fnum_low);
        }else if(op == 2){
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + genesis[g].opn2.ch3op3_block);
            FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)genesis[g].opn2.ch3op3_fnum_high << 8) | genesis[g].opn2.ch3op3_fnum_low);
        }else{
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + genesis[g].opn2.chan[2].block);
            FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)genesis[g].opn2.chan[2].fnum_high << 8) | genesis[g].opn2.chan[2].fnum_low);
        }
    }
}
void ClearGenesisState_Op(){
    FrontPanel_LEDRingSet(FP_LEDR_HARM, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_DETUNE, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_ATTACK, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_DEC1R, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_DECLVL, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_DEC2R, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_RELRATE, 0xFF, 0);
    FrontPanel_LEDSet(FP_LED_KSR, 0);
    FrontPanel_LEDSet(FP_LED_SSGON, 0);
    FrontPanel_LEDSet(FP_LED_SSGINIT, 0);
    FrontPanel_LEDSet(FP_LED_SSGTGL, 0);
    FrontPanel_LEDSet(FP_LED_SSGHOLD, 0);
    FrontPanel_LEDSet(FP_LED_LFOAM, 0);
    /*
    if(genesis[g].opn2.ch3_mode != 0 && chan == 2){
        FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_1, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_2, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_3, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_4, ' ');
    }
    */
}
void DrawGenesisState_Chan(u8 g, u8 chan){
    u8 op;
    for(op=0; op<4; ++op){
        FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + op, 1, (127 - genesis[g].opn2.chan[chan].op[op].totallevel) >> 3);
        FrontPanel_LEDSet(FP_LED_KEYON_1 + op, genesis[g].opn2.chan[chan].op[op].kon);
    }
    if(!(genesis[g].opn2.ch3_mode != 0 && chan == 2)){
        FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + genesis[g].opn2.chan[chan].block);
        FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)genesis[g].opn2.chan[chan].fnum_high << 8) | genesis[g].opn2.chan[chan].fnum_low);
    }
    FrontPanel_LEDRingSet(FP_LEDR_LFOFDEP, 1, genesis[g].opn2.chan[chan].lfofreqd);
    FrontPanel_LEDRingSet(FP_LEDR_LFOADEP, 1, genesis[g].opn2.chan[chan].lfoampd);
    FrontPanel_LEDRingSet(FP_LEDR_FEEDBACK, 1, genesis[g].opn2.chan[chan].feedback);
    FrontPanel_LEDSet(FP_LED_FEEDBACK, genesis[g].opn2.chan[chan].feedback > 0);
    FrontPanel_DrawAlgorithm(genesis[g].opn2.chan[chan].algorithm);
    FrontPanel_LEDSet(FP_LED_OUTL, genesis[g].opn2.chan[chan].out_l);
    FrontPanel_LEDSet(FP_LED_OUTR, genesis[g].opn2.chan[chan].out_r);
}
void ClearGenesisState_Chan(){
    u8 op;
    for(op=0; op<4; ++op){
        FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + op, 0xFF, 0);
        FrontPanel_LEDSet(FP_LED_KEYON_1 + op, 0);
    }
    //if(!(genesis[g].opn2.ch3_mode != 0 && chan == 2)){
        FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_1, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_2, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_3, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_4, ' ');
    //}
    FrontPanel_LEDRingSet(FP_LEDR_LFOFDEP, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_LFOADEP, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_FEEDBACK, 0xFF, 0);
    FrontPanel_LEDSet(FP_LED_FEEDBACK, 0);
    FrontPanel_DrawAlgorithm(0xFF);
    FrontPanel_LEDSet(FP_LED_OUTL, 0);
    FrontPanel_LEDSet(FP_LED_OUTR, 0);
}
void DrawGenesisState_DAC(u8 g){
    FrontPanel_DrawDACValue(dacstates[g].dacvu);
    FrontPanel_LEDSet(FP_LED_DACEN, genesis[g].opn2.dac_enable);
}
void ClearGenesisState_DAC(){
    FrontPanel_DrawDACValue(0);
    FrontPanel_LEDSet(FP_LED_DACEN, 0);
}
void DrawGenesisState_OPN2(u8 g){
    FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, 1, genesis[g].opn2.timera_high >> 4);
    FrontPanel_LEDRingSet(FP_LEDR_LFOFREQ, 1, genesis[g].opn2.lfo_freq);
    FrontPanel_LEDSet(FP_LED_CH3_NORMAL, genesis[g].opn2.ch3_mode == 0);
    FrontPanel_LEDSet(FP_LED_CH3_4FREQ, genesis[g].opn2.ch3_mode & 1);
    FrontPanel_LEDSet(FP_LED_CH3_CSM, genesis[g].opn2.ch3_mode == 2);
    FrontPanel_LEDSet(FP_LED_CH3FAST, genesis[g].opn2.test_tmrspd);
    FrontPanel_LEDSet(FP_LED_UGLY, genesis[g].opn2.test_ugly);
    FrontPanel_LEDSet(FP_LED_UGLY, genesis[g].opn2.test_ugly);
    FrontPanel_LEDSet(FP_LED_DACOVR, genesis[g].opn2.dac_override);
    FrontPanel_LEDSet(FP_LED_LFO, genesis[g].opn2.lfo_enabled);
    FrontPanel_LEDSet(FP_LED_EG, !genesis[g].opn2.test_noeg);
}
void ClearGenesisState_OPN2(){
    FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_LFOFREQ, 0xFF, 0);
    FrontPanel_LEDSet(FP_LED_CH3_NORMAL, 0);
    FrontPanel_LEDSet(FP_LED_CH3_4FREQ, 0);
    FrontPanel_LEDSet(FP_LED_CH3_CSM, 0);
    FrontPanel_LEDSet(FP_LED_CH3FAST, 0);
    FrontPanel_LEDSet(FP_LED_UGLY, 0);
    FrontPanel_LEDSet(FP_LED_UGLY, 0);
    FrontPanel_LEDSet(FP_LED_DACOVR, 0);
    FrontPanel_LEDSet(FP_LED_LFO, 0);
    FrontPanel_LEDSet(FP_LED_EG, 0);
}
void DrawGenesisState_PSG(u8 g, u8 voice){
    g &= 3;
    voice &= 3;
    FrontPanel_LEDRingSet(FP_LEDR_PSGVOL, 1, 15 - genesis[g].psg.voice[voice].atten);
    if(voice == 3){
        //Noise channel
        FrontPanel_LEDSet(FP_LED_NS_HI,  genesis[g].psg.noise.rate == 0);
        FrontPanel_LEDSet(FP_LED_NS_MED, genesis[g].psg.noise.rate == 1);
        FrontPanel_LEDSet(FP_LED_NS_LOW, genesis[g].psg.noise.rate == 2);
        FrontPanel_LEDSet(FP_LED_NS_SQ3, genesis[g].psg.noise.rate == 3);
        FrontPanel_LEDSet(FP_LED_NS_PLS, genesis[g].psg.noise.type == 0);
        FrontPanel_LEDSet(FP_LED_NS_WHT, genesis[g].psg.noise.type == 1);
        FrontPanel_LEDRingSet(FP_LEDR_PSGFREQ, 0xFF, 0);
    }else{
        FrontPanel_LEDSet(FP_LED_NS_HI,  0);
        FrontPanel_LEDSet(FP_LED_NS_MED, 0);
        FrontPanel_LEDSet(FP_LED_NS_LOW, 0);
        FrontPanel_LEDSet(FP_LED_NS_SQ3, 0);
        FrontPanel_LEDSet(FP_LED_NS_PLS, 0);
        FrontPanel_LEDSet(FP_LED_NS_WHT, 0);
        FrontPanel_LEDRingSet(FP_LEDR_PSGFREQ, 0, ((1023 - genesis[g].psg.square[voice].freq) >> 4) & 0xF);
    }
}
void ClearGenesisState_PSG(){
    FrontPanel_LEDRingSet(FP_LEDR_PSGVOL, 0xFF, 0);
    FrontPanel_LEDRingSet(FP_LEDR_PSGFREQ, 0xFF, 0);
    FrontPanel_LEDSet(FP_LED_NS_HI,  0);
    FrontPanel_LEDSet(FP_LED_NS_MED, 0);
    FrontPanel_LEDSet(FP_LED_NS_LOW, 0);
    FrontPanel_LEDSet(FP_LED_NS_SQ3, 0);
    FrontPanel_LEDSet(FP_LED_NS_PLS, 0);
    FrontPanel_LEDSet(FP_LED_NS_WHT, 0);
}
void DrawGenesisState_All(u8 g, u8 voice, u8 op){
    g &= 3;
    op &= 3;
    if(voice == 0){
        DrawGenesisState_OPN2(g);
    }else if(voice <= 6){
        DrawGenesisState_Chan(g, voice-1);
        DrawGenesisState_Op(g, voice-1, op);
    }else if(voice == 7){
        DrawGenesisState_DAC(g);
    }else if(voice <= 11){
        DrawGenesisState_PSG(g, voice-8);
    }
}


