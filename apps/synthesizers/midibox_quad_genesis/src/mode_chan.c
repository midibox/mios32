/*
 * MIDIbox Quad Genesis: Channel Mode
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
#include "mode_chan.h"

#include "mode_voice.h"
#include "syeng.h"
#include "frontpanel.h"

u8 selchan;
u8 submode;
u8 cursor;

static void DrawMenu(){
    switch(submode){
        case 0:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Prt%d:Ch%2d", (selchan >> 4)+1, (selchan & 0xF)+1);
            MIOS32_LCD_CursorSet(0,1);
            if(channels[selchan].trackermode){
                u8 v = channels[selchan].trackervoice;
                MIOS32_LCD_PrintFormattedString("Trkr  G%d   %s", (v >> 4)+1, GetVoiceName(v & 0xF));
                if(cursor >= 1 && cursor <= 2){
                    MIOS32_LCD_CursorSet(5*cursor,1);
                    MIOS32_LCD_PrintString("~");
                }
            }else{
                MIOS32_LCD_PrintString("Free");
            }
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Chan invalid submode %d!", submode);
    }
}

void Mode_Chan_Init(){
    selchan = 0;
    submode = 0;
    cursor = 0;
}
void Mode_Chan_GotFocus(){
    submode = 0;
    DrawMenu();
    FrontPanel_GenesisLEDSet((selchan >> 4), 0, 1, 1);
    FrontPanel_GenesisLEDSet((selchan >> 2) & 3, (selchan & 3)+8, 1, 1);
    if(channels[selchan].trackermode){
        u8 v = channels[selchan].trackervoice;
        FrontPanel_GenesisLEDSet(v >> 4, v & 0xF, 0, 1);
    }
}

void Mode_Chan_Tick(){

}
void Mode_Chan_Background(){

}

void Mode_Chan_BtnGVoice(u8 gvoice, u8 state){
    if(!state) return;
    u8 g = gvoice >> 4;
    u8 v = gvoice & 0xF;
    u8 t = channels[selchan].trackervoice;
    if(v == 0){
        FrontPanel_GenesisLEDSet(t >> 4, t & 0xF, 0, 0);
        FrontPanel_GenesisLEDSet((selchan >> 4), 0, 1, 0);
        selchan = (g << 4) | (selchan & 0xF);
        FrontPanel_GenesisLEDSet(g, 0, 1, 1);
        if(channels[selchan].trackermode){
            t = channels[selchan].trackervoice;
            FrontPanel_GenesisLEDSet(t >> 4, t & 0xF, 0, 1);
        }
        DrawMenu();
    }else if(v >= 8 && v <= 11){
        v -= 8;
        FrontPanel_GenesisLEDSet(t >> 4, t & 0xF, 0, 0);
        FrontPanel_GenesisLEDSet((selchan >> 2) & 3, (selchan & 3)+8, 1, 0);
        selchan = (selchan & 0xF0) | (g << 2) | v;
        FrontPanel_GenesisLEDSet(g, v+8, 1, 1);
        if(channels[selchan].trackermode){
            t = channels[selchan].trackervoice;
            FrontPanel_GenesisLEDSet(t >> 4, t & 0xF, 0, 1);
        }
        DrawMenu();
    }
}
void Mode_Chan_BtnSoftkey(u8 softkey, u8 state){
    if(!state) return;
    u8 t;
    switch(submode){
        case 0:
            switch(softkey){
                case 0:
                    t = !channels[selchan].trackermode;
                    channels[selchan].trackermode = t;
                    u8 v = channels[selchan].trackervoice;
                    FrontPanel_GenesisLEDSet(v >> 4, v & 0xF, 0, t);
                    cursor = 0;
                    DrawMenu();
                    break;
                case 1:
                    if(!channels[selchan].trackermode) return;
                    cursor = 1;
                    DrawMenu();
                    break;
                case 2:
                    if(!channels[selchan].trackermode) return;
                    cursor = 2;
                    DrawMenu();
                    break;
            }
            break;
    }
}
void Mode_Chan_BtnSelOp(u8 op, u8 state){

}
void Mode_Chan_BtnOpMute(u8 op, u8 state){

}
void Mode_Chan_BtnSystem(u8 button, u8 state){

}
void Mode_Chan_BtnEdit(u8 button, u8 state){

}

void Mode_Chan_EncDatawheel(s32 incrementer){
    s8 i, j;
    switch(submode){
        case 0:
            switch(cursor){
                case 1:
                    i = channels[selchan].trackervoice;
                    j = (i >> 4);
                    i &= 0xF;
                    FrontPanel_GenesisLEDSet(j, i, 0, 0);
                    j += incrementer;
                    if(j < 0) j = 0;
                    if(j >= GENESIS_COUNT) j = GENESIS_COUNT-1;
                    channels[selchan].trackervoice = (j << 4) | i;
                    FrontPanel_GenesisLEDSet(j, i, 0, 1);
                    MIOS32_LCD_CursorSet(7,1);
                    MIOS32_LCD_PrintFormattedString("%d", j+1);
                    break;
                case 2:
                    i = channels[selchan].trackervoice;
                    j = (i & 0xF);
                    i >>= 4;
                    FrontPanel_GenesisLEDSet(i, j, 0, 0);
                    j += incrementer;
                    if(j < 0) j = 0;
                    if(j >= 0xC) j = 0xB;
                    channels[selchan].trackervoice = (i << 4) | j;
                    FrontPanel_GenesisLEDSet(i, j, 0, 1);
                    MIOS32_LCD_CursorSet(11,1);
                    MIOS32_LCD_PrintFormattedString("%s  ", GetVoiceName(j));
                    break;
            }
            break;
    }
}
void Mode_Chan_EncEdit(u8 encoder, s32 incrementer){

}
