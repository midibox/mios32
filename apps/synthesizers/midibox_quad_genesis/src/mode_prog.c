/*
 * MIDIbox Quad Genesis: Program Mode
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
#include "mode_prog.h"

#include "cmdeditor.h"
#include "frontpanel.h"
#include "interface.h"
#include "syeng.h"
#include "mode_vgm.h"
#include "filebrowser.h"
#include "nameeditor.h"

synprogram_t* selprogram;

static u8 submode;
static u8 cursor;
static VgmUsageBits newvgmusage;


static void DrawMenu(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    if(selprogram == NULL){
        MIOS32_LCD_PrintString("No program selected to edit");
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintString("(Create or select program in Chan mode)");
        return;
    }
    switch(submode){
        case 0:
            MIOS32_LCD_PrintString(selprogram->name);
            MIOS32_LCD_CursorSet(15,0);
            MIOS32_LCD_PrintString("Root VGMs: Init KOn KOff ");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("Name  Vel");
            MIOS32_LCD_CursorSet(16,1);
            MIOS32_LCD_PrintFormattedString("%d", selprogram->rootnote);
            MIOS32_LCD_CursorSet(25,1);
            MIOS32_LCD_PrintString(selprogram->initsource != NULL ? " [#] " : " --- ");
            MIOS32_LCD_CursorSet(30,1);
            MIOS32_LCD_PrintString(selprogram->noteonsource != NULL ? " [#] " : " --- ");
            MIOS32_LCD_CursorSet(35,1);
            MIOS32_LCD_PrintString(selprogram->noteoffsource != NULL ? " [#] " : " --- ");
            MIOS32_LCD_CursorSet(5*cursor,1);
            MIOS32_LCD_PrintChar('~');
            break;
        case 1:
            MIOS32_LCD_PrintString("Create new ");
            switch(cursor-5){
                case 0: MIOS32_LCD_PrintString("Init"); break;
                case 1: MIOS32_LCD_PrintString("KOn"); break;
                case 2: MIOS32_LCD_PrintString("KOff"); break;
            }
            MIOS32_LCD_PrintString(" VGM:");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("Row1 usage, Row2 LFO");
            break;
        case 2:
            MIOS32_LCD_PrintString("Modify op TLs for vel=0 (vel=127:normal)");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("(Op 4 also does PSG voices)");
            u8 i;
            for(i=0; i<4; ++i){
                FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + i, 3, selprogram->tlbaseoff[i] >> 3);
            }
            break;
    }
}

static void FilebrowserDoneLoading(char* filename){
    FrontPanel_LEDSet(FP_LED_LOAD, 0);
    if(filename == NULL || cursor < 5 || cursor > 7){
        DrawMenu();
        return;
    }
    VgmSource** ss = SelSource(selprogram, cursor-5);
    if(*ss != NULL){
        DrawMenu();
        return;
    }
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Loading...");
    MIOS32_LCD_CursorSet(0,1);
    MIOS32_LCD_PrintString("(Streaming other VGMs should continue)");
    char* resultMsg = vgmh2_malloc(64);
    s32 ret = VGM_File_Load(filename, ss, resultMsg);
    if(ret < 0){
        DBG("Failed with status %d", ret);
        submode = 0;
        DrawMenu();
        MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintString(resultMsg);
    }else{
        //Load successful!
        SyEng_RecalcSourceAndProgramUsage(selprogram, NULL);
        Mode_Vgm_SelectVgm(*ss);
        Interface_ChangeToMode(MODE_VGM);
    }
    vgmh2_free(resultMsg);
}

static void FilebrowserDoneSaving(char* filename){
    FrontPanel_LEDSet(FP_LED_SAVE, 0);
    if(filename == NULL || cursor < 5 || cursor > 7){
        DrawMenu();
        return;
    }
    VgmSource** ss = SelSource(selprogram, cursor-5);
    if(*ss == NULL){
        DrawMenu();
        return;
    }
    s32 res = VGM_File_SaveRAM(*ss, filename);
    DrawMenu();
    MIOS32_LCD_CursorSet(0,0);
    if(res < 0){
        MIOS32_LCD_PrintFormattedString("Error %d saving", res);
    }else{
        MIOS32_LCD_PrintString("Saved       ");
    }
}

void Mode_Prog_Init(){
    submode = 0;
    cursor = 5;
}
void Mode_Prog_GotFocus(){
    if(submode != 1) submode = 0;
    DrawMenu();
    if(submode == 1) DrawUsageOnVoices(newvgmusage, 0);
}

void Mode_Prog_Tick(){

}
void Mode_Prog_Background(){

}

void Mode_Prog_BtnGVoice(u8 gvoice, u8 state){
    if(selprogram == NULL) return;
    if(!state) return;
    u8 g = gvoice >> 4, v = gvoice & 0x0F;
    switch(submode){
        case 1:
            if(g == 0){
                if(v == 0){
                    newvgmusage.opn2_globals ^= 1;
                }else if(v <= 6){
                    newvgmusage.all ^= (1 << (v-1));
                    newvgmusage.all &= 0xFFFFF03F | ((newvgmusage.all & 0x0000003F) << 6);
                    newvgmusage.lfomode = ((newvgmusage.all & 0x00000FC0) != 0);
                }else if(v == 7){
                    newvgmusage.dac ^= 1;
                }else if(v <= 0xB){
                    newvgmusage.all ^= (1 << (v+16));
                }
                DrawUsageOnVoices(newvgmusage, 0);
            }else if(g == 1){
                if(v >= 1 && v <= 6){
                    newvgmusage.all ^= (1 << (v+5));
                    newvgmusage.all &= 0xFFFFF03F | ((newvgmusage.all & 0x0000003F) << 6);
                    newvgmusage.lfomode = ((newvgmusage.all & 0x00000FC0) != 0);
                    DrawUsageOnVoices(newvgmusage, 0);
                }
            }
            break;
    }
}
void Mode_Prog_BtnSoftkey(u8 softkey, u8 state){
    if(selprogram == NULL) return;
    if(!state) return;
    switch(submode){
        case 0:
            if(softkey >= 5 || softkey == 3){
                cursor = softkey;
                DrawMenu();
            }else if(softkey == 0){
                NameEditor_Start(selprogram->name, 12, "Program", &DrawMenu);
            }else if(softkey == 1){
                submode = 2;
                DrawMenu();
            }
            break;
    }
}
void Mode_Prog_BtnSelOp(u8 op, u8 state){

}
void Mode_Prog_BtnOpMute(u8 op, u8 state){

}
void Mode_Prog_BtnSystem(u8 button, u8 state){
    if(selprogram == NULL) return;
    if(button == FP_B_MENU){
        if(!state) return;
        submode = 0;
        cursor = 5;
        DrawMenu();
        DrawUsageOnVoices((VgmUsageBits){.all=0}, 0);
        u8 i;
        for(i=0; i<4; ++i){
            FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + i, 0, 0);
        }
        FrontPanel_ClearDisplay(FP_LED_DIG_MAIN_1);
        return;
    }
    switch(submode){
        case 0:
            switch(button){
                case FP_B_ENTER:
                    if(!state) return;
                    if(cursor >= 5 && cursor <= 7){
                        VgmSource** ss = SelSource(selprogram, cursor-5);
                        Mode_Vgm_SelectVgm(*ss);
                        Interface_ChangeToMode(MODE_VGM);
                    }
                    break;
                case FP_B_LOAD:
                    if(!state) return;
                    if(cursor >= 5 && cursor <= 7){
                        VgmSource** ss = SelSource(selprogram, cursor-5);
                        if(*ss != NULL){
                            MIOS32_LCD_CursorSet(0,0);
                            MIOS32_LCD_PrintString("Delete VGM before loading a new one!");
                            return;
                        }
                        FrontPanel_LEDSet(FP_LED_LOAD, 1);
                        Filebrowser_Start(NULL, "VGM", 0, &FilebrowserDoneLoading);
                    }
                    break;
                case FP_B_SAVE:
                    if(!state) return;
                    if(cursor >= 5 && cursor <= 7){
                        VgmSource** ss = SelSource(selprogram, cursor-5);
                        if(*ss == NULL){
                            MIOS32_LCD_CursorSet(0,0);
                            MIOS32_LCD_PrintString("No VGM to save!");
                            return;
                        }
                        if((*ss)->type != VGM_SOURCE_TYPE_RAM){
                            MIOS32_LCD_CursorSet(0,0);
                            MIOS32_LCD_PrintString("Can't save streamed VGM!");
                            return;
                        }
                        FrontPanel_LEDSet(FP_LED_SAVE, 1);
                        Filebrowser_Start(NULL, "VGM", 1, &FilebrowserDoneSaving);
                    }
                    break;
                case FP_B_NEW:
                    if(!state) return;
                    if(cursor >= 5 && cursor <= 7){
                        VgmSource** ss = SelSource(selprogram, cursor-5);
                        if(*ss != NULL){
                            MIOS32_LCD_CursorSet(0,0);
                            MIOS32_LCD_PrintString("Delete VGM before creating a new one!");
                            return;
                        }
                        submode = 1;
                        newvgmusage.all = selprogram->usage.all;
                        DrawMenu();
                        DrawUsageOnVoices(newvgmusage, 0);
                    }
                    break;
                case FP_B_DELETE:
                    if(!state) return;
                    if(cursor >= 5 && cursor <= 7){
                        VgmSource** ss = SelSource(selprogram, cursor-5);
                        SyEng_DeleteSource(*ss);
                        *ss = NULL;
                        DrawMenu();
                    }
                    break;
                case FP_B_RELEASE:
                    if(state){
                        //Flush PIs of this program
                        SyEng_HardFlushProgram(selprogram);
                        FrontPanel_LEDSet(FP_LED_RELEASE, 1);
                        MIOS32_LCD_CursorSet(0,0);
                        MIOS32_LCD_PrintString("Flushed     ");
                    }else{
                        FrontPanel_LEDSet(FP_LED_RELEASE, 0);
                        DrawMenu();
                    }
            }
            break;
        case 1:
            switch(button){
                case FP_B_ENTER:
                    submode = 0;
                    VgmSource* vs = CreateNewVGM(cursor-5, newvgmusage);
                    VgmSource** ss = SelSource(selprogram, cursor-5);
                    *ss = vs;
                    SyEng_RecalcSourceAndProgramUsage(selprogram, vs);
                    Mode_Vgm_SelectVgm(*ss);
                    Interface_ChangeToMode(MODE_VGM);
                    break;
            }
            break;
    }
}
void Mode_Prog_BtnEdit(u8 button, u8 state){
    if(selprogram == NULL) return;
    if(!state) return;
    switch(submode){
        case 1:
            switch(button){
                case FP_B_CH3MODE:
                    newvgmusage.fm3_special ^= 1;
                    DrawUsageOnVoices(newvgmusage, 0);
                    break;
                case FP_B_NSFREQ:
                    newvgmusage.noisefreqsq3 ^= 1;
                    DrawUsageOnVoices(newvgmusage, 0);
                    break;
            }
            break;
    }
}

void Mode_Prog_EncDatawheel(s32 incrementer){
    if(selprogram == NULL) return;
    switch(submode){
        case 0:
            if(cursor == 3){
                if((incrementer < 0 && (s32)selprogram->rootnote + incrementer >= 0) || (incrementer > 0 && (s32)selprogram->rootnote + incrementer <= 0x7F)){
                    selprogram->rootnote = (u8)((s32)selprogram->rootnote + incrementer);
                    MIOS32_LCD_CursorSet(16,1);
                    MIOS32_LCD_PrintFormattedString("%d  ", selprogram->rootnote);
                }
            }
            break;
    }
}
void Mode_Prog_EncEdit(u8 encoder, s32 incrementer){
    switch(submode){
        case 2:
            if(encoder >= FP_E_OP1LVL && encoder <= FP_E_OP4LVL){
                u8 e = encoder - FP_E_OP1LVL;
                s16 val = selprogram->tlbaseoff[e];
                val -= incrementer;
                if(val < 0) val = 0;
                if(val > 0x7F) val = 0x7F;
                selprogram->tlbaseoff[e] = (u8)val;
                FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + e, 3, (u8)val >> 3);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, -val);
            }
            break;
    }
}
