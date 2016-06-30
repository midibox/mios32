/*
 * VGM Data and Playback Driver: Tracker system
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "vgmtracker.h"

#include "vgmhead.h"
#include "vgmqueue.h"
#include "vgmtuning.h"
#include <genesis.h>

static VgmSource* qsource;
static VgmHead* qhead;


void VGM_Tracker_Init(){
    qsource = VGM_SourceQueue_Create();
    qsource->opn2clock = genesis_clock_opn2;
    qsource->psgclock = genesis_clock_psg;
    qhead = VGM_Head_Create(qsource, 0x1000, 0x1000);
    qhead->playing = 1;
}

void VGM_Tracker_Enqueue(VgmChipWriteCmd cmd, u8 fixfreq){
    VGM_HeadQueue_Enqueue(qhead, cmd, fixfreq);
}


void VGM_MidiToGenesis(mios32_midi_package_t midi_package, u8 g, u8 v, u8 ch3_op, u8 keyonmask){
    DBG("VGM_MidiToGenesis(event %X, cc %d, value %d, g%d, v%d)", midi_package.event, midi_package.cc_number, midi_package.value, g, v);
    //TODO implement keyonmask
    if(g >= GENESIS_COUNT) return;
    u8 isopn2 = (v >= 1 && v <= 6);
    u8 ispsg = (v >= 8 && v <= 11);
    u8 chan = isopn2 ? v-1 : v-8;
    u8 vlo = chan % 3;
    u8 psgcmd = (g << 4);
    u8 opn2globcmd = psgcmd | 2;
    u8 opn2cmd = opn2globcmd | (v >= 4 && v <= 6);
    if(midi_package.event == 0xB){
        //CC
        u8 cc = midi_package.cc_number;
        u8 value = midi_package.value;
        if(ispsg){
            if(cc == 85){
                //Pitch Transposition
                //TODO
            }else if(cc == 83){
                //NTSC/PAL Tuning
                //TODO
            }
        }else if(isopn2){
            //OPN2
            u8 op;
            //Operator CCs
            if(cc >= 16 && cc <= 27){
                op = cc & 0x03;
                cc &= 0xFC;
            }else if(cc >= 39 && cc <= 62){
                op = (cc-39) & 0x03;
                cc = ((cc-39) & 0xFC) + 39;
            }else if(cc >= 70 && cc <= 73){
                op = cc-70;
                cc = 70;
            }else if(cc >= 90 && cc <= 93){
                op = cc-90;
                cc = 90;
            }else if(cc >= 100 && cc <= 113){
                op = cc-100; //actually for DAC custom wave, not op
                cc = 100;
            }else{
                op = 0;
            }
            u8 opaddr = ((op & 1) << 3) | ((op & 2) << 1) | vlo;
            switch(cc){
            case 74:
                //LFO Enable
                genesis[g].opn2.lfo_enabled = GENMDM_DECODE(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x22, 
                        .data = genesis[g].opn2.lforeg }, 0);
                break;
            case 1:
                //LFO Speed
                genesis[g].opn2.lfo_freq = GENMDM_DECODE(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x22, 
                        .data = genesis[g].opn2.lforeg }, 0);
                break;
            case 85:
                //Pitch Transposition
                //TODO
                break;
            case 84:
                //Octave Division
                //TODO and also wut?
                break;
            case 83:
                //PAL/NTSC Tuning
                //TODO
                break;
            case 80:
                //Voice 3 Special Mode
                genesis[g].opn2.ch3_mode = GENMDM_DECODE(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[g].opn2.timerctrlreg }, 0);
                break;
            case 98: //originally 92, conflicts with SSG-EG
                //Timer Controls
                genesis[g].opn2.timerctrlreg = (genesis[g].opn2.timerctrlreg & 0xC0) | GENMDM_DECODE(value,6);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[g].opn2.timerctrlreg }, 0);
                break;
            case 99: //originally 93, conflicts with SSG-EG
                //Ch3 CSM mode (rather, bit 7 of the register)
                genesis[g].opn2.ch3_mode = (genesis[g].opn2.ch3_mode & 1) | (GENMDM_DECODE(value,1) << 1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[g].opn2.timerctrlreg }, 0);
                break;
            case 94:
                //Test Register 0x21 Lowest Four Bits
                genesis[g].opn2.testreg21 = (genesis[g].opn2.testreg21 & 0xF0) | (GENMDM_DECODE(value,4));
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x21, 
                        .data = genesis[g].opn2.testreg21 }, 0);
                break;
            case 95:
                //Test Register 0x21 Highest Four Bits
                genesis[g].opn2.testreg21 = (genesis[g].opn2.testreg21 & 0x0F) | (GENMDM_DECODE(value,4) << 4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x21, 
                        .data = genesis[g].opn2.testreg21 }, 0);
                break;
            case 96:
                //Test Register 0x2C Lowest Four Bits
                genesis[g].opn2.testreg2C = (genesis[g].opn2.testreg2C & 0xF0) | (GENMDM_DECODE(value,4));
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2C, 
                        .data = genesis[g].opn2.testreg2C }, 0);
                break;
            case 97:
                //Test Register 0x2C Highest Four Bits
                genesis[g].opn2.testreg2C = (genesis[g].opn2.testreg2C & 0x0F) | (GENMDM_DECODE(value,4) << 4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2C, 
                        .data = genesis[g].opn2.testreg2C }, 0);
                break;
            case 6:
                //Preset Instrument Setting Store in RAM
                //TODO
                break;
            case 9:
                //Preset Instrument Setting Recall from RAM
                //TODO
                break;
            case 81:
                //Pitch Bend Amount
                //TODO
                break;
            case 14:
                //FM Algorithm
                genesis[g].opn2.chan[chan].algorithm = GENMDM_DECODE(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB0 | vlo, 
                        .data = genesis[g].opn2.chan[chan].algfbreg }, 0);
                break;
            case 15:
                //FM Feedback
                genesis[g].opn2.chan[chan].feedback = GENMDM_DECODE(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB0 | vlo, 
                        .data = genesis[g].opn2.chan[chan].algfbreg }, 0);
                break;
            case 77:
                //Stereo Configuration
                value = GENMDM_DECODE(value,2);
                genesis[g].opn2.chan[chan].out_l = value & 1;
                genesis[g].opn2.chan[chan].out_r = value >> 1;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | vlo, 
                        .data = genesis[g].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 76:
                //Amplitude Modulation Level
                genesis[g].opn2.chan[chan].lfoampd = GENMDM_DECODE(value,2);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | vlo, 
                        .data = genesis[g].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 75:
                //Frequency Modulation Level
                genesis[g].opn2.chan[chan].lfofreqd = GENMDM_DECODE(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | vlo, 
                        .data = genesis[g].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 16:
                //Total Level
                genesis[g].opn2.chan[chan].op[op].totallevel = value;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x40 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].tlreg }, 0);
                break;
            case 20:
                //Multiple
                genesis[g].opn2.chan[chan].op[op].fmult = GENMDM_DECODE(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x30 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].freqreg }, 0);
                break;
            case 24:
                //Detune
                genesis[g].opn2.chan[chan].op[op].detune = GENMDM_DECODE(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x30 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].freqreg }, 0);
                break;
            case 39:
                //Rate Scaling
                genesis[g].opn2.chan[chan].op[op].ratescale = GENMDM_DECODE(value,2);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x50 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].atkrsreg }, 0);
                break;
            case 43:
                //Attack Rate
                genesis[g].opn2.chan[chan].op[op].attackrate = GENMDM_DECODE(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x50 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].atkrsreg }, 0);
                break;
            case 47:
                //First Decay Rate
                genesis[g].opn2.chan[chan].op[op].decay1rate = GENMDM_DECODE(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x60 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].dec1amreg }, 0);
                break;
            case 51:
                //Secondary Decay Rate
                genesis[g].opn2.chan[chan].op[op].decay2rate = GENMDM_DECODE(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x70 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].dec2reg }, 0);
                break;
            case 55:
                //Secondary Amplitude Level
                genesis[g].opn2.chan[chan].op[op].decaylevel = GENMDM_DECODE(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x80 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].rellvlreg }, 0);
                break;
            case 59:
                //Release Rate
                genesis[g].opn2.chan[chan].op[op].releaserate = GENMDM_DECODE(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x80 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].rellvlreg }, 0);
                break;
            case 70:
                //Amplitude Modulation Enable
                genesis[g].opn2.chan[chan].op[op].amplfo = GENMDM_DECODE(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x60 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].dec1amreg }, 0);
                break;
            case 90:
                //SSG-EG
                genesis[g].opn2.chan[chan].op[op].ssgreg = GENMDM_DECODE(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x90 | opaddr, 
                        .data = genesis[g].opn2.chan[chan].op[op].ssgreg }, 0);
                break;
            case 78:
                //DAC Enable
                genesis[g].opn2.dac_enable = GENMDM_DECODE(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2B, 
                        .data = genesis[g].opn2.dacenablereg }, 0);
                break;
            case 79:
                //DAC Direct Data
                genesis[g].opn2.dac_high = value << 1;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2A, 
                        .data = genesis[g].opn2.dac_high }, 0);
                break;
            case 86:
                //DAC Sample Pitch Speed
                //TODO
                break;
            case 88:
                //DAC Sample Oversample
                //TODO
                break;
            case 89:
                //DAC Noise / Custom Wave Mode
                //TODO
                break;
            case 100:
                //Custom Wave Byte
                //TODO
                break;
            };
        }
    }else if(midi_package.event == 0x8 || (midi_package.event == 0x9 && midi_package.velocity == 0)){
        //Note Off
        if(isopn2 && (v != 3 || !ch3_op)){
            //Key off
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x28, .data = ((chan < 3) ? chan : chan+1) }, 0);
        }else if(ispsg){
            //PSG square or noise channel
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = (chan<<5)|0x9F }, 0);
        }else if(ch3_op){
            //OPN2 ch3 additional frequency
            //TODO key off individual operators?
        }
    }else if(midi_package.event == 0x9){
        //Note On
        if(isopn2){
            //TODO under what conditions is velocity mapped to TL?
            //TODO key on individual operators if in Ch3 special/CSM mode?
            u8 subchan = (chan < 3) ? chan : chan+1;
            VgmChipWriteCmd cmd = VGM_getOPN2Frequency(midi_package.note, 0, genesis_clock_opn2); //TODO cents
            cmd.cmd = (g << 4) | 2 | (subchan>>2);
            cmd.addr = 0xA4 | (subchan & 0x03);
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
            //Key on
            cmd.cmd &= 0xFE; //Switch to low bank
            cmd.addr = 0x28;
            cmd.data = 0xF0 + subchan;
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
        }else if(ispsg){
            //PSG square channel
            VgmChipWriteCmd cmd = VGM_getPSGFrequency(midi_package.note, 0, genesis_clock_psg); //TODO cents
            cmd.data |= (chan<<5) | 0x80;
            cmd.cmd = (g << 4);
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = (chan<<5)|0x90|(15-GENMDM_DECODE(midi_package.velocity, 4)) }, 0);
        }else if(chan == 9){
            //PSG noise channel
            u8 mod, noiseopts;
            mod = (midi_package.note % 12);
            switch(mod){
            //turn your head 90^ clockwise (:
            case 0:
                case 1:  noiseopts = 0b000; break;
            case 2:
                case 3:  noiseopts = 0b001; break;
            case 4:      noiseopts = 0b010; break;
            case 5:      noiseopts = 0b100; break;
                case 6:  noiseopts = 0b101; break;
            case 7:
                case 8:  noiseopts = 0b110; break;
            case 9:
                case 10: noiseopts = 0b011; break;
            case 11:     noiseopts = 0b111; break;
            //get it? (:
            };
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = 0xE0|noiseopts }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = 0xF0|(15-GENMDM_DECODE(midi_package.velocity, 4)) }, 0);
        }else if(ch3_op){
            //OPN2 ch3 additional frequency
            //TODO key on individual operators?
            VgmChipWriteCmd cmd = VGM_getOPN2Frequency(midi_package.note, 0, genesis_clock_opn2); //TODO cents
            cmd.cmd = (g << 4) | 2;
            cmd.addr = 0xAC | (ch3_op - 1);
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
        }
    }
}
