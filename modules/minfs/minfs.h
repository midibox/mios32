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

#ifndef _MINFS_H
#define _MINFS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MINFS_FS_ID "MIFS"

#define MINFS_FLAG_PEC 0x00001

#define MINFS_BUFTYPE_FSINFO 0x01
#define MINFS_BUFTYPE_CHAINMAP 0x02
#define MINFS_BUFTYPE_FILE 0x04

#define MINFS_SEEK_END 0xFFFFFFFF

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct{
  uint8_t block_size; // block size (2^x)
  uint32_t num_blocks; // number of blocks in file-system
  u16 flags; // file system flags
  uint8_t fs_id; // remember fs_id value
} MINFS_fs_info_t;


typedef struct{
  uint32_t size; // file size in bytes
} MINFS_file_info_t;


typedef struct{
  MINFS_fs_info_t *p_fs_info; // pointer to filesytem-info structure
  MINFS_file_info_t file_info; // file-info structure
  uint32_t file_id; // file id (1 - x)
  uint32_t data_ptr; // file-pointer, offset from beginning (exclusive pec/file-info data)
  uint32_t current_block; // current block number
  uint32_t current_block_offset; // data pointer offset in the current block
  uint32_t first_block; // first block of the file
} MINFS_file_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_Format(MINFS_fs_info_t *p_fs_info);

extern int32_t MINFS_GetFSInfo(uint8_t fs_id, uint8 *block_buf, MINFS_fs_info_t *p_fs_info);
extern int32_t MINFS_FileOpen(MINFS_fs_info_t *p_fs_info, uint32_t file_id, uint8 *block_buf, MINFS_file_t *p_file);
extern int32_t MINFS_FileClose(MINFS_file_t *p_file);

extern int32_t MINFS_FileRead(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf);
extern int32_t MINFS_FileWrite(MINFS_file_t *p_file, uint8_t *buffer, uint32_t len, uint8_t *block_buf);
extern int32_t MINFS_FileSeek(MINFS_file_t *p_file, uint8_t *buffer, uint32_t pos);
extern int32_t MINFS_FileSetSize(MINFS_file_t *p_file, uint32_t new_size);

extern int32_t MINFS_Unlink(uint8_t file_id);
extern int32_t MINFS_Move(uint8_t src_file_id, dst_file_id);
extern int32_t MINFS_FileExists(uint8_t file_id);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern s32 MODULE_X_global_var;

#endif /* _MINFS_H */
