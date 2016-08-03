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
#include "syeng.h"

static u8 submode;

static u8 counter;
static const u8 button_lut[10] = {44, 44, 45, 45, 37, 38, 37, 38, 53, 51};

static void DrawUsage(){
    vgm_meminfo_t m = VGM_PerfMon_GetMemInfo();
    u8 chipuse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CHIP);
    u8 carduse = VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CARD);
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("Chip %2d%% Card %2d%%", chipuse, carduse);
    MIOS32_LCD_CursorSet(21,0);
    MIOS32_LCD_PrintFormattedString("|   RAM %5d/%5d", m.main_used, m.main_total);
    MIOS32_LCD_CursorSet(21,1);
    MIOS32_LCD_PrintFormattedString("| Heap2 %5d/%5d", m.vgmh2_used, m.vgmh2_total);
}
static void DrawMenu(){
    switch(submode){
        case 0:
            MIOS32_LCD_Clear();
            DrawUsage();
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintFormattedString("Fltr Opts");
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
        case 2:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Options   VClr");
            MIOS32_LCD_CursorSet(10,1);
            MIOS32_LCD_PrintString(voiceclearfull ? "Full" : "KOff");
            break;
        case 3:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Demo mode unlocked!");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("(Coming Soon)");
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("System invalid submode %d!", submode);
    }
}

void Mode_System_Init(){
    submode = 0;
}
void Mode_System_GotFocus(){
    submode = 0;
    counter = 0;
    DrawMenu();
}

void Mode_System_Tick(){
    static u16 prescaler = 0;
    ++prescaler;
    if(prescaler == 500){
        if(submode == 0){
            DrawUsage();
        }
        prescaler = 0;
    }
}
void Mode_System_Background(){

}

void Mode_System_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_System_BtnSoftkey(u8 softkey, u8 state){
    if(!state) return;
    switch(submode){
        case 0:
            switch(softkey){
                case 0:
                    submode = 1;
                    DrawMenu();
                    break;
                case 1:
                    submode = 2;
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
        case 2:
            switch(softkey){
                case 2:
                    voiceclearfull = !voiceclearfull;
                    DrawMenu();
                    break;
            }
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("System invalid submode %d!", submode);
    }
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
        return;
    }
    switch(submode){
        case 2:
            if(button == button_lut[counter]){
                if(counter == 9){
                    submode = 3;
                    DrawMenu();
                    return;
                }else{
                    ++counter;
                }
            }else{
                counter = 0;
            }
            break;
    }
}
void Mode_System_BtnEdit(u8 button, u8 state){
    Mode_System_BtnSystem(button, state);
}

void Mode_System_EncDatawheel(s32 incrementer){

}
void Mode_System_EncEdit(u8 encoder, s32 incrementer){

}
