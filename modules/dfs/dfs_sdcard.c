// $Id$
/*
 * Access layer between DOSFS and MIOS32
 * See README_1st.txt for details
 *
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <sys/types.h>
#include "dosfs.h"

// for caching - this feature has to be explicitely enabled, as it isn't reentrant
// and requires to use the same buffer pointer whenever reading a file.
uint32_t last_sector;
u8 caching_enabled   = 0;

void DFS_CachingEnabledSet(uint8_t enable)
{
  caching_enabled = enable;
  last_sector = 0xffffffff;
}

/*
	Read sector from SD Card
	Returns 0 OK, nonzero for any error
*/
uint32_t DFS_ReadSector(uint8_t unit, uint8_t *buffer, uint32_t sector, uint32_t count)
{
  // only allow access to single unit
  if( unit != 0 )
    return 1; 

  // according to README.txt, count is always 1 - check this!
  if( count != 1 )
    return 2;

  // cache:
  if( caching_enabled && sector == last_sector ) {
    // we assume that sector is already in *buffer
    // since the user has to take care that the same buffer is used for file reads, this
    // feature has to be explicitely enabled with DFS_CachingEnabledSet(uint8_t enable)
    return 0;
  }

  last_sector = sector;

  // forward to MIOS, reconnect to SD Card if required
  s32 status;
  if( (status=MIOS32_SDCARD_SectorRead(sector, buffer) < 0) ) {
    // try re-connection
    if( (status=MIOS32_SDCARD_PowerOn()) >= 0 ) {
      status=MIOS32_SDCARD_SectorRead(sector, buffer);
    }

    if( status < 0 )
      return 3; // cannot access SD Card
  }

  // success
  return 0;
}

/*
	Write sector to SD Card
	Returns 0 OK, nonzero for any error
*/
uint32_t DFS_WriteSector(uint8_t unit, uint8_t *buffer, uint32_t sector, uint32_t count)
{
  // only allow access to single unit
  if( unit != 0 )
    return 1; 

  // according to README.txt, count is always 1 - check this!
  if( count != 1 )
    return 2;

  // invalidate cache if same sector is written
  if( last_sector == sector )
    last_sector = 0xffffffff;

  // forward to MIOS, reconnect to SD Card if required
  s32 status;
  if( (status=MIOS32_SDCARD_SectorWrite(sector, buffer) < 0) ) {
    // try re-connection
    if( (status=MIOS32_SDCARD_PowerOn()) >= 0 ) {
      status=MIOS32_SDCARD_SectorWrite(sector, buffer);
    }

    if( status < 0 )
      return 3; // cannot access SD Card
  }

  // success
  return 0;
}
