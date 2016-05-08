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

void DrawUsage(){
    vgm_meminfo_t meminfo = VGM_PerfMon_GetMemInfo();
    u8 chipuse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CHIP);
    u8 carduse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CARD);
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("RAM %d/%d Chip %d%% Card %d%%", meminfo.numusedblocks, meminfo.numblocks, chipuse, carduse);
}

void Mode_System_Init(){
    
}
void Mode_System_GotFocus(){
    DrawUsage();
}

void Mode_System_Tick(){
    static u16 prescaler = 0;
    if(prescaler == 1000){
        DrawUsage();
        prescaler = 0;
    }
}
void Mode_System_Background(){

}

void Mode_System_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_System_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_System_BtnSelOp(u8 op, u8 state){

}
void Mode_System_BtnOpMute(u8 op, u8 state){

}
void Mode_System_BtnSystem(u8 button, u8 state){

}
void Mode_System_BtnEdit(u8 button, u8 state){

}

void Mode_System_EncDatawheel(s32 incrementer){

}
void Mode_System_EncEdit(u8 encoder, s32 incrementer){

}
