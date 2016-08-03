/*
 * MIDIbox Quad Genesis: VGM Editor Mode
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
#include "mode_vgm.h"

#include <genesis.h>
#include <vgm.h>
#include "syeng.h"
#include "frontpanel.h"

VgmSource* selvgm;
static u8 submode;

static void DrawMenu(){
    MIOS32_LCD_Clear();
    if(selvgm == NULL){
        MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintString("No VGM selected to edit");
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintString("(Create or select VGM in Prog mode)");
        return;
    }
    switch(submode){
        case 0:
            MIOS32_LCD_CursorSet(5,0);
            MIOS32_LCD_PrintFormattedString("Ck:%d/%d", selvgm->opn2clock, selvgm->psgclock);
            MIOS32_LCD_CursorSet(0,0);
            switch(selvgm->type){
                case VGM_SOURCE_TYPE_RAM:
                    MIOS32_LCD_PrintString("RAM  ");
                    break;
                case VGM_SOURCE_TYPE_STREAM:
                    MIOS32_LCD_PrintString("STRM ");
                    break;
                case VGM_SOURCE_TYPE_QUEUE:
                    MIOS32_LCD_PrintString("QUEUE");
                    break;
                default:
                    MIOS32_LCD_PrintString("ERROR");
                    break;
            }
            break;
        case 1:
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Mute voices");
            break;
    }
}
static void DrawMuteStates(){
    u8 i;
    u16 mutes = selvgm->mutes;
    for(i=0; i<12; ++i){
        FrontPanel_GenesisLEDSet(0, i, 0, mutes & 1);
        mutes >>= 1;
    }
}
static void SetMuteState(u8 voice, u8 state){
    if(state) selvgm->mutes |= (1 << voice);
    else selvgm->mutes &= ~(1 << voice);
    MIOS32_IRQ_Disable();
    u8 i;
    VgmHead* head;
    for(i=0; i<vgm_numheads; ++i){
        head = vgm_heads[i];
        if(head != NULL && head->source == selvgm){
            VgmHead_Channel* c = &head->channel[voice];
            c->mute = state;
            //Key off
            if(voice >= 1 && voice <= 6){
                VGM_Tracker_Enqueue((VgmChipWriteCmd){.cmd = (c->map_chip << 4)|2, .addr = 0x28, .data = (c->map_voice >= 3 ? c->map_voice+1 : c->map_voice), .data2 = 0}, 0);
            }else if(voice >= 8 && voice <= 10){
                VGM_Tracker_Enqueue((VgmChipWriteCmd){.cmd = (c->map_chip << 4), .addr = 0, .data = 0x9F|(c->map_voice << 5), .data2 = 0}, 0);
            }else if(voice == 11){
                VGM_Tracker_Enqueue((VgmChipWriteCmd){.cmd = (c->map_chip << 4), .addr = 0, .data = 0xFF, .data2 = 0}, 0);
            }
        }
    }
    MIOS32_IRQ_Enable();
}

void Mode_Vgm_Init(){

}
void Mode_Vgm_GotFocus(){
    submode = 0;
    DrawMenu();
}

void Mode_Vgm_Tick(){

}
void Mode_Vgm_Background(){

}

void Mode_Vgm_BtnGVoice(u8 gvoice, u8 state){
    if(selvgm == NULL) return;
    if(gvoice >= 0x10) return;
    gvoice &= 0xF;
    u8 a;
    switch(submode){
        case 1:
            if(!state) break;
            a = !(selvgm->mutes & (1 << gvoice));
            SetMuteState(gvoice, a);
            DrawMuteStates();
            break;
    }
}
void Mode_Vgm_BtnSoftkey(u8 softkey, u8 state){
    if(selvgm == NULL) return;

}
void Mode_Vgm_BtnSelOp(u8 op, u8 state){

}
void Mode_Vgm_BtnOpMute(u8 op, u8 state){

}
void Mode_Vgm_BtnSystem(u8 button, u8 state){
    if(selvgm == NULL) return;
    if(button == FP_B_MENU){
        submode = 0;
        DrawMenu();
        return;
    }
    switch(submode){
        case 0:
            switch(button){
                case FP_B_MUTE:
                    if(!state) return;
                    FrontPanel_LEDSet(FP_LED_MUTE, 1);
                    submode = 1;
                    DrawMenu();
                    DrawMuteStates();
                    break;
            }
            break;
        case 1:
            switch(button){
                case FP_B_MUTE:
                    if(state) return;
                    FrontPanel_LEDSet(FP_LED_MUTE, 0);
                    submode = 0;
                    DrawMenu();
                    break;
            }
            break;
    }
}
void Mode_Vgm_BtnEdit(u8 button, u8 state){
    if(selvgm == NULL) return;

}

void Mode_Vgm_EncDatawheel(s32 incrementer){
    if(selvgm == NULL) return;

}
void Mode_Vgm_EncEdit(u8 encoder, s32 incrementer){

}
