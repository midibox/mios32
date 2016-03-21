/*
 * VGM Data and Playback Driver: VGM Playback Head
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmhead.h"
#include "vgmram.h"
#include "vgmstream.h"

VgmHead* vgm_heads[VGM_HEAD_MAXNUM];
u32 vgm_numheads;

void VGM_Head_Init(){
    vgm_numheads = 0;
}

VgmHead* VGM_Head_Create(VgmSource* source){
    if(source == NULL) return NULL;
    VgmHead* head = malloc(sizeof(VgmHead));
    head->playing = 0;
    head->source = source;
    head->ticks = 0; //will get changed at restart
    //don't initialize mapping
    head->opn2mult = ((7670454 << 8) / 500000); //0x1000; //TODO adjust in real time
    head->psgmult = 0x1000; //TODO adjust in real time
    head->psgfreq0to1 = 1;
    head->tempomult = 0x1000;
    head->iswait = 1;
    head->iswrite = 0;
    head->isdone = 0;
    if(source->type == VGM_SOURCE_TYPE_RAM){
        head->data = VGM_HeadRAM_Create(source);
    }else if(source->type == VGM_SOURCE_TYPE_STREAM){
        head->data = VGM_HeadStream_Create(source);
    }
    MIOS32_IRQ_Disable();
    if(vgm_numheads == VGM_HEAD_MAXNUM){
        MIOS32_IRQ_Enable();
        free(head);
        return NULL;
    }
    vgm_heads[vgm_numheads] = head;
    ++vgm_numheads;
    MIOS32_IRQ_Enable();
    return head;
}
s32 VGM_Head_Delete(VgmHead* head){
    if(head == NULL) return -1;
    s32 ret = -1;
    MIOS32_IRQ_Disable();
    u32 i;
    for(i=0; i<vgm_numheads; ++i){
        if(vgm_heads[i] == head){
            for(; i<vgm_numheads-1; ++i){
                vgm_heads[i] = vgm_heads[i+1];
            }
            vgm_heads[i] = NULL;
            --vgm_numheads;
            ret = 0;
            break;
        }
    }
    MIOS32_IRQ_Enable();
    if(head->source->type == VGM_SOURCE_TYPE_RAM){
        VGM_HeadRAM_Delete(head->data);
    }else if(head->source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_HeadStream_Delete(head->data);
    }
    free(head);
    return ret;
}

void VGM_Head_Restart(VgmHead* head, u32 vgm_time){
    if(head == NULL) return;
    head->ticks = vgm_time;
    if(head->source->type == VGM_SOURCE_TYPE_RAM){
        VGM_HeadRAM_Restart(head);
        VGM_HeadRAM_cmdNext(head, vgm_time);
    }else if(head->source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_HeadStream_Restart(head);
        VGM_HeadStream_cmdNext(head, vgm_time);
    }
}
void VGM_Head_cmdNext(VgmHead* head, u32 vgm_time){
    if(head == NULL) return;
    if(head->source->type == VGM_SOURCE_TYPE_RAM){
        VGM_HeadRAM_cmdNext(head, vgm_time);
    }else if(head->source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_HeadStream_cmdNext(head, vgm_time);
    }
}

void VGM_fixOPN2Frequency(VgmChipWriteCmd* writecmd, u32 opn2mult){
    u8 block; u32 freq;
    block = (writecmd->data >> 3) & 0x07;
    freq = ((u32)(writecmd->data & 0x07) << 8) | writecmd->data2; //Up to 11 bits set
    freq <<= block; //Up to 18 bits set
    freq *= opn2mult; //If unity (0x1000), up to 30 bits set (29 downto 0)
    //Check for overflow
    if(freq & 0xC0000000){
        freq = 0x3FFFFFFF;
    }
    //To floating point: find most-significant 1
    for(block=8; block>1; --block){
        if(freq & 0x20000000) break;
        freq <<= 1;
    }
    --block;
    freq >>= 19; //Previously up to 30 bits set, now up to 11 bits set
    writecmd->data = (freq >> 8) | (block << 3);
    writecmd->data2 = (freq & 0xFF);
}

void VGM_fixPSGFrequency(VgmChipWriteCmd* writecmd, u32 psgmult, u8 psgfreq0to1){
    u32 freq = (writecmd->data & 0x0F) | ((writecmd->data2 & 0x3F) << 4);
    //TODO psgmult
    if(freq == 0 && psgfreq0to1){
        freq = 1;
    }
    writecmd->data = (writecmd->data & 0xF0) | (freq & 0x0F);
    writecmd->data2 = (freq >> 4) & 0x3F;
}
