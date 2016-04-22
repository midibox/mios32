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
#include "app.h"

/*
u8 DEBUG_Ring;
u8 DEBUG_RingState;
s8 DEBUG_RingDir;
*/


void Interface_Init(){
    selgvoice = 0;
    FrontPanel_GenesisLEDSet((selgvoice >> 4), (selgvoice & 0xF), 1, 1);
}


void Interface_BtnGVoice(u8 gvoice, u8 state){
    DBG("Genesis Voice button %x state %d", gvoice, state);
    u8 g = (gvoice >> 4);
    u8 v = (gvoice & 0xF);
    if(state){
        FrontPanel_GenesisLEDSet((selgvoice >> 4), (selgvoice & 0xF), 1, 0);
        selgvoice = gvoice;
        FrontPanel_GenesisLEDSet((selgvoice >> 4), (selgvoice & 0xF), 1, 1);
    }
    
    if(v >= 8 && v <= 0xB){
        v -= 8;
        if(state){
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), 
                    .data = (0b10000000 | (v << 5)), .data2 = 0b00010000 }, 1);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = (0b10010000 | (v << 5)) }, 0);
        }else{
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (g << 4), .data = (0b10011111 | (v << 5)) }, 0);
        }
    }else if(v >= 1 && v <= 6){
        u8 ah = (v >= 4);
        u8 cmd = (g << 4) | ah | 0x02;
        if(state){
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x30, .data=0x71 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x34, .data=0x0D }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x38, .data=0x33 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x3C, .data=0x01 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x40, .data=0x23 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x44, .data=0x2D }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x48, .data=0x26 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x4C, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x50, .data=0x5F }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x54, .data=0x99 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x58, .data=0x5F }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x5C, .data=0x94 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x60, .data=0x05 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x64, .data=0x05 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x68, .data=0x05 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x6C, .data=0x07 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x70, .data=0x02 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x74, .data=0x02 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x78, .data=0x02 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x7C, .data=0x02 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x80, .data=0x11 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x84, .data=0x11 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x88, .data=0x11 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x8C, .data=0xA6 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x90, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x94, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x98, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0x9C, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0xB0, .data=0x32 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0xB4, .data=0xC0 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=cmd, .addr=0xA4, .data=0x22, .data2=0x69 }, 1);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(g<<4)|2, .addr=0x28, .data=0xF0|(ah<<2) }, 0);
        }else{
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(g<<4)|2, .addr=0x28, .data=0x00|(ah<<2) }, 0);
        }
    }
    
}
void Interface_BtnSoftkey(u8 softkey, u8 state){
    //TODO
    DBG("Softkey %x state %d", softkey, state);
    if(state){
        if(softkey < 4){
            selgenesis = softkey;
            updatescreen = 1;
        }else if(softkey == 4){
            genesis[0].board.cap_psg = !genesis[0].board.cap_psg;
            genesis[0].board.cap_opn2 = !genesis[0].board.cap_opn2;
            Genesis_WriteBoardBits(0);
        }else if(softkey == 5){
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x21, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x2C, .data=0x20 }, 0);
        }else if(softkey == 6){
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x21, .data=0x10 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x2C, .data=0x00 }, 0);
        }else if(softkey == 7){
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x21, .data=0x00 }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=(selgenesis<<4)|2, .addr=0x2C, .data=0x00 }, 0);
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
    }else if(button == FP_B_RESTART && state){
        DBG("Pressed stop");
        if(sources[selgenesis] != NULL){
            VGM_Head_Delete(heads[selgenesis]);
            VGM_Source_Delete(sources[selgenesis]);
            sources[selgenesis] = NULL;
            heads[selgenesis] = NULL;
            Genesis_Reset(selgenesis);
            updatescreen = 1;
        }
    }else if(button == FP_B_MENU && state){
        DBG("Pressed menu");
        vegasactive = !vegasactive;
        if(!vegasactive){
            u16 i;
            for(i=0; i<88*8; i++){
                BLM_X_LEDSet(i, 0, 0);
            }
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

