/*
 * VGM Data and Playback Driver: VGM Data Source
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmsource.h"
#include "vgmhead.h"
#include "vgmqueue.h"
#include "vgmram.h"
#include "vgmstream.h"
#include "vgm_heap2.h"

s32 VGM_Source_Delete(VgmSource* source){
    if(source == NULL) return -1;
    s32 i;
    for(i=0; i<vgm_numheads; ++i){
        if(vgm_heads[i] != NULL && vgm_heads[i]->source == source){
            VGM_Head_Delete(vgm_heads[i]);
            --i;
        }
    }
    if(source->type == VGM_SOURCE_TYPE_RAM){
        VGM_SourceRAM_Delete(source->data);
    }else if(source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_SourceStream_Delete(source->data);
    }else if(source->type == VGM_SOURCE_TYPE_QUEUE){
        VGM_SourceQueue_Delete(source->data);
    }
    vgmh2_free(source);
    return 0;
}
