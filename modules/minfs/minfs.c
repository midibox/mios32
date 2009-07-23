/*
 * MINFS
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch / thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include <stdlib.h>

#include "minfs.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Function Hooks to caching and device layer
//
// These hook functions to OS-level provide data buffers and data. The buffers
// could also be provided with the application-level functions found in minfs.h
//
// A write sequence to a data block does call this sequence of OS-side functions:
// - MINFS_WriteBufGet (if no buffer was provided with the app-level function)
// - MINFS_Read (data_len==0), read the whole data block into write-buffer (only if 
//   just a part of the buffer will be re-written). 
// - 1-x times MINFS_Write (data_len > 0), once for each part in the data block
// - MINFS_Write (data_len==0), to write back the whole data block
//
// A read sequence from a data block does call this sequence of OS-side functions:
// - MINFS_Read (data_len==0), read the whole data block
// - 1-x times MINFS_Read (data_len > 0), once for each needed part in the data block
//
// If PEC is enabled, calls to MINFS_Read/MINFS_Write with data_len>0 will be
// omitted.
// Different buffer- and caching concepts may be implemented on OS side this way.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Hook to OS to read data. The function is first called with data_len==0 for
// a data block needed, after that, for each part of the block needed (data_len > 0). 
// The function may either read the whole block from the target device, or the single
// data chunks, depending on the device type.
//
// IN:  <block_type>  The Block's content type (the MINFS_BLOCK_TYPE_XXX)
//      <buffer>    Pointer to buffer-pointer, if NULL, the buffer has to be provided
//                  from here (application did not provide a buffer). The buffer's 
//                  size has to be at least data_offset+data_len, or 2^p_fs_info->block_size, 
//                  if data_len==0.
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id> Set to the file's ID if there's a file related to this read-operation
//      else set to 0.
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Read(uint8_t block_type, uint8_t **buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);


/////////////////////////////////////////////////////////////////////////////
// Hook to OS to write data. The function is first called for each part in the
// block to be written (data_len > 0), at the end it is called once with 
// data_len == 0.
// The function may either write the whole block to the target device, or the single
// data chunks, depending on the device type.
//
// IN:  <block_type>  The Block's content type (the MINFS_BLOCK_TYPE_XXX)
//      <buffer>    Pointer to buffer (size at least = sizeof(MINFS_fs_info_t) )
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id> Set to the file's ID if there's a file related to this write-operation
//      else set to 0.
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);


/////////////////////////////////////////////////////////////////////////////
// Provides a data buffer for a write operation (if no buffer was provided by 
// the application)
//
// IN:  <block_type>  The Block's content type (the MINFS_BLOCK_TYPE_XXX)
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id> Set to the file's ID if there's a file related to this write-operation
//      else set to 0.
// OUT: Pointer to a buffer of at least size 2^p_fs_info->block_size bytes
/////////////////////////////////////////////////////////////////////////////
extern uint8_t* MINFS_WriteBufGet(uint8_t block_type, uint32_t block_n, MINFS_fs_t *p_fs, uint32_t file_id);



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void CalcFSParams(MINFS_fs_t *p_fs);

static int32_t BlockSeek(MINFS_fs_t *p_fs, uint8_t *block_buffer, uint32_t start_block, uint32_t offset);
static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block, uint32_t link_to, uint8_t *block_buffer);

static uint32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks);
static uint32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t first_block);

// gets file header information
static uint32_t GetFileHeader(MINFS_fs_t *p_fs, uint32_t block_n, MINFS_file_header_t *p_file_header);

static uint32_t Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);
static uint32_t Read(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);
static uint32_t GetPECValue(uint8_t *buffer, uint16_t data_len);

/////////////////////////////////////////////////////////////////////////////
// High level functions
/////////////////////////////////////////////////////////////////////////////

int32_t MINFS_Format(MINFS_fs_t *p_fs, uint8_t *block_buf){
  uint8_t *buf;
  // get a write buffer
  if( !block_buf ){
    if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FSHEAD, 0, p_fs, 0)) )
      return MINFS_ERROR_NO_BUFFER;
  } 
  else buf = block_buf;
  uint16_t buf_i = 0; // byte index in data buffer
  // put fs-header data to buffer
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)buf;
  p_fs_header->fs_type = MINFS_FS_TYPE;
  p_fs_header->fs_info = p_fs->fs_info;
  buf_i += sizeof(MINFS_fs_header_t);
  // calculate fs-params
  CalcFSParams(p_fs);
  // now write the block-pointer map
  uint32_t i, block_ptr, block_n = 0;
  int32_t status;
  for(i = 0 ; i < p_fs->fs_info.num_blocks ; i++){
    // check if buffer is full
    if( buf_i >= p_fs->calc.block_data_len ){
      // write data
      if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, block_n, 0, p_fs->calc.block_data_len, p_fs, 0) ) < 0 )
        return status;
      if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, block_n, 0, 0, p_fs, 0) ) < 0 )
        return status;
      // get a write buffer
      if( !block_buf ){
	if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FSHEAD, 0, p_fs, 0)) )
	  return MINFS_ERROR_NO_BUFFER;
      } else buf = block_buf;
      // set block_n and buffer byte-index
      block_n++;
      buf_i = 0;
    }
    // calculate block pointer and write it to the buffer
    block_ptr = i ? i+1 : 0; // 0 is file-index-start, else free blocks chain
    // copy endian-independent
    if( p_fs->calc.bp_size > 2 )
      *( (uint32_t*)(buf + buf_i) ) = block_ptr;
    else if( p_fs->calc.bp_size > 1 )
      *( (uint16_t*)(buf + buf_i) ) = (uint16_t)block_ptr;
    else
      *( (uint8_t*)(buf + buf_i) ) = (uint8_t)block_ptr;
  }
  // end-pad block with zero
  memset(buf + buf_i, 0, p_fs->calc.block_data_len - buf_i);
  // write data
  if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, block_n, 0, p_fs->calc.block_data_len, p_fs, 0) ) < 0 )
    return status;
  if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, block_n, 0, 0, p_fs, 0) ) < 0 )
    return status;
  // get write buffer and write filesize 0 to file-index
  if( !block_buf ){
    if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FSHEAD, 0, p_fs, 0)) )
      return MINFS_ERROR_NO_BUFFER;
  } else buf = block_buf;
  *( (uint32_t*)buf ) = 0;
  if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, p_fs->calc.first_datablock, 0, 4, p_fs, 0) ) < 0 )
    return status;
  if( (status = Write(MINFS_BLOCK_TYPE_FSHEAD, buf, p_fs->calc.first_datablock, 0, 0, p_fs, 0) ) < 0 )
    return status;
  // finished
  return 0;
}

int32_t MINFS_GetFSInfo(MINFS_fs_t *p_fs, uint8_t *block_buf){
  // read fs header data
  MINFS_Read(MINFS_BLOCK_TYPE_FSHEAD, &block_buf, 0, 0, 0, p_fs, 0);
  MINFS_Read(MINFS_BLOCK_TYPE_FSHEAD, &block_buf, 0, 0, sizeof(MINFS_fs_header_t), p_fs, 0);
  if( !block_buf )
    return MINFS_ERROR_NO_BUFFER;
  // map header struct
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)block_buf;
  // check file system type
  if( p_fs_header->fs_type != MINFS_FS_TYPE )
    return MINFS_ERROR_BAD_FSTYPE;
  // copy fs-info to *p_fs struct
  p_fs->fs_info = p_fs_header->fs_info;
  // calculate fs-params
  CalcFSParams(p_fs);
  // success
  return 0;
}

int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, uint8_t *block_buf){
}

int32_t MINFS_FileRead(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf){
}

int32_t MINFS_FileWrite(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf){
}

int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint32_t pos, uint8_t *block_buf){
}

int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size, uint8_t *block_buf){
}

int32_t MINFS_Unlink(uint32_t file_id, uint8_t *block_buf){
}

int32_t MINFS_Move(uint32_t src_file_id, uint32_t dst_file_id, uint8_t *block_buf){
}

int32_t MINFS_FileExists(uint32_t file_id, uint8_t *block_buf){
}

/////////////////////////////////////////////////////////////////////////////
// Low level functions
/////////////////////////////////////////////////////////////////////////////
static void CalcFSParams(MINFS_fs_t *p_fs){
  // calculate block-pointer size
  if(p_fs->fs_info.num_blocks & 0xffff0000)
    p_fs->calc.bp_size = 4;
  else if(p_fs->fs_info.num_blocks & 0xff00)
    p_fs->calc.bp_size = 2;
  else
    p_fs->calc.bp_size = 1;
  // calculate pec-size
  uint8_t pec_val;
  if(pec_val = p_fs->fs_info.flags & MINFS_FLAGMASK_PEC)
    p_fs->calc.pec_size = 1 << (pec_val-1);
  else
    p_fs->calc.pec_size = 0;
  // calculate block_data_len
  p_fs->calc.block_data_len = (1 << p_fs->fs_info.block_size) - p_fs->calc.pec_size;
  // calculate first data-block
  uint32_t fshead_size = sizeof(MINFS_fs_header_t) + p_fs->fs_info.num_blocks * p_fs->calc.bp_size;
  p_fs->calc.first_datablock = fshead_size / p_fs->calc.block_data_len;
  if(fshead_size % p_fs->calc.block_data_len)
    p_fs->calc.first_datablock++;
}

static int32_t BlockSeek(MINFS_fs_t *p_fs, uint8_t *block_buffer, uint32_t start_block, uint32_t offset){
}

static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block, uint32_t link_to, uint8_t *block_buffer){
}

static uint32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks){
}

static uint32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t first_block){
}

static uint32_t GetFileHeader(MINFS_fs_t *p_fs, uint32_t block_n, MINFS_file_header_t *p_file_header){
}

static uint32_t Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

static uint32_t Read(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

static uint32_t GetPECValue(uint8_t *buffer, uint16_t data_len){
}










