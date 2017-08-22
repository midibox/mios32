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

#include "filebrowser.h"
#include "frontpanel.h"
#include "interface.h"
#include "syeng.h"
#include "mode_prog.h"
#include "mode_voice.h"
#include "mode_vgm.h"
#include "nameeditor.h"
#include <string.h>

u8 selchan;
static u8 submode;
static u8 cursor;

static const char* const vgmtypelabels[] = {
    "INIT",
    "KON",
    "KOFF"
};

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
                synprogram_t* prog = channels[selchan].program;
                if(prog == NULL){
                    MIOS32_LCD_PrintString("~Free      Trkr");
                    MIOS32_LCD_CursorSet(20,0);
                    MIOS32_LCD_PrintFormattedString("Prog: <none>");
                }else{
                    MIOS32_LCD_PrintString("~Free               Edit");
                    MIOS32_LCD_CursorSet(20,0);
                    MIOS32_LCD_PrintFormattedString("Prog: %s", prog->name);
                }
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

static void FilebrowserDoneLoading(char* filepath){
    
}

static void FilebrowserDoneSaving(char* filepath){
    if(filepath == NULL){
        DrawMenu();
        return;
    }
    synprogram_t* prog = channels[selchan].program;
    if(prog == NULL){
        DrawMenu();
        return;
    }
    //Get and extend path
    char* tempbuf = vgmh2_malloc(256);
    u8 sl = strlen(filepath);
    memcpy(tempbuf, filepath, sl);
    char* filenamestart = tempbuf + sl;
    *filenamestart = 0;
    while(*filenamestart != '.') --filenamestart;
    *filenamestart = '/';
    ++filenamestart;
    //Start program file
    if(FILE_FileExists(filepath)){
        FILE_Remove(filepath);
    }
    MUTEX_SDCARD_TAKE;
    FILE_WriteOpen(filepath, 1);
    FILE_WriteBuffer((u8*)"MBQG Program", 12);
    FILE_WriteWord(0);
    FILE_WriteByte(prog->rootnote);
    FILE_WriteBuffer((u8*)prog->name, 13);
    u8 v;
    VgmSource** ss;
    char* filenameend;
    for(v=0; v<3; ++v){
        ss = SelSource(prog, v);
        if(*ss == NULL){
            FILE_WriteByte(0);
            continue;
        }
        if((*ss)->type == VGM_SOURCE_TYPE_RAM){
            sl = strlen(vgmtypelabels[v]);
            memcpy(filenamestart, vgmtypelabels[v], sl);
            filenameend = filenamestart + sl;
            memcpy(filenameend, ".VGM", 4);
            filenameend += 4;
            *filenameend = 0;
            sl = filenameend - tempbuf;
            FILE_WriteByte(sl);
            FILE_WriteBuffer((u8*)tempbuf, sl);
        }else if((*ss)->type == VGM_SOURCE_TYPE_STREAM){
            VgmSourceStream* vss = (VgmSourceStream*)(*ss)->data;
            if(vss->filepath == NULL){
                DBG("Program saving error, unstarted stream!");
                FILE_WriteByte(0);
            }else{
                sl = strlen(vss->filepath);
                FILE_WriteByte(sl);
                FILE_WriteBuffer((u8*)vss->filepath, sl);
            }
        }else{
            DBG("Program saving error, unknown source type %d!", (*ss)->type);
            FILE_WriteByte(0);
        }
    }
    FILE_WriteClose();
    MUTEX_SDCARD_GIVE;
    vgmh2_free(tempbuf);
    DrawMenu();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Saved       ");
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
void EditProgram(){
    if(channels[selchan].program == NULL) return;
    selprogram = channels[selchan].program;
    Mode_Vgm_InvalidateVgm(NULL);
    Interface_ChangeToMode(MODE_PROG);
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
                    EditProgram();
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
                    if(channels[selchan].program != NULL){
                        MIOS32_LCD_CursorSet(0,0);
                        MIOS32_LCD_PrintFormattedString("Delete prog first!");
                    }else{
                        Filebrowser_Start(NULL, "PGM", 0, &FilebrowserDoneLoading);
                    }
                    break;
                case FP_B_SAVE:
                    if(channels[selchan].program == NULL){
                        MIOS32_LCD_CursorSet(0,0);
                        MIOS32_LCD_PrintFormattedString("No prog to save!");
                    }else{
                        Filebrowser_Start(NULL, "PGM", 1, &FilebrowserDoneSaving);
                    }
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
                        prog->tlbaseoffs = 0;
                        prog->name[0] = 'A';
                        prog->name[1] = 0;
                        selprogram = prog;
                        Mode_Vgm_InvalidateVgm(NULL);
                        NameEditor_Start(selprogram->name, 12, "New program", &NameEditorDone);
                    }
                    break;
                case FP_B_DELETE:
                    SyEng_DeleteProgram(selchan);
                    DrawMenu();
                    break;
                case FP_B_ENTER:
                    EditProgram();
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
