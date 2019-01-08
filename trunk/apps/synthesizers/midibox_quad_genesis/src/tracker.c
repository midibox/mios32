/*
 * MIDIbox Quad Genesis: Tracker system engine
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
#include "tracker.h"

#include <genesis.h>
#include <vgm.h>
#include "frontpanel.h"


static inline u8 Tracker_Clip(u8 v, s32 incrementer, u8 bits){
    s32 nv = (s32)v + incrementer;
    if(nv < 0) nv = 0;
    if(nv >= (1 << bits)) nv = (1 << bits) - 1;
    return (u8)nv;
}

void Tracker_EncToMIDI(u8 encoder, s32 incrementer, u8 selvoice, u8 selop, u8 midiport, u8 midichan){
    DBG("Tracker_EncToMIDI(enc %d, inc %d, voice %d, op %d, port %d, chan %d)", encoder, incrementer, selvoice, selop, midiport, midichan);
    mios32_midi_package_t pkg;
    pkg.type = 1; //USB
    pkg.cable = midiport;
    pkg.chn = midichan;
    pkg.event = 0xB; //CC
    u8 g = selvoice >> 4;
    u8 v = selvoice & 0xF;
    u8 chan;
    u8 input, bits;
    if(v >= 1 && v <= 6){
        //FM
        chan = v - 1;
        switch(encoder){
            case FP_E_OP1LVL:
            case FP_E_OP2LVL:
            case FP_E_OP3LVL:
            case FP_E_OP4LVL:
                selop = encoder - FP_E_OP1LVL;
                pkg.cc_number = 16 + selop;
                incrementer = 0 - incrementer;
                input = genesis[g].opn2.chan[chan].op[selop].totallevel;
                bits = 7;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 127 - input);
                break;
            case FP_E_HARM:
                pkg.cc_number = 20 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].fmult;
                bits = 4;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_DETUNE:
                pkg.cc_number = 24 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].detune;
                bits = 3;
                if(input == 5 && incrementer > 0){
                    input = incrementer - 1;
                }else if(input == 0 && incrementer < 0){
                    input = 4 - incrementer;
                }else if(input == 7 && incrementer < 0){
                    return;
                }else if(input == 3 && incrementer > 0){
                    return;
                }else if(input >= 4){
                    input -= incrementer;
                }else{
                    input += incrementer;
                }
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (input >= 4) ? (4 - (s16)input) : input);
                break;
            case FP_E_ATTACK:
                pkg.cc_number = 43 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].attackrate;
                bits = 5;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_DEC1R:
                pkg.cc_number = 47 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].decay1rate;
                bits = 5;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_DECLVL:
                incrementer = 0 - incrementer;
                pkg.cc_number = 55 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].decaylevel;
                bits = 4;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 15 - input);
                break;
            case FP_E_DEC2R:
                pkg.cc_number = 51 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].decay2rate;
                bits = 5;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_RELRATE:
                pkg.cc_number = 59 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].releaserate;
                bits = 4;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_LFOFDEP:
                pkg.cc_number = 75;
                input = genesis[g].opn2.chan[chan].lfofreqd;
                bits = 3;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_LFOADEP:
                pkg.cc_number = 76;
                input = genesis[g].opn2.chan[chan].lfoampd;
                bits = 2;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_FEEDBACK:
                pkg.cc_number = 15;
                input = genesis[g].opn2.chan[chan].feedback;
                bits = 3;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            case FP_E_CSMFREQ:
                if(v != 3) return;
                pkg.cc_number = 119;
                input = genesis[g].opn2.timera_high >> 1;
                bits = 7;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            default:
                return;
        }
    }else if(v >= 8 && v <= 10){
        //SQ
        return; //Changes are handled by note and velocity
    }else if(v == 11){
        //NOISE
        return; //Changes are handled by note and velocity
    }else if(v == 0){
        //OPN2 Chip
        switch(encoder){
            case FP_E_LFOFREQ:
                pkg.cc_number = 1;
                input = genesis[g].opn2.lfo_freq;
                bits = 3;
                input = Tracker_Clip(input, incrementer, bits);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
                break;
            default:
                return;
        }
    }else if(v == 7){
        //DAC
        return;
    }else{
        return;
    }
    pkg.value = GENMDM_ENCODE(input,bits);
    VGM_MidiToGenesis(pkg, g, v, 0, 0);
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
    pkg.type = 2; //UART
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
}

void Tracker_BtnToMIDI(u8 button, u8 value, u8 selvoice, u8 selop, u8 midiport, u8 midichan){
    DBG("Tracker_BtnToMIDI(btn %d, value %d, voice %d, op %d, port %d, chan %d)", button, value, selvoice, selop, midiport, midichan);
    mios32_midi_package_t pkg;
    pkg.type = 1; //USB
    pkg.cable = midiport;
    pkg.chn = midichan;
    pkg.event = 0xB; //CC
    u8 g = selvoice >> 4;
    u8 v = selvoice & 0xF;
    u8 chan;
    u8 input, bits;
    if(v >= 1 && v <= 6){
        //FM
        chan = v - 1;
        switch(button){
            case FP_B_OUT:
                pkg.cc_number = 77;
                input = value & 3;
                bits = 2;
                break;
            case FP_B_ALG:
                pkg.cc_number = 14;
                input = value & 7;
                bits = 3;
                break;
            case FP_B_KSR:
                pkg.cc_number = 39 + selop;
                input = value & 3;
                bits = 2;
                break;
            case FP_B_SSGON:
                pkg.cc_number = 90 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x8;
                bits = 4;
                break;
            case FP_B_SSGINIT:
                pkg.cc_number = 90 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x4;
                bits = 4;
                break;
            case FP_B_SSGTGL:
                pkg.cc_number = 90 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x2;
                bits = 4;
                break;
            case FP_B_SSGHOLD:
                pkg.cc_number = 90 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x1;
                bits = 4;
                break;
            case FP_B_LFOAM:
                pkg.cc_number = 70 + selop;
                input = genesis[g].opn2.chan[chan].op[selop].amplfo ^ 0x1;
                bits = 1;
                break;
            case FP_B_CH3MODE:
                if(v != 3) return;
                value = (genesis[g].opn2.ch3_mode + 1) & 3;
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, value);
                //Command 1
                pkg.cc_number = 80;
                input = value & 1;
                bits = 1;
                pkg.value = (u8)GENMDM_ENCODE(input,bits);
                VGM_MidiToGenesis(pkg, g, v, 0, 0);
                MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
                pkg.type = 2; //UART
                MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
                pkg.type = 1; //USB
                //Command 2
                pkg.cc_number = 99;
                input = value >> 1;
                bits = 1;
                pkg.value = (u8)GENMDM_ENCODE(input,bits);
                VGM_MidiToGenesis(pkg, g, v, 0, 0);
                MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
                pkg.type = 2; //UART
                MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
                return;
            case FP_B_CH3FAST:
                if(v != 3) return;
                pkg.cc_number = 98;
                input = (genesis[g].opn2.timerctrlreg ^ 0x1) & 0x3F;
                bits = 6;
                break;
            default:
                return;
        }
    }else if(v >= 8 && v <= 10){
        //SQ
        return;
    }else if(v == 11){
        //NOISE
        return; //Changes are handled by the MIDI note
    }else if(v == 0){
        //OPN2 Chip
        switch(button){
            case FP_B_UGLY:
                pkg.cc_number = 95;
                input = (genesis[g].opn2.testreg21 ^ 0x10) >> 4;
                bits = 4;
                break;
            case FP_B_DACOVR:
                pkg.cc_number = 97;
                input = (genesis[g].opn2.testreg2C ^ 0x20) >> 4;
                bits = 4;
                break;
            case FP_B_LFO:
                pkg.cc_number = 74;
                input = !genesis[g].opn2.lfo_enabled;
                bits = 1;
                break;
            case FP_B_EG:
                pkg.cc_number = 95;
                input = (genesis[g].opn2.testreg21 ^ 0x20) >> 4;
                bits = 4;
                break;
            default:
                return;
        }
    }else if(v == 7){
        //DAC
        switch(button){
            case FP_B_DACEN:
                pkg.cc_number = 78;
                input = !genesis[g].opn2.dac_enable;
                bits = 1;
                break;
            default:
                return;
        }
    }else{
        return;
    }
    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, input);
    pkg.value = (u8)GENMDM_ENCODE(input,bits);
    VGM_MidiToGenesis(pkg, g, v, 0, 0);
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
    pkg.type = 2; //UART
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
}
