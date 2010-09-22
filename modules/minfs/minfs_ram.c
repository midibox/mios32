/*
 * MINFS caching layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch / thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include "minfs.h"
#include <string.h>


#define DEV_BLOCK_SIZE 64
#define DEV_NUM_BLOCKS 32

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

typedef uint8_t databuf_t[DEV_BLOCK_SIZE];

// virtual device blocks (or memory-fs)
static databuf_t storage_blocks[DEV_NUM_BLOCKS];

// block-buffer
static MINFS_block_buf_t block_buf;
static databuf_t block_buf_buffer;

/////////////////////////////////////////////////////////////////////////////
// Blockbuffer initialization
////////////////////////////////////////////////////////////////////////////

void MINFS_RAM_Init(void){
  MINFS_InitBlockBuffer(&block_buf);
  block_buf.p_buf = block_buf_buffer;
}

/////////////////////////////////////////////////////////////////////////////
// MINFS OS Hook functions
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  if( p_fs->fs_id == 1){
    // simulate an external storage device (with random-access ability).
    // if PEC is enabled, only whole blocks will be read (data_len == 0)
    if(!data_len){
      data_len = DEV_BLOCK_SIZE; // read entire block
      p_block_buf->flags.populated = 1; // indicate that the whole block was read and copied to the buffer
   }
    memcpy( (uint8_t*)( (uint8_t*)(p_block_buf->p_buf) + data_offset ), (uint8_t*)( (uint8_t*)(storage_blocks[p_block_buf->block_n]) + data_offset ), data_len);
  }
  return 0;
}

int32_t MINFS_Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  if( p_fs->fs_id == 1){
    // simulate an external storage device (with random-access ability).
    // if PEC is enabled, only whole blocks will be written (data_len == 0)
    memcpy(&(storage_blocks[p_block_buf->block_n][data_offset]), (uint8_t*)(p_block_buf->p_buf) + data_offset, data_len ? data_len : DEV_BLOCK_SIZE);
    p_block_buf->flags.changed = 0;
  }
  return 0;
}

int32_t MINFS_GetBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id){
  // assign blockbuffer to return
  (*pp_block_buf) = &block_buf;
  // if used as in-memory-filesystem, the data-block is assigned directly
  // flags and block_n are set, no call to read or write - hook will ever occur!
  if( p_fs->fs_id == 0){
    block_buf.block_n = block_n;
    block_buf.flags.populated = 1;
    block_buf.p_buf = storage_blocks[block_n];
  }
  return 0;
}
