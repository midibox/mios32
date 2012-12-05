// $Id$
/*
 * Patch Layer for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"

#include "mbng_patch.h"
#include "mbng_dout.h"
#include "mbng_file.h"
#include "mbng_file_p.h"


/////////////////////////////////////////////////////////////////////////////
// Preset patch
/////////////////////////////////////////////////////////////////////////////

mbng_patch_matrix_entry_t mbng_patch_matrix[MBNG_PATCH_NUM_MATRIX];

mbng_patch_cfg_t mbng_patch_cfg = {
  .debounce_ctr = 20,
  .global_chn = 0,
  .all_notes_off_chn = 0,
  .button_group_size = 128,
  .enc_group_size = 64,
  .ain_group_size = 6,
  .ainser_group_size = 64,
};


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int matrix;
  mbng_patch_matrix_entry_t *m = (mbng_patch_matrix_entry_t *)&mbng_patch_matrix[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX; ++matrix, ++m) {
    m->sr_din = 0;
    m->sr_dout = 0;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Load(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBNG_FILE_P_Read(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_PATCH_Store(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBNG_FILE_P_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}
