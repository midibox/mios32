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
        sg = syngenesis[pimap.map_chip];
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
            if(i == 3) sg->fm3_special = 0;
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
        sg = syngenesis[pimap.map_chip];
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
            ClearPI(proginstances[syngenesis[bestg].channels[v].pi_using]);
        }
    }
    return bestg;
}

void AssignVoiceToOPN2(u8 piindex, synproginstance_t* pi, u8 g, u8 v, u8 alsodac, u8 vlfo){
    syngenesis_usage_t* sgusage = &syngenesis[g].channels[v];
    if(sgusage->use > 0){
        ClearPI(proginstances[sgusage->pi_using]);
    }
    //Assign voices
    sgusage->use = 2;
    sgusage->pi_using = piindex;
    syngenesis[g].lfobits |= vlfo << (v-1);
    //Map PI
    pi->mapping[v] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = g, .map_voice = v-1, .option = vlfo};
    //DAC only
    if(alsodac){
        sgusage = &syngenesis[g].channels[7];
        sgusage->use = 2;
        sgusage->pi_using = piindex;
        pi->mapping[7] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = g, .map_voice = 0, .option = 0};
    }
}

void AllocatePI(u8 piindex, usage_t pusage){
    synproginstance_t* pi = &proginstances[piindex];
    syngenesis_t* sg;
    syngenesis_usage_t* sgusage;
    u8 i, g, v, score;
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
        for(g=0; g<GENESIS_COUNT; ++g){
            score = 0;
            for(v=0; v<8; ++v){
                i = syngenesis[g].channels[v].use;
                if(i >= 2) score += 10;
                else if(i == 1) score += 1;
            }
            if(score < bestscore){
                bestscore = score;
                bestg = g;
            }
        }
        //Replace this one
        sg = &syngenesis[bestg];
        for(v=0; v<8; ++v){
            sgusage = sg->channels[v];
            //Clear previous PIs using this chip
            if(sgusage.use > 0){
                ClearPI(proginstances[sgusage->pi_using]);
            }
            //Assign voices to this PI
            sgusage->use = 2;
            sgusage->pi_using = piindex;
            //Set up mapping in PI
            pi->mapping[v] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = bestg, .map_voice = (v >= 1 && v <= 6) ? (v-1) : 0, .option = 1};
        }
        //Set up additional bits in chip allocation record
        sg->lfobits = 0x3F;
        sg->lfovaries = 1;
        sg->lfofixed = 0;
    }else{
        lfovaries = 0;
        if((pusage.all & 0x00000FC0) && !pusage.lfofixed){ //Any LFO used and not fixed
            lfog = FindOPN2ClearLFO();
            lfovaries = 1;
            syngenesis[lfog].lfovaries = 1;
            syngenesis[lfog].lfofixed = 0;
        }
        //Assign Ch6 if DAC used
        if(pusage.dac){
            if(pusage.fm6_lfo){
                if(lfovaries){
                    //LFO non-fixed; has to get assigned to lfog:6
                    AssignVoiceToOPN2(piindex, pi, lfog, 6, 1, 1);
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
                    AssignVoiceToOPN2(piindex, pi, g, 6, 1, 1);
                }
            }else{
                //DAC without LFO
                //Find the best to replace
                bestscore = 0xFF;
                for(g=0; g<GENESIS_COUNT; ++g){
                    score = 0;
                    for(v=6; v<8; ++v){
                        i = syngenesis[g].channels[v].use;
                        if(i >= 2) score += 10;
                        else if(i == 1) score += 1;
                    }
                    if(score < bestscore){
                        bestscore = score;
                        bestg = g;
                    }
                }
                //Use bestg
                AssignVoiceToOPN2(piindex, pi, bestg, 6, 1, 0);
            }
            pusage.fm6 = 0;
        }
        //Assign FM3 if FM3 Special mode
        if(pusage.fm3_special){
            if(pusage.fm3_lfo){
                if(lfovaries){
                    //LFO non-fixed; has to get assigned to lfog:3
                    AssignVoiceToOPN2(piindex, pi, lfog, 3, 0, 1);
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
                    AssignVoiceToOPN2(piindex, pi, g, 3, 0, 1);
                }
            }else{
                //FM3 without LFO
                //Find the best to replace
                bestscore = 0xFF;
                for(g=0; g<GENESIS_COUNT; ++g){
                    i = syngenesis[g].channels[3].use;
                    score = (i >= 2) ? 10 : i;
                    if(score < bestscore){
                        bestscore = score;
                        bestg = g;
                    }
                }
                //Use bestg
                AssignVoiceToOPN2(piindex, pi, bestg, 3, 0, 0);
            }
            pusage.fm3 = 0;
        }
        //Assign normal voices
        for(v=1; v<=6; ++v){
            if(pusage.all & (1 << (v-1))){ //Voice in use
                if(pusage.all & (1 << (v+5))){ //LFO in use
                    if(lfovaries){
                        //Have to use lfog, find best voice
                        //TODO
                    }else{
                        //LFO fixed; can we find an OPN2 with an open voice with the same LFO fixed?
                        //If not create it
                        //TODO
                    }
                }else{
                    //No LFO, find best voice anywhere
                    //TODO
                }
            }
        }
    }
}



void CopyPIMappingToHead(synproginstance_t* pi, VgmHead* head){
    u8 i;
    for(i=0; i<12; ++i){
        head->channel[i] = pi->mapping[i];
    }
}

s32 SyEng_Allocate(synproginstance_t* pi, usage_t pusage){
    static const u8 voicetryorder[] = {0, 1, 3, 4, 2, 5};
    u8 o, lfoo = GENESIS_COUNT, v, vc, succeeded, vlfo, i;
    usage_t* u;
    //Make temporary copy of chip usage to modify
    usage_t gusage_copy[GENESIS_COUNT];
    for(o=0; o<GENESIS_COUNT; ++o){
        gusage_copy[o].all = syngenesis[o].usage.all;
    }
    //Initialize channels to have no data
    for(i=0; i<12; ++i){
        pi->mapping[i] = (VgmHead_Channel){.nodata = 1, .mute = 0, .map_chip = 0, .map_voice = 0, .option = 0};
    }
    //Allocate OPN2 voices
    if(pusage.opn2_globals
            || ((pusage.all & 0x00000FC0) /* Any LFO used */ && !pusage.lfofixed)){
        //We need a whole unallocated chip
        for(o=0; o<GENESIS_COUNT; ++o){
            if(!(gusage_copy[o].all & 0x00FFFFFF)) break; //Ignore PSG mapping
        }
        if(o == GENESIS_COUNT) return 0; //No free chip
        //We're good
        DBG("--Allocating to entire OPN2 %d", o);
        u = &gusage_copy[o];
        u->all |= 0x00007FFF; //Say we're using everything, prevent anything else from being
                              //assigned to this chip
        //Assign all voices straightforward
        pi->mapping[0] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 0};
        pi->mapping[1] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 1};
        pi->mapping[2] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 1, .option = 1};
        pi->mapping[3] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 2, .option = 1};
        pi->mapping[4] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 3, .option = 1};
        pi->mapping[5] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 4, .option = 1};
        pi->mapping[6] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 5, .option = 1};
        pi->mapping[7] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 0};
        //Go on to PSG allocation
    }else{
        if(pusage.lfofixed){
            //Allocate all voices using LFO to a chip with the same speed (probably
            //other PIs from the same program)
            for(o=0; o<GENESIS_COUNT; ++o){
                if(gusage_copy[o].lfofixed 
                        && gusage_copy[o].lfofixedspeed == pusage.lfofixedspeed
                        && !(pusage.fm6_lfo && pusage.dac && !gusage_copy[o].dac)
                        && !(pusage.fm3_lfo && pusage.fm3_special && !gusage_copy[o].fm3)) break;
            }
            if(o == GENESIS_COUNT){
                //Maybe there's one which has no LFO used at all?
                for(o=0; o<GENESIS_COUNT; ++o){
                    if(!(gusage_copy[o].all & 0x00000FC0)
                        && !(pusage.fm6_lfo && pusage.dac && !gusage_copy[o].dac)
                        && !(pusage.fm3_lfo && pusage.fm3_special && !gusage_copy[o].fm3)) break;
                }
                if(o == GENESIS_COUNT) return 0; //No chip with appropriate LFO available
                //Set up this chip to have lfofixed
                gusage_copy[o].lfofixed = 1;
                gusage_copy[o].lfofixedspeed = pusage.lfofixedspeed;
            }
            //We have our chip, use chip o for all voices that use LFO
            DBG("--Allocating LFO voices to OPN2 %d", o);
            lfoo = o;
        }
        if(pusage.dac){
            //Find an OPN2 with an available DAC
            if(pusage.fm6_lfo){
                //We already made sure this chip has both DAC and LFO free when
                //figuring out the LFO
                o = lfoo;
            }else{
                for(o=0; o<GENESIS_COUNT; ++o){
                    if(!gusage_copy[o].dac && !gusage_copy[o].fm6) break;
                }
            }
            if(o == GENESIS_COUNT) return 0; //No available DAC
            //Use DAC and Ch6
            DBG("--Allocating DAC to OPN2 %d", o);
            gusage_copy[o].dac = 1;
            gusage_copy[o].fm6 = 1;
            //Set up routing
            pi->mapping[6] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 5, .option = pusage.fm6_lfo};
            pi->mapping[7] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 0};
            //Mark FM6 as taken care of
            pusage.fm6 = 0;
        }
        if(pusage.fm3_special){
            //Find an OPN2 with an available Ch3
            if(pusage.fm3_lfo){
                //We already made sure this chip has both Ch3 Special and LFO free when
                //figuring out the LFO
                o = lfoo;
            }else{
                for(o=0; o<GENESIS_COUNT; ++o){
                    if(!gusage_copy[o].fm3) break;
                }
            }
            if(o == GENESIS_COUNT) return 0; //No available Ch3 Special
            //Use Ch3 and special
            DBG("--Allocating Ch3 Special to OPN2 %d", o);
            gusage_copy[o].fm3 = 1;
            gusage_copy[o].fm3_special = 1;
            //Set up routing
            pi->mapping[3] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 2, .option = pusage.fm3_lfo};
            //Mark FM3 as taken care of
            pusage.fm3 = 0;
        }
        //Allocate normal OPN2 voices
        for(v=0; v<6; ++v){
            if(!(pusage.all & (1 << v))) continue; //Voice not in use
            vlfo = pusage.all & (1 << (v+6)); //LFO enabled on this voice
            succeeded = 0;
            //Find a voice
            for(i=0; i<6 && !succeeded; ++i){
                vc = voicetryorder[i]; //Try normal voices first, then Ch3, then Ch6
                //Find a chip
                if(vlfo){
                    o = lfoo; //Only use the chip being used for LFO-needing voices
                }else{
                    o = 0; //Try all chips
                }
                do{
                    if(!(gusage_copy[o].all & (1 << vc))){
                        DBG("--Allocating FM%d to OPN2 %d voice %d", v, o, vc);
                        gusage_copy[o].all |= 1 << vc;
                        pi->mapping[1+v] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = vc, .option = vlfo};
                        succeeded = 1;
                        break;
                    }
                    ++o;
                }while(!vlfo && o < GENESIS_COUNT); //Loop through chips, or exit if LFO
            }
            if(!succeeded) return 0; //Could not find a voice
        }
    }
    //Allocate PSG voices
    if(pusage.noisefreqsq3){
        //Find a chip with both sq3 and ns available
        for(o=0; o<GENESIS_COUNT; ++o){
            if(gusage_copy[o].sq3 || gusage_copy[o].noise) continue;
            //Found it
            DBG("--Allocating SQ3/NS to PSG %d", o);
            gusage_copy[o].sq3 = 1;
            gusage_copy[o].noise = 1;
            pi->mapping[10] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 2, .option = 0};
            pi->mapping[11] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 1};
            pusage.sq3 = 0;
            pusage.noise = 0;
            break;
        }
        if(o == GENESIS_COUNT) return 0; //Could not find a chip
    }else if(pusage.noise){
        //Find a chip with noise available
        for(o=0; o<GENESIS_COUNT; ++o){
            if(gusage_copy[o].noise) continue;
            //Found it
            DBG("--Allocating NS to PSG %d", o);
            gusage_copy[o].noise = 1;
            pi->mapping[11] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = 0, .option = 1};
            break;
        }
        if(o == GENESIS_COUNT) return 0; //Could not find a chip
    }
    //Allocate square voices
    for(v=0; v<3; ++v){
        if(!(pusage.all & (1 << (24+v)))) continue; //Voice not used
        succeeded = 0;
        //Find a voice
        for(vc=0; vc<3 && !succeeded; ++vc){
            //Find a chip
            for(o=0; o<GENESIS_COUNT; ++o){
                if(!(gusage_copy[o].all & (1 << (24+vc)))){
                    DBG("--Allocating SQ%d to PSG %d voice %d", v, o, vc);
                    gusage_copy[o].all |= 1 << (24+vc);
                    pi->mapping[8+v] = (VgmHead_Channel){.nodata = 0, .mute = 0, .map_chip = o, .map_voice = vc, .option = 0};
                    succeeded = 1;
                    break;
                }
            }
        }
        if(!succeeded) return 0; //Could not find a voice
    }
    //Copy usage back to actual chip usage
    for(o=0; o<GENESIS_COUNT; ++o){
        syngenesis[o].usage.all = gusage_copy[o].all;
    }
    return 1;
}

void SyEng_Deallocate(synproginstance_t* pi){
    u8 i, v;
    VgmHead_Channel pimap;
    usage_t* usage;
    for(i=0; i<12; ++i){
        pimap = pi->mapping[i];
        if(pimap.nodata) continue;
        usage = &syngenesis[pimap.map_chip].usage;
        if(i == 0){
            //We were using OPN2 globals, so there can't be anything else
            //allocated on this OPN2 besides this PI
            DBG("--Deallocate: clearing whole OPN2 %d", pimap.map_chip);
            usage->all &= 0xFF000000;
            //Skip to PSG section
            i = 7;
            continue;
        }
        if(i >= 1 && i <= 6){
            //FM voice
            v = pimap.map_voice;
            usage->all &= ~((u32)((1 << v) | (1 << (v+6)))); //Clear using voice and voice LFO
        }else if(i == 7){
            //DAC
            usage->dac = 0;
        }else if(i >= 8 && i <= 10){
            //SQ voice
            v = pimap.map_voice;
            usage->all &= ~((u32)(1 << (v+24)));
        }else{
            //Noise
            usage->noise = 0;
            usage->noisefreqsq3 = 0;
        }
        DBG("--Deallocate: clearing voice %d", i);
    }
}

void SyEng_Init(){
    u8 i;
    //Initialize syngenesis
    for(i=0; i<GENESIS_COUNT; ++i){
        syngenesis[i].usage.all = 0;
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
    //TODO testing only
    //Create program on channel 1
    synprogram_t* prog = malloc(sizeof(synprogram_t));
    channels[1].program = prog;
    prog->usage = (usage_t){.fm1=1, .fm2=0, .fm3=0, .fm4=0, .fm5=0, .fm6=0,
                            .fm1_lfo=0, .fm2_lfo=0, .fm3_lfo=0, .fm4_lfo=0, .fm5_lfo=0, .fm6_lfo=0,
                            .dac=0, .fm3_special=0, .opn2_globals=0, .lfofixed=0, .lfofixedspeed=0,
                            .sq1=0, .sq2=0, .sq3=0, .noise=0, .noisefreqsq3=0};
    prog->rootnote = 60;
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
        DBG("--Deallocating existing PI resources");
        SyEng_Deallocate(pi);
    }
    //Find best allocation
    DBG("--Allocating resources for new PI");
    if(SyEng_Allocate(pi, prog->usage)){
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
    }else{
        //Could not allocate voices
        DBG("--Could not allocate voices!");
    }
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
    DBG("--Deallocating resources");
    pi->playing = 0;
    SyEng_Deallocate(pi);
    DBG("--Done");
}
