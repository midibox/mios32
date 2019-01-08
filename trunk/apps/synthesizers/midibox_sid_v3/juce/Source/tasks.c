// $Id$
/*
 * Temporary Wrapper code/stubs for Application Tasks
 * Only functions used by MBSID applications are listed here!
 * TODO: emulate FreeRTOS in background via Posix driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>

s32 TASKS_SDCardSemaphoreTake(void) { return -1; }
s32 TASKS_SDCardSemaphoreGive(void) { return -1; }
s32 TASKS_MIDIINSemaphoreTake(void) { return -1; }
s32 TASKS_MIDIINSemaphoreGive(void) { return -1; }
s32 TASKS_MIDIOUTSemaphoreTake(void) { return -1; }
s32 TASKS_MIDIOUTSemaphoreGive(void) { return -1; }
s32 TASKS_LCDSemaphoreTake(void) { return -1; }
s32 TASKS_LCDSemaphoreGive(void) { return -1; }

s32 TASKS_Init(u32 mode) { return -1; }

s32 SEQ_TASK_SID(void) { return -1; }
s32 SEQ_TASK_Period1mS(void) { return -1; }
s32 SEQ_TASK_Period1mS_LowPrio(void) { return -1; }
s32 SEQ_TASK_Period1S(void) { return -1; }

s32 TASK_MSD_EnableSet(u8 enable) { return -1; }
s32 TASK_MSD_EnableGet() { return -1; }
s32 TASK_MSD_FlagStrGet(char str[5]) { return -1; }
