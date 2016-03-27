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
#include "frontpanel.h"
#include "app.h"


void Interface_Init(){

}


void Interface_BtnGVoice(u8 gvoice, u8 state){
    DBG("Genesis Voice button %x state %d", gvoice, state);
}
void Interface_BtnSoftkey(u8 softkey, u8 state){
    //TODO
    DBG("Softkey %x state %d", softkey, state);
    if(state){
        if(softkey < 4){
            selgenesis = softkey;
            updatescreen = 1;
        }
    }
}
void Interface_BtnSelOp(u8 op, u8 state){
    //TODO
    DBG("Operator selection button %x state %d", op, state);
}
void Interface_BtnOpMute(u8 op, u8 state){
    //TODO
    DBG("Operator mute button %x state %d", op, state);
}
void Interface_BtnSystem(u8 button, u8 state){
    //TODO
    DBG("System button %x state %d", button, state);
    if(button == FP_B_PLAY && state){
        DBG("Pressed play");
        u8 i;
        if(sources[selgenesis] != NULL){
            //Stop first
            VGM_Head_Delete(heads[selgenesis]);
            VGM_Source_Delete(sources[selgenesis]);
            sources[selgenesis] = NULL;
            heads[selgenesis] = NULL;
            Genesis_Reset(selgenesis);
        }
        char* tempbuf = malloc(13);
        for(i=0; i<8; i++){
            if(filenamelist[(9*selfile)+i] <= ' ') break;
            tempbuf[i] = filenamelist[(9*selfile)+i];
        }
        tempbuf[i++] = '.';
        tempbuf[i++] = 'v';
        tempbuf[i++] = 'g';
        tempbuf[i++] = 'm';
        tempbuf[i++] = 0;
        VgmSource* vgms = VGM_SourceStream_Create();
        sources[selgenesis] = vgms;
        s32 res = VGM_SourceStream_Start(vgms, tempbuf);
        if(res >= 0){
            VgmHead* vgmh = VGM_Head_Create(vgms);
            heads[selgenesis] = vgmh;
            VGM_Head_Restart(vgmh, VGM_Player_GetVGMTime());
            for(i=0; i<0xC; ++i){
                vgmh->channel[i].map_chip = selgenesis;
            }
            vgmh->playing = 1;
        }else{
            VGM_Source_Delete(vgms);
        }
        updatescreen = 1;
        free(tempbuf);
    }else if(button == FP_B_MENU && state){
        DBG("Pressed menu");
        if(sources[selgenesis] != NULL){
            VGM_Head_Delete(heads[selgenesis]);
            VGM_Source_Delete(sources[selgenesis]);
            sources[selgenesis] = NULL;
            heads[selgenesis] = NULL;
            Genesis_Reset(selgenesis);
            updatescreen = 1;
        }
    }
}
void Interface_BtnEdit(u8 button, u8 state){
    //TODO
    DBG("Edit button %x state %d", button, state);
}

void Interface_EncDatawheel(s32 incrementer){
    //TODO
    DBG("Datawheel moved %d", incrementer);
    s32 newselfile = incrementer + (s32)selfile;
    if(newselfile < numfiles && newselfile >= 0){
        selfile = newselfile;
        updatescreen = 1;
    }
}
void Interface_EncEdit(u8 encoder, s32 incrementer){
    //TODO
    DBG("Edit encoder %d moved %d", encoder, incrementer);
}

