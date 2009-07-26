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
// macros for big-endian translation and typecasted copy
/////////////////////////////////////////////////////////////////////////////

static const uint32_t BECV = 1; // dummy variable to check endianess

// swaps the value's bytes if we are on a bigendian-system. the optimizer should 
// remove the code when compiled for little-endian platforms (?)
#define BE_SWAP_16(val) if( *( (uint8_t*)(&BECV) ) == 0 ){ \
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

#define LE_SET(p_dst, src, len){ \
  *( (uint8_t*)p_dst ) = (uint8_t)src; \
  if( len > 1 ) \
    *( (uint8_t*)(p_dst + 1) ) |= (uint8_t)( src >> 8 ); \
  if( len > 2 ) \
    *( (uint8_t*)(p_dst + 2) ) |= (uint8_t)( src >> 16 ); \
  if( len > 3 ) \
    *( (uint8_t*)(p_dst + 3) ) |= (uint8_t)( src >> 24 ); \
}

// copies a value type-casted
// p_dst: pointer; src: typed var; len: number of bytes to copy

#define TC_SET(p_dst, src, len){ \
  if( len > 2 ) \
    *((uint32_t*)p_dst) = (uint32_t)src; \
  else if( len > 1 ) \
    *((uint16_t*)p_dst) = (uint16_t)src; \
  else \
    *((uint8_t*)p_dst) = (uint8_t)src; \
}

// copies a value type-casted
// dst: typed var; p_src: pointer; len: number of bytes to copy

#define TC_GET(dst, p_src, len){ \
  if( len > 2 ) \
    (uint32_t)dst = *((uint32_t*)p_src); \
  else if( len > 1 ) \
    (uint16_t)dst = *((uint16_t*)p_src); \
  else \
    (uint8_t)dst = *((uint8_t*)p_src); \
}



/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Function Hooks to caching and device layer
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Hook to OS to read data. If data_len is 0, this is an request to populate
// the whole block, else just a portion of data can be read. 
// Complete population of blocks may be not implemented if:
// 1. Portion-write in MINFS_Write is implemented (flush is not necessray).
// 2. No PEC - enabled file-system should be used. If PEC is enabled, only
//    whole blocks will be read (data_len == 0).
// Portion-read must not be ignored, instead just populate the whole block if 
// you can't/don't want to read data portion-wise.
// Once a block-buffer is completly populated, the function should set 
// p_block_buf->flags.populated (ommits further MINFS_Read-calls for this block).
//
// IN:  <p_fs> Pointer to filesystem-info structure (fs_id may be used to select the source)
//      <block_buf> Pointer to MINFS_block_buf_t structure
//      <data_offset> first byte to read into the buffer
//      <data_len> number of bytes to read into the buffer. If 0, the whole buffer has to be read
//      <file_id> Set to the file's ID if there's a file related to this read-operation
//                (file_id 0 is the file-index) else set to MINFS_FILE_NULL.
// OUT: 0 on success, else < 0
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);


/////////////////////////////////////////////////////////////////////////////
// Hook to OS to write data. If data_len is 0, this is an request to flush
// the whole buffer, else just a portion of data may be written.
// Either portion-write or flush may be implemented. If just flush-write is implemented,
// MINFS_Read *must* provide block population and set the populated-flag, else you
// will get a MINFS_ERROR_FLUSH_BNP (buffer not populated) - error.
//
// If PEC enabled file-systems should be used, flush-write *must* be implemented,
// as only whole blocks will be written (data_len == 0). No data will be written
// if PEC is enabled and flush-write is not implemented.
// If data was written like requested, the function should reset p_block_buf->flags.changed to
// signal that there are no more unsaved changes in the block-buffer (ommits
// further MINFS_Write-calls until changed will be set again).
//
// IN:  <p_fs> Pointer to filesystem-info structure(fs_id may be used to select the target)
//      <block_buf> Pointer to MINFS_block_buf_t structure
//      <data_offset> first byte to write
//      <data_len> number of bytes to write
//      <file_id> Set to the file's ID if there's a file related to this write-operation
//                (file_id 0 is the file-index) else set to MINFS_FILE_NULL.
// OUT: 0 on success, else < 0
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);


/////////////////////////////////////////////////////////////////////////////
// Hook to OS to provide a data buffer. This function is called even if
// we already have a buffer-struct (e.g. passed to a MINFS_XXX function), but
// the block_n of the buffer differs from the one needed to write/read.
// If this function is not implemented, the buffers must be passed to the
// MINFS_XXX functions directly.
// If the returned (*pp_block_buf)->block_n != block_n passed to the function,
// the flags of the buffer will be reset and block_n set. Be sure to initialize
// buffer structures ( MINFS_InitBlockBuffer ) before using them. Also make sure
// you assign a valid buffer-pointer to (*pp_block_buf)->p_buf (buffer - size
// must be at least 2^p_fs->fs_info.block_size )
//
// IN:  <p_fs> Pointer to filesystem-info structure.
//      <block_buf> Pointer to MINFS_block_buf_t pointer, if NULL, the buffer-struct 
//                  has to be provided by this function. The buffer's  size has to be 
//                  at least data_offset+data_len, or 2^p_fs_info->block_size,  if data_len==0.
//      <block_n>   Block number (0-x), that the buffer will be used for
//      <file_id> Set to the file's ID if there's a file related to the block
//                (file_id 0 is the file-index) else set to MINFS_FILE_NULL.
//                May be used for an advanced caching implementation.
// OUT: 0 on success, else < 0
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_GetBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id);



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static int32_t CalcFSParams(MINFS_fs_t *p_fs);

static int32_t BlockSeek(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t offset, MINFS_block_buf_t **pp_block_buf);

static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, MINFS_block_buf_t **pp_block_buf);

static int32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks, MINFS_block_buf_t **pp_block_buf);
static int32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t start_block, MINFS_block_buf_t **pp_block_buf);

static int32_t FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t **pp_block_buf);

static int32_t GetBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id, uint8_t populate);
static int32_t FlushBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf);

static int32_t Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);
static int32_t Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);
static uint32_t GetPECValue(MINFS_fs_t *p_fs, uint8_t *p_buf, uint16_t data_len);

/////////////////////////////////////////////////////////////////////////////
// High level functions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Initialized a MINFS_block_buf_t struct. The buffer pointer p_block_buf->p_buf
// has to be set by the caller. Each MINFS_block_buf_t struct must be initialized
// this way before first use (by Application / OS layer).
// The function does not initialize the p_buf-pointer, it has to be set by the
// caller to a valid buffer.
// 
// IN:  <p_block_buf> pointer to a MINFS_block_buf_t - struct.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_InitBlockBuffer(MINFS_block_buf_t *p_block_buf){
  // valid buffer pointer?
  if( p_block_buf == NULL )
    return MINFS_ERROR_NO_BUFFER;
  // reset flags and block_n
  p_block_buf->flags.ALL = 0;
  p_block_buf->block_n = MINFS_BLOCK_NULL;
  // success
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Flushes a block-buffer if the changed-flag is set. This has to be called
// at the end of each MINFS-session for each used buffer that was changed
// (changed-flag set). Only if the MINFS_Write - implementation does write
// the single data poritions (not only whole blocks) consequently, flush can
// be ommited as all data will be written immediately after it was changed.
// NOTE: MINFS_Write has to reset the changed flag!
// 
// IN:   <p_fs> Pointer to filesystem-info structure.
//       <p_block_buf> pointer to a MINFS_block_buf_t - struct.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FlushBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  // valid buffer pointer?
  if( p_block_buf == NULL )
    return MINFS_ERROR_NO_BUFFER;
  // valid block_n ?
  if( p_block_buf->block_n >= p_fs->fs_info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // valid buffer pointer?
  if( p_block_buf == NULL )
    return MINFS_ERROR_NO_BUFFER;
  return FlushBlockBuffer(p_fs, p_block_buf);
}

/////////////////////////////////////////////////////////////////////////////
// Formats a file-system (fs_id): [FS-header : block-map : data-blocks]
// The file starting with data-block 0 will contain the block-pointers to
// the first data-blocks of the files.
// The free-blocks chain starts with the virtual data-block <num_blocks>:
// data-block <num_blocks> -> 
// data-block 1 -> data-block 2 -> .. -> data block <num_blocks - 1>
// 
// unused data sections will not be initialized with 0
//
// IN:  <p_fs> Pointer to a fs-structure. <fs_id> and <fs_info> must be populated
//      <block_buf> pointer to a block-sized buffer. If it is NULL, 
//                  WriteBufGet must provide the buffer
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_Format(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // calculate and validate fs-params
  if( status = CalcFSParams(p_fs) )
    return status;
  // get buffer
  if( status = GetBlockBuffer(p_fs, &p_block_buf, 0, 0, 0) )
    return status; // return error status
  uint16_t buf_i = 0; // byte offset in data buffer
  // put fs-header data to buffer
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)(p_block_buf->p_buf);
  p_fs_header->fs_type = MINFS_FS_TYPE;
  BE_SWAP_32(p_fs_header->fs_type); // conditional byte-swap
  p_fs_header->fs_info = p_fs->fs_info;
  BE_SWAP_32(p_fs_header->fs_info.num_blocks); // conditional byte-swap
  BE_SWAP_16(p_fs_header->fs_info.os_flags); // conditional byte-swap
  buf_i += sizeof(MINFS_fs_header_t);
  // now write the block-pointer map
  // add a virtual block (start of free-blocks-chain)
  uint32_t i, block_map_entry, block_buf_n = 0;
  for(i = p_fs->calc.first_datablock ; 1 ; i++){
    // check if current buffer is full or if we are finished with the block-map
    if( buf_i >= p_fs->calc.block_data_len || i > p_fs->fs_info.num_blocks ){
      // write data
      if( status = Write(p_fs, p_block_buf, 0, p_fs->calc.block_data_len) )
        return status;
      // increment block_buf_n, set offset to 0
      block_buf_n++;
      buf_i = 0;
      // get a buffer for the next block
      if( status = GetBlockBuffer(p_fs, &p_block_buf, block_buf_n, 0, 0) )
	return status; // return error status
     // quit the loop here if we wrote the last block-map buffer
      // the buffer we got last will be used for the file-index first block
      if( i > p_fs->fs_info.num_blocks )
        break;
    }
    // calculate block pointer and write it to the buffer. 0 is file-index-start, 
    // free blocks chain: p_fs->fs_info.num_blocks, 1, 2, .., p_fs->fs_info.num_blocks - 1
    if ( i > 0 && i < p_fs->fs_info.num_blocks - 1 )
      block_map_entry = i + 1; // free blocks chain
    else if( i == p_fs->fs_info.num_blocks ) // start of free blocks chain (virtual block)
      block_map_entry = p_fs->calc.first_datablock; 
    else
      block_map_entry = MINFS_BLOCK_EOC; // EOC
    BE_SWAP_32(block_map_entry);
    // copy little-endian value
    *( (uint32_t*)(p_block_buf->p_buf + buf_i) ) = block_map_entry;
    buf_i += p_fs->calc.bp_size;
  }
  // write filesize 0 to file-index
  *( (uint32_t*)p_block_buf->p_buf ) = 0;
  if( status = Write(p_fs, p_block_buf, 0, 4) )
    return status;
  // finished
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Opens a file-system p_fs->fs_id and populates the *p_fs struct
//
// IN:  <p_fs> Pointer to a fs-structure. <fs_id> must be set
//      <block_buf> pointer to a block-sized buffer. If it is NULL, 
//                  MINFS_Read must provide the buffer
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FSOpen(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // prepare fs-struct
  p_fs->fs_info.block_size = 4; // minimal block size
  p_fs->fs_info.flags = 0;
  // get buffer
  if( status = GetBlockBuffer(p_fs, &p_block_buf, 0, 0, 0) )
    return status; // return error status
  // read fs header data
  if( status = Read(p_fs, p_block_buf, 0, sizeof(MINFS_fs_header_t) ) )
    return 0;
  // map header struct
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)(p_block_buf->p_buf);
  // check file system type
  BE_SWAP_32(p_fs_header->fs_type); // conditional byte swap
  if( p_fs_header->fs_type != MINFS_FS_TYPE )
    return MINFS_ERROR_FS_TYPE;
  // copy fs-info to *p_fs struct
  p_fs->fs_info = p_fs_header->fs_info;
  BE_SWAP_32(p_fs->fs_info.num_blocks); // conditional byte swap
  BE_SWAP_16(p_fs->fs_info.os_flags); // conditional byte swap
  // calculate and validate fs-params
  if( status = CalcFSParams(p_fs) )
    return status;
  // success
  return 0;
}

int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_FileRead(MINFS_file_t *p_file, uint8_t *p_buf, uint32_t len, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_FileWrite(MINFS_file_t *p_file, uint8_t *p_buf, uint32_t len, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint32_t pos, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_Unlink(uint32_t file_id, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_Move(uint32_t src_file_id, uint32_t dst_file_id, MINFS_block_buf_t *p_block_buf){
}

int32_t MINFS_FileExists(uint32_t file_id, MINFS_block_buf_t *p_block_buf){
}

/////////////////////////////////////////////////////////////////////////////
// Low level functions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Populates a structure with pre-calculated parameters for the file-system
// 
// IN:  <p_fs> Pointer to a populated fs-structure. p_fs->calc will be populated
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
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
    p_fs->calc.pec_width = 1 << (pec_val-1);
  else
    p_fs->calc.pec_width = 0;
  // calculate block_data_len
  p_fs->calc.block_data_len = (1 << p_fs->fs_info.block_size) - p_fs->calc.pec_width;
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

/////////////////////////////////////////////////////////////////////////////
// Seeks in a data-block-chain starting with block_n
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_n> Block number, seek start
//      <offset> How much elements to advance in the chain 
//               (MINFS_SEEK_END finds last element != EOC)
//       <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: block pointer on success (may be MINFS_BLOCK_EOC), else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t BlockSeek(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t offset, MINFS_block_buf_t **pp_block_buf){
  // check if block_n is a data-block (failsafe)
  if( block_n < p_fs->calc.first_datablock || block_n > p_fs->fs_info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // seek blocks
  uint32_t block_chain_block_n, last_block_n;
  uint16_t block_chain_entry_offset;
  int32_t status;
  while( offset > 0 ){
    // calculate block and offset where the next block-pointer is stored
    block_chain_entry_offset = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * (block_n - p_fs->calc.first_datablock); // chain-pointer offset from beginning of fs
    block_chain_block_n = block_chain_entry_offset / p_fs->calc.block_data_len; // block where the chain pointer resides
    block_chain_entry_offset %= p_fs->calc.block_data_len; // chain-pointer offset from beginning of block
    // get buffer
    if( status = GetBlockBuffer(p_fs, pp_block_buf, block_chain_block_n, 0, 0) )
      return status; // return error status
    // read fs header data
    if( status = Read(p_fs, *pp_block_buf, block_chain_entry_offset, p_fs->calc.bp_size) )
      return status;
    // get block pointer
    last_block_n = block_n;
    LE_GET(block_n, (*pp_block_buf)->p_buf + block_chain_entry_offset, p_fs->calc.bp_size);
    // check if EOC
    if( block_n == MINFS_BLOCK_EOC ){
      // if seek-end requested, return the last block number
      if( offset = MINFS_SEEK_END )
        return last_block_n;
      // offset was not reached, return EOC
      return MINFS_BLOCK_EOC;
    }
    // goto next element
    offset--;
  }
  // reached the block-offset
  return block_n;
}

/////////////////////////////////////////////////////////////////////////////
// Links a block in the blocks-chain-map to a target block
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_n> Block number to link from
//      <block_target> Block number to link to
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: 0, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t BlockLink(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, MINFS_block_buf_t **pp_block_buf){
  // check if block_n is a data-block (failsafe)
  if( block_n < p_fs->calc.first_datablock || block_n > p_fs->fs_info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // calculate block and offset where the next block-pointer is stored
  uint16_t block_chain_entry_offset = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * (block_n - p_fs->calc.first_datablock); // chain-pointer offset from beginning of fs
  uint32_t block_chain_block_n = block_chain_entry_offset / p_fs->calc.block_data_len; // block where the chain pointer resides
  block_chain_entry_offset %= p_fs->calc.block_data_len; // chain-pointer offset from beginning of block
  // get buffer
  int32_t status;
  if( status = GetBlockBuffer(p_fs, pp_block_buf, block_chain_block_n, 0, 1) )
    return status; // return error status
  // write the block-chain-entry
  LE_SET( (*pp_block_buf + block_chain_entry_offset), block_target, p_fs->calc.bp_size);
  if( status = Write(p_fs, *pp_block_buf, block_chain_entry_offset, p_fs->calc.bp_size) )
    return status;
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Pops num_block from the free-block-chain and returns the first block number
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <num_blocks> Number of free blocks to pop
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: First block number, on error < 0 (MINFS_ERROR_XXXX or MINFS_STATUS_FULL)
/////////////////////////////////////////////////////////////////////////////
static int32_t PopFreeBlocks(MINFS_fs_t *p_fs, uint32_t num_blocks, MINFS_block_buf_t **pp_block_buf){
  if( num_blocks == 0 )
    return MINFS_BLOCK_EOC; // return EOC-pointer
  // seek first block to pop off the fbc 
  // p_fs->fs_info.num_blocks is the virtual start block of the fbc
  int32_t start_block;
  if( (start_block = BlockSeek(p_fs, p_fs->fs_info.num_blocks, 1, pp_block_buf)) < 0 )
    return start_block; // return error status
  else if(start_block == MINFS_BLOCK_EOC)
    return MINFS_STATUS_FULL; // no more free blocks
  // seek last block to pop off the fbc
  int32_t end_block;
  if( num_blocks > 1 ){
    if( (end_block  = BlockSeek(p_fs, start_block, num_blocks-1, pp_block_buf)) < 0 )
      return end_block; // return error status
    else if(end_block == MINFS_BLOCK_EOC)
      return MINFS_STATUS_FULL; // can't get the requested number of free blocks
  } else end_block = start_block;
  // now get the block-number next in chain after the last block to pop off the fbc
  int32_t fbc_cont_block;
  if( (fbc_cont_block = BlockSeek(p_fs, end_block, 1, pp_block_buf)) < 0 )
    return fbc_cont_block; // return error status
  // set the new block-chain-entries
  // set entry for last poped block to EOC
  int32_t status;
  if( status = BlockLink(p_fs, end_block, MINFS_BLOCK_EOC, pp_block_buf) )
    return status;
  // continue free blocks chain at entry after the last popped block
  if( status = BlockLink(p_fs, p_fs->fs_info.num_blocks, fbc_cont_block, pp_block_buf) )
    return status;
  return 0;  
}


/////////////////////////////////////////////////////////////////////////////
// Adds a block-chain (start_block -> EOC) to the free-blocks-chain
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <start_block> Start-block of the chain
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: First block number, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t PushFreeBlocks(MINFS_fs_t *p_fs, uint32_t start_block, MINFS_block_buf_t **pp_block_buf){
  // check if start_block is a data-block (failsafe)
  if( start_block < p_fs->calc.first_datablock || start_block > p_fs->fs_info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // find last block of the chain
  uint32_t end_block;
  if( (end_block = BlockSeek(p_fs, start_block, MINFS_SEEK_END, pp_block_buf)) < 0 )
    return end_block; // return error status
  // find the second element in the free-blocks chain
  uint32_t fbc_cont_block;
  if( (fbc_cont_block = BlockSeek(p_fs, p_fs->fs_info.num_blocks, 1, pp_block_buf)) < 0 )
    return fbc_cont_block; // return error status
  // link fbc first (virtual) block to start_block
  int32_t status;
  if( status = BlockLink(p_fs, p_fs->fs_info.num_blocks, start_block, pp_block_buf) )
    return status;
  // link end_block to the former second element of free-blocks
  if( status = BlockLink(p_fs, end_block, fbc_cont_block, pp_block_buf) )
    return status;
  // success
  return 0;
}

static int32_t FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t **pp_block_buf){


}

/////////////////////////////////////////////////////////////////////////////
// Returns a buffer. Will be called before any call to Read or Write, at least
// one time for each block that should be read from / wrote to.
// The function populates the buffer when needed. If *p_block_buf points to 
// a buffer that was changed, it will be flushed.
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
//      <block_n> Block number the buffer is used for
//      <populate> If > 0, the returned buffer must be populated (for writes) 
//      <file_id> The file's ID if the block is part of a file
//                (file_id 0 is the file-index) else set to MINFS_FILE_NULL.
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
// TODO: Optimize this function 
static int32_t GetBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id, uint8_t populate){
  int32_t status;
  // passed buffer was already used?
  if( *pp_block_buf != NULL && (*pp_block_buf)->block_n != MINFS_BLOCK_NULL ){
    // return if buffer is already valid (right block number, poplulated if this was requested)
    if( ( (*pp_block_buf)->block_n ) == block_n && ( populate ? (*pp_block_buf)->flags.populated : 1) )
      return 0;
    // flush the block-buffer if (*p_block_buf)->flags.changed )
    if( status = FlushBlockBuffer(p_fs, *pp_block_buf) )
      return status; // return error status
    }
  // if buffer is not set at all, or has wrong block_n, get it by OS-hook
  if( *pp_block_buf == NULL || (*pp_block_buf)->block_n != block_n ){
    // get the buffer by OS-hook
    if( status = MINFS_GetBlockBuffer(p_fs, pp_block_buf, block_n, file_id) )
      return status; // return error status
    if( *pp_block_buf == NULL )
      return MINFS_ERROR_NO_BUFFER; // could not get buffer
    }
  // if the buffer was not used for block_n, reset flags and set block_n
  if( block_n != (*pp_block_buf)->block_n ){
    (*pp_block_buf)->flags.ALL = 0;
    (*pp_block_buf)->block_n = block_n;
    }
  // populate the buffer if requested and not already populated
  if( populate && !((*pp_block_buf)->flags.populated) ){
    if( status = Read(p_fs, *pp_block_buf, 0, 0) )
      return status; // return error status
    }
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Flushes a buffer (only if changed-flag is set).
// The application / OS layer is responsible to call MINFS_FlushBlockBuffer
// the last time. Each call to GetBlockBuffer will flush the passed
// buffer if a new one should be used (different block-number).
// NOTE: MINFS_Write has to reset the changed flag!
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t FlushBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  // check if buffer has changed -> write whole buffer
  // NOTE: MINFS_Write has to reset the changed flag!
  if( p_block_buf->flags.changed ){
    if( !p_block_buf->flags.populated )
      return MINFS_ERROR_FLUSH_BNP; // buffer was not populated
    int32_t status;
    if( status = Write(p_fs, p_block_buf, 0, 0) )
      return status; // return error status
  }
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Request to write a data portion or to flush the buffer (data_len == 0).
// FlushBlockBuffer is the only function that calls Write with data_len = 0,
// because MINFS_Write may ignore flush requests if it writes data portions
// immediately. All other functions call Write for each data-portion to
// write.
// The changed-flag of the buffer will be set. MINFS_Write has to reset this
// flag after each processed write request.
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
//      <data_offset> First byte in block to write
//      <data_len> Number of bytes to write. If 0, this is a request to flush the whole buffer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  // set the changed-flag
  // NOTE: MINFS_Write has to reset the changed flag!
  p_block_buf->flags.changed = 1;
  // if PEC is enabled, only whole-block-writes (flush) will be forwarded
  if( p_fs->fs_info.flags & MINFS_FLAGMASK_PEC ){
    if( data_len )
      return 0;
    // caculate and set PEC-value
    uint32_t pec = GetPECValue(p_fs, p_block_buf->p_buf, p_fs->calc.block_data_len);
    TC_SET( (p_block_buf->p_buf + p_fs->calc.block_data_len), pec, p_fs->calc.pec_width );
  }
  // write buffer
  int32_t status;
  if( status = MINFS_Write(p_fs, p_block_buf, data_offset, data_len) )
    return status; // return error status
  // success
  return 0;
}



/////////////////////////////////////////////////////////////////////////////
// Request to read a data portion or to populate the entire buffer (data_len == 0).
// GetBlockBuffer is the only function that calls Read with data_len = 0,
// because MINFS_Read may ignore population requests if MINFS_Write writes data portions
// immediately, and PEC will not be used. All other functions call Read for each 
// data-portion to read.
// If the buffer is already populated, the function will return immediately.
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
//      <data_offset> First byte in block to read
//      <data_len> Number of bytes to read. If 0, this is a request to populate the whole buffer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  // if the buffer is populated, nothing has to be read
  if( p_block_buf->flags.populated )
    return 0;
  // If PEC is enabled, only reads of a whole block are valid (populate)
  if( p_fs->fs_info.flags & MINFS_FLAGMASK_PEC )
    data_len = 0;
  // NOTE: MINFS_Read has to set the populated - flag after a whole-block was read!
  int32_t status;
  if( status = MINFS_Read(p_fs, p_block_buf, data_offset, data_len) )
    return status; // return error status
  // if PEC is enabled, a populate-request must not be ignored
  if( (p_fs->fs_info.flags & MINFS_FLAGMASK_PEC) && !p_block_buf->flags.populated )
    return MINFS_ERROR_READ_BNP;
  // check PEC value if PEC is enabled
  if( p_fs->fs_info.flags & MINFS_FLAGMASK_PEC ){
    if ( GetPECValue(p_fs, p_block_buf->p_buf, p_fs->calc.block_data_len + p_fs->calc.pec_width) )
      return MINFS_ERROR_PEC; // PEC value was not 0
  }
  // success
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Calculates the PEC - value for the data in *p_buf (data_len bytes)
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <p_buf> Pointer to a data buffer
//      <data_len> Number of bytes to build the PEC - value from
// OUT: PEC - value
/////////////////////////////////////////////////////////////////////////////
static uint32_t GetPECValue(MINFS_fs_t *p_fs, uint8_t *p_buf, uint16_t data_len){
  // PEC value generation: the PEC of valid data incl. PEC must be zero.
  return 0;
}










