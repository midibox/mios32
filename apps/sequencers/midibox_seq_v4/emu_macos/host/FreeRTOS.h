// Emulator: the few FreeRTOS names the modules use (heap) - everything else of the OS is in host/emu_os.c
#ifndef _EMU_FREERTOS_H
#define _EMU_FREERTOS_H
#include <stdlib.h>
#define pvPortMalloc(n) malloc(n)
#define vPortFree(p)    free(p)
static inline size_t xPortGetFreeHeapSize(void) { return 64*1024; }
#endif
