/*
 * Header file for minfs.c
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch / thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <stdint.h>
#include <stddef.h>


#ifndef _MINFS_H
#define _MINFS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// filesystem-signature
#define MINFS_FS_SIG "MIFS"

// file signature
#define MINFS_FILE_SIG "MIFL"

// filesystem flags
#define MINFS_FLAGS_NOPEC 0x00
#define MINFS_FLAGS_PEC8 0x01
#define MINFS_FLAGS_PEC16 0x02
#define MINFS_FLAGS_PEC32 0x03
#define MINFS_FLAGMASK_PEC 0x03

// used to seek until end of a file / block-chain
#define MINFS_SEEK_END 0xFFFFFFFF

// NULL block pointer
#define MINFS_BLOCK_NULL 0xFFFFFFFF

// NULL file id
#define MINFS_FILE_NULL 0xFFFFFFFF

// EOC block pointer
#define MINFS_BLOCK_EOC 0

// errors
#define MINFS_ERROR_NO_BUFFER -1
#define MINFS_ERROR_FS_SIG -2
#define MINFS_ERROR_NUM_BLOCKS -3
#define MINFS_ERROR_BLOCK_SIZE -4
#define MINFS_ERROR_BLOCK_N -5
#define MINFS_ERROR_PEC -6
#define MINFS_ERROR_FLUSH_BNP -7
#define MINFS_ERROR_READ_BNP -8
#define MINFS_ERROR_FILE_SIG -9
#define MINFS_ERROR_FILE_CHAIN -10


// return status
#define MINFS_STATUS_EOF -128
#define MINFS_STATUS_FULL -129


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct{
  uint32_t num_blocks; // number of blocks in file-system
  uint8_t block_size; // block size (2^x, min 2^4)
  uint8_t flags; // file system flags
  uint16_t os_flags; // this may be used by the OS
} MINFS_fs_info_t;


typedef struct{
  uint32_t fs_type;
  MINFS_fs_info_t info;
} MINFS_fs_header_t;


typedef struct{
  uint8_t bp_size; // size of a block-pointer in bytes (1,2,4)
  uint16_t block_data_len; // block-length 
  uint32_t first_datablock; // block number where data blocks begin
  uint8_t pec_width; // size in bytes of the pec-value  
 } MINFS_fs_calc_t;


typedef struct{
  MINFS_fs_info_t info;
  uint8_t fs_id; // remember fs_id value (identifies the fs on OS level)
  MINFS_fs_calc_t calc; // values calculated once at fs-info-get / format
} MINFS_fs_t;

typedef struct{
  uint32_t size; // file size in bytes  
} MINFS_file_info_t;

typedef struct{
  uint32_t file_sig; // file-signature
  MINFS_file_info_t info; // file size in bytes
} MINFS_file_header_t;


typedef struct{
  MINFS_fs_t *p_fs; // pointer to filesytem-structure
  MINFS_file_info_t info; // file-header structure
  uint32_t file_id; // file id
  uint32_t data_ptr; // file-pointer, offset from beginning (exclusive pec/file-info data)
  uint32_t current_block_n; // current block number
  uint32_t data_ptr_block_offset; // data pointer offset in the current block
  uint32_t first_block_n; // first block of the file
} MINFS_file_t;

// structure to hold information about a block-buffer
typedef struct{
  void *p_buf; // pointer to the buffer
  uint32_t block_n;
  union{
    struct{
      unsigned populated:1;
      unsigned changed:1;
      };
    uint8_t ALL;
  } flags;
} MINFS_block_buf_t;

/////////////////////////////////////////////////////////////////////////////
// High level functions
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_InitBlockBuffer(MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FlushBlockBuffer(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf);

extern int32_t MINFS_Format(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf);

extern int32_t MINFS_FSOpen(MINFS_fs_t *p_fs, MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, MINFS_block_buf_t *p_block_buf);

extern int32_t MINFS_FileRead(MINFS_file_t *p_file, void *p_buf, uint32_t len, MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FileWrite(MINFS_file_t *p_file, void *p_buf, uint32_t len, MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint32_t pos, MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size, MINFS_block_buf_t *p_block_buf);

extern int32_t MINFS_Unlink(uint32_t file_id, MINFS_block_buf_t *block_buf);
extern int32_t MINFS_Move(uint32_t src_file_id, uint32_t dst_file_id, MINFS_block_buf_t *p_block_buf);
extern int32_t MINFS_FileExists(uint32_t file_id, MINFS_block_buf_t *p_block_buf);


#endif /* _MINFS_H */
