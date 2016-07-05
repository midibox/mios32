
This is a downstripped copy of FreeRTOS V7.4.0

It only contains the required sources for building on STM32 and LPC17 target

The complete version can be downloaded from:
http://sourceforge.net/projects/freertos/files/FreeRTOS/V7.4.0/

umm_malloc by R. Hempel has been added to Source/portable/MemMang
It is referenced by MIOS32 instead of heap_2.c

umm_malloc modified by Sauraen 7/2016:
  - Add umm_numusedblocks and the appropriate changes to its value upon malloc,
    realloc, and free. This variable can be read externally for near-zero-
    overhead tracking of heap usage. (The existing umm_info routine has to
    iterate through the heap, so it can take several hundred ms.)
  - Change the size of umm_block.body.data from portBYTE_ALIGNMENT (i.e. 8) to
    sizeof(umm_ptr) (i.e. 4), which causes the block size to change from 12 to
    8. Probably it was never intended to be 12 in the first place.
