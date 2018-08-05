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
#include <blm_x.h>
#include <jsw_rand.h>
#include "frontpanel.h"
#include "syeng.h"
#include "interface.h"

static u8 submode;
static u8 test_character;

static u16 tick_prescaler;
static u8 counter;
static const u8 button_lut[10] = {44, 44, 45, 45, 37, 38, 37, 38, 53, 51};

static s8 vegastextpos;
static u32 vegascounter;
static u16 vegasprescaler;
static u8 vegasstate;
static u8 vegassub;
static const char mbx_name[29] = "AUTHENTIC QUAD GENESIS SOUND";
static const u8 mbx_name_len = 28;
static s8 p_ballx, p_bally;
static s8 p_ballvx, p_ballvy;
static s8 p_pdll, p_pdlr;
static u8 p_scorel, p_scorer, p_scoretmr;

static void InitPong(u8 initscore){
    u32 r = jsw_rand();
    p_ballvx = (r & 1) ? 1 : -1;
    p_ballvy = (r & 2) ? 1 : -1;
    if(r & 4) p_ballx = 7; else p_ballx = 5;
    if(r & 8) p_ballx = 6;
    p_bally = 1 + ((r >> 4) & 3);
    p_pdll = p_pdlr = 2;
    if(initscore){
        p_scorel = p_scorer = 0;
        p_scoretmr = 0x08;
    }
}

static void DrawPong(){
    u8 i;
    //Clear
    for(i=0; i<7; ++i){
        FrontPanel_VGMMatrixRow(i, 0);
    }
    if(p_scoretmr < 0x08){
        //Ball
        FrontPanel_VGMMatrixPoint(p_bally, p_ballx, 1);
        FrontPanel_VGMMatrixPoint(p_bally+1, p_ballx, 1);
        FrontPanel_VGMMatrixPoint(p_bally, p_ballx+1, 1);
        FrontPanel_VGMMatrixPoint(p_bally+1, p_ballx+1, 1);
        //Paddles
        for(i=0; i<3; ++i){
            FrontPanel_VGMMatrixPoint(p_pdll+i, 0, 1);
            FrontPanel_VGMMatrixPoint(p_pdlr+i, 13, 1);
        }
    }else{
        for(i=0; i<7; ++i){
            if(p_scorel > i){
                FrontPanel_VGMMatrixPoint(6-i, 0, 1);
            }
            if(p_scorer > i){
                FrontPanel_VGMMatrixPoint(6-i, 13, 1);
            }
        }
    }
}

static void UpdatePong(u8 aiL, u8 aiR){
    u32 r = jsw_rand();
    if(p_scoretmr){
        --p_scoretmr;
        return;
    }
    if(aiL && p_ballvx == -1 && p_ballx < 9){
        if(p_bally > p_pdll + 1 && p_pdll < 4) ++p_pdll;
        else if(p_bally < p_pdll && p_pdll > 0) --p_pdll;
    }
    if(aiR && p_ballvx == 1 && p_ballx > 4){
        if(p_bally > p_pdlr + 1 && p_pdlr < 4) ++p_pdlr;
        else if(p_bally < p_pdlr && p_pdlr > 0) --p_pdlr;
    }
    if(p_bally == 0) p_ballvy = 1;
    if(p_bally == 5) p_ballvy = -1;
    if(p_ballx == 1 && p_ballvx == -1 
            && (p_bally - p_pdll >= -1) && (p_bally - p_pdll <= 2)){
        p_ballvx = 1;
        if((r & 1)) p_ballx = 0;
    }
    if(p_ballx == 11 && p_ballvx == 1
            && (p_bally - p_pdlr >= -1) && (p_bally - p_pdlr <= 2)){
        p_ballvx = -1;
        if((r & 2)) p_ballx = 12;
    }
    if(p_ballx == -2){
        ++p_scorer;
        p_scoretmr = 0x10;
        InitPong(0);
    }
    if(p_ballx == 14){
        ++p_scorel;
        p_scoretmr = 0x10;
        InitPong(0);
    }
    p_ballx += p_ballvx;
    p_bally += p_ballvy;
}

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
            MIOS32_LCD_PrintFormattedString("Fltr Opts Ctrlr");
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
            MIOS32_LCD_PrintString("Options   VClr  Test");
            MIOS32_LCD_CursorSet(10,1);
            MIOS32_LCD_PrintString(voiceclearfull ? "Full" : "KOff");
            MIOS32_LCD_PrintString("  LCD");
            break;
        case 3:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Character %d: ", test_character);
            MIOS32_LCD_PrintChar(test_character);
            break;
        case 4:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Welcome to DEMO MODE! :)");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("LED Test  --Vegas--  -Tetris-  --Pong--");
            break;
        case 5:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("LED Test");
            break;
        case 6:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Vegas");
            break;
        case 7:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Tetris not implemented yet");
            break;
        case 8:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Pong");
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("System invalid submode %d!", submode);
    }
}

void Mode_System_Init(){
    submode = 0;
    tick_prescaler = 0;
    test_character = 0;
    vegasprescaler = 0;
    vegasstate = 0;
    vegascounter = 0;
    vegassub = 0;
    jsw_seed(1337);
    InitPong(1);
}
void Mode_System_GotFocus(){
    jsw_seed(tick_prescaler ^ 420);
    submode = 0;
    counter = 0;
    DrawMenu();
}

void Mode_System_Tick(){
    ++tick_prescaler;
    if(tick_prescaler == 500){
        if(submode == 0){
            DrawUsage();
        }
        tick_prescaler = 0;
    }
}
void Mode_System_Background(){
    u8 i;
    u16 j;
    switch(submode){
        case 5:
            for(j=0; j<8*88; ++j){
                BLM_X_LEDSet(j, 0, 1);
            }
            break;
        case 6:
            //==================================================================
            //============================ VEGAS ===============================
            //==================================================================
            if(vegasprescaler == tick_prescaler) return;
            vegasprescaler = tick_prescaler;
            switch(vegasstate){
                case 0:
                    BLM_X_LEDSet(((vegascounter & 7) * 88) + (vegascounter >> 3), 0, 1);
                    if(++vegascounter == 88*8){
                        vegascounter = 0;
                        vegasstate = 1;
                    }
                    break;
                case 1:
                    BLM_X_LEDSet(((vegascounter & 7) * 88) + (vegascounter >> 3), 0, 0);
                    if(++vegascounter == 88*8){
                        vegascounter = 0;
                        if(++vegassub == 2){
                            vegassub = 0;
                            vegasstate = 2;
                        }else{
                            vegasstate = 0;
                        }
                    }
                    break;
                case 2:
                    BLM_X_LEDSet(vegascounter, 0, 1);
                    if(++vegascounter == 88*8){
                        vegascounter = 0;
                        vegasstate = 3;
                    }
                    break;
                case 3:
                    BLM_X_LEDSet(vegascounter, 0, 0);
                    if(++vegascounter == 88*8){
                        vegascounter = 0;
                        if(++vegassub == 2){
                            vegassub = 0;
                            vegasstate = 4;
                            vegastextpos = -9;
                        }else{
                            vegasstate = 2;
                        }
                    }
                    break;
                case 4:
                    for(i=0; i<18; i++){
                        FrontPanel_LEDRingSet(i, 1, (i&1) ? 
                                (15 - ((vegascounter >> 6) & 0xF)) : ((vegascounter >> 6) & 0xF));
                    }
                    FrontPanel_GenesisLEDSet((vegascounter >> 6) & 3, 
                            (vegascounter & 0xF), (vegascounter >> 8) & 1, 1);
                    FrontPanel_GenesisLEDSet((vegascounter >> 6) & 3, 
                            (vegascounter & 0xF), ~(vegascounter >> 8) & 1, 0);
                    if(++vegascounter == 0x400){
                        vegascounter = 0;
                        vegasstate = 5;
                    }
                    break;
                case 5:
                    for(i=0; i<18; i++){
                        FrontPanel_LEDRingSet(i, 1, !(i&1) ? 
                                (15 - ((vegascounter >> 6) & 0xF)) : ((vegascounter >> 6) & 0xF));
                    }
                    FrontPanel_GenesisLEDSet(3 - ((vegascounter >> 6) & 3), 
                            (vegascounter & 0xF), (vegascounter >> 8) & 1, 1);
                    FrontPanel_GenesisLEDSet(3 - ((vegascounter >> 6) & 3), 
                            (vegascounter & 0xF), ~(vegascounter >> 8) & 1, 0);
                    if(++vegascounter == 0x400){
                        vegascounter = 0;
                        if(++vegassub == 2){
                            vegassub = 0;
                            vegasstate = 6;
                        }else{
                            vegasstate = 4;
                        }
                    }
                    break;
                case 6:
                    for(i=0; i<18; i++){
                        FrontPanel_LEDRingSet(i, 2, ((vegascounter >> 6) & 0xF));
                    }
                    i = ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (vegascounter & 1)
                            ^ ((vegascounter & 0xF) == 0 || (vegascounter & 0xF) == 7);
                    FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 0, i);
                    FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 1, !i);
                    if(++vegascounter == 0x400){
                        vegascounter = 0;
                        vegasstate = 7;
                    }
                    break;
                case 7:
                    for(i=0; i<18; i++){
                        FrontPanel_LEDRingSet(i, 3, 15 - ((vegascounter >> 6) & 0xF));
                    }
                    i = ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (vegascounter & 1)
                            ^ ((vegascounter & 0xF) == 0 || (vegascounter & 0xF) == 7);
                    FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 0, i);
                    FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 1, !i);
                    if(++vegascounter == 0x400){
                        vegascounter = 0;
                        if(++vegassub == 1){
                            vegassub = 0;
                            vegasstate = 0;
                        }else{
                            vegasstate = 6;
                        }
                    }
                    break;
            }
            if(vegasstate >= 4 && vegasstate <= 7){
                char k;
                for(j=0; j<9; ++j){
                    if(vegastextpos + j < 0 || vegastextpos + j >= mbx_name_len){
                        k = ' ';
                    }else{
                        k = mbx_name[vegastextpos + j];
                    }
                    if(j < 4){
                        i = FP_LED_DIG_MAIN_1 + j;
                    }else if(j == 4){
                        i = FP_LED_DIG_OCT;
                    }else{
                        i = FP_LED_DIG_FREQ_1 + j - 5;
                    }
                    FrontPanel_DrawDigit(i, k);
                }
                if(!(vegascounter & 0x7F)){
                    ++vegastextpos;
                    if(vegastextpos > mbx_name_len){
                        vegastextpos = -9;
                    }
                }
                DrawPong();
                if(!(vegascounter & 0x3F)){
                    UpdatePong(1, 1);
                }
            }
            //==================================================================
            //========================== END VEGAS =============================
            //==================================================================
            break;
        case 7:
            
            break;
        case 8:
            if(vegasprescaler == tick_prescaler) return;
            vegasprescaler = tick_prescaler;
            ++vegascounter;
            DrawPong();
            if(!(vegascounter & 0x3F)){
                UpdatePong(0, 0);
            }
            break;
    }
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
                case 2:
                    Interface_ChangeToMode(MODE_CONTROLLER);
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
                case 3:
                    submode = 3;
                    DrawMenu();
                    break;
            }
            break;
        case 3:
            //nothing
            break;
        case 4:
            switch(softkey){
                case 0:
                case 1:
                    submode = 5;
                    DrawMenu();
                    break;
                case 2:
                case 3:
                    submode = 6;
                    DrawMenu();
                    break;
                case 4:
                case 5:
                    submode = 7;
                    DrawMenu();
                case 6:
                case 7:
                    submode = 8;
                    InitPong(1);
                    DrawMenu();
                    break;
            }
            break;
        case 5:
            //nothing
            break;
        case 6:
            //TODO
            break;
        case 7:
            //TODO
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
        if(submode > 4) submode = 4;
        else submode = 0;
        DrawMenu();
        return;
    }
    switch(submode){
        case 2:
            if(button == button_lut[counter]){
                if(counter == 9){
                    submode = 4;
                    DrawMenu();
                    return;
                }else{
                    ++counter;
                }
            }else{
                counter = 0;
            }
            break;
        case 8:
            switch(button){
                case FP_B_MOVEUP:
                    if(p_pdll > 0) --p_pdll;
                    break;
                case FP_B_MOVEDN:
                    if(p_pdll < 4) ++p_pdll;
                    break;
                case FP_B_CMDUP:
                    if(p_pdlr > 0) --p_pdlr;
                    break;
                case FP_B_CMDDN:
                    if(p_pdlr < 4) ++p_pdlr;
                    break;
            }
            break;
    }
}
void Mode_System_BtnEdit(u8 button, u8 state){
    Mode_System_BtnSystem(button, state);
}

void Mode_System_EncDatawheel(s32 incrementer){
    switch(submode){
        case 3:
            test_character += incrementer;
            DrawMenu();
            break;
    }
}
void Mode_System_EncEdit(u8 encoder, s32 incrementer){
    
}
