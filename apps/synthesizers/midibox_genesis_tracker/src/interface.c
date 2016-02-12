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

u8 DEBUG_Ring;
u8 DEBUG_RingState;
s8 DEBUG_RingDir;

void Interface_Init(){
    //TODO
}


void Interface_BtnGVoice(u8 gvoice, u8 state){
    //TODO
    DBG("Genesis Voice button %x state %d", gvoice, state);
    u8 g = (gvoice >> 4);
    u8 v = (gvoice & 0xF);
    if(v >= 8 && v <= 0xB){
        v -= 8;
        if(state){
            Genesis_PSGWrite(g, (0b10000000 | (v << 5)));
            while(Genesis_CheckPSGBusy(g));
            Genesis_PSGWrite(g, 0b00010000);
            while(Genesis_CheckPSGBusy(g));
            Genesis_PSGWrite(g, (0b10010000 | (v << 5)));
            while(Genesis_CheckPSGBusy(g));
        }else{
            Genesis_PSGWrite(g, (0b10011111 | (v << 5)));
            while(Genesis_CheckPSGBusy(g));
        }
    }else if(v >= 1 && v <= 6){
        u8 ah = (v >= 4);
        if(state){
            Genesis_OPN2Write(g, 0, 0x22, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, 0, 0x27, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, 0, 0x2B, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x30, 0x71); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x34, 0x0D); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x38, 0x33); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x3C, 0x01); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x40, 0x23); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x44, 0x2D); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x48, 0x26); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x4C, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x50, 0x5F); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x54, 0x99); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x58, 0x5F); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x5C, 0x94); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x60, 0x05); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x64, 0x05); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x68, 0x05); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x6C, 0x07); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x70, 0x02); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x74, 0x02); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x78, 0x02); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x7C, 0x02); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x80, 0x11); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x84, 0x11); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x88, 0x11); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x8C, 0xA6); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x90, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x94, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x98, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0x9C, 0x00); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0xB0, 0x32); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0xB4, 0xC0); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0xA4, 0x22); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, ah, 0xA0, 0x69); while(Genesis_CheckOPN2Busy(g));
            Genesis_OPN2Write(g, 0, 0x28, 0xF0 | (ah << 2)); while(Genesis_CheckOPN2Busy(g));
        }else{
            Genesis_OPN2Write(g, 0, 0x28, 0x00 | (ah << 2)); while(Genesis_CheckOPN2Busy(g));
        }
    }
}
void Interface_BtnSoftkey(u8 softkey, u8 state){
    //TODO
    DBG("Softkey %x state %d", softkey, state);
    if(state){
        if(softkey == 0){
            DEBUG_Ring++;
        }else if(softkey == 1){
            DEBUG_Ring--;
        }else if(softkey == 2){
            DEBUG_RingDir = 1;
        }else if(softkey == 3){
            DEBUG_RingDir = -1;
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
}
void Interface_BtnEdit(u8 button, u8 state){
    //TODO
    DBG("Edit button %x state %d", button, state);
}

void Interface_EncDatawheel(s32 incrementer){
    //TODO
    DBG("Datawheel moved %d", incrementer);
}
void Interface_EncEdit(u8 encoder, s32 incrementer){
    //TODO
    DBG("Edit encoder %d moved %d", encoder, incrementer);
}

