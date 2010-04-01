/*
 * MINFS
 *
 * Minimal Filesystem
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

//------------------------------------------------------------------------------
//---------- macros for big-endian translation and typecasted copy -------------
//------------------------------------------------------------------------------


static const uint32_t BECV = 1; // dummy value to check endianes

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

// copies 1-4 bytes from little-endian format to the platform's format
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

// copies 1-4 bytes from the platform's format to little-endian format.
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

// copies 1-4 bytes type-casted
// p_dst: pointer; src: typed var; len: number of bytes to copy

#define TC_SET(p_dst, src, len){ \
  if( len > 2 ) \
    *((uint32_t*)p_dst) = (uint32_t)src; \
  else if( len > 1 ) \
    *((uint16_t*)p_dst) = (uint16_t)src; \
  else \
    *((uint8_t*)p_dst) = (uint8_t)src; \
}

// copies 1-4 bytes type-casted
// dst: typed var; p_src: pointer; len: number of bytes to copy

#define TC_GET(dst, p_src, len){ \
  if( len > 2 ) \
    (uint32_t)dst = *((uint32_t*)p_src); \
  else if( len > 1 ) \
    (uint16_t)dst = *((uint16_t*)p_src); \
  else \
    (uint8_t)dst = *((uint8_t*)p_src); \
}


//------------------------------------------------------------------------------
//------------------------------- Local definitions ----------------------------
//------------------------------------------------------------------------------

// modes
#define MINFS_RW_MODE_READ 0
#define MINFS_RW_MODE_WRITE 1


//------------------------------------------------------------------------------
//------------------- Function Hooks to caching and device layer ---------------
//------------------------------------------------------------------------------

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
// must be at least 2^p_fs->info.block_size )
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


//------------------------------------------------------------------------------
//----------------------------- Local prototypes -------------------------------
//------------------------------------------------------------------------------

// helper functions
static int32_t CalcFSParams(MINFS_fs_t *p_fs);
static uint32_t GetPECValue(MINFS_fs_t *p_fs, void *p_buf, uint16_t data_len);


// File-index functions
static int32_t File_GetFilePointer(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf);
static int32_t File_SetFilePointer(MINFS_file_t *p_file_0, uint32_t file_id, uint32_t block_n, MINFS_block_buf_t **pp_block_buf);
static int32_t File_Unlink(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf);
static int32_t File_Touch(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf);

// File functions
static int32_t File_Open(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t **pp_block_buf);
static int32_t File_Seek(MINFS_file_t *p_file, uint32_t pos, MINFS_block_buf_t **pp_block_buf);
static int32_t File_SetSize(MINFS_file_t *p_file, uint32_t new_size, MINFS_block_buf_t **pp_block_buf);
static int32_t File_ReadWrite(MINFS_file_t *p_file, void *p_buf, uint32_t *p_len, uint8_t mode, MINFS_block_buf_t **pp_block_buf);
static int32_t File_HeaderWrite(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t file_id, uint32_t file_size, MINFS_block_buf_t **pp_block_buf);

// Block chain layer
static int32_t BlockChain_Seek(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t offset, MINFS_block_buf_t **pp_block_buf);
static int32_t BlockChain_Link(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, MINFS_block_buf_t **pp_block_buf);
static int32_t BlockChain_PopFree(MINFS_fs_t *p_fs, uint32_t num_blocks, MINFS_block_buf_t **pp_block_buf);
static int32_t BlockChain_PushFree(MINFS_fs_t *p_fs, uint32_t start_block, MINFS_block_buf_t **pp_block_buf);

// Buffer and RW layer
static int32_t BlockBuffer_Get(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id, uint8_t populate);
static int32_t BlockBuffer_Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);
static int32_t BlockBuffer_Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len);
static int32_t BlockBuffer_Flush(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf);


//------------------------------------------------------------------------------
//--------------------------- High level functions -----------------------------
//------------------------------------------------------------------------------


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
//       <p_block_buf> pointer to a  MINFS_block_buf_t - struct.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FlushBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  // valid buffer pointer?
  if( p_block_buf == NULL )
    return MINFS_ERROR_NO_BUFFER;
  // valid block_n ?
  if( p_block_buf->block_n >= p_fs->info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // request flush buffer
  return BlockBuffer_Flush(p_fs, p_block_buf);
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
// IN:  <p_fs> Pointer to a fs-structure. <fs_id> and <info> must be populated
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_Format(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // calculate and validate fs-params
  if( status = CalcFSParams(p_fs) )
    return status;
  // get buffer
  if( status = BlockBuffer_Get(p_fs, &p_block_buf, 0, MINFS_FILE_NULL, 0) )
    return status; // return error status
  uint16_t buf_i = 0; // byte offset in data buffer
  // put fs-header data to buffer
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)(p_block_buf->p_buf);
  memcpy(&(p_fs_header->fs_type), MINFS_FS_SIG, 4); // FS-signature
  p_fs_header->info = p_fs->info; // info structure
  // convert words to little-endian
  BE_SWAP_32(p_fs_header->info.num_blocks);
  BE_SWAP_16(p_fs_header->info.os_flags);
  buf_i += sizeof(MINFS_fs_header_t);
  // now write the block-chain-map
  // add a virtual block (start of free-blocks-chain)
  uint32_t i, block_map_entry, block_n = 0;
  for(i = p_fs->calc.first_datablock_n ; i <= p_fs->info.num_blocks ; i++){
    // check if current block-buffer is full
    if( buf_i >= p_fs->calc.block_data_len ){
      // write data
      if( status = BlockBuffer_Write(p_fs, p_block_buf, 0, p_fs->calc.block_data_len) )
        return status;
      // increment block_n, set offset to 0
      block_n++;
      buf_i = 0;
      // get a buffer for the next block
      if( status = BlockBuffer_Get(p_fs, &p_block_buf, block_n, MINFS_FILE_NULL, 0) )
	return status; // return error status
    }
    // calculate block pointer and write it to the buffer. 0 is file-index-start, 
    // free blocks chain: p_fs->info.num_blocks, 1, 2, .., p_fs->info.num_blocks - 1
    if ( i > 0 && i < p_fs->info.num_blocks - 1 )
      block_map_entry = i + 1; // free blocks chain
    else if( i == p_fs->info.num_blocks ) // start of free blocks chain (virtual block)
      block_map_entry = p_fs->calc.first_datablock_n; 
    else
      block_map_entry = MINFS_BLOCK_EOC; // EOC
    // convert to little-endian and copy to buffer
    BE_SWAP_32(block_map_entry);
    *( (uint32_t*)( (uint8_t*)(p_block_buf->p_buf) + buf_i) ) = block_map_entry;
    // increment buf_i pointer
    buf_i += p_fs->calc.bp_size;
  }
  // write data (last block-chain buffer)
  if( status = BlockBuffer_Write(p_fs, p_block_buf, 0, p_fs->calc.block_data_len) )
    return status;
  // write filesize 0 to file-index
  if( status = File_HeaderWrite(p_fs, p_fs->calc.first_datablock_n, 0, 0, &p_block_buf) )
    return status; // return error status
  // success
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Opens a file-system identified by <p_fs->fs_id> and populates the 
// *p_fs struct
//
// IN:  <p_fs> Pointer to a fs-structure. <fs_id> must be set
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FSOpen(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // prepare fs-struct
  p_fs->info.block_size = 4; // minimal block size
  p_fs->info.flags = 0;
  // get buffer
  if( status = BlockBuffer_Get(p_fs, &p_block_buf, 0, MINFS_FILE_NULL, 0) )
    return status; // return error status
  // read fs header data
  if( status = BlockBuffer_Read(p_fs, p_block_buf, 0, sizeof(MINFS_fs_header_t) ) )
    return 0;
  // map header struct
  MINFS_fs_header_t *p_fs_header = (MINFS_fs_header_t*)(p_block_buf->p_buf);
  // check file system type
  if( memcmp(&(p_fs_header->fs_type), MINFS_FS_SIG, 4) )
    return MINFS_ERROR_FS_SIG;
  // copy fs-info to *p_fs struct
  p_fs->info = p_fs_header->info;
  // convert words from little-endian to platform's format
  BE_SWAP_32(p_fs->info.num_blocks);
  BE_SWAP_16(p_fs->info.os_flags);
  // calculate and validate fs-params
  if( status = CalcFSParams(p_fs) )
    return status;
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file identified by <file_id>. If the file does not exist, it will
// be created with size 0.
// NOTE: A high file-id number will extend the file-index accordingly. The
// file-index will *not* be truncated if you unlink the file! You are
// responsible for the file-id management, use a range as small as possible.
// You can use MINFS_GetFreeFileID(..) to find the lowest free file-id.
//
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <file_id> File ID (1 - x)
//      <p_file> Pointer to a MINFS_file_t struct (will be populated on success).
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, p_file, &p_block_buf) )
    return status; // return error status
  // Get file-pointer
  if( (status = File_GetFilePointer(p_file, file_id, &p_block_buf)) < 0 ){
    if( status != MINFS_ERROR_FILE_NOT_EXISTS )
      return status; // return error-status
    // file does not exist, create it
    if( (status = File_Touch(p_file, file_id, &p_block_buf)) < 0 )
      return status;
  }
  // now open the file (status contains first file block_n now)
  if( status = File_Open(p_fs, status, file_id, p_file, &p_block_buf) )
    return status; // return error status
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Reads data from the current position in the file and increments the 
// file's data-pointer.
//
// IN:  <p_file> Pointer to a populated MINFS_file_t struct (use MINFS_FileOpen)
//      <p_buf> Pointer to a buffer where the read data should be copied to
//      <p_len> Pointer to a uint32_t containing the number of bytes to read
//              On return, this value will be set to the bytes actually read.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, MINFS_STATUS_EOF if end of file was reached, on error
//      MINFS_ERROR_XXXX.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileRead(MINFS_file_t *p_file, void *p_buf, uint32_t *p_len, MINFS_block_buf_t *p_block_buf){
  return File_ReadWrite(p_file, p_buf, p_len, MINFS_RW_MODE_READ, &p_block_buf);
}


/////////////////////////////////////////////////////////////////////////////
// Writes data to the current position in the file and increments the 
// file's data-pointer. If the file is too small, it will be extended.
//
// IN:  <p_file> Pointer to a populated MINFS_file_t struct (use MINFS_FileOpen)
//      <p_buf> Pointer to a buffer containing the data to write
//      <len> Number of bytes to write.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, MINFS_STATUS_FULL if the file could not be extended (no
//      more free blocks), on error MINFS_ERROR_XXXX.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileWrite(MINFS_file_t *p_file, void *p_buf, uint32_t len, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  // extend file if it is to small
  if( p_file->info.size < p_file->data_ptr + len ){
    if( status = File_SetSize(p_file, p_file->data_ptr + len, &p_block_buf) )
      return status; // return error status
  }
  // write data
  return File_ReadWrite(p_file, p_buf, &len, MINFS_RW_MODE_WRITE, &p_block_buf);
}


/////////////////////////////////////////////////////////////////////////////
// Seeks to a position in a file.
// 
// IN:  <p_file> Pointer to a populated MINFS_file_t struct (use MINFS_FileOpen)
//      <pos> Position in the file to seek to (bytes)
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX (-1 to -127)
//      If the end of the file was reached, MINFS_STATUS_EOF will be returned.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint32_t pos, MINFS_block_buf_t *p_block_buf){
  return File_Seek(p_file, pos, &p_block_buf);
}


/////////////////////////////////////////////////////////////////////////////
// Extends or truncates a file. 
// 
// IN:  <p_file> Pointer to a populated MINFS_file_t struct (use MINFS_FileOpen)
//      <new_size> New file-size in bytes.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX (-1 to -127). If the file 
//      could not be extended (no more free blocks), MINFS_STATUS_FULL will be 
//      returned. In this case, the file-size will not be changed.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size, MINFS_block_buf_t *p_block_buf){
  return File_SetSize(p_file, new_size, &p_block_buf);
}


/////////////////////////////////////////////////////////////////////////////
// Creates a non-existing file with size 0.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <file_id> ID of file to create (1 - x)
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX. If file already exists,
//      MINFS_ERROR_FILE_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileTouch(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status; // return error status
  // Try to create file
  if( (status = File_Touch(&file_0, file_id, &p_block_buf)) < 0 )
    return status; // return error status
  // success
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Unlinks a file. If check_last_truncate > 0 and <file_id> is the last 
// entry in the file-index, the index will be truncated by one entry.
// Alternatively, you may also use MINFS_FileIndexTruncate to truncate the 
// index if there are unlinked files at the end of the file-index.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <file_id> The ID of the file to unlink (1 - x)
//      <check_last_truncate> If > 0 and file_id is the last entry of the
//                            file-index, the file-index will be truncated.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX. If file does not exist,
//      MINFS_ERROR_FILE_NOT_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileUnlink(MINFS_fs_t *p_fs,uint32_t file_id, uint8_t check_last_truncate, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status; // return error status
  // Unlink file
  if( status = File_Unlink(&file_0, file_id, &p_block_buf) )
    return status; // return error status
  // If this was the last entry of the file-index, truncate
  if( check_last_truncate && (file_id == file_0.info.size / p_fs->calc.bp_size) ){
    if( status = File_SetSize(&file_0, file_0.info.size - p_fs->calc.bp_size, &p_block_buf) )
      return status; // return error status
  }
  // sucess
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Moves file <src_file_id> to <dst_file_id>. If check_last_truncate > 0 
// and <src_file_id> is the last entry in the file-index, the index will be 
// truncated by one entry. Alternatively, you may also use 
// MINFS_FileIndexTruncate to truncate the index if there are unlinked files 
// at the end of the file-index.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <src_file_id> The ID of the file to move (1 - x)
//      <dst_file_id> New ID of the file to move (1 - x)
//      <check_last_truncate> If > 0 and src_file_id is the last entry of the
//                            file-index, the file-index will be truncated.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX. If destination file exists,
//      MINFS_ERROR_FILE_EXISTS will be returned. If source file does not
//      exist, MINFS_ERROR_FILE_NOT_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileMove(MINFS_fs_t *p_fs,uint32_t src_file_id, uint32_t dst_file_id, uint8_t check_last_truncate, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status;
  // Check if destination file exists
  if( (status = File_GetFilePointer(&file_0, dst_file_id, &p_block_buf)) > 0 )
    return MINFS_ERROR_FILE_EXISTS; // destination file exists
  if( status != MINFS_ERROR_FILE_NOT_EXISTS )
    return status; // return error status
  // get source file first block pointer
  if( (status = File_GetFilePointer(&file_0, src_file_id, &p_block_buf)) < 0 )
    return status; // return error status
  // set file-pointer of destination file
  if( (status = File_SetFilePointer(&file_0, dst_file_id, status, &p_block_buf)) < 0 )
    return status; // return error status
  // set file pointer of source-file to MINFS_BLOCK_EOC
  if( status = File_SetFilePointer(&file_0, src_file_id, MINFS_BLOCK_EOC, &p_block_buf) )
    return status;
  // If src_file_id was the last entry of the file-index, truncate
  if( check_last_truncate && (src_file_id == file_0.info.size / p_fs->calc.bp_size) ){
    if( status = File_SetSize(&file_0, file_0.info.size - p_fs->calc.bp_size, &p_block_buf) )
      return status; // return error status
  }
  // sucess
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Checks if file <file_id> exists.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <file_id> The ID of the file to check for existence (1 - x)
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX. If file does not exist,
//      MINFS_ERROR_FILE_NOT_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileExists(MINFS_fs_t *p_fs,uint32_t file_id, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status;
  // Check if file exists
  if( (status = File_GetFilePointer(&file_0, file_id, &p_block_buf)) < 0 )
    return status; // return error status
  // file does exist
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a free file-id.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <mode> MINFS_MODE_FFID_NEXT: will return the next free file-id after
//             the end of the file-index. This is fast, but does not check
//             for free unlinked files.
//             MINFS_MODE_FFID_FIRST: will walk the file-index and return the
//             id of the first non-existing file. Using this file-id will not
//             extend the size of the file-index if a file was unlinked before, 
//             but can be slower than MINFS_MODE_FFID_NEXT, and the speed of
//             the function is not predictable.
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: Free file-id on success (1-x), on error MINFS_ERROR_XXXX.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileGetFreeID(MINFS_fs_t *p_fs, uint8_t mode, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status;
  // Get free file-id after end of index
  if( mode == MINFS_MODE_FFID_NEXT )
    return file_0.info.size / p_fs->calc.bp_size + 1;
  // Get first free file-id
  if( mode == MINFS_MODE_FFID_FIRST ){
    uint32_t file_id = 0;
    do{
      status = File_GetFilePointer(&file_0, ++file_id, &p_block_buf);
    } while (status > 0);
    // return free file-id or error-status
    return ( status == MINFS_ERROR_FILE_NOT_EXISTS ) ? file_id : status;
  }
  // unsupported mode
  return MINFS_ERROR_BAD_MODE;
}


/////////////////////////////////////////////////////////////////////////////
// Truncates the file-index if there are unlinked file-entries at the end of
// the index.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct (see MINFS_FSOpen)
//      <p_block_buf> pointer to a MINFS_block_buf_t - struct. If NULL, 
//                    MINFS_GetBlockBuffer(..) must provide a buffer.
// OUT: 0 on success, on error MINFS_ERROR_XXXX.
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_FileIndexTruncate(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  int32_t status;
  MINFS_file_t file_0;
  // Open File-index
  if( status = File_Open(p_fs, p_fs->calc.first_datablock_n, MINFS_FILE_INDEX, &file_0, &p_block_buf) )
    return status;
  // Walk file-index backwards to find the last existing file
  uint32_t last_file_id = file_0.info.size / p_fs->calc.bp_size;
  while( last_file_id ){
    if( (status = File_GetFilePointer(&file_0, last_file_id, &p_block_buf)) > 0 )
      break; // file exists
    if( status != MINFS_ERROR_FILE_NOT_EXISTS )
      return status; // return error status
    last_file_id--;
  }
  // Truncate file-index
  return File_SetSize(&file_0, p_fs->calc.bp_size * last_file_id, &p_block_buf);
}


//-----------------------------------------------------------------------------
//--------------------------- Low level functions -----------------------------
//-----------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// Populates a structure with pre-calculated parameters for the file-system
// 
// IN:  <p_fs> Pointer to a populated fs-structure. p_fs->calc will be populated
// OUT: 0 on success, else < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t CalcFSParams(MINFS_fs_t *p_fs){
  // calculate block-pointer size
  if(p_fs->info.num_blocks & 0xffff0000)
    p_fs->calc.bp_size = 4;
  else if(p_fs->info.num_blocks & 0xff00)
    p_fs->calc.bp_size = 2;
  else
    p_fs->calc.bp_size = 1;
  // calculate pec-size
  uint8_t pec_val;
  if(pec_val = p_fs->info.flags & MINFS_FLAGMASK_PEC)
    p_fs->calc.pec_width = 1 << (pec_val-1);
  else
    p_fs->calc.pec_width = 0;
  // calculate block_data_len
  p_fs->calc.block_data_len = (1 << p_fs->info.block_size) - p_fs->calc.pec_width;
  // calculate first data-block
  uint32_t fshead_size = sizeof(MINFS_fs_header_t) + p_fs->info.num_blocks * p_fs->calc.bp_size;
  p_fs->calc.first_datablock_n = fshead_size / p_fs->calc.block_data_len + (fshead_size % p_fs->calc.block_data_len ? 1 : 0);
  // check fs-info values
  if( p_fs->info.block_size > MINFS_MAX_BLOCKSIZE_EXP || p_fs->info.block_size < MINFS_MIN_BLOCKSIZE_EXP)
    return MINFS_ERROR_BLOCK_SIZE;
  if( p_fs->info.num_blocks > MINFS_MAX_NUMBLOCKS  || p_fs->info.num_blocks < p_fs->calc.first_datablock_n + 1 )
    return MINFS_ERROR_NUM_BLOCKS;
  // validated
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
static uint32_t GetPECValue(MINFS_fs_t *p_fs, void *p_buf, uint16_t data_len){
  // PEC value generation: the PEC of valid data incl. PEC must be zero.
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Gets a file-pointer from p_file_0
// 
// IN:  <p_file_0> Pointer to a populated MINFS_file_t struct, refering a file
//                 containing the block-pointers.
//      <file_id> The ID of the file to set the poiner for.
//      <block_n> The block number to point to (file's first block)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: Block number ( > 0) or < 0 on error (MINFS_ERROR_XXXX), if file does not
//      exist, MINFS_ERROR_FILE_NOT_EXISTS will be returned
/////////////////////////////////////////////////////////////////////////////
static int32_t File_GetFilePointer(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf){
  // check if file-id is in valid range
  if(file_id < 1 || file_id > MINFS_MAX_FILE_ID )
    return MINFS_ERROR_FILE_ID;
  // calc offset and check file-index size
  uint32_t fp_offset;
  if( (fp_offset = (file_id - 1) * p_file_0->p_fs->calc.bp_size) >= p_file_0->info.size )
    return MINFS_ERROR_FILE_NOT_EXISTS; // file-index is too small to contain a entry for file_id
  // now read value
  int32_t status, fp_block_n = 0;
  if( status = File_Seek(p_file_0, fp_offset, pp_block_buf) )
    return status; // return error status
  uint32_t len = p_file_0->p_fs->calc.bp_size;
  if( status = File_ReadWrite(p_file_0, &fp_block_n, &len, MINFS_RW_MODE_READ, pp_block_buf) )
    return status; // return error status
  // convert value from little-endian to platform's byte order
  BE_SWAP_32(fp_block_n);
  // return block pointer or MINFS_ERROR_FILE_NOT_EXISTS
  return (fp_block_n == MINFS_BLOCK_EOC) ? MINFS_ERROR_FILE_NOT_EXISTS : fp_block_n;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a file-pointer in p_file_0, extends the file if necessary. Does not
// check if file_id is already in use. If so, a stray blocks chain would be
// the result.
// 
// IN:  <p_file_0> Pointer to a populated MINFS_file_t struct, refering a file
//                 containing the block-pointers.
//      <file_id> The ID of the file to set the poiner for.
//      <block_n> The block number to point to (file's first block)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error MINFS_ERROR_XXXX
/////////////////////////////////////////////////////////////////////////////
static int32_t File_SetFilePointer(MINFS_file_t *p_file_0, uint32_t file_id, uint32_t block_n, MINFS_block_buf_t **pp_block_buf){
  // check if file-id is in valid range
  if(file_id < 1 || file_id > MINFS_MAX_FILE_ID )
    return MINFS_ERROR_FILE_ID;
  uint32_t fp_offset, len;
  int32_t status;
  // calc offset and extend file-index if necessary
  if( (fp_offset = (file_id - 1) * p_file_0->p_fs->calc.bp_size) >= p_file_0->info.size ){
    // seek to current end
    if( (status = File_Seek(p_file_0, MINFS_SEEK_END, pp_block_buf)) != MINFS_STATUS_EOF )
      return status; // return error status
    // cache old file size
    uint32_t old_indexf_size = p_file_0->info.size;
    // extend file
    if( status = File_SetSize(p_file_0, fp_offset + p_file_0->p_fs->calc.bp_size, pp_block_buf) )
      return status; // return error status
    // init added space with 0
    len = fp_offset - old_indexf_size;
    if( status = File_ReadWrite(p_file_0, NULL, &len, MINFS_RW_MODE_WRITE, pp_block_buf) )
      return status; // return error status
  } else {
    // seek to pointer position
    if( status = File_Seek(p_file_0, fp_offset, pp_block_buf) )
      return status; // return error status
  }
  // convert value from platform's byte order to little-endian
  BE_SWAP_32(block_n);
  // now write value
  len = p_file_0->p_fs->calc.bp_size;
  if( (status = File_ReadWrite(p_file_0, &block_n, &len, MINFS_RW_MODE_WRITE, pp_block_buf)) && status!= MINFS_STATUS_EOF && len==0  )
    return status; // return error status
  return block_n;
}


/////////////////////////////////////////////////////////////////////////////
// Unlinks a file: gets the file's first-block-pointer from p_file_0,
// overwrites the pointer with MINFS_BLOCK_EOC in p_file_0 and pushes the
// file's former blocks to the free-blocks-chain.
// 
// IN:  <p_file_0> Pointer to a populated MINFS_file_t struct, refering a file
//                 containing the block-pointer.
//      <file_id> The ID of the file to unlink (1 - x)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error MINFS_ERROR_XXXX. If file does not exist,
//      MINFS_ERROR_FILE_NOT_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
static int32_t File_Unlink(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf){
  // get pointer to file
  uint32_t file_bp;
  if( (file_bp = File_GetFilePointer(p_file_0, file_id, pp_block_buf)) < 0)
    return file_bp; // return error status
  // set file pointer to MINFS_BLOCK_EOC
  int32_t status;
  if( status = File_SetFilePointer(p_file_0, file_id, MINFS_BLOCK_EOC, pp_block_buf) )
    return status; // return error status
  // push the file's former block chain to the free-blocks-chain
  if( status = BlockChain_PushFree(p_file_0->p_fs, file_bp, pp_block_buf) )
    return status;
  // success
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Creates a non-existing file with size 0.
// 
// IN:  <p_file_0> Pointer to a populated MINFS_file_t struct, refering a file
//                 containing the block-pointer.
//      <file_id> The ID of the file to unlink (1 - x)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: First block_n of new file on success, on error MINFS_ERROR_XXXX. If file already exists,
//      MINFS_ERROR_FILE_EXISTS will be returned.
/////////////////////////////////////////////////////////////////////////////
static int32_t File_Touch(MINFS_file_t *p_file_0, uint32_t file_id, MINFS_block_buf_t **pp_block_buf){
  int32_t status, block_n;
  // check if file does exist
  if( (status = File_GetFilePointer(p_file_0, file_id, pp_block_buf)) > 0 )
    return MINFS_ERROR_FILE_EXISTS; // file already exists
  if( status != MINFS_ERROR_FILE_NOT_EXISTS )
    return status; // return error status
  // file does not exists, pop a free block
  if( (block_n = BlockChain_PopFree( p_file_0->p_fs, 1, pp_block_buf )) < 0 )
    return block_n; // return error status
  // set file pointer to the free block
  if( status = File_SetFilePointer(p_file_0, file_id, block_n, pp_block_buf) )
    return status; // return error status
  // write file header
  if( status = File_HeaderWrite(p_file_0->p_fs, block_n, file_id, 0, pp_block_buf) )
    return status; // return error status
  // success
  return block_n;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file by starting-block. The passed file_id will be forwarded to
// BlockBuffer_Get, and will be stored in the MINFS_file_t struct.
// Populates the *p_file struct. There's a signature at the beginning of 
// each file to minimize the risk to open a file by bad starting block.
// 
// IN:  <p_fs> Pointer to a populated MINFS_fs_t struct
//      <block_n> Number of the starting-block of the file.
//      <file_id> File identification, passed to BlockBuffer_Get and stored in *p_file
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t File_Open(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t **pp_block_buf){
  // check if block_n is a data-block
  if( block_n < p_fs->calc.first_datablock_n || block_n > p_fs->info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // get a buffer
  int32_t status;
  if( status = BlockBuffer_Get(p_fs, pp_block_buf, block_n, file_id, 0) )
    return status; // return error status
  // read data
  if( status = BlockBuffer_Read(p_fs, *pp_block_buf, 0, sizeof(MINFS_file_header_t)) )
    return status;
  // map file header struct
  MINFS_file_header_t *p_file_header = (MINFS_file_header_t*)( (*pp_block_buf)->p_buf );
  // check file signature
  if( memcmp( &(p_file_header->file_sig), MINFS_FILE_SIG, 4) )
    return MINFS_ERROR_FILE_SIG; // bad file signature
  // copy info structure
  p_file->info = p_file_header->info;
  // init values
  p_file->file_id = file_id;
  p_file->data_ptr = 0;
  p_file->current_block_n = block_n;
  p_file->data_ptr_block_offset = sizeof(MINFS_file_header_t);
  p_file->first_block_n = block_n;
  p_file->p_fs = p_fs;
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Seeks to a position in a file.
// 
// IN:  <p_file> Pointer to a populated MINFS_file_t struct
//      <pos> Position in the file to seek to (bytes)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error MINFS_ERROR_XXXX (-1 to -127)
//      If the end of the file was reached, MINFS_STATUS_EOF will be returned.
/////////////////////////////////////////////////////////////////////////////
static int32_t File_Seek(MINFS_file_t *p_file, uint32_t pos, MINFS_block_buf_t **pp_block_buf){
  // already there?
  if( pos == p_file->data_ptr )
    return 0;
  int32_t ret_status = 0;
  uint32_t seek_start_data_ptr;
  uint32_t seek_start_block_n;
  // move forward ?
  if( pos > p_file->data_ptr ){
    // will EOF be reached?
    if( pos >= p_file->info.size ){
      ret_status = MINFS_STATUS_EOF;
      pos = p_file->info.size; // set position to end of file
    }
    // (still in current buffer) or (EOF reached in current buffer) ?
    // NOTE: the second part of the condition is essential, without it the block seek would target a
    // non-existing block if the file ends at a block end!
    if( ( (p_file->data_ptr_block_offset + (pos - p_file->data_ptr) ) < p_file->p_fs->calc.block_data_len )
        || ( ret_status == MINFS_STATUS_EOF 
          && (p_file->data_ptr_block_offset + (pos - p_file->data_ptr) ) == p_file->p_fs->calc.block_data_len 
	) 
    ){
      p_file->data_ptr = pos;
      p_file->data_ptr_block_offset += pos - p_file->data_ptr;
      return ret_status;
    } 
    // seek from current position
    seek_start_data_ptr = p_file->data_ptr;
    seek_start_block_n = p_file->current_block_n;
  } else {
    // still in current buffer ?
    if( p_file->data_ptr - pos  <= p_file->data_ptr_block_offset ){
      p_file->data_ptr = pos;
      p_file->data_ptr_block_offset -= p_file->data_ptr - pos;
      return ret_status;
    }
    // seek from start
    seek_start_data_ptr = 0;
    seek_start_block_n = p_file->first_block_n;
  }
  // calculate block offset and seek
  uint32_t block_n_offset = (pos + sizeof(MINFS_file_header_t)) / p_file->p_fs->calc.block_data_len 
      - ( seek_start_data_ptr + sizeof(MINFS_file_header_t)) /  p_file->p_fs->calc.block_data_len;
  int32_t status;
  if( (status = BlockChain_Seek(p_file->p_fs, seek_start_block_n, block_n_offset, pp_block_buf)) < 0)
    return status; // return error status
  // if EOC, the file's block chain is broken
  if( status == MINFS_BLOCK_EOC )
    return MINFS_ERROR_FILE_CHAIN;
  // update *p_file fields
  p_file->current_block_n = status;
  p_file->data_ptr = pos;
  p_file->data_ptr_block_offset = (pos + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len;
  // success
  return ret_status; // return 0 or EOF
}

/////////////////////////////////////////////////////////////////////////////
// Extends or truncates a file. 
// 
// IN:  <p_file> Pointer to a populated MINFS_file_t struct
//      <new_size> New file-size in bytes.
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error MINFS_ERROR_XXXX (-1 to -127). If the file 
//      could not be extended (no more free blocks), MINFS_STATUS_FULL will be 
//      returned. In this case, the file-size will not be changed.
/////////////////////////////////////////////////////////////////////////////
static int32_t File_SetSize(MINFS_file_t *p_file, uint32_t new_size, MINFS_block_buf_t **pp_block_buf){
  // already right size?
  if( p_file->info.size == new_size )
    return 0;
  uint16_t current_size_block_offset = ( (p_file->info.size + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len ) + 1;
  // shrink file ?
  if( new_size < p_file->info.size ){
    // truncate blocks ?
    if( (p_file->info.size - new_size) >= current_size_block_offset ){
      int32_t new_last_block_n;
      int32_t fbc_cont_block;
      // find new last block
      uint32_t new_num_blocks = ( new_size + sizeof(MINFS_file_header_t)) / p_file->p_fs->calc.block_data_len
        + ( ( new_size + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len ) ? 1 : 0;
      if( (new_last_block_n = BlockChain_Seek(p_file->p_fs, p_file->first_block_n, new_num_blocks - 1, pp_block_buf)) < 0 )
        return new_last_block_n; // return error status
      // if EOC, the file's block chain is broken
      if( new_last_block_n == MINFS_BLOCK_EOC )
	return MINFS_ERROR_FILE_CHAIN;
      // find block after new last block
      if( (fbc_cont_block = BlockChain_Seek(p_file->p_fs, new_last_block_n, 1, pp_block_buf)) < 0 )
        return fbc_cont_block; // return error status
      // if EOC, the file's block chain is broken
      if( fbc_cont_block == MINFS_BLOCK_EOC )
	return fbc_cont_block;
      // cut block chain
      int32_t status;
      if( status = BlockChain_Link(p_file->p_fs, new_last_block_n, MINFS_BLOCK_EOC, pp_block_buf) )
        return status; // return error status
      // push cut-off free blocks
      if( status = BlockChain_PushFree(p_file->p_fs, fbc_cont_block, pp_block_buf) )
        return status; // return error status
      // set new current-block to new last block if data_ptr is beyond file size
      if( p_file->data_ptr > new_size )
	p_file->current_block_n = new_last_block_n;
    }
    // set new data_ptr and calc block offset if data_ptr is beyond file size
    if( p_file->data_ptr > new_size ){
      p_file->data_ptr = new_size;
      p_file->data_ptr_block_offset = ( new_size + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len;
      // we are at the end of the file, if data_ptr_block_offset is 0, this means we'r at the end of the last block,
      // the correct offset is block_data_len
      if( p_file->data_ptr_block_offset == 0 )
        p_file->data_ptr_block_offset = p_file->p_fs->calc.block_data_len;
    }
  }else{// extend file
    // add blocks ?
    if( (new_size - p_file->info.size + current_size_block_offset) >  p_file->p_fs->calc.block_data_len ){
      // find last block of file
      int32_t file_last_block_n;
      if( (file_last_block_n = BlockChain_Seek(p_file->p_fs, p_file->current_block_n, MINFS_SEEK_END, pp_block_buf)) < 0 )
        return file_last_block_n; // return error status
      // pop free blocks
      uint32_t add_blocks_count = ( new_size + sizeof(MINFS_file_header_t)) / p_file->p_fs->calc.block_data_len
        + ( ( new_size + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len ) ? 1 : 0
        - ( p_file->info.size + sizeof(MINFS_file_header_t)) / p_file->p_fs->calc.block_data_len
        - ( ( p_file->info.size + sizeof(MINFS_file_header_t)) % p_file->p_fs->calc.block_data_len ) ? 1 : 0;
      int32_t add_blocks_first_n;
      if( (add_blocks_first_n = BlockChain_PopFree(p_file->p_fs, add_blocks_count, pp_block_buf)) < 0)
        return add_blocks_first_n; // return error status
      // add free blocks to files block-chain
      int32_t status;
      if( status = BlockChain_Link(p_file->p_fs, file_last_block_n, add_blocks_first_n, pp_block_buf) )
        return status; // return error status
    }
  }
  // finally, update file-header with new size
  int32_t status;
  if( status = File_HeaderWrite(p_file->p_fs, p_file->first_block_n, p_file->file_id, new_size, pp_block_buf) )
    return status; // return error status
  // update info struct
  p_file->info.size = new_size;
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Reads or writes data from/to a file. Does not extend the file when writing.
// *len will contain the number of bytes actually read/written.
// 
// IN:  <p_file> Pointer to a populated MINFS_file_t struct
//      <p_buf> Pointer to a buffer where data should be read from / written to
//              If it is NULL, 0-bytes will be written.
//      <p_len> Pointer to an integer containing the number of bytes to transfer.
//              Will contain the number of byts actually copied
//      <write> Read/write indicator (0: read; >0 : write)
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 or MINFS_STATUS_EOF on success, on error MINFS_ERROR_XXXX (-1 to -127)
/////////////////////////////////////////////////////////////////////////////
static int32_t File_ReadWrite(MINFS_file_t *p_file, void *p_buf, uint32_t *p_len, uint8_t mode, MINFS_block_buf_t **pp_block_buf){
  uint32_t remain, status;
  uint16_t delta;
  int32_t ret_status = 0;
  // alternate p_len if we will reach EOF
  if( p_file->data_ptr + *p_len >= p_file->info.size ){
    *p_len = p_file->info.size - p_file->data_ptr;
    ret_status = MINFS_STATUS_EOF;
  }
  // walk blocks
  remain = *p_len;
  *p_len = 0;
  while(remain){
    // skip read/write if delta is 0 (block_data_len == data_ptr_block_offset)
    if( delta = ( remain > (p_file->p_fs->calc.block_data_len - p_file->data_ptr_block_offset) ) ?  
    p_file->p_fs->calc.block_data_len - p_file->data_ptr_block_offset : remain ){
      // get buffer for current block
      if( status = BlockBuffer_Get(p_file->p_fs, pp_block_buf, p_file->current_block_n, 
        p_file->file_id, (mode == MINFS_RW_MODE_WRITE) ? 1 : 0) 
      )
	return status; // return error status
      if(mode == MINFS_RW_MODE_WRITE){
	// copy data
	if(p_buf == NULL) // fill with 0-bytes
          memset( (uint8_t*)( (*pp_block_buf)->p_buf ) + p_file->data_ptr_block_offset, 0, delta );
	else // copy data from p_buf
          memcpy( (uint8_t*)( (*pp_block_buf)->p_buf) + p_file->data_ptr_block_offset, p_buf, delta);
	// write data
	if( status = BlockBuffer_Write(p_file->p_fs, *pp_block_buf, p_file->data_ptr_block_offset, delta) )
	  return status; // return error status
      }else if(mode == MINFS_RW_MODE_READ){
        // read data
	if( status = BlockBuffer_Read(p_file->p_fs, *pp_block_buf, p_file->data_ptr_block_offset, delta) )
	  return status; // return error status
	// copy data
	memcpy(p_buf, (uint8_t*)( (*pp_block_buf)->p_buf) + p_file->data_ptr_block_offset, delta);
      }
    }
    remain -= delta;
    // switch current block?
    if( (p_file->data_ptr_block_offset >=  p_file->p_fs->calc.block_data_len) && remain ){
      int32_t new_current_block_n;
      if( (new_current_block_n = BlockChain_Seek(p_file->p_fs, p_file->current_block_n, 1, pp_block_buf)) < 0 )
        return new_current_block_n; // return error status
      if( new_current_block_n == MINFS_BLOCK_EOC )
        return MINFS_ERROR_FILE_CHAIN; // file chain is broken
      // set new current block, and data_ptr offset to block start
      p_file->current_block_n = new_current_block_n;
      p_file->data_ptr_block_offset = 0;
    } else 
      p_file->data_ptr_block_offset += delta; // just increment data_ptr offset
    // update variables
    *p_len += delta;
    p_file->data_ptr += delta; 
  }
  // success ( return EOF or 0 )
  return ret_status;
}


/////////////////////////////////////////////////////////////////////////////
// Writes file-header data to block_n.
//
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_n> First block of file
//      <file_id> File identification, passed to BlockBuffer_Get
//      <pp_block_buf> Pointer to a Buffer-struct pointer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t File_HeaderWrite(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t file_id, uint32_t file_size, MINFS_block_buf_t **pp_block_buf){
  int32_t status;
  // get a buffer
  if( status = BlockBuffer_Get(p_fs, pp_block_buf, block_n, file_id, 0) )
    return status; // return error status
  // map file-header struct to buffer and set file-signature and size
  MINFS_file_header_t *p_file_header = (MINFS_file_header_t*)( (*pp_block_buf)->p_buf );
  memcpy(&(p_file_header->file_sig), MINFS_FILE_SIG, 4);
  p_file_header->info.size = 0;
  if( status = BlockBuffer_Write(p_fs, *pp_block_buf, 0, sizeof(MINFS_file_header_t)) );
    return status;
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Pops num_block from the free-block-chain and returns the first block number.
//
// IN:  <p_fs> Pointer to a populated fs-structure
//      <num_blocks> Number of free blocks to pop
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: First block number, on error < 0 (MINFS_ERROR_XXXX or MINFS_STATUS_FULL)
/////////////////////////////////////////////////////////////////////////////
static int32_t BlockChain_PopFree(MINFS_fs_t *p_fs, uint32_t num_blocks, MINFS_block_buf_t **pp_block_buf){
  if( num_blocks == 0 )
    return MINFS_BLOCK_EOC; // return EOC-pointer
  // seek first block to pop off the fbc 
  // p_fs->info.num_blocks is the virtual start block of the fbc
  int32_t start_block;
  if( (start_block = BlockChain_Seek(p_fs, p_fs->info.num_blocks, 1, pp_block_buf)) < 0 )
    return start_block; // return error status
  else if(start_block == MINFS_BLOCK_EOC)
    return MINFS_STATUS_FULL; // no more free blocks
  // seek last block to pop off the fbc
  int32_t end_block;
  if( num_blocks > 1 ){
    if( (end_block  = BlockChain_Seek(p_fs, start_block, num_blocks-1, pp_block_buf)) < 0 )
      return end_block; // return error status
    else if(end_block == MINFS_BLOCK_EOC)
      return MINFS_STATUS_FULL; // can't get the requested number of free blocks
  } else end_block = start_block;
  // now get the block-number next in chain after the last block to pop off the fbc
  int32_t fbc_cont_block;
  if( (fbc_cont_block = BlockChain_Seek(p_fs, end_block, 1, pp_block_buf)) < 0 )
    return fbc_cont_block; // return error status
  // set the new block-chain-entries
  // set entry for last poped block to EOC
  int32_t status;
  if( status = BlockChain_Link(p_fs, end_block, MINFS_BLOCK_EOC, pp_block_buf) )
    return status;
  // continue free blocks chain at entry after the last popped block
  if( status = BlockChain_Link(p_fs, p_fs->info.num_blocks, fbc_cont_block, pp_block_buf) )
    return status;
  // success, return start block of poped blocks chain
  return start_block;  
}


/////////////////////////////////////////////////////////////////////////////
// Adds a block-chain (start_block -> EOC) to the free-blocks-chain
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <start_block> Start-block of the chain
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: First block number, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t BlockChain_PushFree(MINFS_fs_t *p_fs, uint32_t start_block, MINFS_block_buf_t **pp_block_buf){
  // check if start_block is a data-block
  if( start_block < p_fs->calc.first_datablock_n || start_block > p_fs->info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // find last block of the chain
  uint32_t end_block;
  if( (end_block = BlockChain_Seek(p_fs, start_block, MINFS_SEEK_END, pp_block_buf)) < 0 )
    return end_block; // return error status
  // find the second element in the free-blocks chain
  uint32_t fbc_cont_block;
  if( (fbc_cont_block = BlockChain_Seek(p_fs, p_fs->info.num_blocks, 1, pp_block_buf)) < 0 )
    return fbc_cont_block; // return error status
  // link fbc first (virtual) block to start_block
  int32_t status;
  if( status = BlockChain_Link(p_fs, p_fs->info.num_blocks, start_block, pp_block_buf) )
    return status;
  // link end_block to the former second element of free-blocks
  if( status = BlockChain_Link(p_fs, end_block, fbc_cont_block, pp_block_buf) )
    return status;
  // success
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
static int32_t BlockChain_Seek(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t offset, MINFS_block_buf_t **pp_block_buf){
  // check if block_n is a data-block
  if( block_n < p_fs->calc.first_datablock_n || block_n > p_fs->info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // seek blocks
  uint32_t block_chain_block_n, last_block_n;
  uint16_t block_chain_entry_offset;
  int32_t status;
  while( offset > 0 ){
    // calculate block and offset where the next block-pointer is stored
    block_chain_entry_offset = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * (block_n - p_fs->calc.first_datablock_n); // chain-pointer offset from beginning of fs
    block_chain_block_n = block_chain_entry_offset / p_fs->calc.block_data_len; // block where the chain pointer resides
    block_chain_entry_offset %= p_fs->calc.block_data_len; // chain-pointer offset from beginning of block
    // get buffer
    if( status = BlockBuffer_Get(p_fs, pp_block_buf, block_chain_block_n, MINFS_FILE_NULL, 0) )
      return status; // return error status
    // read fs header data
    if( status = BlockBuffer_Read(p_fs, *pp_block_buf, block_chain_entry_offset, p_fs->calc.bp_size) )
      return status;
    // get block pointer
    last_block_n = block_n;
    LE_GET(block_n, (uint8_t*)( (*pp_block_buf)->p_buf) + block_chain_entry_offset, p_fs->calc.bp_size);
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
static int32_t BlockChain_Link(MINFS_fs_t *p_fs, uint32_t block_n, uint32_t block_target, MINFS_block_buf_t **pp_block_buf){
  // check if block_n is a data-block
  if( block_n < p_fs->calc.first_datablock_n || block_n > p_fs->info.num_blocks )
    return MINFS_ERROR_BLOCK_N;
  // calculate block and offset where the next block-pointer is stored
  uint16_t block_chain_entry_offset = sizeof(MINFS_fs_header_t) + p_fs->calc.bp_size * (block_n - p_fs->calc.first_datablock_n); // chain-pointer offset from beginning of fs
  uint32_t block_chain_block_n = block_chain_entry_offset / p_fs->calc.block_data_len; // block where the chain pointer resides
  block_chain_entry_offset %= p_fs->calc.block_data_len; // chain-pointer offset from beginning of block
  // get buffer
  int32_t status;
  if( status = BlockBuffer_Get(p_fs, pp_block_buf, block_chain_block_n, MINFS_FILE_NULL, 1) )
    return status; // return error status
  // write the block-chain-entry
  LE_SET( (*pp_block_buf + block_chain_entry_offset), block_target, p_fs->calc.bp_size);
  if( status = BlockBuffer_Write(p_fs, *pp_block_buf, block_chain_entry_offset, p_fs->calc.bp_size) )
    return status;
  // success
  return 0;
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
static int32_t BlockBuffer_Get(MINFS_fs_t *p_fs, MINFS_block_buf_t **pp_block_buf, uint32_t block_n, uint32_t file_id, uint8_t populate){
  int32_t status;
  // return if buffer is already valid (right block number, poplulated if this was requested)
  if(
    (*pp_block_buf != NULL) && 
    ((*pp_block_buf)->block_n == block_n) && 
    ( populate ? (*pp_block_buf)->flags.populated : 1) 
  ){
    return 0;
  }
  // if buffer is not set at all, or has wrong block_n, try to get it by OS-hook
  if( *pp_block_buf == NULL || (*pp_block_buf)->block_n != block_n ){
    // get the buffer by OS-hook
    if( status = MINFS_GetBlockBuffer(p_fs, pp_block_buf, block_n, file_id) )
      return status; // return error status
    if( *pp_block_buf == NULL )
      return MINFS_ERROR_NO_BUFFER; // could not get buffer
   }
  // if the buffer was not used for block_n, flush the current buffer content, reset flags and set block_n
  if( block_n != (*pp_block_buf)->block_n ){
	// flush the block-buffer if (*p_block_buf)->flags.changed )
    if( status = BlockBuffer_Flush(p_fs, *pp_block_buf) )
      return status; // return error status
    (*pp_block_buf)->flags.ALL = 0;
    (*pp_block_buf)->block_n = block_n;
  }
  // populate the buffer if requested and not already populated
  if( populate && !((*pp_block_buf)->flags.populated) ){
    if( status = BlockBuffer_Read(p_fs, *pp_block_buf, 0, 0) )
      return status; // return error status
  }
  // success
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Flushes a buffer (only if changed-flag is set).
// The application / OS layer is responsible to call MINFS_FlushBlockBuffer
// the last time. Each call to BlockBuffer_Get will flush the passed
// buffer if a new one should be used (different block-number).
// NOTE: MINFS_Write has to reset the changed flag!
// 
// IN:  <p_fs> Pointer to a populated fs-structure
//      <block_buf> Pointer to a MINFS_block_buf_t pointer
// OUT: 0 on success, on error < 0 (MINFS_ERROR_XXXX)
/////////////////////////////////////////////////////////////////////////////
static int32_t BlockBuffer_Flush(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf){
  // check if buffer has changed -> write whole buffer
  // NOTE: MINFS_Write has to reset the changed flag!
  if( p_block_buf->flags.changed ){
    if( !p_block_buf->flags.populated )
      return MINFS_ERROR_FLUSH_BNP; // buffer was not populated
    int32_t status;
    if( status = BlockBuffer_Write(p_fs, p_block_buf, 0, 0) )
      return status; // return error status
  }
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Request to write a data portion or to flush the buffer (data_len == 0).
// BlockBuffer_Flush is the only function that calls Write with data_len = 0,
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
static int32_t BlockBuffer_Write(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  // set the changed-flag
  // NOTE: MINFS_Write has to reset the changed flag!
  p_block_buf->flags.changed = 1;
  // if PEC is enabled, only whole-block-writes (flush) will be forwarded
  if( p_fs->info.flags & MINFS_FLAGMASK_PEC ){
    if( data_len )
      return 0;
    // caculate and set PEC-value
    uint32_t pec = GetPECValue(p_fs, p_block_buf->p_buf, p_fs->calc.block_data_len);
    TC_SET( ( (uint8_t*)(p_block_buf->p_buf) + p_fs->calc.block_data_len), pec, p_fs->calc.pec_width );
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
// BlockBuffer_Get is the only function that calls Read with data_len = 0,
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
static int32_t BlockBuffer_Read(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf, uint16_t data_offset, uint16_t data_len){
  // if the buffer is populated, nothing has to be read
  if( p_block_buf->flags.populated )
    return 0;
  // If PEC is enabled, only reads of a whole block are valid (populate)
  if( p_fs->info.flags & MINFS_FLAGMASK_PEC )
    data_len = 0;
  // NOTE: MINFS_Read has to set the populated - flag after a whole-block was read!
  int32_t status;
  if( status = MINFS_Read(p_fs, p_block_buf, data_offset, data_len) )
    return status; // return error status
  // if PEC is enabled, a populate-request must not be ignored
  if( (p_fs->info.flags & MINFS_FLAGMASK_PEC) && !p_block_buf->flags.populated )
    return MINFS_ERROR_READ_BNP;
  // check PEC value if PEC is enabled
  if( p_fs->info.flags & MINFS_FLAGMASK_PEC ){
    if ( GetPECValue(p_fs, p_block_buf->p_buf, p_fs->calc.block_data_len + p_fs->calc.pec_width) )
      return MINFS_ERROR_PEC; // PEC value was not 0
  }
  // success
  return 0;
}











