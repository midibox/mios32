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


/////////////////////////////////////////////////////////////////////////////
// Reads a block from FRAM device
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <buffer>    Pointer to buffer-pointer, if NULL, the buffer has to be provided
//                  from here. If buf_type == MINFS_BUFTYPE_FSINFO, the buffer's size
//                  has to be at least sizeof(MINFS_fs_info_t), else the at least
//                  2^p_fs_info->block_size bytes.
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_ReadBlock(u8 buf_type, uint8_t **buffer, uint32_t bufn, MINFS_fs_info_t *p_fs_info){
  
}

/////////////////////////////////////////////////////////////////////////////
// Provides a data buffer for a write operation (if no buffer was provided before)
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block
// OUT: Pointer to a buffer of size 2^p_fs_info->block_size bytes
/////////////////////////////////////////////////////////////////////////////
uint8_t* MINFS_WriteBufGet(u8 buf_type, uint32_t bufn, MINFS_fs_info_t *p_fs_info){
  
}

/////////////////////////////////////////////////////////////////////////////
// Reads a block from FRAM device
// IN:  <buf_type>  The Buffer's content type (the MINFS_BUFTYPE_XXX)
//      <buffer>    Pointer to buffer-pointer, if NULL, the buffer has to be provided
//                  from here. If buf_type == MINFS_BUFTYPE_FSINFO, the buffer's size
//                  has to be at least sizeof(MINFS_fs_info_t), else the at least
//                  2^p_fs_info->block_size bytes.
//      <block_n>   Block number (0-x)
//      <p_fs_info> Pointer to filesystem-info structure.
//      <file_id>   Is 0 if no file-block
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_WriteBlock(u8 buf_type, uint8_t *buffer, uint32_t block_n, MINFS_fs_info_t *p_fs_info){
  
}