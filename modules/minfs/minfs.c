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
#include "minfs.h"

/////////////////////////////////////////////////////////////////////////////
// macros for big-endian translation 
/////////////////////////////////////////////////////////////////////////////

static const uint32_t BECV = 1; // dummy variable to check endianess

// swaps the value's bytes if we are on a bigendian-system. the optimizer should 
// remove the code when compiled for little-endian platforms (?)
#define BE_SWP_16(val) if( *( (uint8_t*)(&BECV) ) == 0 ){ \
  *(uint8_t*)(&val) ^= *(uint8_t*)(&val + 1); \
  *(uint8_t*)(&val + 1) ^= *(uint8_t*)(&val); \
  *(uint8_t*)(&val) ^= *(uint8_t*)(&val + 1); \
}

#define BE_SWAP_32(val) if( *( (uint8_t*)(&BECV) ) == 0 ){ \
  *(uint8_t*)(&val) ^= *(uint8_t*)(&val + 3); \
  *(uint8_t*)(&val + 3) ^= *(uint8_t*)(&val); \
  *(uint8_t*)(&val) ^= *(uint8_t*)(&val + 3); \
  *(uint8_t*)(&val + 1) ^= *(uint8_t*)(&val + 2); \
  *(uint8_t*)(&val + 2) ^= *(uint8_t*)(&val + 1); \
  *(uint8_t*)(&val + 1) ^= *(uint8_t*)(&val + 2); \
}

// copies a value from little-endian format to the platform's format
// dst: typed var; p_src: pointer; len: number of bytes to copy

#define LE_GET(dst, p_src, len){ \
  dst = *( (uint8_t*) (p_src)  ); \
  if( len > 1 ) \
    dst |= (uint16_t)( *( (uint8_t*) (p_src + 1)  ) ) << 8; \
  if( len > 2 ) \
    dst |= (uint32_t)( *( (uint8_t*) (p_src + 2)  ) ) << 16; \
  if( len > 3 ) \
    dst |= (uint32_t)( *( (uint8_t*) (p_src + 3)  ) ) << 24; \
}

// copies a value from the platform's format to little-endian format.
// p_dst: pointer; src: typed var; len: number of bytes to copy

#define LE_PUT(p_dst, src, len){ \
  *( (uint8_t*)p_dst ) = (uint8_t)src; \
  if( len > 1 ) \
    *( (uint8_t*)(p_dst + 1) ) |= (uint8_t)( src >> 8 ); \
  if( len > 2 ) \
    *( (uint8_t*)(p_dst + 2) ) |= (uint8_t)( src >> 16 ); \
  if( len > 3 ) \
    *( (uint8_t*)(p_dst + 3) ) |= (uint8_t)( src >> 24 ); \
}


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
//                  if data_len==0. If 
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

static int32_t CalcFSParams(MINFS_fs_t *p_fs);

static int32_t BlockSeek(MINFS_fs_t *p_fs, uint8_t *block_buf, uint32_t block_n, uint32_t offset);

// initialize buf_block_n_cache with MINFS_BLOCK_NULL before the first call. after the last link is set, the caller must
// call it once more with block_n == MINFS_BLOCK_NULL to trigger a write command to write the whole block
static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, uint8_t *block_buf, uint32_t *buf_block_n_cache, uint8_t **block_buf_cache);

static int32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks);
static int32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t first_block);

// gets file header information
static int32_t FileOpen(MINFS_fs_t *p_fs, uint32_t block_n, MINFS_file_t *p_file);

static int32_t Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);
static int32_t Read(uint8_t block_type, uint8_t **buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id);
static uint32_t GetPECValue(uint8_t *buffer, uint16_t data_len);

/////////////////////////////////////////////////////////////////////////////
// High level functions
/////////////////////////////////////////////////////////////////////////////

int32_t MINFS_Format(MINFS_fs_t *p_fs, uint8_t *block_buf){
  uint8_t *buf;
  int32_t status;
  // calculate and validate fs-params
  if( status = CalcFSParams(p_fs) )
    return status;
  // get a write buffer
  if( !block_buf ){
    if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FS, 0, p_fs, 0)) )
      return MINFS_ERROR_NO_BUFFER;
  } 
  else buf = block_buf;
  uint16_t buf_i = 0; // byte index in data buffer
  // put fs-header data to buffer
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)buf;
  p_fs_header->fs_type = MINFS_FS_TYPE;
  BE_SWAP_32(p_fs_header->fs_type); // conditional byte-swap
  p_fs_header->fs_info = p_fs->fs_info;
  BE_SWAP_32(p_fs_header->fs_info.num_blocks); // conditional byte-swap
  BE_SWP_16(p_fs_header->fs_info.os_flags); // conditional byte-swap
  buf_i += sizeof(MINFS_fs_header_t);
  // now write the block-pointer map
  uint32_t i, block_chain_entry, block_n = 0;
  // add a virtual block (start of free-blocks-chain)
  for(i = 0 ; i <= p_fs->fs_info.num_blocks ; i++){
    // check if buffer is full
    if( buf_i >= p_fs->calc.block_data_len ){
      // write data
      if( status = Write(MINFS_BLOCK_TYPE_FS, buf, block_n, 0, p_fs->calc.block_data_len, p_fs, 0) )
        return status;
      if( status = Write(MINFS_BLOCK_TYPE_FS, buf, block_n, 0, 0, p_fs, 0) )
        return status;
      // get a write buffer
      if( !block_buf ){
	if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FS, 0, p_fs, 0)) )
	  return MINFS_ERROR_NO_BUFFER;
      } else buf = block_buf;
      // set block_n and buffer byte-index
      block_n++;
      buf_i = 0;
    }
    // calculate block pointer and write it to the buffer. 0 is file-index-start, 
    // free blocks chain: p_fs->fs_info.num_blocks, 1, 2, .., p_fs->fs_info.num_blocks - 1
    if ( i > 0 && i < p_fs->fs_info.num_blocks - 1 )
      block_chain_entry = i + 1; // free blocks chain
    else if( i == p_fs->fs_info.num_blocks )
      block_chain_entry = 1; // start of free blocks chain (virtual block)
    else
      block_chain_entry = 0; // EOC
    BE_SWAP_32(block_chain_entry);
    // copy little-endian value
    *( (uint32_t*)(buf + buf_i) ) = block_chain_entry;
    buf_i += p_fs->calc.bp_size;
  }
  // write data
  if( status = Write(MINFS_BLOCK_TYPE_FS, buf, block_n, 0, p_fs->calc.block_data_len, p_fs, 0) )
    return status;
  if( status = Write(MINFS_BLOCK_TYPE_FS, buf, block_n, 0, 0, p_fs, 0) )
    return status;
  // get write buffer and write filesize 0 to file-index
  if( !block_buf ){
    if( !(buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FS, 0, p_fs, 0)) )
      return MINFS_ERROR_NO_BUFFER;
  } else buf = block_buf;
  *( (uint32_t*)buf ) = 0;
  if( status = Write(MINFS_BLOCK_TYPE_FS, buf, p_fs->calc.first_datablock, 0, 4, p_fs, 0) )
    return status;
  if( status = Write(MINFS_BLOCK_TYPE_FS, buf, p_fs->calc.first_datablock, 0, 0, p_fs, 0) )
    return status;
  // finished
  return 0;
}

int32_t MINFS_FSOpen(MINFS_fs_t *p_fs, uint8_t *block_buf){
  // prepare fs-struct
  p_fs->fs_info.block_size = 4; // minimal block size
  p_fs->fs_info.flags = 0;
  // read fs header data
  Read(MINFS_BLOCK_TYPE_FS, &block_buf, 0, 0, 0, p_fs, 0);
  Read(MINFS_BLOCK_TYPE_FS, &block_buf, 0, 0, sizeof(MINFS_fs_header_t), p_fs, 0);
  if( !block_buf )
    return MINFS_ERROR_NO_BUFFER;
  // map header struct
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)block_buf;
  // check file system type
  BE_SWAP_32(p_fs_header->fs_type); // conditional byte swap
  if( p_fs_header->fs_type != MINFS_FS_TYPE )
    return MINFS_ERROR_FS_TYPE;
  // copy fs-info to *p_fs struct
  p_fs->fs_info = p_fs_header->fs_info;
  BE_SWAP_32(p_fs->fs_info.num_blocks); // conditional byte swap
  BE_SWP_16(p_fs->fs_info.os_flags); // conditional byte swap
  // calculate and validate fs-params
  int32_t status;
  if( status = CalcFSParams(p_fs) )
    return status;
  // success
  return 0;
}

int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, uint8_t *block_buf){
}

int32_t MINFS_FileRead(MINFS_file_t *p_file, uint8_t *buffer, uint32_t *len, uint8_t *block_buf){
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
static int32_t CalcFSParams(MINFS_fs_t *p_fs){
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
  p_fs->calc.first_datablock = fshead_size / p_fs->calc.block_data_len + (fshead_size % p_fs->calc.block_data_len ? 1 : 0);
  // check fs-info values
  if( p_fs->fs_info.num_blocks & 0xFFF00000 || p_fs->fs_info.num_blocks < p_fs->calc.first_datablock + 1 )
    return MINFS_ERROR_NUM_BLOCKS;
  if( p_fs->fs_info.block_size > 12 || p_fs->fs_info.block_size < 4)
    return MINFS_ERROR_BLOCK_SIZE;
  // validated
  return 0;
}

static int32_t BlockSeek(MINFS_fs_t *p_fs, uint8_t *block_buf, uint32_t block_n, uint32_t offset){
  uint32_t block_chain_block_n, last_block_chain_block_n = 0;
  uint16_t block_chain_entry_offset;
  int32_t status;
  while( offset > 0 ){
    // check if EOC
    if( block_n == 0 )
      return MINFS_STATUS_EOC;
    // calculate block and offset where the next block-pointer is stored
    block_chain_block_n = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * block_n;
    block_chain_entry_offset = block_chain_block_n % p_fs->calc.block_data_len; // chain-pointer offset from beginning of block
    block_chain_block_n /= p_fs->calc.block_data_len; // block where the chain pointer resides
    // read block (whole block only once)
    if( last_block_chain_block_n != block_chain_block_n ){
      if( status = Read(MINFS_BLOCK_TYPE_FS, &block_buf, block_chain_block_n, 0, 0, p_fs, 0) )
        return status;
      last_block_chain_block_n = block_chain_block_n;
    }
    if( status = Read(MINFS_BLOCK_TYPE_FS, &block_buf, block_chain_block_n, block_chain_entry_offset, p_fs->calc.bp_size, p_fs, 0) )
      return status;
    if( !block_buf )
      return MINFS_ERROR_NO_BUFFER;
    // get block pointer
    LE_GET(block_n, block_buf + block_chain_entry_offset, p_fs->calc.bp_size);
    // goto next element
    offset--;
  }
  // reached the block-offset
  return block_n;
}

static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, uint8_t *block_buf, uint32_t *buf_block_n_cache, uint8_t **block_buf_cache){
  // calculate block and offset where the next block-pointer is stored
  uint32_t block_chain_block_n;
  uint16_t block_chain_entry_offset;
  if( block_n != MINFS_BLOCK_NULL ){
    block_chain_block_n = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * block_n;
    block_chain_entry_offset = block_chain_block_n % p_fs->calc.block_data_len; // offset from block_chain_block_n begin to the chain pointer
    block_chain_block_n /= p_fs->calc.block_data_len; // block where the chain pointer resides
  } else
    block_chain_block_n = MINFS_BLOCK_NULL;
  // check if entire-block write is necessary
  int32_t status;
  if(*buf_block_n_cache != MINFS_BLOCK_NULL && *buf_block_n_cache != block_chain_block_n){
    if( status = Write(MINFS_BLOCK_TYPE_FS, *block_buf_cache, *buf_block_n_cache, 0, 0, p_fs, 0) )
      return status;
  }
  // return if block is NULL (nothing more to do)
  if( block_n == MINFS_BLOCK_NULL )
    return 0;
  // if the data in *block_buf is not already initialized, fetch whole block now
  if( *buf_block_n_cache != block_chain_block_n ){
    // get a buffer if NULL - buffer was passed
    if( !block_buf ){
      if( !(block_buf = MINFS_WriteBufGet(MINFS_BLOCK_TYPE_FS, block_chain_block_n, p_fs, 0)) )
	return MINFS_ERROR_NO_BUFFER;
    }
    // read the whole block
    if( status = Read(MINFS_BLOCK_TYPE_FS, &block_buf, block_chain_block_n, 0, 0, p_fs, 0) )
      return status;
    // cache block-buffer-pointer and block-n
    *block_buf_cache = block_buf;
    *buf_block_n_cache = block_chain_block_n;
  }
  // write the block-chain-entry
  LE_PUT( (block_buf + block_chain_entry_offset), block_target, p_fs->calc.bp_size);
  if( status = Write(MINFS_BLOCK_TYPE_FS, block_buf, block_chain_block_n, block_chain_entry_offset, p_fs->calc.bp_size, p_fs, 0) )
    return status;
  // success
  return 0;
}

static int32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks){
}

static int32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t first_block){
}

static int32_t FileOpen(MINFS_fs_t *p_fs, uint32_t block_n, MINFS_file_t *p_file){
}

static int32_t Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

static int32_t Read(uint8_t block_type, uint8_t **buffer, uint32_t block_n, uint16_t data_offset, uint16_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

static uint32_t GetPECValue(uint8_t *buffer, uint16_t data_len){
}










