// $Id$
/*
 * Patch Layer for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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

#include <app.h>
#include <MbCvEnvironment.h>


#include "mbcv_map.h"
#include "mbcv_patch.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Preset patch
/////////////////////////////////////////////////////////////////////////////

u8 mbcv_patch_gateclr_cycles = 3; // 3 mS

                       // <------------------>
u8 mbcv_patch_name[21] = "BasicDefault Patch  ";


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns a parameter from patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
u16 MBCV_PATCH_ReadPar(u16 addr)
{
  int cv = addr / CV_PATCH_SIZE;

  if( cv >= CV_SE_NUM )
    return 0;

  u16 nrpnNumber = addr % CV_PATCH_SIZE;

  MbCvEnvironment* env = APP_GetEnv();
  MbCv *s = &env->mbCv[cv];

  u16 value = 0;
  s->getNRPN(nrpnNumber, &value);
  return value;
}


/////////////////////////////////////////////////////////////////////////////
// This function writes a parameter into patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_WritePar(u16 addr, u8 value)
{
  int cv = addr / CV_PATCH_SIZE;

  if( cv >= CV_SE_NUM )
    return -1; // invalid address

  u16 nrpnNumber = addr % CV_PATCH_SIZE;

  MbCvEnvironment* env = APP_GetEnv();
  MbCv *s = &env->mbCv[cv];

  return s->setNRPN(nrpnNumber, value) ? 0 : -1; // value not mapped
}


/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Load(u8 bank, u8 patch)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_B_PatchRead(bank, patch);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Store(u8 bank, u8 patch)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_B_PatchWrite(bank, patch, 1);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_LoadGlobal(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Read(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_StoreGlobal(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Link to MBCV Environment
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Copy(u8 channel, u16* buffer)
{
  MbCvEnvironment* env = APP_GetEnv();
  env->channelCopy(channel, buffer);
  return 0;
}

s32 MBCV_PATCH_Paste(u8 channel, u16* buffer)
{
  MbCvEnvironment* env = APP_GetEnv();
  env->channelPaste(channel, buffer);
  return 0;
}

s32 MBCV_PATCH_Clear(u8 channel)
{
  MbCvEnvironment* env = APP_GetEnv();
  env->channelClear(channel);
  return 0;
}

u16* MBCV_PATCH_CopyBufferGet(void)
{
  MbCvEnvironment* env = APP_GetEnv();
  return (u16 *)env->copyBuffer;
}
