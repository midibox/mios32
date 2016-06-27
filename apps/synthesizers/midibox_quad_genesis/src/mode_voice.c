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

u8 selvoice;
u8 selop;

void Mode_Voice_Init(){
    selvoice = 1;
    selop = 3;
}
void Mode_Voice_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Voice mode");
    //Clear all Genesis state lights
    ClearGenesisState_Op();
    ClearGenesisState_Chan();
    ClearGenesisState_DAC();
    ClearGenesisState_OPN2();
    ClearGenesisState_PSG();
    //Clear all voice buttons
    u8 g, v;
    for(g=0; g<GENESIS_COUNT; ++g){
        for(v=0; v<12; ++v){
            FrontPanel_GenesisLEDSet(g, v, 0, 0);
            FrontPanel_GenesisLEDSet(g, v, 1, 0);
        }
    }
    //Clear all op buttons
    for(g=0; g<4; ++g){
        FrontPanel_LEDSet(FP_LED_SELOP_1 + g, 0);
    }
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
