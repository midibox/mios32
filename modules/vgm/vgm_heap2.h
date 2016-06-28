/*
 * VGM Data and Playback Driver: Secondary heap header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGM_HEAP2_H
#define _VGM_HEAP2_H

#include <mios32.h>

#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "vgm_heap2 only supported on STM32F4!"
#endif

#include <stm32f4xx.h>

#ifndef VGMH2_HEAPSTART
#define VGMH2_HEAPSTART CCMDATARAM_BASE
#endif

#ifndef VGMH2_HEAPSIZE
#define VGMH2_HEAPSIZE 0x00010000
#endif

#define VGMH2_NUMBLOCKS (VGMH2_HEAPSIZE / 8)

extern void vgmh2_init();

extern void* vgmh2_malloc(size_t size);
extern void* vgmh2_realloc(void* ptr, size_t size);
extern void vgmh2_free(void* ptr);

extern volatile u16 vgmh2_numusedblocks;


#endif /* _VGM_HEAP2_H */
