
This is a downstripped copy of FreeRTOS V9.0.0

It only contains the required sources for building on STM32 and LPC17 target

The complete version can be downloaded from:
http://sourceforge.net/projects/freertos/files/FreeRTOS/V9.0.0/


Modifications:

- TK (2016-11-03): added vPortMallocDebugInfo to Source/portable/MemMang/heap_4.c
- Sauraen (2017-02-02): added pvPortRealloc to Source/portable/MemMang/heap_4.c.
  This also included a definition in Source/include/portable.h and the function
  which maps realloc to pvPortRealloc in ../programming_models/traditional/
  freertos_heap.cpp.
