/*
 * VGM Data and Playback Driver: Secondary heap
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 * 
 * Heavily based on (i.e. copied with some changes):
 * umm_malloc.c in FreeRTOS/Source/portable/MemMang
 * Copyright (C) Ralph Hempel 2009. See LICENSE.TXT in that folder.
 * 
 * Some important changes/notes:
 * 
 * Sizes are hard-coded for a 32-bit platform with 8-byte blocks.
 *
 * For clarity (this is the same in the original implementation): next and
 * previous blocks are the actual block numbers of the consecutive blocks.
 * I.e. sizeof(c) == NBLOCK(c) - c, and sizeof(previous) == c - PBLOCK(c).
 * However next and previous FREE blocks are in an arbitrary order; newly
 * freed blocks are added to the head of the free list.
 */


#include "vgm_heap2.h"

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

//--------------------------------Data types------------------------------------

typedef struct{
    u16 next;
    u16 prev;
} vgmh2_ptr;


typedef struct{
    union {
        vgmh2_ptr used;
    } header;
    union {
        vgmh2_ptr free;
        u8 data[4];
    } body;
} vgmh2_block;

#define VGMH2_FREELIST_MASK (0x8000)
#define VGMH2_BLOCKNO_MASK  (0x7FFF)

//--------------------------------The heap--------------------------------------

vgmh2_block *const vgmh2_heap = (vgmh2_block *const)(VGMH2_HEAPSTART);

volatile u16 vgmh2_numusedblocks;

//---------------------Some macros for quick access-----------------------------

#define VGMH2_BLOCK(b)  (vgmh2_heap[b])

#define VGMH2_NBLOCK(b) (VGMH2_BLOCK(b).header.used.next)
#define VGMH2_PBLOCK(b) (VGMH2_BLOCK(b).header.used.prev)
#define VGMH2_NFREE(b)  (VGMH2_BLOCK(b).body.free.next)
#define VGMH2_PFREE(b)  (VGMH2_BLOCK(b).body.free.prev)
#define VGMH2_DATA(b)   (VGMH2_BLOCK(b).body.data)

//----------------------------Helper functions----------------------------------

//How many blocks will we need to fit size bytes of data?
u16 vgmh2_blocks(size_t size){
    if(size <= 4) return 1; //We can fit it in a single block's body
    return 2 + ((size-5)/8);
}

//Create a new block at c+blocks, link it into the list.
void vgmh2_make_new_block(u16 c, u16 blocks, u16 c_freemask){
     VGMH2_NBLOCK(c+blocks) = VGMH2_NBLOCK(c) & VGMH2_BLOCKNO_MASK; //New block's N is originally-next block
     VGMH2_PBLOCK(c+blocks) = c; //New block's P is current block

     VGMH2_PBLOCK(VGMH2_NBLOCK(c) & VGMH2_BLOCKNO_MASK) = (c+blocks); //Originally-next block's P is new block
     VGMH2_NBLOCK(c)                                    = (c+blocks) | c_freemask; //Current block's N is new block, and mark the current block as free or not
}

//Disconnect this block from the free list.
void vgmh2_disconnect_from_free_list(u16 c){
    VGMH2_NFREE(VGMH2_PFREE(c)) = VGMH2_NFREE(c); //Connect previous to next
    VGMH2_PFREE(VGMH2_NFREE(c)) = VGMH2_PFREE(c); //Connect next to previous
    VGMH2_NBLOCK(c) &= (~VGMH2_FREELIST_MASK); //Mark current block as not free
}

//If the next block is free, combine the current block with it.
void vgmh2_assimilate_up(u16 c){
    if(VGMH2_NBLOCK(VGMH2_NBLOCK(c)) & VGMH2_FREELIST_MASK){ //Next block is free
        vgmh2_disconnect_from_free_list(VGMH2_NBLOCK(c)); //Disconnect next block
        VGMH2_PBLOCK(VGMH2_NBLOCK(VGMH2_NBLOCK(c)) & VGMH2_BLOCKNO_MASK) = c; //Next-next-previous point to current
        VGMH2_NBLOCK(c) = VGMH2_NBLOCK(VGMH2_NBLOCK(c)) & VGMH2_BLOCKNO_MASK; //Next point to next-next
    } 
}

//Combine the current block with the previous one.
u16 vgmh2_assimilate_down(u16 c, u16 p_freemask){
    VGMH2_NBLOCK(VGMH2_PBLOCK(c)) = VGMH2_NBLOCK(c) | p_freemask; //Previous-next point to next, and mark previous as free or not
    VGMH2_PBLOCK(VGMH2_NBLOCK(c)) = VGMH2_PBLOCK(c); //Next-previous point to previous
    return VGMH2_PBLOCK(c); //Return previous
}

//------------------------------Main functions----------------------------------
//TODO use regular malloc functions if these fail; also check input pointers to
//see whether they're for these functions or regular malloc

void vgmh2_init(){
    //Clear heap data 4 bytes at a time
    u32* head = (u32*)(vgmh2_heap);
    u32* end = (u32*)((u8*)head + VGMH2_HEAPSIZE);
    while(head < end) *head++ = 0;
    //Initialize used blocks counter
    vgmh2_numusedblocks = 2; //The head and the tail
    //Initialize head of heap
    VGMH2_NBLOCK(0) = 1;
    VGMH2_NFREE(0)  = 1;
}

void *vgmh2_malloc(size_t size){
    if(size == 0) return NULL;
    u16 blocksNeeded, blockSize, bestSize, bestBlock, cf;
    blocksNeeded = vgmh2_blocks(size);
    vTaskSuspendAll(); //Enter critical section

    //Scan free list to find first/best fit
    cf = VGMH2_NFREE(0);
    bestBlock = cf;
    bestSize  = 0x7FFF;
    blockSize = 0;
    while(VGMH2_NFREE(cf)){
        blockSize = (VGMH2_NBLOCK(cf) & VGMH2_BLOCKNO_MASK) - cf;
        //#if defined VGMH2_FIRST_FIT
        //if((blockSize >= blocksNeeded))
        //    break;
        //#elif defined VGMH2_BEST_FIT
        if((blockSize >= blocksNeeded) && (blockSize < bestSize)){
            bestBlock = cf;
            bestSize  = blockSize;
        }
        //#endif
        cf = VGMH2_NFREE(cf);
    }
    if(0x7FFF != bestSize){
        cf        = bestBlock;
        blockSize = bestSize;
    }//else there are no free blocks, try at the end of the heap
    //Now cf is the block we're going to use, and blockSize is its size

    if(VGMH2_NBLOCK(cf) & VGMH2_BLOCKNO_MASK){
        //cf is a free block, allocate within that
        if(blockSize == blocksNeeded){
            //Exact fit, just mark it as not free
            vgmh2_disconnect_from_free_list(cf);
        }else{
            //Create our block in the later half of this block; mark the original
            //(lower/remaining) portion as free
            vgmh2_make_new_block(cf, blockSize-blocksNeeded, VGMH2_FREELIST_MASK);
            //Going to return the later block
            cf += blockSize-blocksNeeded;
        }
    }else{
        //cf is the block starting the free space at the end of the heap
        //Is there enough remaining heap space to allocate the desired number of blocks?
        u16 newEndOfHeap = cf + blocksNeeded; //The block to mark the end of the heap
        if(newEndOfHeap >= VGMH2_NUMBLOCKS){
            //The end of heap block would be outside the memory segment
            xTaskResumeAll(); //Leave critical section
            return NULL;
        }
        VGMH2_NFREE(VGMH2_PFREE(cf)) = newEndOfHeap; //Previous-next point to the end of the heap
        memcpy(&VGMH2_BLOCK(newEndOfHeap), &VGMH2_BLOCK(cf), sizeof(vgmh2_block)); //Copied data: NBLOCK and NFREE are zeros, PFREE is whatever the last free block is, and PBLOCK will be overwritten next
        VGMH2_PBLOCK(newEndOfHeap) = cf; //Previous of the end of the heap is current
        VGMH2_NBLOCK(cf)           = newEndOfHeap; //Next of current is the end of the heap
    }

    vgmh2_numusedblocks += blocksNeeded; //Count the number of blocks used
    xTaskResumeAll(); //Leave critical section
    return((void *)&VGMH2_DATA(cf)); //Return a pointer to the data section of the current block
}

// ----------------------------------------------------------------------------

void *vgmh2_realloc(void *ptr, size_t size){
    if(ptr == NULL) return vgmh2_malloc(size);
    if(size == 0){
        vgmh2_free(ptr);
        return NULL;
    }
    u16 blocksNeeded, curSizeBlocks, c, new_numusedblocks;
    size_t curSizeBytes;
    vTaskSuspendAll(); //Enter critical section

    blocksNeeded = vgmh2_blocks(size);
    c = (ptr-(void *)(VGMH2_HEAPSTART))/sizeof(vgmh2_block); //What block this pointer points to
    curSizeBlocks = (VGMH2_NBLOCK(c) - c); //Current size of region in blocks
    curSizeBytes  = (curSizeBlocks*8)-4; //Current size of region in bytes
    new_numusedblocks = vgmh2_numusedblocks + blocksNeeded - curSizeBlocks; //Save the overall change in number of used blocks for if we succeed, because the malloc and free operations in here will mess up vgmh2_numusedblocks

    if(curSizeBlocks == blocksNeeded){
        //No change, or change by less than a block
        xTaskResumeAll(); //Leave critical section
        return(ptr);
    }

    //Combine this block with any free space following it (this checks whether the next block is free)
    vgmh2_assimilate_up(c);

    //If the previous block is free, and the combined previous and current blocks
    //would be enough space, combine them and move the data down to the lower block
    if((VGMH2_NBLOCK(VGMH2_PBLOCK(c)) & VGMH2_FREELIST_MASK) &&
            (blocksNeeded <= (VGMH2_NBLOCK(c)-VGMH2_PBLOCK(c)))){
        vgmh2_disconnect_from_free_list(VGMH2_PBLOCK(c)); //Mark lower block as not free
        c = vgmh2_assimilate_down(c, 0); //Combine the blocks, don't mark lower block as free, and now point to the lower one
        memmove((void *)&VGMH2_DATA(c), ptr, curSizeBytes); //Move the original data down
        ptr = (void *)&VGMH2_DATA(c); //Make the returnable data pointer point to the new location
    }

    curSizeBlocks = (VGMH2_NBLOCK(c) - c); //Calculate the block size again
    if(curSizeBlocks == blocksNeeded){
        //do nothing, return the existing pointer
    }else if(curSizeBlocks > blocksNeeded){
        //Make a new block in the upper (unused) portion; mark the lower one as used
        vgmh2_make_new_block(c, blocksNeeded, 0);
        //Free the new (upper) block
        vgmh2_free((void *)&VGMH2_DATA(c+blocksNeeded));
    }else{
        //We still don't have enough room
        void *oldptr = ptr; //Save original pointer
        ptr = vgmh2_malloc(size); //Malloc a new block that's actually big enough
        if(ptr != NULL){
            memcpy(ptr, oldptr, curSizeBytes); //Copy our data to it
        }
        vgmh2_free(oldptr); //In both cases free the original pointer
        //Return ptr whether it's null or not
    }

    if(ptr != NULL){
        //Realloc succeeded, store the new size (calculated above):
        vgmh2_numusedblocks = new_numusedblocks;
    }else{
        //New malloc failed, so free already took away and un-counted the used
        //memory; vgmh2_numusedblocks was only updated by free, and is now correct
    }

    xTaskResumeAll(); //Leave critical section
    return(ptr);
}

void vgmh2_free(void *ptr){
    if(ptr == NULL) return;
    u16 c;
    vTaskSuspendAll(); //Enter critical section
    c = (ptr-(void *)(&(vgmh2_heap[0])))/sizeof(vgmh2_block); //What block this pointer points to
    vgmh2_numusedblocks -= (VGMH2_NBLOCK(c) - c); //Mark the size of this block as unused

    //Combine this block with the next if possible
    vgmh2_assimilate_up(c);

    //If the previous block is free,
    if(VGMH2_NBLOCK(VGMH2_PBLOCK(c)) & VGMH2_FREELIST_MASK){
        //combine this with it, marking the lower block as free
        c = vgmh2_assimilate_down(c, VGMH2_FREELIST_MASK);
    }else{
        //Add this block to the head of the free list
        VGMH2_PFREE(VGMH2_NFREE(0)) = c; //Previous of the first free block points to current one
        VGMH2_NFREE(c)              = VGMH2_NFREE(0); //Current-next points to originally first free
        VGMH2_PFREE(c)              = 0; //Previous of current is the head
        VGMH2_NFREE(0)              = c; //First free block (next of head) is current
        VGMH2_NBLOCK(c)          |= VGMH2_FREELIST_MASK; //Mark current block as free
    }
    xTaskResumeAll(); //Leave critical section
}

