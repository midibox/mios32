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
