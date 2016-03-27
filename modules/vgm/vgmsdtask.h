/*
 * VGM Data and Playback Driver: SD Card Buffer Task header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMSDTASK_H
#define _VGMSDTASK_H

#include <mios32.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

extern xSemaphoreHandle xSDCardSemaphore;
#define MUTEX_SDCARD_TAKE { while( xSemaphoreTakeRecursive(xSDCardSemaphore, (portTickType)1) != pdTRUE ); }
#define MUTEX_SDCARD_GIVE { xSemaphoreGiveRecursive(xSDCardSemaphore); }

extern u8 vgm_sdtask_disable;
extern u8 vgm_sdtask_usingsdcard;

extern void VGM_SDTask_Init();

#endif /* _VGMSDTASK_H */
