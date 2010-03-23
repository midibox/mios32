/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "mios32.h" // Needed for mios32_sdcard_csd_t
#include "diskio.h"

// TK: defined in integer.h as bool - alternative enum here
//typedef enum { FALSE = 0, TRUE } BOOL;
#define FALSE 0
#define TRUE 1

/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

#define SDCARD	        0

static DWORD sdcard_sector_count;


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
  if( drv == SDCARD ) {
    // check availability of SD Card
    // we assume that it has been initialized by application
    sdcard_sector_count = 0xffffffff; // TODO
#if DEBUG_VERBOSE_LEVEL >= 2
    MIOS32_MIDI_SendDebugMessage("[disk_init] size = %u\n", sdcard_sector_count);
#endif

    int status;
    if( (status=MIOS32_SDCARD_CheckAvailable(1)) < 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[disk_initialize] error while checking for SD Card (status %d)\n", status);
#endif
      return STA_NODISK;
    }

    return 0;
  }

  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
  if( drv == SDCARD ) {
    // we assume that SD Card is always available until Read/Write operation fails
    // we don't use MIOS32_SDCARD_CheckAvailable() here, since the status is checked very frequently!
    return 0;
  }

  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
  if( drv == SDCARD ) {
    int i;

    for(i=0; i<count; ++i) {
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("[disk_read] sector %d (#%d/%d)\n", sector+i, i+1, count);
#endif

      if( MIOS32_SDCARD_SectorRead(sector + i, buff + i*512) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	MIOS32_MIDI_SendDebugMessage("[disk_read] error while reading sector %d\n", sector+i);
#endif
	return RES_ERROR;
      }
    }

    return RES_OK;
  }

  return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
  if( drv == SDCARD ) {
    int i;

    for(i=0; i<count; ++i) {
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("[disk_write] sector %d (#%d/%d)\n", sector+i, i+1, count);
#endif
      if( MIOS32_SDCARD_SectorWrite(sector + i, (u8 *)buff + 512*i) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	MIOS32_MIDI_SendDebugMessage("[disk_write] error while writing to sector %d\n", sector+i);
#endif
	return RES_ERROR;
      }
    }

    return RES_OK;
  }

  return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)

{

  DRESULT res;
  if (drv) return RES_PARERR; // Only 1 drive is supported!
  res = RES_ERROR;

  if( drv == SDCARD ) {
    mios32_sdcard_csd_t csd; /* Buffer to hold CSD */
    switch( ctrl ) {
    case CTRL_SYNC: /* Mandatory for write functions */
      // Make sure that the disk drive has finished pending write process.
      // When the disk I/O module has a write back cache, flush the dirty sector immediately.
      // This command is not required in read-only configuration.
      res= RES_OK; // nothing to do (yet)
	  break;

    case GET_SECTOR_COUNT: /* Mandatory for only f_mkfs() */
      // Returns total sectors on the drive into the DWORD variable pointed by Buffer.
      // This command is used in only f_mkfs function.
      //*(DWORD*)buff = sdcard_sector_count;
	  MIOS32_SDCARD_CSDRead(&csd);
	  u32 sectors;
	  if (csd.CSDStruct==1) // SD V2 
  	    sectors = (DWORD)(csd.DeviceSize + 1) << 10;
      else { // V1 and MMC cards use the same (size << multiplier) scheme.
		sectors = (DWORD)(csd.DeviceSize + 1) << (csd.DeviceSizeMul + 2);
	  }
	  *(DWORD*)buff=sectors;
      res= RES_OK;
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[GET_SECTOR_COUNT] Count is %d\n", sectors);
#endif
	  break;
    case GET_SECTOR_SIZE: /* Mandatory for multiple sector size cfg */
      // Returns sector size of the drive into the WORD variable pointed by Buffer.
      // This command is not required in single sector size configuration, _MAX_SS is 512.
#if _MAX_SS
      *(DWORD*)buff = 512; // only single size supported by MIOS32
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[GET_SECTOR_SIZE] Sector Size is %d\n", 512);
#endif

      res = RES_OK;
#else
      res = RES_PARERR;
#endif
	  break;

    case GET_BLOCK_SIZE: /* Mandatory for only f_mkfs() */
      // Returns erase block size of the memory array in unit of sector into the DWORD variable
      // pointed by Buffer. When the erase block size is unknown or magnetic disk device, 
      // return 1. This command is used in only f_mkfs function.
	  MIOS32_SDCARD_CSDRead(&csd);
	  u32 size;
	  if (csd.CSDStruct==1){  // SD V2
		// Some SDHC cards seem to report block size differently.
		// Try one way, if result is too small, try another! 
		size = 16UL << (csd.DeviceSizeMul >> 4);
		if (size<2)
			size = 16UL << (csd.DeviceSizeMul);
	  } else { // SD v1
	    size = (DWORD)(csd.EraseGrSize)*(csd.MaxWrBlockLen);
	  }
	  res=RES_OK;
		
	  *(DWORD*)buff = size;
	  
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[GET_BLOCK_SIZE] Block Size is %d\n", size);
#endif
	  break;

    default:
      // other commands listed in diskio.h not mandatory and not supported yet
      res = RES_PARERR;
    }
  }

  return res;
}


/*-----------------------------------------------------------------------*/
/* TK: temporary implemented here                                        */

DWORD get_fattime(void)
{
  /* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
  /* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
  return 0;
}
