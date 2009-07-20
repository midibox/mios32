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

#include "minfs.h"

/////////////////////////////////////////////////////////////////////////////
// Function Hooks to caching and device layer
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Hook to OS to read a data block
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <buffer>    Pointer to buffer-pointer, if NULL, the buffer has to be provided
//                  from here. If buf_type == MINFS_BUFTYPE_FSINFO, the buffer's size
//                  has to be at least sizeof(MINFS_fs_info_t), else the at least
//                  2^p_fs_info->block_size bytes.
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block is read
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_ReadBlock(u8 buf_type, uint8_t **buffer, uint32_t bufn, MINFS_fs_info_t *p_fs_info, file_id);

/////////////////////////////////////////////////////////////////////////////
// Hook to OS to write a data block
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <buffer>    Pointer to buffer-pointer, if NULL, the buffer has to be provided
//                  from here. If buf_type == MINFS_BUFTYPE_FSINFO, the buffer's size
//                  has to be at least sizeof(MINFS_fs_info_t), else the at least
//                  2^p_fs_info->block_size bytes.
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block is written
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
extern int32_t MINFS_WriteBlock(u8 buf_type, uint8_t *buffer, uint32_t block_n, MINFS_fs_info_t *p_fs_info);

/////////////////////////////////////////////////////////////////////////////
// Provides a data buffer for a write operation (if no buffer was provided before)
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block
// OUT: Pointer to a buffer of size 2^p_fs_info->block_size bytes
/////////////////////////////////////////////////////////////////////////////
extern uint8_t* MINFS_WriteBufGet(u8 buf_type, uint32_t bufn, MINFS_fs_info_t *p_fs_info, file_id);



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

int32_t MINFS_BlockSeek(MINFS_fs_info_t *p_fs_info, uint8_t *block_buffer, uint32_t start_block, uint32_t offset);
int32_t MINFS_BlockChain(MINFS_fs_info_t *p_fs_info, uint32_t block, uint32_t chain_to, uint8_t *block_buffer);

uint32_t MINFS_PopFreeBlocks(MINFS_fs_info_t *p_fs_info, uint32_t num_blocks);
uint32_t MINFS_PushFreeBlocks(MINFS_fs_info_t *p_fs_info, uint32_t first_block);

// special handling for file_id 0 (file containing first-block-pointers) 
uint32_t MINFS_GetFileInfo(MINFS_fs_info_t *p_fs_info, file_id uint32_t, MINFS_file_t *p_file);




