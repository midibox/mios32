/*
 * MIDIbox Quad Genesis: Genesis State Drawing Functions
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
#include "genesisstate.h"

#include <genesis.h>
#include "frontpanel.h"

void DrawGenesisActivity(u8 g){
    static u8 lastdac = 0x80;
    //FM voices
    if(g >= GENESIS_COUNT) return;
    u8 v, o, k;
    for(v=0; v<6; v++){
        k = 0;
        for(o=0; o<4; o++){
            k |= genesis[g].opn2.chan[v].op[0].kon;
        }
        FrontPanel_GenesisLEDSet(g, v+1, 0, k);
    }
    //DAC
    FrontPanel_GenesisLEDSet(g, 7, 0, (genesis[g].opn2.dac_enable && genesis[g].opn2.dac_high != lastdac));
    lastdac = genesis[g].opn2.dac_high;
    //PSG voices
    for(v=0; v<4; v++){
        FrontPanel_GenesisLEDSet(g, v+8, 0, genesis[g].psg.voice[v].atten != 0xF);
    }
}
