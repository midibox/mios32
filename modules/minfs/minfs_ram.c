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


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

typedef uint8_t databuf_t[64];

databuf_t buffers[64];
MINFS_block_buf_t block_buf;

/////////////////////////////////////////////////////////////////////////////
// MINFS OS Hook functions
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  p_block_buf->flags.populated = 1;
  return 0;
}

int32_t MINFS_Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  p_block_buf->flags.changed = 0;
  return 0;
}

int32_t MINFS_GetBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id){
  (*pp_block_buf) = &block_buf;
  block_buf.p_buf = buffers[block_n];
  block_buf.block_n = block_n;
  block_buf.flags.populated = 1;
  block_buf.flags.changed = 0;
  return 0;
}
