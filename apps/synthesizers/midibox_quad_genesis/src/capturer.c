/*
 * MIDIbox Quad Genesis: Capturer dialog
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
#include "capturer.h"

#include "cmdeditor.h"
#include "frontpanel.h"
#include "interface.h"
#include "mode_chan.h"
#include "syeng.h"

static u8 cvoice;

static void DrawMenu(){
    FrontPanel_GenesisLEDSet((selchan>>4), 0, 1, 1);
    FrontPanel_GenesisLEDSet(((selchan>>2)&3), (selchan&3)+8, 1, 1);
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("Capture voice to port %d chn %d", (selchan>>4)+1, (selchan&0xF)+1);
}

void Capturer_Start(u8 origvoice){
    cvoice = origvoice;
    //Clear voice LEDs
    u8 g, v;
    for(g=0; g<GENESIS_COUNT; ++g){
        for(v=0; v<12; ++v){
            FrontPanel_GenesisLEDSet(g, v, 0, 0);
            FrontPanel_GenesisLEDSet(g, v, 1, 0);
        }
    }
    DrawMenu();
}

void Capturer_BtnSystem(u8 button, u8 state){
    if(!state) return;
    if(button == FP_B_MENU){
        subscreen = 0;
        return;
    }else if(button == FP_B_CAPTURE || button == FP_B_ENTER){
        //Delete old program
        channels[selchan].trackermode = 0;
        if(channels[selchan].program != NULL) SyEng_DeleteProgram(selchan);
        //Compute usage
        VgmUsageBits usage; //TODO
        //Create program
        synprogram_t* prog = vgmh2_malloc(sizeof(synprogram_t));
        channels[selchan].program = prog;
        prog->usage.all = usage.all;
        prog->initsource = NULL;
        prog->noteonsource = NULL;
        prog->noteoffsource = NULL;
        prog->rootnote = 60;
        sprintf(prog->name, "Captured");
        //Create init VGM
        //TODO
        //Create note-on VGM
        prog->noteonsource = CreateNewVGM(1, usage);
        //Create note-off VGM
        prog->noteoffsource = CreateNewVGM(2, usage);
        subscreen = 0;
        return;
    }
}

void Capturer_BtnGVoice(u8 gvoice, u8 state){
    //Draw old channel LED
    FrontPanel_GenesisLEDSet((selchan>>4), 0, 1, 0);
    FrontPanel_GenesisLEDSet(((selchan>>2)&3), (selchan&3)+8, 1, 0);
    //Change channels
    if((gvoice & 0xF) == 0){
        selchan = (selchan & 0xF) | (gvoice & 0xF0);
    }else if((gvoice & 0xF) >= 8){
        selchan = (selchan & 0xF0) | ((gvoice >> 2) & 0xC) | ((gvoice - 8) & 3);
    }
    DrawMenu();
}
