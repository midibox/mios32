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

#include "frontpanel.h"
#include "interface.h"
#include "syeng.h"
#include "mode_prog.h"
#include "mode_voice.h"
#include "mode_vgm.h"
#include "nameeditor.h"

static u8 selchan;
static u8 submode;
static u8 cursor;

static void DrawMenu(){
    switch(submode){
        case 0:
            FrontPanel_LEDSet(FP_LED_NEW, 0);
            FrontPanel_LEDSet(FP_LED_DELETE, 0);
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Prt%d:Ch%2d", (selchan >> 4)+1, (selchan & 0xF)+1);
            MIOS32_LCD_CursorSet(0,1);
            if(channels[selchan].trackermode){
                u8 v = channels[selchan].trackervoice;
                MIOS32_LCD_PrintFormattedString(" Free     ~Trkr G%d:%s", (v >> 4)+1, GetVoiceName(v & 0xF));
            }else{
                MIOS32_LCD_PrintString("~Free      Trkr     Edit");
                synprogram_t* prog = channels[selchan].program;
                MIOS32_LCD_CursorSet(20,0);
                MIOS32_LCD_PrintFormattedString("Prog: %s", prog == NULL ? "<none>" : prog->name);
            }
            break;
        case 1:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Select voice to control with Prt%d:Ch%2d", (selchan >> 4)+1, (selchan & 0xF)+1);
            break;
        case 2:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Which operator? (Ch3 4Freq/CSM)");
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("  1    2    3  4/All");
            break;
        default:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Chan invalid submode %d!", submode);
    }
}

void NameEditorDone(){
    Interface_ChangeToMode(MODE_PROG);
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
    switch(submode){
        case 0:
            if(v == 0){
                FrontPanel_GenesisLEDSet(t >> 4, (t & 0xF) < 0xC ? (t & 0xF) : 3 , 0, 0);
                FrontPanel_GenesisLEDSet((selchan >> 4), 0, 1, 0);
                selchan = (g << 4) | (selchan & 0xF);
                FrontPanel_GenesisLEDSet(g, 0, 1, 1);
                if(channels[selchan].trackermode){
                    t = channels[selchan].trackervoice;
                    FrontPanel_GenesisLEDSet(t >> 4, (t & 0xF) < 0xC ? (t & 0xF) : 3, 0, 1);
                }
                DrawMenu();
            }else if(v >= 8 && v <= 11){
                v -= 8;
                FrontPanel_GenesisLEDSet(t >> 4, (t & 0xF) < 0xC ? (t & 0xF) : 3, 0, 0);
                FrontPanel_GenesisLEDSet((selchan >> 2) & 3, (selchan & 3)+8, 1, 0);
                selchan = (selchan & 0xF0) | (g << 2) | v;
                FrontPanel_GenesisLEDSet(g, v+8, 1, 1);
                if(channels[selchan].trackermode){
                    t = channels[selchan].trackervoice;
                    FrontPanel_GenesisLEDSet(t >> 4, (t & 0xF) < 0xC ? (t & 0xF) : 3, 0, 1);
                }
                DrawMenu();
            }
            break;
        case 1:
            if(v == 3 && syngenesis[g].channels[3].use == 3){
                cursor = g;
                submode = 2;
            }else{
                SyEng_ClearVoice(g, v);
                syngenesis[g].channels[v].use = 3;
                channels[selchan].trackervoice = (g << 4) | v;
                channels[selchan].trackermode = 1;
                FrontPanel_GenesisLEDSet(g, v, 0, 1);
                submode = 0;
            }
            DrawMenu();
            break;
    }
}
void Mode_Chan_BtnSoftkey(u8 softkey, u8 state){
    if(!state) return;
    switch(submode){
        case 0:
            switch(softkey){
                case 0:
                    if(!channels[selchan].trackermode) return;
                    channels[selchan].trackermode = 0;
                    u8 v = channels[selchan].trackervoice;
                    u8 g = v >> 4;
                    v &= 0xF;
                    u8 c, t;
                    if(v == 3){
                        //Is there any other channel controlling Ch3 main?
                        for(c=0; c<16*MBQG_NUM_PORTS; ++c){
                            if(channels[c].trackermode && channels[c].trackervoice == 3){
                                break;
                            }
                        }
                        if(c == 16*MBQG_NUM_PORTS){ //there was none
                            syngenesis[g].channels[3].use = 0;
                            //Also clear trackermode from voices controlling Ch3 operator frequencies
                            for(c=0; c<16*MBQG_NUM_PORTS; ++c){
                                if(channels[c].trackermode){
                                    t = channels[c].trackervoice;
                                    if((t >> 4) == g && (t & 0xF) >= 0xC){
                                        channels[c].trackermode = 0;
                                    }
                                }
                            }
                        }
                    }else if(v >= 0xC){
                        //don't clear use
                        v = 3; //turn off ch3 LED
                    }else{
                        //Is there any other channel controlling Ch3 main?
                        for(c=0; c<16*MBQG_NUM_PORTS; ++c){
                            if(channels[c].trackermode && channels[c].trackervoice == v){
                                break;
                            }
                        }
                        if(c == 16*MBQG_NUM_PORTS){ //there was none
                            syngenesis[g].channels[v].use = 0;
                        }
                    }
                    FrontPanel_GenesisLEDSet(g, v, 0, 0);
                    DrawMenu();
                    break;
                case 2:
                    if(channels[selchan].trackermode) return;
                    submode = 1;
                    DrawMenu();
                    break;
                case 4:
                    selprogram = channels[selchan].program;
                    Mode_Vgm_InvalidateVgm(NULL);
                    Interface_ChangeToMode(MODE_PROG);
                    break;
            }
            break;
        case 2:
            if(softkey == 3){
                SyEng_ClearVoice(cursor, 3);
                syngenesis[cursor].channels[3].use = 3;
                channels[selchan].trackervoice = (cursor << 4) | 3;
            }else if(softkey <= 2){
                channels[selchan].trackervoice = (cursor << 4) | (0xC + softkey);
            }else{
                return;
            }
            channels[selchan].trackermode = 1;
            FrontPanel_GenesisLEDSet(cursor, 3, 0, 1);
            submode = 0;
            DrawMenu();
            break;
    }
}
void Mode_Chan_BtnSelOp(u8 op, u8 state){

}
void Mode_Chan_BtnOpMute(u8 op, u8 state){

}
void Mode_Chan_BtnSystem(u8 button, u8 state){
    switch(subscreen){
        case SUBSCREEN_NAMEEDITOR:
            NameEditor_BtnSystem(button, state);
            return;
    }
    if(!state) return;
    if(button == FP_B_MENU){
        submode = 0;
        DrawMenu();
        return;
    }
    switch(submode){
        case 0:
            switch(button){
                case FP_B_LOAD:
                    //TODO
                    break;
                case FP_B_SAVE:
                    //TODO
                    break;
                case FP_B_NEW:
                    if(channels[selchan].program != NULL){
                        MIOS32_LCD_CursorSet(0,0);
                        MIOS32_LCD_PrintFormattedString("Delete prog first!");
                    }else{
                        synprogram_t* prog = vgmh2_malloc(sizeof(synprogram_t));
                        channels[selchan].program = prog;
                        prog->usage.all = 0;
                        prog->initsource = NULL;
                        prog->noteonsource = NULL;
                        prog->noteoffsource = NULL;
                        prog->rootnote = 60;
                        prog->name[0] = 'A';
                        prog->name[1] = 0;
                        selprogram = prog;
                        Mode_Vgm_InvalidateVgm(NULL);
                        NameEditor_Start(selprogram->name, 12, "New program", &NameEditorDone);
                    }
                    break;
                case FP_B_DELETE:
                    //TODO
                    break;
            }
            break;
    }
}
void Mode_Chan_BtnEdit(u8 button, u8 state){
    
}

void Mode_Chan_EncDatawheel(s32 incrementer){
    
}
void Mode_Chan_EncEdit(u8 encoder, s32 incrementer){

}
