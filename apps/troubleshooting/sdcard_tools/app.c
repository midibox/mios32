// $Id$
/*
 * SD Card Info
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <string.h>
#include <ff.h>

#include "app.h"
# include <FreeRTOS.h>
# include <semphr.h>
/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
// should be enabled for this application!
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// FatFs variables
static FATFS fs; // Work area (file system object) for logical drives
static u8 line_buffer[100];
static u8 dir_path[MAX_PATH];
static u8 tmp_buffer[_MAX_SS]; //_MAX_SS
static u16 line_ix;
static u8 disk_label[12];
static FIL fsrc, fdst; // File handles for copy routine.

xSemaphoreHandle xSDCardSemaphore;
# define MUTEX_SDCARD_TAKE { while( xSemaphoreTakeRecursive(xSDCardSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_SDCARD_GIVE { xSemaphoreGiveRecursive(xSDCardSemaphore); }

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();

  MIOS32_SDCARD_Init(0);
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(APP_TERMINAL_Parse);
  
  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;
  
#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
#endif
}




/////////////////////////////////////////////////////////////////////////////
// This function prints some useful SD card informations
/////////////////////////////////////////////////////////////////////////////
s32 SDCARD_Mount(void)
{
  int status = 0;
  // mount SD Card
  FRESULT res;
  DIR dir;
  DEBUG_MSG("Mounting SD Card...\n");
  if( (res=f_mount(0, &fs)) != FR_OK ) {
    DEBUG_MSG("Failed to mount SD Card - error status: %d\n", res);
    return -1; // error
  }

    // Set dir_path to / as we have a new card.
  strcpy(dir_path, "/");
  if( (res=f_opendir(&dir, dir_path)) != FR_OK ) {
    DEBUG_MSG("Failed to open directory %s - error status: %d\n",dir_path, res);
    return -1; // error
  }

  // Get volume label from base sector  
  if(disk_read(0, &dir.fs->win, dir.fs->dirbase,  1) != 0){
    DEBUG_MSG("Couldn't read directory sector...\n");
    return -1;   
  }  
  strncpy( disk_label, dir.fs->win, 11 );
  
  return 0;
}  


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  static u8 sdcard_available = 0;

  // init LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // turn off LED (will be turned on once SD card connected/detected)
  MIOS32_BOARD_LED_Set(0x1, 0x0);

  // endless loop
  while( 1 ) {
    // check if SD card is available
    // High-speed access if SD card was previously available
    u8 prev_sdcard_available = sdcard_available;
	MUTEX_SDCARD_TAKE
    sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);
	MUTEX_SDCARD_GIVE
    if( sdcard_available && !prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x1); // turn on LED
#if DEBUG_VERBOSE_LEVEL >= 0
      // always print...
      DEBUG_MSG("SD Card has been connected!\n");
#endif

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("connected       ");

#if DEBUG_VERBOSE_LEVEL >= 1
      if( SDCARD_Mount() < 0 ) {
	DEBUG_MSG("ERROR while mounting SD Card!\n");
      } else {
	DEBUG_MSG("SUCCESS: SD Card mounted!\n");
      }
#endif
    } else if( !sdcard_available && prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
#if DEBUG_VERBOSE_LEVEL >= 0
      // always print...
      DEBUG_MSG("SD Card disconnected!\n");
#endif

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("disconnected    ");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}

void SDCARD_Read_CID(void)
{
  int status = 0;
  // read CID data
  mios32_sdcard_cid_t cid;
  if( (status=MIOS32_SDCARD_CIDRead(&cid)) < 0 ) {
    DEBUG_MSG("ERROR: Reading CID failed with status %d!\n", status);
  } else {
    DEBUG_MSG("--------------------\n");
    DEBUG_MSG("CID:\n");
    DEBUG_MSG("- ManufacturerID:\n", cid.ManufacturerID);
    DEBUG_MSG("- OEM AppliID:\n", cid.OEM_AppliID);
    DEBUG_MSG("- ProdName: %s\n", cid.ProdName);
    DEBUG_MSG("- ProdRev: %u\n", cid.ProdRev);
    DEBUG_MSG("- ProdSN: 0x%08x\n", cid.ProdSN);
    DEBUG_MSG("- Reserved1: %u\n", cid.Reserved1);
    DEBUG_MSG("- ManufactDate: %u\n", cid.ManufactDate);
    DEBUG_MSG("- msd_CRC: 0x%02x\n", cid.msd_CRC);
    DEBUG_MSG("- Reserved2: %u\n", cid.Reserved2);
    DEBUG_MSG("--------------------\n");
  }
}


void SDCARD_Read_CSD(void)
{
  int status = 0;
  // read CSD data
  mios32_sdcard_csd_t csd;
  if( (status=MIOS32_SDCARD_CSDRead(&csd)) < 0 ) {
    DEBUG_MSG("ERROR: Reading CSD failed with status %d!\n", status);
  } else {
    DEBUG_MSG("--------------------\n");
    DEBUG_MSG("- CSDStruct: %u\n", csd.CSDStruct);
    DEBUG_MSG("- SysSpecVersion: %u\n", csd.SysSpecVersion);
    DEBUG_MSG("- Reserved1: %u\n", csd.Reserved1);
    DEBUG_MSG("- TAAC: %u\n", csd.TAAC);
    DEBUG_MSG("- NSAC: %u\n", csd.NSAC);
    DEBUG_MSG("- MaxBusClkFrec: %u\n", csd.MaxBusClkFrec);
    DEBUG_MSG("- CardComdClasses: %u\n", csd.CardComdClasses);
    DEBUG_MSG("- RdBlockLen: %u\n", csd.RdBlockLen);
    DEBUG_MSG("- PartBlockRead: %u\n", csd.PartBlockRead);
    DEBUG_MSG("- WrBlockMisalign: %u\n", csd.WrBlockMisalign);
    DEBUG_MSG("- RdBlockMisalign: %u\n", csd.RdBlockMisalign);
    DEBUG_MSG("- DSRImpl: %u\n", csd.DSRImpl);
    DEBUG_MSG("- Reserved2: %u\n", csd.Reserved2);
    DEBUG_MSG("- DeviceSize: %u\n", csd.DeviceSize);
    DEBUG_MSG("- MaxRdCurrentVDDMin: %u\n", csd.MaxRdCurrentVDDMin);
    DEBUG_MSG("- MaxRdCurrentVDDMax: %u\n", csd.MaxRdCurrentVDDMax);
    DEBUG_MSG("- MaxWrCurrentVDDMin: %u\n", csd.MaxWrCurrentVDDMin);
    DEBUG_MSG("- MaxWrCurrentVDDMax: %u\n", csd.MaxWrCurrentVDDMax);
    DEBUG_MSG("- DeviceSizeMul: %u\n", csd.DeviceSizeMul);
    DEBUG_MSG("- EraseGrSize: %u\n", csd.EraseGrSize);
    DEBUG_MSG("- EraseGrMul: %u\n", csd.EraseGrMul);
    DEBUG_MSG("- WrProtectGrSize: %u\n", csd.WrProtectGrSize);
    DEBUG_MSG("- WrProtectGrEnable: %u\n", csd.WrProtectGrEnable);
    DEBUG_MSG("- ManDeflECC: %u\n", csd.ManDeflECC);
    DEBUG_MSG("- WrSpeedFact: %u\n", csd.WrSpeedFact);
    DEBUG_MSG("- MaxWrBlockLen: %u\n", csd.MaxWrBlockLen);
    DEBUG_MSG("- WriteBlockPaPartial: %u\n", csd.WriteBlockPaPartial);
    DEBUG_MSG("- Reserved3: %u\n", csd.Reserved3);
    DEBUG_MSG("- ContentProtectAppli: %u\n", csd.ContentProtectAppli);
    DEBUG_MSG("- FileFormatGrouop: %u\n", csd.FileFormatGrouop);
    DEBUG_MSG("- CopyFlag: %u\n", csd.CopyFlag);
    DEBUG_MSG("- PermWrProtect: %u\n", csd.PermWrProtect);
    DEBUG_MSG("- TempWrProtect: %u\n", csd.TempWrProtect);
    DEBUG_MSG("- FileFormat: %u\n", csd.FileFormat);
    DEBUG_MSG("- ECC: %u\n", csd.ECC);
    DEBUG_MSG("- msd_CRC: 0x%02x\n", csd.msd_CRC);
    DEBUG_MSG("- Reserved4: %u\n", csd.Reserved4);
    DEBUG_MSG("--------------------\n");
  }
}

void SDCARD_Messages(FRESULT res)
{
 switch (res){
    case FR_OK: 			DEBUG_MSG("Operation completed successfully\n");break;
    case FR_INVALID_DRIVE: 	DEBUG_MSG("Invalid Drive\n");break;
    case FR_NOT_READY: 		DEBUG_MSG("Drive not ready\n");break;
    case FR_NOT_ENABLED:	DEBUG_MSG("Drive has no work area\n");break;
    case FR_DISK_ERR:		DEBUG_MSG("Disk Function Error\n");break;
    case FR_MKFS_ABORTED:	DEBUG_MSG("Drive Format Aborted!\n");break;
    case FR_NO_PATH: 		DEBUG_MSG("Could not find the path.\n");break;
    case FR_INVALID_NAME: 	DEBUG_MSG("The path name is invalid.\n");break;
    case FR_DENIED: 		DEBUG_MSG("Directory table or disk full.\n");break;
    case FR_EXIST: 			DEBUG_MSG("A file or directory with the same name already exists.\n");break;
    case FR_WRITE_PROTECTED:DEBUG_MSG("The drive is write protected.\n");break;
    case FR_INT_ERR: 		DEBUG_MSG("FAR structure or internal error.\n");break;
    case FR_NO_FILESYSTEM: 	DEBUG_MSG("The drive does not contain a valid FAT12/16/32 volume.\n");break;
    default: 				DEBUG_MSG("Unknown Error\n"); 
  }
}  

void SDCARD_Format(void)
{
#if _USE_MKFS && !_FS_READONLY
  DEBUG_MSG("Formatting SDCARD....\n");
  SDCARD_Messages(f_mkfs(0,0,0));
#else
  DEBUG_MSG("Please set _MKFS=1 and _FS_READONLY=0 in ffconf.h\n"); 
#endif
}

///////////////////////////////////////////////////////////////////
// These time and date functions and other bits of following code were adapted from 
// Rickey's world of Microelectronics under the creative commons 2.5 license.
// http://www.8051projects.net/mmc-sd-interface-fat16/final-code.php
void ShowFatTime( WORD ThisTime, char* msg )
{
   BYTE AM = 1;

   int Hour, Minute, Second;

   Hour = ThisTime >> 11;        // bits 15 through 11 hold Hour...
   Minute = ThisTime & 0x07E0;   // bits 10 through 5 hold Minute... 0000 0111 1110 0000
   Minute = Minute >> 5;
   Second = ThisTime & 0x001F;   //bits 4 through 0 hold Second...   0000 0000 0001 1111
   
   if( Hour > 11 )
   {
      AM = 0;
      if( Hour > 12 )
         Hour -= 12;
   }
     
   sprintf( msg, "%02d:%02d:%02d %s", Hour, Minute, Second*2,
         (AM)?"AM":"PM");
   return;
}

void ShowFatDate( WORD ThisDate, char* msg )
{

   int Year, Month, Day;

   Year = ThisDate >> 9;         // bits 15 through 9 hold year...
   Month = ThisDate & 0x01E0;    // bits 8 through 5 hold month... 0000 0001 1110 0000
   Month = Month >> 5;
   Day = ThisDate & 0x001F;      //bits 4 through 0 hold day...    0000 0000 0001 1111
   sprintf( msg, "%02d/%02d/%02d", Month, Day, Year-20);
   return;
}

void SDCARD_Dir(void)
{
  FRESULT res;
  DWORD free_clust;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;
  
#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif

  if (disk_label[0]==' ')
	DEBUG_MSG("Volume in Drive 0 has no label.\n");
  else
	DEBUG_MSG("Volume in Drive 0 %s\n",disk_label);
	
  DEBUG_MSG("Directory of 0:%s\n\n", dir_path);

  if( (res=f_opendir(&dir, dir_path)) != FR_OK ) {
    DEBUG_MSG("Failed to open directory %s - error status: %d\n",dir_path, res);
    return; // error
  }
  
  while (( f_readdir(&dir,&fno) == FR_OK) && fno.fname[0]) {

#if _USE_LFN
    fn = *fno.lfname ? fno.lfname : fno.fname;
#else
    fn = fno.fname;
#endif
    char date[10];
	ShowFatDate(fno.fdate,(char*)&date);
	char time[12];
	ShowFatTime(fno.ftime,(char*)&time);
	DEBUG_MSG("[%s%s%s%s%s%s%s] %s  %s   %s %u %s\n",
		(fno.fattrib & AM_RDO ) ? "r" : ".",
		(fno.fattrib & AM_HID ) ? "h" : ".",
		(fno.fattrib & AM_SYS ) ? "s" : ".",
		(fno.fattrib & AM_VOL ) ? "v" : ".",
		(fno.fattrib & AM_LFN ) ? "l" : ".",
		(fno.fattrib & AM_DIR ) ? "d" : ".",
		(fno.fattrib & AM_ARC ) ? "a" : ".",
		date,time,
		(fno.fattrib & AM_DIR) ? "<DIR>" : " ",
		fno.fsize,fn);
  }
  if (f_getfree("", &free_clust, &dir.fs))
  {
	DEBUG_MSG("f_getfree() failed...\n");
	return;
  }
  DEBUG_MSG("%u KB total disk space.\n",(DWORD)(fs.max_clust-2)*fs.csize/2);
  DEBUG_MSG("%u KB available on the disk.\n\n",free_clust*fs.csize/2);
  return; 
}


void SDCARD_FileSystem(void)
{
	DIR dir;
	FRESULT res;
    if( (res=f_opendir(&dir, "/")) != FR_OK ) {
      DEBUG_MSG("Failed to open directory %s - error status: %d\n",dir_path, res);
      return; // error
    }

    DEBUG_MSG("%u sector/s per cluster, %u clusters.\n", dir.fs->csize, dir.fs->max_clust);
    DEBUG_MSG("%u sectors per FAT, first FAT at sector #%u, root dir at #%u.\n", dir.fs->sects_fat, dir.fs->fatbase, dir.fs->dirbase);
    DEBUG_MSG("%u root dir entries (not valid for FAT32)\n", dir.fs->n_rootdir);
    char file_system[20];
    if( dir.fs->fs_type == FS_FAT12 )
      strcpy(file_system, "FAT12");
    else if( dir.fs->fs_type == FS_FAT16 )
      strcpy(file_system, "FAT16");
    else if( dir.fs->fs_type == FS_FAT32 )
      strcpy(file_system, "FAT32");
    else
      strcpy(file_system, "unknown FS");
    DEBUG_MSG("Filesystem: 0x%02x (%s)\n", dir.fs->fs_type, file_system);

}

void fullpath(char *source, char*dest)
{
  if (source[0]=='/') {
    strcpy(dest, source);
  } else {	
    strcpy(dest,dir_path);
	if (dest[1]!='\0')
	  strcat(dest,"/");
    strcat(dest,source);
  }
}

void SDCARD_CD(char *directory)
{
  u8 new_path[MAX_PATH];
  DIR dir;
  fullpath(directory,(char *)&new_path);
  if((f_opendir(&dir, new_path)) != FR_OK ) {
    DEBUG_MSG("The system cannot find the path specified");
  } else {
    strcpy(dir_path,new_path);
  }
  return;
}

void SDCARD_Delete(char *directory)
{
  u8 new_path[MAX_PATH];
  fullpath(directory,(char *)&new_path);
  if((f_unlink(new_path)) != FR_OK ) {
    DEBUG_MSG("The system cannot find the file/dir specified");
  } else {
    DEBUG_MSG("File or Directory deleted.");
  }
  return;
}

void SDCARD_Mkdir(char *directory)
{
  u8 new_path[MAX_PATH];
  fullpath(directory,(char *)&new_path);
  SDCARD_Messages(f_mkdir(new_path));
  return;
}

void SDCARD_Pwd(void)
{
  DEBUG_MSG("%s\n",dir_path);
  return;
}


void SDCARD_Rename(char* source, char* dest)
{  
  u8 new_source[MAX_PATH];
  u8 new_dest[MAX_PATH];

  fullpath(source,(char *)&new_source);
  fullpath(dest,(char *)&new_dest);

  DEBUG_MSG("Renaming/Moving from: %s to %s",new_source,new_dest);
  SDCARD_Messages(f_rename(new_source,new_dest));
  return;
}

SDCARD_Copy(char* source, char* dest)
{

  FRESULT res;
  s32 status = 0;
  u8 new_source[MAX_PATH];
  u8 new_dest[MAX_PATH];
  
  fullpath(source,(char *)&new_source);
  fullpath(dest,(char *)&new_dest);

  if( (res=f_open(&fsrc, new_source, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
    DEBUG_MSG("%s doesn't exist!\n", source);
  } else {
    if( (res=f_open(&fdst, new_dest, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK ) {
      DEBUG_MSG("Cannot create %s - file exists or invalid path\n", dest);
          return;
    }
  }
  DEBUG_MSG("Copying %s to %s\n",new_source,new_dest);

  mios32_sys_time_t t;
  t.seconds=0;
  t.fraction_ms=0;

  MIOS32_SYS_TimeSet(t);

  UINT successcount;
  UINT successcount_wr;
  u32 num_bytes = 0;
  do {
    if( (res=f_read(&fsrc, tmp_buffer, _MAX_SS, &successcount)) != FR_OK ) {
          DEBUG_MSG("Failed to read sector at position 0x%08x, status: %u\n", fsrc.fptr, res);
          successcount=0;
          status=-1;
    } else if( f_write(&fdst, tmp_buffer, successcount, &successcount_wr) != FR_OK ) {
          DEBUG_MSG("Failed to write sector at position 0x%08x, status: %u\n", fdst.fptr, res);
          status=-1;
    } else {
        num_bytes += successcount_wr;
    }
  } while( status==0 && successcount > 0 );
  mios32_sys_time_t t1=MIOS32_SYS_TimeGet();
  
  DEBUG_MSG("Copied: %d bytes in %d.%d seconds (%d KB/s)\n",num_bytes,t1.seconds,t1.fraction_ms,
		(long long)((((long long)num_bytes*1000)/((t1.seconds*1000)+t1.fraction_ms))/1000));

  f_close(&fdst);
}


void SDCARD_Benchmark(void)
{

  FRESULT res;
  s32 status = 0;
  u8 source[MAX_PATH];
  u8 dest[MAX_PATH];
  strcpy(source,"/benchmrk.tmp");
  strcpy(dest,"/benchmrk.cpy");
  int f;

  for(f=0;f<_MAX_SS;f++)
	tmp_buffer[f]='X';

  DEBUG_MSG("benchmark: Starting\n");

  mios32_sys_time_t t;
  t.seconds=0;
  t.fraction_ms=0;
  
  if( (res=f_open(&fsrc, source, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK ) {
    DEBUG_MSG("benchmark: Cannot create %s - file exists or invalid path\n", source);
	return;
  }

  UINT successcount;
  UINT successcount_wr;
  u32 num_bytes = 0;

  MIOS32_SYS_TimeSet(t);

  for (f=0;f<2048;f++) {
    if( f_write(&fsrc, tmp_buffer, _MAX_SS, &successcount_wr) != FR_OK ) {
	  DEBUG_MSG("Failed to write sector at position 0x%08x, status: %u\n", fsrc.fptr, res);
	  status=-1;
	  break;
	} else {
		num_bytes += successcount_wr;
	}
  }
  f_close(&fsrc);

  mios32_sys_time_t t1=MIOS32_SYS_TimeGet();

  DEBUG_MSG("Write: %d bytes in %d.%d seconds (%d KB/s)\n", (num_bytes), t1.seconds,t1.fraction_ms,
		(u32)(((num_bytes*1000)/((t1.seconds*1000)+t1.fraction_ms))/1000));
  

  if( (res=f_open(&fsrc, source, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
    DEBUG_MSG("%s doesn't exist!\n", source);
	return;
  }

  num_bytes = 0;

  MIOS32_SYS_TimeSet(t);

  do {
    if( (res=f_read(&fsrc, tmp_buffer, _MAX_SS, &successcount)) != FR_OK ) {
	  DEBUG_MSG("Failed to read sector at position 0x%08x, status: %u\n", fsrc.fptr, res);
	} else {
	num_bytes += successcount;
    }
  } while( successcount > 0 );

  t1 = MIOS32_SYS_TimeGet();
  
  DEBUG_MSG("Read: %d bytes in %d.%d seconds (%d KB/s)\n",num_bytes,t1.seconds,t1.fraction_ms,
		(u32)(((num_bytes*1000)/((t1.seconds*1000)+t1.fraction_ms))/1000));

  f_close(&fsrc);

  if( (res=f_open(&fsrc, source, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
    DEBUG_MSG("%s doesn't exist!\n", source);
  } else {
    if( (res=f_open(&fdst, dest, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK ) {
      DEBUG_MSG("Cannot create %s - file exists or invalid path\n", dest);
	  return;
    }
  }

  MIOS32_SYS_TimeSet(t);

  num_bytes = 0;
  do {
    if( (res=f_read(&fsrc, tmp_buffer, _MAX_SS, &successcount)) != FR_OK ) {
	  DEBUG_MSG("Failed to read sector at position 0x%08x, status: %u\n", fsrc.fptr, res);
	  successcount=0;
	  status=-1;
    } else if( f_write(&fdst, tmp_buffer, successcount, &successcount_wr) != FR_OK ) {
	  DEBUG_MSG("Failed to write sector at position 0x%08x, status: %u\n", fdst.fptr, res);
	  status=-1;
    } else {
	num_bytes += successcount_wr;
    }
  } while( status==0 && successcount > 0 );

  t1 = MIOS32_SYS_TimeGet();
  
  DEBUG_MSG("Copy: %d bytes in %d.%d seconds (%d KB/s)\n",num_bytes,t1.seconds,t1.fraction_ms,
		(u32)(((num_bytes*1000)/((t1.seconds*1000)+t1.fraction_ms))/1000));

  f_close(&fdst);
  f_unlink(source);
  f_unlink(dest);
}



/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
void APP_TERMINAL_Parse(mios32_midi_port_t port, u8 byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    // example for parsing the command:
    char *separators = " \t";
    char *brkt;
    char *parameter[10];
    if( parameter[0] = strtok_r(line_buffer, separators, &brkt) ) {
	  u8 f;
	  for(f=1;f<10;f++) {
	    if( (parameter[f] = strtok_r(NULL, separators, &brkt)) == NULL) 
		  break;
	  }
      MUTEX_SDCARD_TAKE
      if( strncmp(parameter[0], "help", 4) == 0 ) {
	DEBUG_MSG("Following commands are available:");
	DEBUG_MSG("  cid:         Display CID structure\n");
	DEBUG_MSG("  csd:         Display CSD structure\n");
	DEBUG_MSG("  filesys:     Display filesystem info\n");
	DEBUG_MSG("  dir:         Display files in current directory\n");
	DEBUG_MSG("  cd:          Print or Change current directory\n");
	DEBUG_MSG("  mkdir:       Create new directory\n");
	DEBUG_MSG("  rename:      Rename/Move file or directory\n");
	DEBUG_MSG("  copy:        Copy file\n");
	DEBUG_MSG("  delete:      Delete file or directory\n");
	DEBUG_MSG("  benchmark:   benchmark (read/write/copy 1MB file)\n");
	DEBUG_MSG("  format:      Format sdcard *destroys all contents of card*\n");
      } else if( strncmp(parameter[0], "format", 6) == 0 ) {
		  if( strncmp(parameter[1], "yes", 3) == 0 ) 
			SDCARD_Format();
		  else
		    DEBUG_MSG("Please type \"format yes\" to format sd/mmc card");
      } else if( strncmp(parameter[0], "cid", 3) == 0 ) {
		SDCARD_Read_CID();
      } else if( strncmp(parameter[0], "csd", 3) == 0 ) {
		SDCARD_Read_CSD();
      } else if( strncmp(parameter[0], "filesys", 7) == 0 ) {
		SDCARD_FileSystem();
      } else if( strncmp(parameter[0], "dir", 3) == 0 ) {
		SDCARD_Dir();
      } else if( strncmp(parameter[0], "cd", 2) == 0 ) {
	    if(parameter[1]) 
	      SDCARD_CD(parameter[1]);
		else
	      SDCARD_Pwd();
      } else if( strncmp(parameter[0], "mkdir", 5) == 0 ) {
	    if(parameter[1]) 
	      SDCARD_Mkdir(parameter[1]);
	    else
	      DEBUG_MSG("mkdir: No directory specified");
      } else if( strncmp(parameter[0], "delete", 6) == 0 ) {
	    if(parameter[1]) 
	      SDCARD_Delete(parameter[1]);
	    else
	      DEBUG_MSG("delete: No file/directory specified");
      } else if( strncmp(parameter[0], "rename", 6) == 0 ) {
	    if(parameter[1] && parameter[2]) 
	      SDCARD_Rename(parameter[1],parameter[2]);
	    else
	      DEBUG_MSG("rename: <source> and <destination> filenames required");
      } else if( strncmp(parameter[0], "copy", 4) == 0 ) {
	    if(parameter[1] && parameter[2]) 
	      SDCARD_Copy(parameter[1],parameter[2]);
	    else
	      DEBUG_MSG("copy: <source> and <destination> filenames required");
      } else if( strncmp(parameter[0], "benchmark", 9) == 0 ) {
		SDCARD_Benchmark();
      } else {
	DEBUG_MSG("Unknown command - type 'help' to list available commands!\n");
      }
	  MUTEX_SDCARD_GIVE
    }

    line_ix = 0;

  } else if( line_ix < (100-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return;
}
