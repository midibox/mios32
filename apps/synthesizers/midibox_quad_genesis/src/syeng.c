/*
 * MIDIbox Quad Genesis: Synth engine
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
#include "syeng.h"

#include <genesis.h>
#include <vgm.h>


syngenesis_t syngenesis[GENESIS_COUNT];
synproginstance_t proginstances[MBQG_NUM_PROGINSTANCES];
synchannel_t channels[16*MBQG_NUM_PORTS];

//TODO reset voice/genesis states when releasing
//TODO divide chips that use globals by the types of globals used, so multiple
//voices using the same effect e.g. "ugly" can play together
//TODO take recency into account when items have the same score otherwise

void ReleaseAllPI(synproginstance_t* pi){
    u8 i, v;
    VgmHead_Channel pimap;
    syngenesis_t* sg;
    for(i=0; i<12; ++i){
        pimap = pi->mapping[i];
        if(pimap.nodata) continue;
        sg = &syngenesis[pimap.map_chip];
        if(i == 0){
            //We were using OPN2 globals, so there can't be anything else
            //allocated on this OPN2 besides this PI
            DBG("--ReleaseAllPI: clearing whole OPN2 %d", pimap.map_chip);
            sg->lfobits = 0;
            sg->optionbits = 0;
            for(v=0; v<8; ++v) sg->channels[v].ALL = 0;
            //Skip to PSG section
            i = 7;
            continue;
        }
        if(i >= 1 && i <= 6){
            //FM voice
            v = pimap.map_voice;
            sg->channels[v+1].ALL = 0;
            sg->lfobits &= ~(1 << v);
            if(sg->lfobits == 0){
                sg->lfovaries = 0;
                sg->lfofixed = 0;
            }
        }else if(i == 7){
            //DAC
            sg->channels[7].ALL = 0;
        }else if(i >= 8 && i <= 10){
            //SQ voice
            v = pimap.map_voice;
            sg->channels[v+8].ALL = 0;
        }else{
            //Noise
            sg->channels[11].ALL = 0;
            sg->noisefreqsq3 = 0;
        }
        DBG("--ReleaseAllPI: clearing voice %d", i);
    }
}

void ClearPI(synproginstance_t* pi){
    //Stop actually playing
    if(pi->head != NULL){
        VGM_Head_Delete(pi->head);
        pi->head = NULL;
    }
    //Invalidate PI
    pi->valid = 0;
    pi->playing = 0;
    pi->playinginit = 0;
    pi->recency = VGM_Player_GetVGMTime() - (u32)0x80000000; //as far away as possible
    //Release all resources
    ReleaseAllPI(pi);
}

void StandbyPI(synproginstance_t* pi){
    u8 i, v;
    VgmHead_Channel pimap;
    syngenesis_t* sg;
    for(i=0; i<12; ++i){
        pimap = pi->mapping[i];
        if(pimap.nodata) continue;
        sg = &syngenesis[pimap.map_chip];
        if(i == 0){
            //We were using OPN2 globals
            DBG("--StandbyPI: setting all voices to standby");
            for(v=0; v<8; ++v){
                sg->channels[v].use = 1;
            }
            //Skip to PSG section
            i = 7;
            continue;
        }
        if(i >= 1 && i <= 6){
            //FM voice
            v = pimap.map_voice;
            sg->channels[v+1].use = 1;
        }else if(i == 7){
            //DAC
            sg->channels[7].use = 1;
        }else if(i >= 8 && i <= 10){
            //SQ voice
            v = pimap.map_voice;
            sg->channels[v+8].use = 1;
        }else{
            //Noise
            sg->channels[11].use = 1;
        }
        DBG("--StandbyPI: voice %d standing by", i);
    }
}

u8 FindOPN2ClearLFO(){
    //Find an OPN2 with as few voices as possible using the LFO
    u8 i, g, v, score, bestscore, bestg;
    bestscore = 0xFF;
    for(g=0; g<GENESIS_COUNT; ++g){
        score = 0;
        for(v=1; v<6; ++v){
            if(syngenesis[g].lfobits & (1 << (v-1))){
                //Only for voices using the LFO
                i = syngenesis[g].channels[v].use;
                if(i >= 2) score += 10;
                else if(i == 1) score += 1;
            }
        }
        if(score < bestscore){
            bestscore = score;
            bestg = g;
        }
    }
    //Kick out voices using the LFO
    for(v=1; v<6; ++v){
        if((syngenesis[bestg].lfobits & (1 << (v-1))) && syngenesis[bestg].channels[v].use > 0){
            ClearPI(&proginstances[syngenesis[bestg].channels[v].pi_using]);
        }
    }
    return bestg;
}

void AssignVoiceToOPN2(u8 piindex, synproginstance_t* pi, u8 g, u8 vsource, u8 vdest, u8 vlfo){
    DBG("--Assigning PI %d voice %d to OPN2 %d voice %d, vlfo=%d", piindex, vsource, g, vdest, vlfo);
    syngenesis_usage_t* sgusage = &syngenesis[g].channels[vdest];
    if(sgusage->use > 0){
        ClearPI(&proginstances[sgusage->pi_using]);
    }
    //Assign voices
    u8 proper = (vdest >= 1 && vdest <= 6);
    sgusage->use = 2;
    sgusage->pi_using = piindex;
    if(vlfo && proper){
        syngenesis[g].lfobits |= 1 << (vdest-1);
    }
    //Map PI
    pi->mapping[vsource] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = g, .map_voice = proper ? vdest-1 : 0, .option = vlfo};
}

void AllocatePI(u8 piindex, usage_bits_t pusage){
    synproginstance_t* pi = &proginstances[piindex];
    syngenesis_t* sg;
    u8 i, g, v, use, score;
    u8 bestg, bestv, bestscore;
    u8 lfog, lfovaries;
    u32 recency, maxrecency, now = VGM_Player_GetVGMTime();
    //Initialize channels to have no data
    for(i=0; i<12; ++i){
        pi->mapping[i] = (VgmHead_Channel){.nodata = 1, .mute = 0, .map_chip = 0, .map_voice = 0, .option = 0};
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// OPN2 ALLOCATION ///////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    if(pusage.opn2_globals){
        //We need an entire OPN2; find the best one to replace
        bestscore = 0xFF;
        bestg = 0;
        for(g=0; g<GENESIS_COUNT; ++g){
            score = 0;
            for(v=0; v<8; ++v){
                use = syngenesis[g].channels[v].use;
                if(use >= 2) score += 10;
                else if(use == 1) score += 1;
            }
            if(score < bestscore){
                bestscore = score;
                bestg = g;
            }
        }
        //Replace this one
        sg = &syngenesis[bestg];
        for(v=0; v<8; ++v){
            AssignVoiceToOPN2(piindex, pi, bestg, v, v, 1);
        }
        //Set up additional bits in chip allocation record
        sg->lfovaries = 1;
        sg->lfofixed = 0;
    }else{
        if((pusage.all & 0x00000FC0) && !pusage.lfofixed){ //Any LFO used and not fixed
            lfovaries = 1;
            lfog = FindOPN2ClearLFO();
            syngenesis[lfog].lfovaries = 1;
            syngenesis[lfog].lfofixed = 0;
        }else{
            lfovaries = 0;
            lfog = 0;
        }
        //Assign Ch6 if DAC used
        if(pusage.dac){
            if(pusage.fm6_lfo){
                if(lfovaries){
                    //LFO non-fixed; has to get assigned to lfog:6
                    AssignVoiceToOPN2(piindex, pi, lfog, 6, 6, 1);
                    AssignVoiceToOPN2(piindex, pi, lfog, 7, 7, 0);
                }else{
                    //LFO fixed; can we find an OPN2 with DAC open with the same LFO fixed?
                    for(g=0; g<GENESIS_COUNT; ++g){
                        sg = &syngenesis[lfog];
                        if(sg->lfofixed && sg->lfofixedspeed == pusage.lfofixedspeed && sg->channels[6].use == 0){
                            break;
                        }
                    }
                    if(g == GENESIS_COUNT){
                        //Clear an OPN2 of LFO
                        g = FindOPN2ClearLFO();
                        //Set it up to be fixed to us
                        syngenesis[g].lfovaries = 0;
                        syngenesis[g].lfofixed = 1;
                        syngenesis[g].lfofixedspeed = pusage.lfofixedspeed;
                        //Assign DAC to it, possibly overriding what was there
                    }
                    AssignVoiceToOPN2(piindex, pi, g, 6, 6, 1);
                    AssignVoiceToOPN2(piindex, pi, g, 7, 7, 1);
                }
            }else{
                //DAC without LFO
                //Find the best to replace
                bestscore = 0xFF;
                bestg = 0;
                for(g=0; g<GENESIS_COUNT; ++g){
                    score = 0;
                    for(v=6; v<8; ++v){
                        use = syngenesis[g].channels[v].use;
                        if(use >= 2) score += 10;
                        else if(use == 1) score += 1;
                    }
                    if(score < bestscore){
                        bestscore = score;
                        bestg = g;
                    }
                }
                //Use bestg
                AssignVoiceToOPN2(piindex, pi, bestg, 6, 6, 0);
                AssignVoiceToOPN2(piindex, pi, bestg, 7, 7, 0);
            }
            pusage.fm6 = 0;
        }
        //Assign FM3 if FM3 Special mode
        if(pusage.fm3_special){
            if(pusage.fm3_lfo){
                if(lfovaries){
                    //LFO non-fixed; has to get assigned to lfog:3
                    AssignVoiceToOPN2(piindex, pi, lfog, 3, 3, 1);
                }else{
                    //LFO fixed; can we find an OPN2 with FM3 open with the same LFO fixed?
                    for(g=0; g<GENESIS_COUNT; ++g){
                        sg = &syngenesis[lfog];
                        if(sg->lfofixed && sg->lfofixedspeed == pusage.lfofixedspeed && sg->channels[3].use == 0){
                            break;
                        }
                    }
                    if(g == GENESIS_COUNT){
                        //Clear an OPN2 of LFO
                        g = FindOPN2ClearLFO();
                        //Set it up to be fixed to us
                        syngenesis[g].lfovaries = 0;
                        syngenesis[g].lfofixed = 1;
                        syngenesis[g].lfofixedspeed = pusage.lfofixedspeed;
                        //Assign FM3 to it, possibly overriding what was there
                    }
                    AssignVoiceToOPN2(piindex, pi, g, 3, 3, 1);
                }
            }else{
                //FM3 without LFO
                //Find the best to replace
                bestscore = 0xFF;
                bestg = 0;
                for(g=0; g<GENESIS_COUNT; ++g){
                    use = syngenesis[g].channels[3].use;
                    score = (use >= 2) ? 10 : use;
                    if(score < bestscore){
                        bestscore = score;
                        bestg = g;
                    }
                }
                //Use bestg
                AssignVoiceToOPN2(piindex, pi, bestg, 3, 3, 0);
            }
            pusage.fm3 = 0;
        }
        //Assign normal voices
        for(i=1; i<=6; ++i){
            if(pusage.all & (1 << (i-1))){ //Voice in use
                if(pusage.all & (1 << (i+5))){ //LFO in use
                    if(lfovaries){
                        //Have to use lfog, find best voice
                        bestscore = 0xFF;
                        bestv = 1;
                        for(v=1; v<=6; ++v){
                            use = syngenesis[lfog].channels[v].use;
                            score = (use >= 2) ? 10 : (use << 1);
                            if(v == 3 || v == 6) ++score;
                            if(score < bestscore){
                                bestscore = score;
                                bestv = v;
                            }
                        }
                        //Use this voice
                        AssignVoiceToOPN2(piindex, pi, lfog, i, bestv, 1);
                    }else{
                        //LFO fixed: First is there a chip with LFO Fixed correct and a free voice?
                        bestscore = 0xFF;
                        for(g=0; g<GENESIS_COUNT; ++g){
                            sg = &syngenesis[g];
                            if(!sg->lfofixed || sg->lfofixedspeed != pusage.lfofixedspeed) continue;
                            //If we have a chip with the right LFO Fixed, check the voices
                            score = 0;
                            for(v=1; v<=6; ++v){
                                use = syngenesis[g].channels[v].use;
                                if(use >= 2){
                                    score = 0xFF;
                                    break;
                                }
                            }
                            if(score == 0) break;
                        }
                        if(g == GENESIS_COUNT){
                            //Find the OPN2 with the least LFO use
                            g = FindOPN2ClearLFO();
                        }
                        //Find best voice
                        bestscore = 0xFF;
                        for(v=1; v<=6; ++v){
                            use = syngenesis[g].channels[v].use;
                            score = (use >= 2) ? 10 : (use << 1);
                            if(v == 3 || v == 6) ++score;
                            if(score < bestscore){
                                bestscore = score;
                                bestv = v;
                            }
                        }
                        //Use this voice
                        AssignVoiceToOPN2(piindex, pi, g, i, bestv, 1);
                    }
                }else{
                    //No LFO, find best voice anywhere
                    bestscore = 0xFF;
                    bestg = 0;
                    bestv = 1;
                    for(g=0; g<GENESIS_COUNT; ++g){
                        for(v=1; v<=6; ++v){
                            use = syngenesis[g].channels[v].use;
                            score = (use >= 2) ? 10 : (use << 1);
                            if(v == 3 || v == 6) ++score;
                            if(score < bestscore){
                                bestscore = score;
                                bestv = v;
                                bestg = g;
                            }
                        }
                    }
                    //Use this voice
                    AssignVoiceToOPN2(piindex, pi, bestg, i, bestv, 0);
                }
            }
        }
    }
    //TODO allocate PSG voices
}


void CopyPIMappingToHead(synproginstance_t* pi, VgmHead* head){
    u8 i;
    for(i=0; i<12; ++i){
        head->channel[i] = pi->mapping[i];
    }
}

void SyEng_Init(){
    u8 i, j;
    //Initialize syngenesis
    for(i=0; i<GENESIS_COUNT; ++i){
        syngenesis[i].lfobits = 0;
        syngenesis[i].optionbits = 0;
        for(j=0; j<12; ++j){
            syngenesis[i].channels[j].ALL = 0;
        }
    }
    //Initialize proginstances
    for(i=0; i<MBQG_NUM_PROGINSTANCES; ++i){
        proginstances[i].valid = 0;
        proginstances[i].head = NULL;
    }
    //Initialize channels
    for(i=0; i<16*MBQG_NUM_PORTS; ++i){
        channels[i].trackermode = 0;
        channels[i].program = NULL;
    }
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////// TEST PROGRAM CH 2 //////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //Create program on channel 1
    synprogram_t* prog = malloc(sizeof(synprogram_t));
    channels[1].program = prog;
    prog->usage = (usage_bits_t){.fm1=1, .fm2=0, .fm3=0, .fm4=0, .fm5=0, .fm6=0,
                                 .fm1_lfo=0, .fm2_lfo=0, .fm3_lfo=0, .fm4_lfo=0, .fm5_lfo=0, .fm6_lfo=0,
                                 .dac=0, .fm3_special=0, .opn2_globals=0, .lfofixed=0, .lfofixedspeed=0,
                                 .sq1=0, .sq2=0, .sq3=0, .noise=0, .noisefreqsq3=0};
    prog->rootnote = 72;
    //Create init VGM file
    VgmSource* source = VGM_SourceRAM_Create();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 30;
    VgmChipWriteCmd* data = malloc(30*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //data[0] = (VgmChipWriteCmd){.cmd = 0x02, .addr = 0x5C, .data = 0x1F, .data2 = 0}; //Set Ch1:Op4 attack rate to full
    //
    i=0;
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x30, .data=0x71 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x34, .data=0x0D };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x38, .data=0x33 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x3C, .data=0x01 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x40, .data=0x23 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x44, .data=0x2D };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x48, .data=0x26 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x4C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x50, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x54, .data=0x99 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x58, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x5C, .data=0x94 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x60, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x64, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x68, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x6C, .data=0x07 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x70, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x74, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x78, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x7C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x80, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x84, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x88, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x8C, .data=0xA6 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x90, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x94, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x98, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x9C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB0, .data=0x32 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB4, .data=0xC0 };
    //
    prog->initsource = source;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 2;
    data = malloc(2*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = VGM_getOPN2Frequency(60, 0, 8000000); //Middle C
        data[0].cmd  = 0x52;
        data[0].addr = 0xA4;
    //data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xA4, .data=0x22, .data2=0x69 };
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF0, .data2=0}; //Key on Ch1
    prog->noteonsource = source;
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 1;
    data = malloc(1*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x00, .data2=0}; //Key off Ch1
    prog->noteoffsource = source;
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////// TEST PROGRAM CH 3 //////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //Create program on channel 1
    prog = malloc(sizeof(synprogram_t));
    channels[2].program = prog;
    prog->usage = (usage_bits_t){.fm1=0, .fm2=1, .fm3=0, .fm4=1, .fm5=1, .fm6=0,
                                 .fm1_lfo=0, .fm2_lfo=0, .fm3_lfo=0, .fm4_lfo=0, .fm5_lfo=0, .fm6_lfo=0,
                                 .dac=0, .fm3_special=0, .opn2_globals=0, .lfofixed=0, .lfofixedspeed=0,
                                 .sq1=0, .sq2=0, .sq3=0, .noise=0, .noisefreqsq3=0};
    prog->rootnote = 60;
    //Create init VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;

    vsr->numcmds = 27;
    data = malloc(27*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //data[0] = (VgmChipWriteCmd){.cmd = 0x02, .addr = 0x5C, .data = 0x1F, .data2 = 0}; //Set Ch1:Op4 attack rate to full
    //
    i=0;
    //Voice 2
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x3D, .data=0x01 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x49, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x4D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x5D, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x6D, .data=0x02 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x7D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x8D, .data=0xA6 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB1, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB5, .data=0xC0 };
    //Voice 4
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x3C, .data=0x01 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x48, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x4C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x5C, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x6C, .data=0x02 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x7C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x8C, .data=0xA6 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB0, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB4, .data=0xC0 };
    //Voice 5
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x3D, .data=0x01 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x49, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x4D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x5D, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x6D, .data=0x02 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x7D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x8D, .data=0xA6 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB1, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB5, .data=0xC0 };
    //
    prog->initsource = source;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 6;
    data = malloc(6*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = VGM_getOPN2Frequency(60, 0, 8000000); //Middle C
        data[0].cmd  = 0x52;
        data[0].addr = 0xA5;
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF1, .data2=0}; //Key on Ch2
    data[2] = VGM_getOPN2Frequency(64, 0, 8000000); //E
        data[2].cmd  = 0x53;
        data[2].addr = 0xA4;
    data[3] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF4, .data2=0}; //Key on Ch4
    data[4] = VGM_getOPN2Frequency(67, 0, 8000000); //G
        data[4].cmd  = 0x53;
        data[4].addr = 0xA5;
    data[5] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF5, .data2=0}; //Key on Ch5
    prog->noteonsource = source;
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 3;
    data = malloc(3*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x01, .data2=0}; //Key off Ch2
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x04, .data2=0}; //Key off Ch2
    data[2] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x05, .data2=0}; //Key off Ch2
    prog->noteoffsource = source;
}

void SyEng_Tick(){
    u8 i;
    synproginstance_t* pi;
    synprogram_t* prog;
    for(i=0; i<MBQG_NUM_PROGINSTANCES; ++i){
        pi = &proginstances[i];
        if(!pi->valid) continue;
        if(pi->head == NULL) continue;
        if(pi->head->isdone){
            DBG("Stopping playing VGM which isdone");
            VGM_Head_Delete(pi->head);
            pi->head = NULL;
            if(!pi->playinginit) continue;
            prog = channels[pi->sourcechannel].program;
            if(prog == NULL){
                DBG("ERROR program disappeared while playing, could not switch from init to noteon!");
                continue;
            }
            //Switch from init to noteon VGM
            DBG("PI %d ch %d note %d switching from init to noteon VGM", i, pi->sourcechannel, pi->note);
            pi->playinginit = 0;
            pi->head = VGM_Head_Create(prog->noteonsource, VGM_getFreqMultiplier((s8)pi->note - (s8)prog->rootnote), 0x1000);
            CopyPIMappingToHead(pi, pi->head);
            VGM_Head_Restart(pi->head, VGM_Player_GetVGMTime());
            pi->head->playing = 1;
        }
    }
}

void SyEng_Note_On(mios32_midi_package_t pkg){
    synprogram_t* prog = channels[pkg.chn].program;
    if(prog == NULL){
        DBG("Note on %d ch %d, no program on this channel", pkg.note, pkg.chn);
        return;
    }
    //Look for an existing proginstance that's the best fit to replace
    //In order from best to worst:
    //0: Same channel, same note (playing or not playing doesn't matter)
    //1: Same channel, not playing
    //2: Invalid
    //3: Other channel, not playing
    //4: Same channel, playing
    //5: Other channel, playing
    u8 bestrating = 0xFF;
    u8 bestrated = 0xFF;
    u8 rating, i;
    synproginstance_t* pi;
    for(i=0; i<MBQG_NUM_PROGINSTANCES; ++i){
        pi = &proginstances[i];
        if(!pi->valid){
            rating = 2;
        }else{
            if(pi->sourcechannel == pkg.chn){
                if(pi->note == pkg.note){
                    rating = 0;
                }else if(pi->playing){
                    rating = 4;
                }else{
                    rating = 1;
                }
            }else{
                if(pi->playing){
                    rating = 5;
                }else{
                    rating = 3;
                }
            }
        }
        if(rating < bestrating){
            bestrating = rating;
            bestrated = i;
        }
    }
    pi = &proginstances[bestrated];
    pi->note = pkg.note;
    DBG("Note on %d ch %d, replacing PI %d rating %d", pkg.note, pkg.chn, bestrated, bestrating);
    //Do we already have the right mapping allocated?
    if(pi->valid && pi->sourcechannel == pkg.chn){
        //Skip the init VGM, start the note on VGM
        if(pi->head != NULL){
            //Stop playing whatever it was playing
            DBG("--Stopping PI head");
            VGM_Head_Delete(pi->head);
            pi->head = NULL;
        }
        DBG("--Creating noteon head");
        u32 fmultiplier = VGM_getFreqMultiplier((s8)pkg.note - (s8)prog->rootnote);
        pi->head = VGM_Head_Create(prog->noteonsource, fmultiplier, 0x1000);
        DBG("--Note %d root %d fmult %d, result mults OPN2 %d PSG %d", pkg.note, prog->rootnote, fmultiplier, pi->head->opn2mult, pi->head->psgmult);
        CopyPIMappingToHead(pi, pi->head);
        VGM_Head_Restart(pi->head, VGM_Player_GetVGMTime());
        pi->playing = 1;
        pi->head->playing = 1;
        DBG("--Done");
        return;
    }
    //If we took away from another channel which was playing, release its resources
    if(pi->valid && pi->playing == 1){
        DBG("--Clearing existing PI resources");
        ClearPI(pi);
    }
    //Find best allocation
    DBG("--Allocating resources for new PI");
    AllocatePI(bestrated, prog->usage);
    //Start the init VGM
    if(pi->head != NULL){
        //Stop playing whatever it was playing
        VGM_Head_Delete(pi->head);
        pi->head = NULL;
    }
    DBG("--Creating init head");
    pi->sourcechannel = pkg.chn;
    pi->valid = 1;
    pi->playing = 1;
    pi->playinginit = 1;
    u32 fmultiplier = VGM_getFreqMultiplier((s8)pkg.note - (s8)prog->rootnote);
    pi->head = VGM_Head_Create(prog->initsource, fmultiplier, 0x1000);
    DBG("--Note %d root %d fmult %d, result mults OPN2 %d PSG %d", pkg.note, prog->rootnote, fmultiplier, pi->head->opn2mult, pi->head->psgmult);
    CopyPIMappingToHead(pi, pi->head);
    VGM_Head_Restart(pi->head, VGM_Player_GetVGMTime());
    pi->head->playing = 1;
    DBG("--Done");
}
void SyEng_Note_Off(mios32_midi_package_t pkg){
    synproginstance_t* pi;
    u8 i;
    for(i=0; i<MBQG_NUM_PROGINSTANCES; ++i){
        pi = &proginstances[i];
        if(!pi->valid) continue;
        if(pi->sourcechannel != pkg.chn) continue;
        if(pi->note != pkg.note || !pi->playing) continue;
        break;
    }
    if(i == MBQG_NUM_PROGINSTANCES){
        DBG("Note off %d ch %d, but no PI playing this note", pkg.note, pkg.chn);
        return; //no corresponding note on
    }
    DBG("Note off %d ch %d, being played by PI %d", pkg.note, pkg.chn, i);
    //Found corresponding note on
    if(pi->head != NULL){
        //Stop playing note-on VGM
        DBG("--Stopping noteon head");
        VGM_Head_Delete(pi->head);
        pi->head = NULL;
    }
    synprogram_t* prog = channels[pkg.chn].program;
    if(prog == NULL){
        DBG("--ERROR program disappeared while playing, could not switch to noteoff!");
        return;
    }
    //Start playing note-off VGM
    DBG("--Creating noteoff head");
    pi->head = VGM_Head_Create(prog->noteoffsource, VGM_getFreqMultiplier((s8)pkg.note - (s8)prog->rootnote), 0x1000);
    CopyPIMappingToHead(pi, pi->head);
    VGM_Head_Restart(pi->head, VGM_Player_GetVGMTime());
    pi->head->playing = 1;
    //Mark pi as not playing, release resources
    DBG("--Standing by PI resources");
    StandbyPI(pi);
    pi->playing = 0;
    DBG("--Done");
}
