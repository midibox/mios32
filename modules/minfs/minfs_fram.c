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
// MINFS OS Hook functions
/////////////////////////////////////////////////////////////////////////////
int32_t MINFS_Read(uint8_t block_type, uint8_t **buffer, uint32_t block_n, uint32_t data_offset, uint32_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

int32_t MINFS_Write(uint8_t block_type, uint8_t *buffer, uint32_t block_n, uint32_t data_offset, uint32_t data_len, MINFS_fs_t *p_fs, uint32_t file_id){
}

uint8_t* MINFS_WriteBufGet(uint8_t block_type, uint32_t block_n, MINFS_fs_t *p_fs, uint32_t file_id){
}
