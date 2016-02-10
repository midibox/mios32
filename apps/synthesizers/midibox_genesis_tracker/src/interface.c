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

u8 DEBUG_Ring;
u8 DEBUG_RingState;
s8 DEBUG_RingDir;

void Interface_Init(){
    //TODO
}


void Interface_BtnGVoice(u8 gvoice, u8 state){
    //TODO
    DBG("Genesis Voice button %x state %d", gvoice, state);
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

