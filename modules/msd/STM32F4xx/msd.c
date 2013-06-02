// $Id$
// Dummy

#include <mios32.h>
#include "msd.h"


s32 MSD_Init(u32 mode)
{
  return -1;
}

s32 MSD_Periodic_mS(void)
{
  return -1;
}

s32 MSD_CheckAvailable(void)
{
  return 0;
}

s32 MSD_LUN_AvailableSet(u8 lun, u8 available)
{
  return -1;
}

s32 MSD_LUN_AvailableGet(u8 lun)
{
  return 0;
}

s32 MSD_RdLEDGet(u16 lag_ms)
{
  return 0;
}

s32 MSD_WrLEDGet(u16 lag_ms)
{
  return 0;
}

#include <mios32.h>
