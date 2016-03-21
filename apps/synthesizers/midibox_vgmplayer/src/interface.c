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
        if(softkey == 0){
            //DEBUG_Ring++;
        }else if(softkey == 1){
            //DEBUG_Ring--;
        }else if(softkey == 2){
            genesis[0].board.cap_psg = 1;
            genesis[0].board.cap_opn2 = 1;
            Genesis_WriteBoardBits(0);
        }else if(softkey == 3){
            genesis[0].board.cap_psg = 0;
            genesis[0].board.cap_opn2 = 0;
            Genesis_WriteBoardBits(0);
        }else if(softkey == 4){
            Genesis_OPN2Write(0, 0, 0x21, 0x10); while(Genesis_CheckOPN2Busy(0));
        }else if(softkey == 5){
            Genesis_OPN2Write(0, 0, 0x21, 0x20); while(Genesis_CheckOPN2Busy(0));
        }else if(softkey == 6){
            Genesis_OPN2Write(0, 0, 0x21, 0x02); while(Genesis_CheckOPN2Busy(0));
        }else if(softkey == 7){
            Genesis_OPN2Write(0, 0, 0x21, 0x00); while(Genesis_CheckOPN2Busy(0));
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
        playbackcommand = 3;
    }else if(button == FP_B_MENU && state){
        DBG("Pressed menu");
    }
}
void Interface_BtnEdit(u8 button, u8 state){
    //TODO
    DBG("Edit button %x state %d", button, state);
}

void Interface_EncDatawheel(s32 incrementer){
    //TODO
    DBG("Datawheel moved %d", incrementer);
    if(incrementer > 0){
        playbackcommand = 1;
    }else{
        playbackcommand = 2;
    }
}
void Interface_EncEdit(u8 encoder, s32 incrementer){
    //TODO
    DBG("Edit encoder %d moved %d", encoder, incrementer);
}

