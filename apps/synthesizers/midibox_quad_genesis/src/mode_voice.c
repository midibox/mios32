/*
 * MIDIbox Quad Genesis: Voice Mode
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
#include "mode_voice.h"
#include "frontpanel.h"
#include "genesisstate.h"
#include "syeng.h"
#include "tracker.h"
#include "genesis.h"
#include <vgm.h>

static u8 submode;
static u8 selvoice;
static u8 selop;
static u8 voiceistracker;
static u8 voicetrackerchan;
static u8 vumetermode;

const char* GetVoiceName(u8 subvoice){
    switch(subvoice){
        case 0: return "OPN2";
        case 1: return "FM1";
        case 2: return "FM2";
        case 3: return "FM3";
        case 4: return "FM4";
        case 5: return "FM5";
        case 6: return "FM6";
        case 7: return "DAC";
        case 8: return "SQ1";
        case 9: return "SQ2";
        case 10: return "SQ3";
        case 11: return "Noise";
        case 12: return "FM3O1";
        case 13: return "FM3O2";
        case 14: return "FM3O3";
        default: return "Error";
    }
}

const char* GetKonModeName(u8 konmode){
    switch(konmode){
        case 0: return "Normal   ";
        case 1: return "Const Off";
        case 2: return "Const On ";
        case 3: return "FM3 4Freq";
        default: return "Error    ";
    }
}

static void SeeIfVoiceIsTracker(){
    u8 c;
    voiceistracker = 0;
    for(c=0; c<16*MBQG_NUM_PORTS; ++c){
        if(channels[c].trackermode && channels[c].trackervoice == selvoice){
            voiceistracker = 1;
            voicetrackerchan = c;
            break;
        }
    }
}

static void DrawMenu(){
    u8 g = selvoice>>4;
    u8 v = selvoice & 0xF;
    switch(submode){
        case 0:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            if(voiceistracker){
                MIOS32_LCD_PrintFormattedString("G%d:%s: Tracker on Prt%d:Ch%2d, editable", g+1, GetVoiceName(v), (voicetrackerchan>>4)+1, (voicetrackerchan&0xF)+1);
            }else{
                MIOS32_LCD_PrintFormattedString("G%d:%s: Free mode, state read-only", g+1, GetVoiceName(v));
            }
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintFormattedString("Meter %s", vumetermode ? "Ops" : "PSG");
            break;
        case 1:
            //Channel output (stereo)
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Channel output (stereo):");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("  L    C    R   Off");
            break;
        case 2:
            //Algorithm
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("      1     1   124  12   111  12   1 2 ");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("1234  234  234   3   34   234  3 4  3 4 ");
            break;
        case 3:
            //Key On
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("   Op 1      Op 2      Op 3      Op 4   ");
            u8 k;
            for(k=0; k<4; ++k){
                MIOS32_LCD_CursorSet(10*k,1);
                MIOS32_LCD_PrintString(GetKonModeName((trackerkeyonmodes[(selvoice>>4)*6+(selvoice & 0xF)-1] >> (2*k)) & 3));
            }
            break;
        case 4:
            //KSR
            if(v == 0 || v >= 7) return;
            --v;
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("  0    1    2    3   Key Scale Rate:");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString(" 1/2  1/1  2/1  4/1  Add'l rate/octave");
            u8 ksr = genesis[g].opn2.chan[v].op[selop].ratescale;
            MIOS32_LCD_CursorSet(5*ksr,1);
            MIOS32_LCD_PrintChar('~');
            break;
    }
}

void Mode_Voice_Init(){
    selvoice = 1;
    selop = 3;
    vumetermode = 0;
}
void Mode_Voice_GotFocus(){
    VGM_Player_docapture = 1;
    submode = 0;
    SeeIfVoiceIsTracker();
    DrawMenu();
    //Turn on our voice button
    FrontPanel_GenesisLEDSet(selvoice >> 4, selvoice & 0xF, 1, 1);
    //Op button
    if((selvoice & 0xF) <= 6 && (selvoice & 0xF) >= 1){
        FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 1);
    }
}

void Mode_Voice_Tick(){
    
}
void Mode_Voice_Background(){
    static u8 lastselvoice = 0, lastselop = 0;
    //Draw Genesis voice activiy
    u8 g, v, i;
    for(g=0; g<GENESIS_COUNT; ++g){
        for(v=0; v<12; ++v){
            DrawGenesisActivity(g, v, g, v);
        }
    }
    //Transition displayed voice and op states
    if(lastselop != selop || lastselvoice != selvoice){
        ClearGenesisState_Op();
        FrontPanel_LEDSet(FP_LED_SELOP_1 + lastselop, 0);
        if((selvoice & 0xF) <= 6 && (selvoice & 0xF) >= 1){
            FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 1);
        }
    }
    lastselop = selop;
    if(lastselvoice != selvoice){
        ClearGenesisState_Chan();
        ClearGenesisState_DAC();
        ClearGenesisState_OPN2();
        ClearGenesisState_PSG();
        FrontPanel_GenesisLEDSet(lastselvoice >> 4, lastselvoice & 0xF, 1, 0);
        FrontPanel_GenesisLEDSet(selvoice >> 4, selvoice & 0xF, 1, 1);
        lastselvoice = selvoice;
    }
    //Draw voice state
    g = selvoice >> 4;
    v = selvoice & 0xF;
    DrawGenesisState_All(g, v, selop);
    //Draw op VU meters
    if(v >= 1 && v <= 6 && vumetermode){
        for(i=0; i<4; ++i){
            DrawOpVUMeter(g, v-1, i);
        }
    }else{
        for(i=0; i<4; ++i){
            FrontPanel_VGMMatrixVUMeter(i+10, (15 - genesis[g].psg.voice[i].atten) >> 1);
        }
    }
    //Draw chan VU meters
    for(i=0; i<6; ++i){
        DrawChanVUMeter(g, i, i);
    }
    DrawDACVUMeter(g);
}

void Mode_Voice_BtnGVoice(u8 gvoice, u8 state){
    if(!state) return;
    if(submode) return;
    selvoice = gvoice;
    SeeIfVoiceIsTracker();
    DrawMenu();
}
void Mode_Voice_BtnSoftkey(u8 softkey, u8 state){
    switch(submode){
        case 0:
            if(!state) return;
            switch(softkey){
                case 0:
                case 1:
                    vumetermode = !vumetermode;
                    DrawMenu();
                    return;
            }
            break;
        case 1:
            //Channel output (stereo)
            switch(softkey){
                case 0:
                    Tracker_BtnToMIDI(FP_B_OUT, 1, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
                    break;
                case 1:
                    Tracker_BtnToMIDI(FP_B_OUT, 3, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
                    break;
                case 2:
                    Tracker_BtnToMIDI(FP_B_OUT, 2, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
                    break;
                case 3:
                    Tracker_BtnToMIDI(FP_B_OUT, 0, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
                    break;
            }
            break;
        case 2:
            //Algorithm
            Tracker_BtnToMIDI(FP_B_ALG, softkey, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
            break;
        case 3:
            //KOn
            if(!state) return;
            softkey &= 0xFE;
            u8 i = (selvoice>>4)*6+(selvoice & 0xF)-1;
            u8 j = (trackerkeyonmodes[i] >> softkey) & 3;
            j = (j+1)&3;
            trackerkeyonmodes[i] = (trackerkeyonmodes[i] & ~(3 << softkey)) | (j << softkey);
            DrawMenu();
            break;
        case 4:
            //KSR
            if(softkey >= 4) return;
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintChar(' ');
            MIOS32_LCD_CursorSet(5,1);
            MIOS32_LCD_PrintChar(' ');
            MIOS32_LCD_CursorSet(10,1);
            MIOS32_LCD_PrintChar(' ');
            MIOS32_LCD_CursorSet(15,1);
            MIOS32_LCD_PrintChar(' ');
            MIOS32_LCD_CursorSet(5*softkey,1);
            MIOS32_LCD_PrintChar('~');
            Tracker_BtnToMIDI(FP_B_KSR, softkey, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
            break;
    }
}
void Mode_Voice_BtnSelOp(u8 op, u8 state){
    if(!state) return;
    selop = op;
}
void Mode_Voice_BtnOpMute(u8 op, u8 state){

}
void Mode_Voice_BtnSystem(u8 button, u8 state){
    if(!state) return;
    switch(button){
        case FP_B_CTRL:
            SyEng_PrintEngineDebugInfo();
            break;
    }
    /*
    if(button == FP_B_CAPTURE){
        u8 i;
        for(i=0; i<6; ++i){
            DBG("Chan %d %03d %03d %03d %03d", i+1, genesis[0].opn2.chan[i].op[0].test_statehigh,
                                                    genesis[0].opn2.chan[i].op[1].test_statehigh,
                                                    genesis[0].opn2.chan[i].op[2].test_statehigh,
                                                    genesis[0].opn2.chan[i].op[3].test_statehigh);
        }
    }
    */
}
void Mode_Voice_BtnEdit(u8 button, u8 state){
    u8 v = selvoice & 0xF;
    if(voiceistracker){
        if(v >= 1 && v <= 6){
            switch(button){
                case FP_B_OUT:
                    submode = state ? 1 : 0;
                    DrawMenu();
                    return;
                case FP_B_ALG:
                    submode = state ? 2 : 0;
                    DrawMenu();
                    return;
                case FP_B_KON:
                    submode = state ? 3 : 0;
                    DrawMenu();
                    return;
                case FP_B_KSR:
                    submode = state ? 4 : 0;
                    DrawMenu();
                    return;
            }
        }
        //All other cases:
        if(!state) return;
        Tracker_BtnToMIDI(button, 0, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
    }
}

void Mode_Voice_EncDatawheel(s32 incrementer){

}
void Mode_Voice_EncEdit(u8 encoder, s32 incrementer){
    if(voiceistracker){
        Tracker_EncToMIDI(encoder, incrementer, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
    }
}
