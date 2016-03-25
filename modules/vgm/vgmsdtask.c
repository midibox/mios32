/*
 * VGM Data and Playback Driver: SD Card Buffer Task 
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmsdtask.h"
#include "vgmhead.h"
#include "vgmstream.h"

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#define VGM_SDTASK_PRIORITY 3

u8 vgm_sdtask_disable;
u8 vgm_sdtask_usingsdcard;

static void VGM_SDTask(void* pvParameters){
    portTickType xLastExecutionTime;
    xLastExecutionTime = xTaskGetTickCount();
    u8 i, fi = 0;
    VgmHead* vh;
    while(1){
        vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
        i = fi;
        while(1){
            if(vgm_sdtask_disable){
                fi = i; //start back where you are now
                break; //stop immediately
            }
            vh = vgm_heads[i];
            if(vh != NULL){
                if(vh->playing && vh->source->type == VGM_SOURCE_TYPE_STREAM){
                    VGM_HeadStream_BackgroundBuffer(vh);
                }
            }
            ++i;
            if(i >= vgm_numheads) i = 0;
            if(i == fi) break; //Have done them all
        }
    }
}

void VGM_SDTask_Init(){
    vgm_sdtask_disable = 0;
    vgm_sdtask_usingsdcard = 0;
    xTaskCreate(VGM_SDTask, (signed portCHAR *)"VGM_SD", configMINIMAL_STACK_SIZE, NULL, VGM_SDTASK_PRIORITY, NULL);
}
