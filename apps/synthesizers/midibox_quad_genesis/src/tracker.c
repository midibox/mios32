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


static inline u8 Tracker_Encode(u8 v, s32 incrementer, u8 bits){
    s32 nv = (s32)v + incrementer;
    if(nv < 0) nv = 0;
    if(nv >= (1 << bits)) nv = (1 << bits) - 1;
    return (u8)GENMDM_ENCODE(nv,bits);
}

void Tracker_EncToMIDI(u8 encoder, s32 incrementer, u8 selvoice, u8 selop, u8 midiport, u8 midichan){
    //DBG("Tracker_EncToMIDI(enc %d, inc %d, voice %d, op %d, port %d, chan %d)", encoder, incrementer, selvoice, selop, midiport, midichan);
    mios32_midi_package_t pkg;
    pkg.type = 1; //USB
    pkg.cable = midiport;
    pkg.chn = midichan;
    pkg.event = 0xB; //CC
    u8 g = selvoice >> 4;
    u8 v = selvoice & 0xF;
    u8 chan;
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
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].totallevel, incrementer, 7);
                break;
            case FP_E_HARM:
                pkg.cc_number = 20 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].fmult, incrementer, 4);
                break;
            case FP_E_DETUNE:
                pkg.cc_number = 24 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].detune, incrementer, 3);
                break;
            case FP_E_ATTACK:
                pkg.cc_number = 43 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].attackrate, incrementer, 5);
                break;
            case FP_E_DEC1R:
                pkg.cc_number = 47 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].decay1rate, incrementer, 5);
                break;
            case FP_E_DECLVL:
                incrementer = 0 - incrementer;
                pkg.cc_number = 55 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].decaylevel, incrementer, 4);
                break;
            case FP_E_DEC2R:
                pkg.cc_number = 51 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].decay2rate, incrementer, 5);
                break;
            case FP_E_RELRATE:
                pkg.cc_number = 59 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].releaserate, incrementer, 4);
                break;
            case FP_E_LFOFDEP:
                pkg.cc_number = 75;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].lfofreqd, incrementer, 3);
                break;
            case FP_E_LFOADEP:
                pkg.cc_number = 76;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].lfoampd, incrementer, 2);
                break;
            case FP_E_FEEDBACK:
                pkg.cc_number = 15;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].feedback, incrementer, 3);
                break;
            case FP_E_CSMFREQ:
                //TODO
                break;
            case FP_E_LFOFREQ:
                pkg.cc_number = 1;
                pkg.value = Tracker_Encode(genesis[g].opn2.lfo_freq, incrementer, 3);
                break;
        }
    }else if(v >= 8 && v <= 10){
        //SQ
        return; //Changes are handled by note and velocity
    }else if(v == 11){
        //NOISE
        return; //Changes are handled by note and velocity
    }else if(v == 0){
        //OPN2 Chip
        //TODO
    }else if(v == 7){
        //DAC
        //TODO
    }else{
        return;
    }
    VGM_MidiToGenesis(pkg, g, v, 0, 0);
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
    pkg.type = 2; //UART
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
}

void Tracker_BtnToMIDI(u8 button, u8 value, u8 selvoice, u8 selop, u8 midiport, u8 midichan){
    //DBG("Tracker_BtnToMIDI(btn %d, value %d, voice %d, op %d, port %d, chan %d)", button, value, selvoice, selop, midiport, midichan);
    mios32_midi_package_t pkg;
    pkg.type = 1; //USB
    pkg.cable = midiport;
    pkg.chn = midichan;
    pkg.event = 0xB; //CC
    u8 g = selvoice >> 4;
    u8 v = selvoice & 0xF;
    u8 chan;
    if(v >= 1 && v <= 6){
        //FM
        chan = v - 1;
        switch(button){
            case FP_B_OUT:
                pkg.cc_number = 77;
                pkg.value = Tracker_Encode(value & 3, 0, 2);
                break;
            case FP_B_ALG:
                pkg.cc_number = 14;
                pkg.value = Tracker_Encode(value & 7, 0, 3);
                break;
            case FP_B_KSR:
                pkg.cc_number = 39 + selop;
                pkg.value = Tracker_Encode(value & 3, 0, 2);
                break;
            case FP_B_SSGON:
                pkg.cc_number = 90 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x8, 0, 4);
                break;
            case FP_B_SSGINIT:
                pkg.cc_number = 90 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x4, 0, 4);
                break;
            case FP_B_SSGTGL:
                pkg.cc_number = 90 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x2, 0, 4);
                break;
            case FP_B_SSGHOLD:
                pkg.cc_number = 90 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].ssgreg ^ 0x1, 0, 4);
                break;
            case FP_B_LFOAM:
                pkg.cc_number = 70 + selop;
                pkg.value = Tracker_Encode(genesis[g].opn2.chan[chan].op[selop].amplfo ^ 0x1, 0, 1);
                break;
            case FP_B_CH3MODE:
                //TODO
                break;
            case FP_B_CH3FAST:
                //TODO
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
                //TODO
                break;
            case FP_B_DACOVR:
                //TODO
                break;
            case FP_B_LFO:
                //TODO
                break;
            case FP_B_EG:
                //TODO
                break;
            default:
                return;
        }
    }else if(v == 7){
        //DAC
        switch(button){
            case FP_B_DACEN:
                //TODO
                break;
            default:
                return;
        }
    }else{
        return;
    }
    VGM_MidiToGenesis(pkg, g, v, 0, 0);
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
    pkg.type = 2; //UART
    MIOS32_MIDI_SendPackage_NonBlocking(pkg.type << 4 | pkg.cable, pkg);
}
