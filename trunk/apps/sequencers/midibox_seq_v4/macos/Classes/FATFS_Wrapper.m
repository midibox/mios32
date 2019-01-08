/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
//
//  FATFS_Wrapper.m
//
//  Created by Thorsten Klose on 08.02.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "FATFS_Wrapper.h"
#import "MIOS32_SDCARD_Wrapper.h"

#include <mios32.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// avoid that definitions in fatfs/src/integer.h will be included - BOOL clashes with ObjC definitions
#define _INTEGER
/* These types must be 16-bit, 32-bit or larger integer */
typedef int				INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef signed char		CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types must be 16-bit integer */
typedef short			SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types must be 32-bit integer */
typedef long			LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;


// located in FATFS_Wrapper_Dir.c
extern int f_opendir_hlp(const char *path);
extern int f_readdir_hlp(char *fname, unsigned char *fattrib, unsigned *fsize, unsigned short *fdate, unsigned short *ftime);


#include "ff.h"
#include "diskio.h"


@implementation FATFS_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


/*-----------------------------------------------------------------------*/
/* Mount/Unmount a Locical Drive                                         */
/*-----------------------------------------------------------------------*/
FRESULT f_mount (
	BYTE vol,		/* Logical drive number to be mounted/unmounted */
	FATFS *fs		/* Pointer to new file system object (NULL for unmount)*/
)
{
    if (vol >= _DRIVES)				/* Check if the drive number is valid */
        return FR_INVALID_DRIVE;

    return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Open or Create a File                                                 */
/*-----------------------------------------------------------------------*/
FRESULT f_open (
	FIL *fp,			/* Pointer to the blank file object */
	const XCHAR *path,	/* Pointer to the file name */
	BYTE mode			/* Access mode and file open mode flags */
)
{
    if( SDCARD_Wrapper_getDir() == nil )
        return FR_NOT_ENABLED; // SD Card directory not specified -> "emulated" SD Card not available

    NSString *fullPath = [[NSString alloc] initWithFormat:@"%@/%s", SDCARD_Wrapper_getDir(), path];
    char fullPathC[1024];
    [fullPath getCString:fullPathC];
	[fullPath release];

	//NSLog(@"Opening '%s'\n", fullPathC);
	FILE *f = NULL;
	if( mode & FA_WRITE ) {
        if( (mode & FA_CREATE_NEW) ) {
			f = fopen(fullPathC, "w+"); // create a new file
            if( f == NULL )
                return FR_DENIED;
        } else {
            f = fopen(fullPathC, "r+"); // try to open file for read/write access
            if( f == NULL && (mode & FA_CREATE_ALWAYS) ) // if file doesn't exist
                f = fopen(fullPathC, "w+"); // create a new file
            if( f == NULL )
                return FR_DENIED;
        }
	} else {
		f = fopen(fullPathC, "r");
		if( f == NULL )
			return FR_NO_FILE;
	}

    // determine file length and set pointer
	fseek(f, 0 , SEEK_END);
	fp->fsize = ftell(f);
	rewind(f);
	fp->fptr = 0;

	//NSLog(@"Len: %d\n", fp->fsize);

	// mis-use pointer to dir entry to store file reference
    fp->dir_ptr = (FATFS*)f;

	return FR_OK;
}


/*-----------------------------------------------------------------------*/
/* Read File                                                             */
/*-----------------------------------------------------------------------*/
FRESULT f_read (
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	UINT *br		/* Pointer to number of bytes read */
)
{
	*br = fread(buff, 1, btr, (FILE *)fp->dir_ptr);
	fp->fptr += *br;

	return *br ? FR_OK : FR_DISK_ERR;
}


/*-----------------------------------------------------------------------*/
/* Write File                                                            */
/*-----------------------------------------------------------------------*/

FRESULT f_write (
	FIL *fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	UINT btw,			/* Number of bytes to write */
	UINT *bw			/* Pointer to number of bytes written */
)
{
	*bw = fwrite(buff, 1, btw, (FILE *)fp->dir_ptr);
	fp->fptr += *bw;

	return *bw ? FR_OK : FR_DISK_ERR;
}




/*-----------------------------------------------------------------------*/
/* Synchronize the File Object                                           */
/*-----------------------------------------------------------------------*/
FRESULT f_sync (
	FIL *fp		/* Pointer to the file object */
)
{
    // TODO: sync file
    return FR_OK;
}



/*-----------------------------------------------------------------------*/
/* Close File                                                            */
/*-----------------------------------------------------------------------*/
FRESULT f_close (
	FIL *fp		/* Pointer to the file object to be closed */
)
{
	if( fp->dir_ptr != NULL ) {
//		fseek((FILE *)fp->dir_ptr, 0 , SEEK_END); // important for write operations, otherwise file will be truncated!
		fclose((FILE *)fp->dir_ptr);
	}

	fp->dir_ptr = NULL;

	return FR_OK;
}


/*-----------------------------------------------------------------------*/
/* Seek File R/W Pointer                                                 */
/*-----------------------------------------------------------------------*/
FRESULT f_lseek (
	FIL *fp,		/* Pointer to the file object */
	DWORD ofs		/* File pointer from top of file */
)
{
	fseek((FILE *)fp->dir_ptr, ofs, SEEK_SET);
	fp->fptr = ofs;
	return FR_OK;
}


/*-----------------------------------------------------------------------*/
/* Create a Directroy Object                                             */
/*-----------------------------------------------------------------------*/
FRESULT f_opendir (
	DIR *dj,			/* Pointer to directory object to create */
	const XCHAR *path	/* Pointer to the directory path */
)
{
    if( SDCARD_Wrapper_getDir() == nil )
        return FR_NOT_ENABLED; // SD Card directory not specified -> "emulated" SD Card not available

    NSString *fullPath = [[NSString alloc] initWithFormat:@"%@/%s", SDCARD_Wrapper_getDir(), path];
    char fullPathC[1024];
    [fullPath getCString:fullPathC];
	[fullPath release];

    if( f_opendir_hlp(fullPathC) < 0 )
        return FR_NO_PATH;

    return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Read Directory Entry in Sequense                                      */
/*-----------------------------------------------------------------------*/
FRESULT f_readdir (
	DIR *dj,			/* Pointer to the open directory object */
	FILINFO *fno		/* Pointer to file information to return */
)
{
    if( f_readdir_hlp(fno->fname, &fno->fattrib, &fno->fsize, &fno->fdate, &fno->ftime) < 0 )
        return FR_NO_FILE;

    return FR_OK;
}


/*-----------------------------------------------------------------------*/
/* Get Number of Free Clusters                                           */
/*-----------------------------------------------------------------------*/
FRESULT f_getfree (
	const XCHAR *path,	/* Pointer to the logical drive number (root dir) */
	DWORD *nclst,		/* Pointer to the variable to return number of free clusters */
	FATFS **fatfs		/* Pointer to pointer to corresponding file system object to return */
)
{
    // TODO
    return FR_OK;
}


/*-----------------------------------------------------------------------*/
/* Create a Directory                                                    */
/*-----------------------------------------------------------------------*/
FRESULT f_mkdir (
	const XCHAR *path		/* Pointer to the directory path */
)
{
    if( SDCARD_Wrapper_getDir() == nil )
        return FR_NOT_ENABLED; // SD Card directory not specified -> "emulated" SD Card not available

    NSString *fullPath = [[NSString alloc] initWithFormat:@"%@/%s", SDCARD_Wrapper_getDir(), path];
    char fullPathC[1024];
    [fullPath getCString:fullPathC];
	[fullPath release];

    if( mkdir(fullPathC, 0755) < 0 )
        return FR_EXIST; // or no permission - not checked in this simplified version

    return FR_OK;
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
    // DUMMY
    return FR_OK;
}

@end
