/*
 * MIDIbox Quad Genesis: System Mode
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
#include "mode_system.h"
#include <vgm.h>
#include <genesis.h>
#include "frontpanel.h"
#include "app.h" //XXX
#include <umm_malloc.h> //Definitely XXX

u8 submode;

//TODO XXX FIXME
u8* pointers[8];
u8 pointermodes[8];

void DrawUsage(){
    vgm_meminfo_t meminfo = VGM_PerfMon_GetMemInfo();
    u8 chipuse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CHIP);
    u8 carduse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CARD);
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("RAM %d/%d Chip %d%% Card %d%%", meminfo.numusedblocks, meminfo.numblocks, chipuse, carduse);
    MIOS32_LCD_CursorSet(30,0);
    MIOS32_LCD_PrintFormattedString("DBG %d %d", DEBUG, DEBUG2); //XXX
}
void DrawMenu(){
    switch(submode){
        case 0:
            DrawUsage();
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintFormattedString("Fltr");
            break;
        case 1:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("3kHz 6dB LPF (MD1 sound): FM   PSG");
            MIOS32_LCD_CursorSet(26,1);
            MIOS32_LCD_PrintFormattedString("%s  %s", 
                    (genesis[0].board.cap_opn2 ? "On " : "Off"), 
                    (genesis[0].board.cap_psg  ? "On " : "Off"));
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("System invalid submode %d!", submode);
    }
}

void Mode_System_Init(){
    submode = 0;
    u8 i;
    for(i=0; i<8; ++i){
        pointers[i] = NULL;
        pointermodes[i] = 0;
    }
}
void Mode_System_GotFocus(){
    submode = 0;
    DrawMenu();
}

void Mode_System_Tick(){
    static u16 prescaler = 0;
    ++prescaler;
    if(prescaler == 250){
        DrawUsage();
        prescaler = 0;
    }
}
void Mode_System_Background(){

}

void Mode_System_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_System_BtnSoftkey(u8 softkey, u8 state){
    if(!state) return;
    u32 lastusedblocks = umm_numusedblocks;
    switch(pointermodes[softkey]){
    case 0:
        pointers[softkey] = malloc(1 << softkey);
        //DBG("Bullshit stalling nonsense %d", softkey);
        DBG("Malloc'd %d bytes, used %d blocks", 1 << softkey, umm_numusedblocks - lastusedblocks);
        break;
    case 1:
        pointers[softkey] = realloc(pointers[softkey], (4 << softkey));
        DBG("Realloc'd from %d to %d bytes, change %d blocks", (1 << softkey), (4 << softkey), (s32)umm_numusedblocks - (s32)lastusedblocks);
        break;
    case 2:
        pointers[softkey] = realloc(pointers[softkey], (2 << softkey));
        DBG("Realloc'd from %d to %d bytes, change %d blocks", (4 << softkey), (2 << softkey), (s32)umm_numusedblocks - (s32)lastusedblocks);
        break;
    case 3:
        free(pointers[softkey]);
        pointers[softkey] = NULL;
        DBG("Freed %d blocks", lastusedblocks - umm_numusedblocks);
        break;
    }
    ++pointermodes[softkey];
    if(pointermodes[softkey] == 4) pointermodes[softkey] = 0;
    /* TODO XXX
    switch(submode){
        case 0:
            switch(softkey){
                case 0:
                    submode = 1;
                    DrawMenu();
                    break;
            }
            break;
        case 1:
            switch(softkey){
                case 5:
                    genesis[0].board.cap_opn2 = !genesis[0].board.cap_opn2;
                    Genesis_WriteBoardBits(0);
                    MIOS32_LCD_CursorSet(26,1);
                    MIOS32_LCD_PrintFormattedString(genesis[0].board.cap_opn2 ? "On " : "Off");
                    break;
                case 6:
                    genesis[0].board.cap_psg = !genesis[0].board.cap_psg;
                    Genesis_WriteBoardBits(0);
                    MIOS32_LCD_CursorSet(31,1);
                    MIOS32_LCD_PrintFormattedString(genesis[0].board.cap_psg ? "On " : "Off");
                    break;
            }
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("System invalid submode %d!", submode);
    }
    */
}
void Mode_System_BtnSelOp(u8 op, u8 state){

}
void Mode_System_BtnOpMute(u8 op, u8 state){

}
void Mode_System_BtnSystem(u8 button, u8 state){
    if(!state) return;
    if(button == FP_B_MENU){
        submode = 0;
        DrawMenu();
    }
}
void Mode_System_BtnEdit(u8 button, u8 state){

}

void Mode_System_EncDatawheel(s32 incrementer){

}
void Mode_System_EncEdit(u8 encoder, s32 incrementer){

}
