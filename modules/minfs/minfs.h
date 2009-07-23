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

#ifndef _MINFS_H
#define _MINFS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// filesystem-type identification
#define MINFS_FS_TYPE 0xAB0145CC

// filesystem flags
#define MINFS_FLAGS_NOPEC 0x00
#define MINFS_FLAGS_PEC8 0x01
#define MINFS_FLAGS_PEC16 0x02
#define MINFS_FLAGS_PEC32 0x03
#define MINFS_FLAGMASK_PEC 0x03

// block type flags
#define MINFS_BLOCK_TYPE_FSHEAD 0x01
#define MINFS_BLOCK_TYPE_FILEINDEX 0x02
#define MINFS_BLOCK_TYPE_FILE 0x03

// used to seek until end of a chain/file
#define MINFS_SEEK_END 0xFFFFFFFF

// errors
#define MINFS_ERROR_NO_BUFFER -1
#define MINFS_ERROR_BAD_FSTYPE -2



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
  MINFS_fs_info_t fs_info;
} MINFS_fs_header_t;


typedef struct{
  uint8_t bp_size; // size of a block-pointer in bytes (1,2,4)
  uint16_t block_data_len; // block-length 
  uint32_t first_datablock; // block number where data blocks begin
  uint8_t pec_size; // size in bytes of the pec-value  
 } MINFS_fs_calc_t;


typedef struct{
  MINFS_fs_info_t fs_info;
  uint8_t fs_os_id; // remember fs_id value (identifies the fs on OS level)
  MINFS_fs_calc_t calc; // values calculated once at fs-info-get / format
} MINFS_fs_t;


typedef struct{
  uint32_t size; // file size in bytes
} MINFS_file_header_t;


typedef struct{
  MINFS_fs_t *p_fs; // pointer to filesytem-structure
  MINFS_file_header_t file_header; // file-info structure
  uint32_t file_id; // file id (1 - x)
  uint32_t data_ptr; // file-pointer, offset from beginning (exclusive pec/file-info data)
  uint32_t current_block; // current block number
  uint32_t current_block_offset; // data pointer offset in the current block
  uint32_t first_block; // first block of the file
} MINFS_file_t;


/////////////////////////////////////////////////////////////////////////////
// High level functions
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Format(MINFS_fs_t *p_fs, uint8_t *block_buf);

extern int32_t MINFS_GetFSInfo(MINFS_fs_t *p_fs, uint8_t *block_buf);
extern int32_t MINFS_FileOpen(MINFS_fs_t *p_fs, uint32_t file_id, MINFS_file_t *p_file, uint8_t *block_buf);

extern int32_t MINFS_FileRead(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf);
extern int32_t MINFS_FileWrite(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf);
extern int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint32_t pos, uint8_t *block_buf);
extern int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size, uint8_t *block_buf);

extern int32_t MINFS_Unlink(uint32_t file_id, uint8_t *block_buf);
extern int32_t MINFS_Move(uint32_t src_file_id, uint32_t dst_file_id, uint8_t *block_buf);
extern int32_t MINFS_FileExists(uint32_t file_id, uint8_t *block_buf);


#endif /* _MINFS_H */
