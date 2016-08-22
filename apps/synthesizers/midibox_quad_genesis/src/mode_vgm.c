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
#include "genesisstate.h"
#include "cmdeditor.h"
#include "mode_prog.h"

static VgmSource* selvgm;
static u8 vgmpreviewpi;
static u8 submode;
static u8 playing;
static u8 statemode;
static u8 selvoice;
static u8 selop;
static u16 vmutes;
static u16 vsolos;
static VgmChipWriteCmd lastcmddrawn;


void Mode_Vgm_SelectVgm(VgmSource* newselvgm){
    if(vgmpreviewpi < MBQG_NUM_PROGINSTANCES){
        SyEng_ReleaseStaticPI(vgmpreviewpi);
        playing = 0;
    }
    selvgm = newselvgm;
    vgmpreviewpi = SyEng_GetStaticPI(selprogram->usage);
    synproginstance_t* pi = &proginstances[vgmpreviewpi];
    SyEng_PlayVGMOnPI(pi, selvgm, 60, 0);
    vmutes = selvgm->mutes;
    vsolos = 0;
}
void Mode_Vgm_InvalidateVgm(VgmSource* maybeselvgm){
    if(maybeselvgm == NULL || maybeselvgm == selvgm){
        if(vgmpreviewpi < MBQG_NUM_PROGINSTANCES){
            SyEng_ReleaseStaticPI(vgmpreviewpi);
            playing = 0;
        }
        selvgm = NULL;
        vgmpreviewpi = 0xFF;
    }
}
void Mode_Vgm_InvalidatePI(synproginstance_t* maybestaticpi){
    if(vgmpreviewpi < MBQG_NUM_PROGINSTANCES && &proginstances[vgmpreviewpi] == maybestaticpi){
        vgmpreviewpi = 0xFF;
        playing = 0;
    }
}
static void EnsurePreviewPiOK(){
    if(vgmpreviewpi >= MBQG_NUM_PROGINSTANCES){
        vgmpreviewpi = SyEng_GetStaticPI(selprogram->usage);
        synproginstance_t* pi = &proginstances[vgmpreviewpi];
        SyEng_PlayVGMOnPI(pi, selvgm, 60, playing);
    }
}

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
            MIOS32_LCD_CursorSet(36,0);
            MIOS32_LCD_PrintString("To");
            MIOS32_LCD_CursorSet(35,1);
            MIOS32_LCD_PrintString("Solo");
            break;
        case 2:
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("Solo voices");
            MIOS32_LCD_CursorSet(36,0);
            MIOS32_LCD_PrintString("To");
            MIOS32_LCD_CursorSet(35,1);
            MIOS32_LCD_PrintString("Mute");
            break;
    }
}
static void DrawMuteStates(u8 solo){
    u8 i;
    u16 d = solo ? vsolos : vmutes;
    FrontPanel_LEDSet(FP_LED_RELEASE, d > 0);
    for(i=0; i<12; ++i){
        FrontPanel_GenesisLEDSet(0, i, 0, d & 1);
        d >>= 1;
    }
}
static void SetMuteState(u8 voice, u8 state){
    state &= 1;
    if(((selvgm->mutes >> voice) & 1) == state) return;
    selvgm->mutes ^= (1 << voice);
    MIOS32_IRQ_Disable();
    u8 i;
    VgmHead* head;
    for(i=0; i<vgm_numheads; ++i){
        head = vgm_heads[i];
        if(head != NULL && head->source == selvgm){
            VgmHead_Channel* c = &head->channel[voice];
            if(c->nodata) continue;
            c->mute = state;
            if(state){
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
    }
    MIOS32_IRQ_Enable();
}
static void RecalculateMuteStates(){
    u16 should = (vsolos > 0) ? ~vsolos : vmutes;
    should &= 0x0FFF;
    if((selvgm->mutes ^ should) == 0) return; //No change
    u8 i, s;
    u16 d = selvgm->mutes;
    for(i=0; i<12; ++i){
        s = should & 1;
        if(s ^ (d&1)){
            SetMuteState(i, s);
        }
        d >>= 1;
        should >>= 1;
    }
}

void Mode_Vgm_Init(){
    playing = 0;
    vgmpreviewpi = 0xFF;
    statemode = 1;
    vmutes = 0;
    vsolos = 0;
}
void Mode_Vgm_GotFocus(){
    submode = 0;
    FrontPanel_LEDSet(FP_LED_PLAY, playing);
    FrontPanel_LEDSet(FP_LED_CMDS, !statemode);
    FrontPanel_LEDSet(FP_LED_STATE, statemode);
    if(statemode || submode == 3){
        //Turn on our voice button
        FrontPanel_GenesisLEDSet(0, selvoice, 1, 1);
        //Op button
        if(selvoice <= 6 && selvoice >= 1){
            FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 1);
        }
    }
    DrawMenu();
}

void Mode_Vgm_Tick(){

}
static u8 GetRealMapVoice(u8 v, u8 mv){
    if(v == 0){
        return 0;
    }else if(v <= 6){
        return mv + 1;
    }else if(v == 7){
        return 7;
    }else if(v <= 0xA){
        return mv + 8;
    }else{
        return 0xB;
    }
}
void Mode_Vgm_Background(){
    static u8 lastselvoice = 0, lastselop = 0, laststatemode = 1;
    if(selvgm == NULL) return;
    if(vgmpreviewpi >= MBQG_NUM_PROGINSTANCES) return;
    u8 v;
    if(statemode || submode == 3){
        //Transition displayed voice and op states
        if(lastselop != selop || lastselvoice != selvoice || laststatemode != statemode){
            ClearGenesisState_Op();
            FrontPanel_LEDSet(FP_LED_SELOP_1 + lastselop, 0);
            if(selvoice <= 6 && selvoice >= 1){
                FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 1);
            }
        }
        lastselop = selop;
        if(lastselvoice != selvoice || laststatemode != statemode){
            ClearGenesisState_Chan();
            ClearGenesisState_DAC();
            ClearGenesisState_OPN2();
            ClearGenesisState_PSG();
            FrontPanel_GenesisLEDSet(0, lastselvoice, 1, 0);
            FrontPanel_GenesisLEDSet(0, selvoice, 1, 1);
        }
        lastselvoice = selvoice;
        if(laststatemode != statemode){
            for(v=0; v<12; ++v){
                FrontPanel_GenesisLEDSet(0, v, 0, 0);
            }
            for(v=0; v<7; ++v){
                FrontPanel_VGMMatrixRow(v, 0);
            }
        }
        laststatemode = statemode;
    }else{
        if(laststatemode != statemode){
            ClearGenesisState_Op();
            ClearGenesisState_Chan();
            ClearGenesisState_DAC();
            ClearGenesisState_OPN2();
            ClearGenesisState_PSG();
            FrontPanel_LEDSet(FP_LED_SELOP_1 + selop, 0);
            FrontPanel_GenesisLEDSet(0, selvoice, 1, 0);
        }
        laststatemode = statemode;
    }
    if(statemode){
        VGM_Player_docapture = 1;
        synproginstance_t* pi = &proginstances[vgmpreviewpi];
        u8 mg, mv;
        if(submode == 0){
            //Activity lights
            for(v=0; v<12; ++v){
                mg = pi->mapping[v].map_chip;
                mv = GetRealMapVoice(v, pi->mapping[v].map_voice);
                DrawGenesisActivity(mg, mv, 0, v);
            }
        }
        //Draw voice state
        mg = pi->mapping[selvoice].map_chip;
        mv = GetRealMapVoice(selvoice, pi->mapping[selvoice].map_voice);
        DrawGenesisState_All(mg, mv, selop);
        //Draw chan VU meters
        for(v=1; v<7; ++v){
            mg = pi->mapping[v].map_chip;
            mv = pi->mapping[v].map_voice;
            DrawChanVUMeter(mg, mv, v-1);
        }
        for(v=8; v<12; ++v){
            mg = pi->mapping[v].map_chip;
            mv = GetRealMapVoice(v, pi->mapping[v].map_voice);
            FrontPanel_VGMMatrixVUMeter(v+2, (15 - genesis[mg].psg.voice[mv-8].atten) >> 1);
        }
        DrawDACVUMeter(pi->mapping[7].map_chip);
    }else{
        VGM_Player_docapture = 0;
        if(selvgm->type == VGM_SOURCE_TYPE_RAM){
            EnsurePreviewPiOK();
            VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
            VgmSourceRAM* vsr = (VgmSourceRAM*)selvgm->data;
            s32 a = vhr->srcaddr - 3;
            u8 r;
            for(r=0; r<7; ++r){
                if(a < 0 || a >= vsr->numcmds){
                    FrontPanel_VGMMatrixRow(r, 0);
                }else{
                    DrawCmdLine(vsr->cmds[a], r);
                }
                ++a;
            }
            a = vhr->srcaddr;
            if(a < 0 || a >= vsr->numcmds){
                ClearGenesisState_Op();
                ClearGenesisState_Chan();
                ClearGenesisState_DAC();
                ClearGenesisState_OPN2();
                ClearGenesisState_PSG();
                lastcmddrawn.all = 0;
            }else{
                VgmChipWriteCmd newcmd = vsr->cmds[a];
                if(newcmd.all != lastcmddrawn.all){
                    MIOS32_IRQ_Disable();
                    if(lastcmddrawn.all != 0){
                        DrawCmdContent(lastcmddrawn, 1);
                    }
                    DrawCmdContent(newcmd, 0);
                    lastcmddrawn = newcmd;
                    MIOS32_IRQ_Enable();
                    if(!playing){
                        char* buf = vgmh2_malloc(36);
                        GetCmdDescription(newcmd, buf);
                        for(r=0; r<35; ++r) if(buf[r] == 0) break;
                        for(; r<35; ++r) buf[r] = ' ';
                        buf[35] = 0;
                        MIOS32_LCD_CursorSet(5,0);
                        MIOS32_LCD_PrintString(buf);
                        vgmh2_free(buf);
                    }
                }
            }
        }
    }
}

void Mode_Vgm_BtnGVoice(u8 gvoice, u8 state){
    if(selvgm == NULL) return;
    if(gvoice >= 0x10) return;
    gvoice &= 0xF;
    switch(submode){
        case 0:
            if(!state) return;
            selvoice = gvoice;
            break;
        case 1:
            if(!state) return;
            vmutes ^= 1 << gvoice;
            RecalculateMuteStates();
            DrawMuteStates(0);
            break;
        case 2:
            if(!state) return;
            vsolos ^= 1 << gvoice;
            RecalculateMuteStates();
            DrawMuteStates(1);
            break;
    }
}
void Mode_Vgm_BtnSoftkey(u8 softkey, u8 state){
    if(selvgm == NULL) return;
    switch(submode){
        case 1:
            if(softkey != 7) return;
            if(!state) return;
            vsolos = ~vmutes & 0x0FFF;
            vmutes = 0;
            RecalculateMuteStates();
            DrawMuteStates(0);
            break;
        case 2:
            if(softkey != 7) return;
            if(!state) return;
            vmutes = ~vsolos & 0x0FFF;
            vsolos = 0;
            RecalculateMuteStates();
            DrawMuteStates(1);
            break;
    }
}
void Mode_Vgm_BtnSelOp(u8 op, u8 state){
    if(selvgm == NULL) return;
    if(submode != 0) return;
    if(!state) return;
    selop = op;
}
void Mode_Vgm_BtnOpMute(u8 op, u8 state){

}
void Mode_Vgm_BtnSystem(u8 button, u8 state){
    if(selvgm == NULL) return;
    switch(button){
        case FP_B_MENU:
            submode = 0;
            DrawMenu();
            return;
        case FP_B_PLAY:
            if(!state) return;
            if(selprogram == NULL) return;
            EnsurePreviewPiOK();
            if(playing){
                FrontPanel_LEDSet(FP_LED_PLAY, 0);
                synproginstance_t* pi = &proginstances[vgmpreviewpi];
                pi->head->playing = 0;
                SyEng_SilencePI(pi);
                playing = 0;
            }else{
                FrontPanel_LEDSet(FP_LED_PLAY, 1);
                synproginstance_t* pi = &proginstances[vgmpreviewpi];
                pi->head->ticks = VGM_Player_GetVGMTime();
                pi->head->playing = 1;
                playing = 1;
            }
            return;
        case FP_B_RESTART:
            if(!state) return;
            if(selprogram == NULL) return;
            EnsurePreviewPiOK();
            synproginstance_t* pi = &proginstances[vgmpreviewpi];
            VGM_Head_Restart(pi->head, VGM_Player_GetVGMTime());
            break;
    }
    switch(submode){
        case 0:
            switch(button){
                case FP_B_MUTE:
                    if(!state) return;
                    FrontPanel_LEDSet(FP_LED_MUTE, 1);
                    submode = 1;
                    DrawMenu();
                    DrawMuteStates(0);
                    break;
                case FP_B_SOLO:
                    if(!state) return;
                    FrontPanel_LEDSet(FP_LED_SOLO, 1);
                    submode = 2;
                    DrawMenu();
                    DrawMuteStates(1);
                    break;
                case FP_B_CMDS:
                    if(!state) return;
                    if(!statemode) return;
                    statemode = 0;
                    FrontPanel_LEDSet(FP_LED_CMDS, 1);
                    FrontPanel_LEDSet(FP_LED_STATE, 0);
                    break;
                case FP_B_STATE:
                    if(!state) return;
                    if(statemode) return;
                    statemode = 1;
                    FrontPanel_LEDSet(FP_LED_CMDS, 0);
                    FrontPanel_LEDSet(FP_LED_STATE, 1);
                    break;
                case FP_B_CMDUP:
                    if(!state) return;
                    if(selvgm->type == VGM_SOURCE_TYPE_RAM){
                        EnsurePreviewPiOK();
                        VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
                        if(vhr->srcaddr > 0) --vhr->srcaddr;
                    }
                    break;
                case FP_B_CMDDN:
                    if(!state) return;
                    if(selvgm->type == VGM_SOURCE_TYPE_RAM){
                        EnsurePreviewPiOK();
                        VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
                        VgmSourceRAM* vsr = (VgmSourceRAM*)selvgm->data;
                        if(vhr->srcaddr < vsr->numcmds-1) ++vhr->srcaddr;
                    }
                    break;
            }
            break;
        case 1:
            switch(button){
                case FP_B_MUTE:
                    if(state) return;
                    FrontPanel_LEDSet(FP_LED_MUTE, 0);
                    FrontPanel_LEDSet(FP_LED_RELEASE, 0);
                    submode = 0;
                    DrawMenu();
                    break;
                case FP_B_SOLO:
                    if(!state) return;
                    FrontPanel_LEDSet(FP_LED_MUTE, 0);
                    FrontPanel_LEDSet(FP_LED_SOLO, 1);
                    submode = 2;
                    DrawMenu();
                    DrawMuteStates(1);
                    break;
                case FP_B_RELEASE:
                    if(!state) return;
                    vmutes = 0;
                    RecalculateMuteStates();
                    DrawMuteStates(0);
            }
            break;
        case 2:
            switch(button){
                case FP_B_MUTE:
                    if(!state) return;
                    FrontPanel_LEDSet(FP_LED_SOLO, 0);
                    FrontPanel_LEDSet(FP_LED_MUTE, 1);
                    submode = 1;
                    DrawMenu();
                    DrawMuteStates(0);
                    break;
                case FP_B_SOLO:
                    if(state) return;
                    FrontPanel_LEDSet(FP_LED_SOLO, 0);
                    FrontPanel_LEDSet(FP_LED_RELEASE, 0);
                    submode = 0;
                    DrawMenu();
                    break;
                case FP_B_RELEASE:
                    if(!state) return;
                    vsolos = 0;
                    RecalculateMuteStates();
                    DrawMuteStates(0);
            }
            break;
    }
}
void Mode_Vgm_BtnEdit(u8 button, u8 state){
    if(selvgm == NULL) return;
    if(selvgm->type == VGM_SOURCE_TYPE_RAM){
        if(!state) return;
        EnsurePreviewPiOK();
        VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
        VgmSourceRAM* vsr = (VgmSourceRAM*)selvgm->data;
        s32 a = vhr->srcaddr;
        if(a < 0 || a >= vsr->numcmds) return;
        vsr->cmds[a] = EditCmd(vsr->cmds[a], 0xFF, 0, button, state); //TODO handle key on and algorithm
    }
}

void Mode_Vgm_EncDatawheel(s32 incrementer){
    if(selvgm == NULL) return;
    if(selvgm->type == VGM_SOURCE_TYPE_RAM){
        EnsurePreviewPiOK();
        VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
        VgmSourceRAM* vsr = (VgmSourceRAM*)selvgm->data;
        s32 a = vhr->srcaddr;
        if(a < 0 || a >= vsr->numcmds) return;
        vsr->cmds[a] = EditCmd(vsr->cmds[a], FP_E_DATAWHEEL, incrementer, 0xFF, 0);
    }
}
void Mode_Vgm_EncEdit(u8 encoder, s32 incrementer){
    if(selvgm == NULL) return;
    if(selvgm->type == VGM_SOURCE_TYPE_RAM){
        EnsurePreviewPiOK();
        VgmHeadRAM* vhr = (VgmHeadRAM*)proginstances[vgmpreviewpi].head->data;
        VgmSourceRAM* vsr = (VgmSourceRAM*)selvgm->data;
        s32 a = vhr->srcaddr;
        if(a < 0 || a >= vsr->numcmds) return;
        vsr->cmds[a] = EditCmd(vsr->cmds[a], encoder, incrementer, 0xFF, 0);
    }
}

