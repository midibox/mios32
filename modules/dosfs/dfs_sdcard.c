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

/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// for caching - this feature has to be explicitely enabled, as it isn't reentrant
// and requires to use the same buffer pointer whenever reading a file.
static uint32_t last_sector;
static u8 caching_enabled   = 0;

void DFS_CachingEnabledSet(uint8_t enable)
{
  caching_enabled = enable;
  last_sector = 0xffffffff;
}


/////////////////////////////////////////////////////////////////////////////
// Converts directory name to canonical name
// (missing pendant to DFS_anonicalToDir)
// dest must point to a 13-byte buffer
/////////////////////////////////////////////////////////////////////////////
char *DFS_DirToCanonical(char *dest, char *src)
{
  u8 pos = 0;

  while( pos < 8 && src[pos] != ' ' )
    *dest++ = src[pos++];

  if( src[8] != ' ' && src[9] != ' ' && src[10] != ' ' ) {
    *dest++ = '.';
  
    pos = 8;
    while( pos < 11 && src[pos] != ' ' )
      *dest++ = src[pos++];
  }
    
  *dest = 0; // terminate string

  return dest;
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

  // forward to MIOS
  s32 status;
  if( (status=MIOS32_SDCARD_SectorRead(sector, buffer)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[DFS_ReadSector] Error during reading sector %u, status %d\n", sector, status);
#endif
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

  // invalidate cache
  last_sector = 0xffffffff;

  // forward to MIOS
  s32 status;
  if( (status=MIOS32_SDCARD_SectorWrite(sector, buffer)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[DFS_WriteSector] Error during writing sector %u, status %d\n", sector, status);
#endif
    return 3; // cannot access SD Card
  }

  // success
  return 0;
}
