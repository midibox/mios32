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

u8 submode;
u8 selvoice;
u8 selop;
u8 voiceistracker;
u8 voicetrackerchan;

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
        default: return "Error";
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
    switch(submode){
        case 0:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            u8 g = selvoice>>4;
            u8 v = selvoice & 0xF;
            if(voiceistracker){
                MIOS32_LCD_PrintFormattedString("G%d:%s: Tracker on Prt%d:Ch%2d, editable", g+1, GetVoiceName(v), (voicetrackerchan>>4)+1, (voicetrackerchan&0xF)+1);
            }else{
                MIOS32_LCD_PrintFormattedString("G%d:%s: Free mode, state read-only", g+1, GetVoiceName(v));
            }
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("Change voice's mapping in Chan mode");
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
    }
}

void Mode_Voice_Init(){
    selvoice = 1;
    selop = 3;
}
void Mode_Voice_GotFocus(){
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
    u8 i;
    for(i=0; i<GENESIS_COUNT; ++i){
        DrawGenesisActivity(i);
    }
    if(lastselop != selop || lastselvoice != selvoice){
        ClearGenesisState_Op();
        FrontPanel_LEDSet(FP_LED_SELOP_1 + lastselop, 0);
        if((selvoice & 0xF) <= 6 && (selvoice & 0xF) >= 1){
            FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 1);
        }
    }
    lastselop = selop;
    //Draw selected voice state
    if(lastselvoice != selvoice){
        //Clear all Genesis state lights
        ClearGenesisState_Chan();
        ClearGenesisState_DAC();
        ClearGenesisState_OPN2();
        ClearGenesisState_PSG();
        FrontPanel_GenesisLEDSet(lastselvoice >> 4, lastselvoice & 0xF, 1, 0);
        FrontPanel_GenesisLEDSet(selvoice >> 4, selvoice & 0xF, 1, 1);
        lastselvoice = selvoice;
    }
    DrawGenesisState_All(selvoice >> 4, selvoice & 0xF, selop);
}

void Mode_Voice_BtnGVoice(u8 gvoice, u8 state){
    if(!state) return;
    selvoice = gvoice;
    SeeIfVoiceIsTracker();
    DrawMenu();
}
void Mode_Voice_BtnSoftkey(u8 softkey, u8 state){
    switch(submode){
        case 0:
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
    }
}
void Mode_Voice_BtnSelOp(u8 op, u8 state){
    if(!state) return;
    selop = op;
}
void Mode_Voice_BtnOpMute(u8 op, u8 state){

}
void Mode_Voice_BtnSystem(u8 button, u8 state){

}
void Mode_Voice_BtnEdit(u8 button, u8 state){
    if(voiceistracker){
        switch(button){
            case FP_B_OUT:
                submode = state ? 1 : 0;
                DrawMenu();
                break;
            case FP_B_ALG:
                submode = state ? 2 : 0;
                DrawMenu();
                break;
            case FP_B_KON:
                submode = state ? 3 : 0;
                DrawMenu();
                break;
            case FP_B_KSR:
                submode = state ? 4 : 0;
                DrawMenu();
                break;
            default:
                if(!state) return;
                Tracker_BtnToMIDI(button, 0, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
        }
    }
}

void Mode_Voice_EncDatawheel(s32 incrementer){

}
void Mode_Voice_EncEdit(u8 encoder, s32 incrementer){
    if(voiceistracker){
        Tracker_EncToMIDI(encoder, incrementer, selvoice, selop, voicetrackerchan >> 4, voicetrackerchan & 0xF);
    }
}
