/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

	vTaskSuspendAll();
	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( pxEnd == NULL )
		{
			prvHeapInit();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
					configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
						configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( pxNewBlockLink );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
				( void ) xTaskResumeAll();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
BlockLink_t *pxFirstFreeBlock;
uint8_t *pucAlignedHeap;
size_t uxAddress;
size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

	/* Ensure the heap starts on a correctly aligned boundary. */
	uxAddress = ( size_t ) ucHeap;

	if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( portBYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
		xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
	}

	pucAlignedHeap = ( uint8_t * ) uxAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
	pxEnd = ( void * ) uxAddress;
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}


// realloc() implementation by Sauraen
#include <string.h>
void* pvPortRealloc(void* pv, size_t xWantedSize){
    uint8_t* puc = (uint8_t*)pv;
    //Initial tests
    if(pv == NULL) return pvPortMalloc(xWantedSize);
    if(xWantedSize == 0){
        vPortFree(pv);
        return NULL;
    }
    //Modify our xWantedSize to include the size of the BlockLink_t structure
    size_t origWantedSize = xWantedSize;
    xWantedSize += xHeapStructSize;
	if((xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00){
		/* Byte alignment required. */
		xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
		configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
	}
	if((xWantedSize & xBlockAllocatedBit) != 0){
	    //Asking for too much memory
	    vPortFree(pv);
        return NULL;
	}
	//Set up our pointers
	puc -= xHeapStructSize; //Point to BlockLink_t structure
    BlockLink_t* pxCur = (void*)puc; //Get pointer as BlockLink_t
    //Enter critical section
    vTaskSuspendAll();
    //Check that the block is actually allocated
    if((pxCur->xBlockSize & xBlockAllocatedBit) == 0 || pxCur->pxNextFreeBlock != NULL){
        //We were passed a bad pointer, or the heap is corrupted
        configASSERT(0);
        xTaskResumeAll();
        return NULL;
    }
    //Get current block size
    size_t cursize = pxCur->xBlockSize & ~xBlockAllocatedBit;
    if(xWantedSize == cursize){
        //No change
        xTaskResumeAll();
        return pv;
    }
    //Traverse the free list until we find the last free block before our
    //block and the first one after--needed for both expanding and shrinking
    BlockLink_t *pxTemp, *pxPrevPrevFree, *pxPrevFree, *pxNextFree;
    size_t backspace = 0, forwardspace = 0;
    pxPrevFree = &xStart;
    pxPrevPrevFree = pxPrevFree;
    for(pxTemp = &xStart; pxTemp->pxNextFreeBlock < pxCur; pxTemp = pxTemp->pxNextFreeBlock){
        pxPrevPrevFree = pxPrevFree;
        pxPrevFree = pxTemp->pxNextFreeBlock;
    }
    pxNextFree = pxTemp->pxNextFreeBlock;
    //Is the next free block immediately adjacent to our block?
    pxTemp = (BlockLink_t*)(puc + cursize);
    if(pxTemp < pxNextFree){
        //No
    }else if(pxTemp > pxNextFree){
        //This block overlaps with free block next, error
        configASSERT(0);
        xTaskResumeAll();
        return NULL;
    }else{
        //Yes
        forwardspace = pxNextFree->xBlockSize;
    }
    //Grow or shrink our block?
    if(xWantedSize > cursize){
        //Is the previous free block immediately adjacent to our block?
        pxTemp = (BlockLink_t*)((uint8_t*)pxPrevFree + pxPrevFree->xBlockSize);
        if(pxTemp < pxCur || pxPrevFree == &xStart){
            //No
        }else if(pxTemp > pxCur){
            //Free block behind overlaps this block, error
            configASSERT(0);
            xTaskResumeAll();
            return NULL;
        }else{
            //Yes
            backspace = pxPrevFree->xBlockSize;
        }
        //With both of these together, will we have enough?
        if(xWantedSize > cursize + backspace + forwardspace){
            //No--try allocating a new memory block and copying the data
            xTaskResumeAll(); //None of this is critical section
            void* newBlock = malloc(origWantedSize);
            if(newBlock == NULL){
                //Too bad, still not enough space
                vPortFree(pv);
                return NULL;
            }
            //Copy payload
            memcpy(newBlock, pv, cursize - xHeapStructSize);
            //Return new pointer
            vPortFree(pv);
            return newBlock;
        }
        //Will we have enough space to only grow forwards?
        if(xWantedSize > cursize + forwardspace){
            //No--have to move backwards, so we might as well go all the way
            void* newpv = ((uint8_t*)pxPrevFree + xHeapStructSize);
            memmove(newpv, pv, cursize);
            pxCur = pxPrevFree;
            pv = newpv;
            pxCur->pxNextFreeBlock = NULL;
            cursize += backspace;
            xFreeBytesRemaining -= backspace;
            pxPrevFree = pxPrevPrevFree;
        }
        if(forwardspace > 0){
            //Going to assimmilate this free block, so point to the one after that
            pxNextFree = pxNextFree->pxNextFreeBlock;
            xFreeBytesRemaining -= forwardspace;
        }
        //Would there be enough left after our block to make a free block
        //with a reasonable size (double the size of the struct)?
        if(cursize + forwardspace - xWantedSize >= (xHeapStructSize << 1)){
            //Yes
            pxCur->xBlockSize = xWantedSize | xBlockAllocatedBit;
            //Create new free block afterwards
            pxTemp = (BlockLink_t*)((uint8_t*)pxCur + xWantedSize);
            pxTemp->pxNextFreeBlock = pxNextFree;
            pxPrevFree->pxNextFreeBlock = pxTemp;
            pxTemp->xBlockSize = cursize + forwardspace - xWantedSize;
            xFreeBytesRemaining += pxTemp->xBlockSize;
        }else{
            //No, just take all the free space
            pxCur->xBlockSize = (cursize + forwardspace) | xBlockAllocatedBit;
            pxPrevFree->pxNextFreeBlock = pxNextFree;
        }
    }else if((forwardspace > 0) || ((cursize - xWantedSize) >= (xHeapStructSize << 1))){
        //Trying to shrink it, and there's either already a free block
        //ahead of it, or we're shrinking it by enough to make a new free block
        pxCur->xBlockSize = xWantedSize | xBlockAllocatedBit;
        if(forwardspace > 0){
            //Going to assimmilate this free block, so point to the one after that
            pxNextFree = pxNextFree->pxNextFreeBlock;
        }
        //Create new free block afterwards
        pxTemp = (BlockLink_t*)((uint8_t*)pxCur + xWantedSize);
        pxTemp->pxNextFreeBlock = pxNextFree;
        pxTemp->xBlockSize = cursize + forwardspace - xWantedSize;
        pxPrevFree->pxNextFreeBlock = pxTemp;
        xFreeBytesRemaining += cursize - xWantedSize;
    }//Otherwise, trying to shrink it by a very small amount--don't bother
    
    //All done--pv was already adjusted in the one case where it needed to be
    if(xFreeBytesRemaining < xMinimumEverFreeBytesRemaining){
        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
    }
    xTaskResumeAll();
    return pv;
}


// TK: inserted for debugging purposes, typically used in a terminal
#include <mios32.h>
void vPortMallocDebugInfo( void )
{
  {
    size_t uxAddress = (size_t)ucHeap;
    if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 ) {
      uxAddress += ( portBYTE_ALIGNMENT - 1 );
      uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    }
    BlockLink_t *pxIterator = (BlockLink_t *)uxAddress;

    unsigned numBlock=0;
    for(numBlock=0; pxIterator->xBlockSize != 0; pxIterator = (BlockLink_t *)((size_t)pxIterator + (pxIterator->xBlockSize & ~xBlockAllocatedBit)), ++numBlock) {
      u8 *mem = (u8 *)((size_t)pxIterator + xHeapStructSize);
      u32 size = (pxIterator->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize;

      if( pxIterator->xBlockSize & xBlockAllocatedBit ) {
	MIOS32_MIDI_SendDebugMessage("Block #%d of size %d bytes located at 0x%08x\n", numBlock+1, size, mem);
	MIOS32_MIDI_SendDebugHexDump(mem, size);
      } else {
	MIOS32_MIDI_SendDebugMessage("Block #%d of size %d bytes located at 0x%08x IS FREE\n", numBlock+1, size, mem);
      }
    }
  }

  {
    s32 heap_size = configTOTAL_HEAP_SIZE;
    s32 free_heap = xPortGetFreeHeapSize();
    s32 ever_free_heap = xPortGetMinimumEverFreeHeapSize();
    s32 used_heap = heap_size - free_heap;

    MIOS32_MIDI_SendDebugMessage("Heap: %d of %d bytes used (%d%%), %d bytes free, %d bytes minimum ever free", used_heap, heap_size, (used_heap*100)/heap_size, free_heap, ever_free_heap);
  }
}

