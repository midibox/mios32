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

u8 selvoice;
u8 selop;

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

void DrawVoiceInfo(){
    u8 g = selvoice>>4;
    u8 v = selvoice & 0xF;
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    if(syngenesis[g].trackerbits & (1 << v)){
        MIOS32_LCD_PrintFormattedString("G%d:%s: Tracker mode, state editable", g, GetVoiceName(v));
    }else{
        MIOS32_LCD_PrintFormattedString("G%d:%s: Free mode, state read-only", g, GetVoiceName(v));
    }
    MIOS32_LCD_CursorSet(0,1);
    MIOS32_LCD_PrintString("Change voice's mapping in Chan mode");
}

void Mode_Voice_Init(){
    selvoice = 1;
    selop = 3;
}
void Mode_Voice_GotFocus(){
    DrawVoiceInfo();
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
        DrawVoiceInfo();
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
}
void Mode_Voice_BtnSoftkey(u8 softkey, u8 state){

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

}

void Mode_Voice_EncDatawheel(s32 incrementer){

}
void Mode_Voice_EncEdit(u8 encoder, s32 incrementer){

}
