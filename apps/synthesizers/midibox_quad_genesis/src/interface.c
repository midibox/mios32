/*
 * MIDIbox Quad Genesis: Main Interface
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
#include "interface.h"

#include <genesis.h>
#include <blm_x.h>
#include <vgm.h>
#include "frontpanel.h"
#include "genesisstate.h"
#include "app.h"

#include "mode_system.h"
#include "mode_voice.h"
#include "mode_chan.h"
#include "mode_prog.h"
#include "mode_vgm.h"
#include "mode_mdltr.h"
#include "mode_sample.h"

#include "filebrowser.h"
#include "nameeditor.h"

u8 interfacemode;
u8 subscreen;
static s8 wantmodechange;


void Interface_Init(){
    Filebrowser_Init();
    Mode_System_Init();
    Mode_Voice_Init();
    Mode_Chan_Init();
    Mode_Prog_Init();
    Mode_Vgm_Init();
    Mode_Mdltr_Init();
    Mode_Sample_Init();
    interfacemode = MODE_SYSTEM;
    subscreen = 0;
    wantmodechange = -1;
    FrontPanel_LEDSet(FP_LED_SYSTEM, 1);
    Mode_System_GotFocus();
}
void Interface_Tick(){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_Tick(); break;
        case MODE_VOICE: Mode_Voice_Tick(); break;
        case MODE_CHAN: Mode_Chan_Tick(); break;
        case MODE_PROG: Mode_Prog_Tick(); break;
        case MODE_VGM: Mode_Vgm_Tick(); break;
        case MODE_MDLTR: Mode_Mdltr_Tick(); break;
        case MODE_SAMPLE: Mode_Sample_Tick(); break;
        default: DBG("Interface_Tick mode error %d!", interfacemode);
    }
}
void Interface_ChangeToMode(u8 ifmode){
    wantmodechange = ifmode;
}
void Interface_Background(){
    MIOS32_IRQ_Disable();
    if(wantmodechange >= 0){
        //Clear all Genesis state lights
        ClearGenesisState_Op();
        ClearGenesisState_Chan();
        ClearGenesisState_DAC();
        ClearGenesisState_OPN2();
        ClearGenesisState_PSG();
        //Clear all voice buttons
        u8 g, v;
        for(g=0; g<GENESIS_COUNT; ++g){
            for(v=0; v<12; ++v){
                FrontPanel_GenesisLEDSet(g, v, 0, 0);
                FrontPanel_GenesisLEDSet(g, v, 1, 0);
            }
        }
        //Clear all op buttons
        for(g=0; g<4; ++g){
            FrontPanel_LEDSet(FP_LED_SELOP_1 + g, 0);
        }
        //Clear all non-system buttons
        for(g=FP_LED_MUTE; g<=FP_LED_STATE; ++g){
            FrontPanel_LEDSet(g, 0);
        }
        //Clear VGM Matrix
        for(g=0; g<14; ++g){
            FrontPanel_VGMMatrixVUMeter(g, 0);
        }
        //Clear main LED display
        FrontPanel_DrawDigit(FP_LED_DIG_MAIN_1, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_MAIN_2, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_MAIN_3, ' ');
        FrontPanel_DrawDigit(FP_LED_DIG_MAIN_4, ' ');
        //Turn off the old mode light
        FrontPanel_LEDSet(FP_LED_SYSTEM + interfacemode - MODE_SYSTEM, 0);
        //Change modes
        interfacemode = wantmodechange;
        VGM_Player_docapture = (interfacemode == MODE_VOICE);
        //Turn on the new mode light
        FrontPanel_LEDSet(FP_LED_SYSTEM + interfacemode - MODE_SYSTEM, 1);
        switch(interfacemode){
            case MODE_SYSTEM: Mode_System_GotFocus(); break;
            case MODE_VOICE: Mode_Voice_GotFocus(); break;
            case MODE_CHAN: Mode_Chan_GotFocus(); break;
            case MODE_PROG: Mode_Prog_GotFocus(); break;
            case MODE_VGM: Mode_Vgm_GotFocus(); break;
            case MODE_MDLTR: Mode_Mdltr_GotFocus(); break;
            case MODE_SAMPLE: Mode_Sample_GotFocus(); break;
            default: DBG("Mode Change mode error %d!", interfacemode);
        }
        wantmodechange = -1;
    }
    MIOS32_IRQ_Enable();
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_Background(); break;
        case MODE_VOICE: Mode_Voice_Background(); break;
        case MODE_CHAN: Mode_Chan_Background(); break;
        case MODE_PROG: Mode_Prog_Background(); break;
        case MODE_VGM: Mode_Vgm_Background(); break;
        case MODE_MDLTR: Mode_Mdltr_Background(); break;
        case MODE_SAMPLE: Mode_Sample_Background(); break;
        default: DBG("Interface_Background mode error %d!", interfacemode);
    }
}

void Interface_BtnGVoice(u8 gvoice, u8 state){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_BtnGVoice(gvoice, state); break;
        case MODE_VOICE: Mode_Voice_BtnGVoice(gvoice, state); break;
        case MODE_CHAN: Mode_Chan_BtnGVoice(gvoice, state); break;
        case MODE_PROG: Mode_Prog_BtnGVoice(gvoice, state); break;
        case MODE_VGM: Mode_Vgm_BtnGVoice(gvoice, state); break;
        case MODE_MDLTR: Mode_Mdltr_BtnGVoice(gvoice, state); break;
        case MODE_SAMPLE: Mode_Sample_BtnGVoice(gvoice, state); break;
        default: DBG("Interface_BtnGVoice mode error %d!", interfacemode);
    }
}
void Interface_BtnSoftkey(u8 softkey, u8 state){
    switch(subscreen){
        case SUBSCREEN_FILEBROWSER:
            Filebrowser_BtnSoftkey(softkey, state);
            return;
        case SUBSCREEN_NAMEEDITOR:
            NameEditor_BtnSoftkey(softkey, state);
            return;
    }
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_BtnSoftkey(softkey, state); break;
        case MODE_VOICE: Mode_Voice_BtnSoftkey(softkey, state); break;
        case MODE_CHAN: Mode_Chan_BtnSoftkey(softkey, state); break;
        case MODE_PROG: Mode_Prog_BtnSoftkey(softkey, state); break;
        case MODE_VGM: Mode_Vgm_BtnSoftkey(softkey, state); break;
        case MODE_MDLTR: Mode_Mdltr_BtnSoftkey(softkey, state); break;
        case MODE_SAMPLE: Mode_Sample_BtnSoftkey(softkey, state); break;
        default: DBG("Interface_BtnSoftkey mode error %d!", interfacemode);
    }
}
void Interface_BtnSelOp(u8 op, u8 state){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_BtnSelOp(op, state); break;
        case MODE_VOICE: Mode_Voice_BtnSelOp(op, state); break;
        case MODE_CHAN: Mode_Chan_BtnSelOp(op, state); break;
        case MODE_PROG: Mode_Prog_BtnSelOp(op, state); break;
        case MODE_VGM: Mode_Vgm_BtnSelOp(op, state); break;
        case MODE_MDLTR: Mode_Mdltr_BtnSelOp(op, state); break;
        case MODE_SAMPLE: Mode_Sample_BtnSelOp(op, state); break;
        default: DBG("Interface_BtnSelOp mode error %d!", interfacemode);
    }
}
void Interface_BtnOpMute(u8 op, u8 state){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_BtnOpMute(op, state); break;
        case MODE_VOICE: Mode_Voice_BtnOpMute(op, state); break;
        case MODE_CHAN: Mode_Chan_BtnOpMute(op, state); break;
        case MODE_PROG: Mode_Prog_BtnOpMute(op, state); break;
        case MODE_VGM: Mode_Vgm_BtnOpMute(op, state); break;
        case MODE_MDLTR: Mode_Mdltr_BtnOpMute(op, state); break;
        case MODE_SAMPLE: Mode_Sample_BtnOpMute(op, state); break;
        default: DBG("Interface_BtnOpMute mode error %d!", interfacemode);
    }
}
void Interface_BtnSystem(u8 button, u8 state){
    switch(subscreen){
        case SUBSCREEN_FILEBROWSER:
            Filebrowser_BtnSystem(button, state);
            return;
        case SUBSCREEN_NAMEEDITOR:
            NameEditor_BtnSystem(button, state);
            return;
    }
    if(button >= FP_B_SYSTEM && button <= FP_B_SAMPLE){
        if(state){
            wantmodechange = button - FP_B_SYSTEM;
        }
    }else{
        switch(interfacemode){
            case MODE_SYSTEM: Mode_System_BtnSystem(button, state); break;
            case MODE_VOICE: Mode_Voice_BtnSystem(button, state); break;
            case MODE_CHAN: Mode_Chan_BtnSystem(button, state); break;
            case MODE_PROG: Mode_Prog_BtnSystem(button, state); break;
            case MODE_VGM: Mode_Vgm_BtnSystem(button, state); break;
            case MODE_MDLTR: Mode_Mdltr_BtnSystem(button, state); break;
            case MODE_SAMPLE: Mode_Sample_BtnSystem(button, state); break;
            default: DBG("Interface_BtnSystem mode error %d!", interfacemode);
        }
    }
}
void Interface_BtnEdit(u8 button, u8 state){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_BtnEdit(button, state); break;
        case MODE_VOICE: Mode_Voice_BtnEdit(button, state); break;
        case MODE_CHAN: Mode_Chan_BtnEdit(button, state); break;
        case MODE_PROG: Mode_Prog_BtnEdit(button, state); break;
        case MODE_VGM: Mode_Vgm_BtnEdit(button, state); break;
        case MODE_MDLTR: Mode_Mdltr_BtnEdit(button, state); break;
        case MODE_SAMPLE: Mode_Sample_BtnEdit(button, state); break;
        default: DBG("Interface_BtnEdit mode error %d!", interfacemode);
    }
}

void Interface_EncDatawheel(s32 incrementer){
    switch(subscreen){
        case SUBSCREEN_FILEBROWSER:
            Filebrowser_EncDatawheel(incrementer);
            return;
        case SUBSCREEN_NAMEEDITOR:
            NameEditor_EncDatawheel(incrementer);
            return;
    }
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_EncDatawheel(incrementer); break;
        case MODE_VOICE: Mode_Voice_EncDatawheel(incrementer); break;
        case MODE_CHAN: Mode_Chan_EncDatawheel(incrementer); break;
        case MODE_PROG: Mode_Prog_EncDatawheel(incrementer); break;
        case MODE_VGM: Mode_Vgm_EncDatawheel(incrementer); break;
        case MODE_MDLTR: Mode_Mdltr_EncDatawheel(incrementer); break;
        case MODE_SAMPLE: Mode_Sample_EncDatawheel(incrementer); break;
        default: DBG("Interface_EncDatawheel mode error %d!", interfacemode);
    }
}
void Interface_EncEdit(u8 encoder, s32 incrementer){
    switch(interfacemode){
        case MODE_SYSTEM: Mode_System_EncEdit(encoder, incrementer); break;
        case MODE_VOICE: Mode_Voice_EncEdit(encoder, incrementer); break;
        case MODE_CHAN: Mode_Chan_EncEdit(encoder, incrementer); break;
        case MODE_PROG: Mode_Prog_EncEdit(encoder, incrementer); break;
        case MODE_VGM: Mode_Vgm_EncEdit(encoder, incrementer); break;
        case MODE_MDLTR: Mode_Mdltr_EncEdit(encoder, incrementer); break;
        case MODE_SAMPLE: Mode_Sample_EncEdit(encoder, incrementer); break;
        default: DBG("Interface_EncEdit mode error %d!", interfacemode);
    }
}

