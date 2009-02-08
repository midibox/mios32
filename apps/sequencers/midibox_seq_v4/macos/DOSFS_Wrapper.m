//
//  DOSFS_Wrapper.m
//
//  Created by Thorsten Klose on 08.02.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "DOSFS_Wrapper.h"
#import "MIOS32_SDCARD_Wrapper.h"

#include <mios32.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dosfs.h"


@implementation DOSFS_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


/*
	Get starting sector# of specified partition on drive #unit
	NOTE: This code ASSUMES an MBR on the disk.
	scratchsector should point to a SECTOR_SIZE scratch area
	Returns 0xffffffff for any error.
	If pactive is non-NULL, this function also returns the partition active flag.
	If pptype is non-NULL, this function also returns the partition type.
	If psize is non-NULL, this function also returns the partition size.
*/
uint32_t DFS_GetPtnStart(uint8_t unit, uint8_t *scratchsector, uint8_t pnum, uint8_t *pactive, uint8_t *pptype, uint32_t *psize)
{
	// not relevant
	return DFS_OK;
}


/*
	Retrieve volume info from BPB and store it in a VOLINFO structure
	You must provide the unit and starting sector of the filesystem, and
	a pointer to a sector buffer for scratch
	Attempts to read BPB and glean information about the FS from that.
	Returns 0 OK, nonzero for any error.
*/
uint32_t DFS_GetVolInfo(uint8_t unit, uint8_t *scratchsector, uint32_t startsector, PVOLINFO volinfo)
{
	// dummy values
	volinfo->unit = unit;
	volinfo->startsector = startsector;
	volinfo->secperclus = 32;
	volinfo->reservedsecs = 0;
	volinfo->numsecs = 2 << 31;
	volinfo->secperfat = 2 << 31;
	memcpy(volinfo->label, "Emulated   ", 11);
	volinfo->label[11] = 0;
	// note: if rootentries is 0, we must be in a FAT32 volume.
	volinfo->rootentries = 0;
	volinfo->fat1 = 0;
	volinfo->rootdir = 0;
	volinfo->dataarea = 0;
	volinfo->numclusters = 1;
	volinfo->filesystem = FAT32;

	return (SDCARD_Wrapper_getDir() == nil) ? DFS_ERRMISC : DFS_OK;
}

/*
	Fetch FAT entry for specified cluster number
	You must provide a scratch buffer for one sector (SECTOR_SIZE) and a populated VOLINFO
	Returns a FAT32 BAD_CLUSTER value for any error, otherwise the contents of the desired
	FAT entry.
	scratchcache should point to a UINT32. This variable caches the physical sector number
	last read into the scratch buffer for performance enhancement reasons.
*/
uint32_t DFS_GetFAT(PVOLINFO volinfo, uint8_t *scratch, uint32_t *scratchcache, uint32_t cluster)
{
	// not relevant
	return DFS_OK;
}


/*
	Set FAT entry for specified cluster number
	You must provide a scratch buffer for one sector (SECTOR_SIZE) and a populated VOLINFO
	Returns DFS_ERRMISC for any error, otherwise DFS_OK
	scratchcache should point to a UINT32. This variable caches the physical sector number
	last read into the scratch buffer for performance enhancement reasons.

	NOTE: This code is HIGHLY WRITE-INEFFICIENT, particularly for flash media. Considerable
	performance gains can be realized by caching the sector. However this is difficult to
	achieve on FAT12 without requiring 2 sector buffers of scratch space, and it is a design
	requirement of this code to operate on a single 512-byte scratch.

	If you are operating DOSFS over flash, you are strongly advised to implement a writeback
	cache in your physical I/O driver. This will speed up your code significantly and will
	also conserve power and flash write life.
*/
uint32_t DFS_SetFAT(PVOLINFO volinfo, uint8_t *scratch, uint32_t *scratchcache, uint32_t cluster, uint32_t new_contents)
{
	// not relevant
	return DFS_OK;
}

/*
	Convert a filename element from canonical (8.3) to directory entry (11) form
	src must point to the first non-separator character.
	dest must point to a 12-byte buffer.
*/
uint8_t *DFS_CanonicalToDir(uint8_t *dest, uint8_t *src)
{
	uint8_t *destptr = dest;

	memset(dest, ' ', 11);
	dest[11] = 0;

	while (*src && (*src != DIR_SEPARATOR) && (destptr - dest < 11)) {
		if (*src >= 'a' && *src <='z') {
			*destptr++ = (*src - 'a') + 'A';
			src++;
		}
		else if (*src == '.') {
			src++;
			destptr = dest + 8;
		}
		else {
			*destptr++ = *src++;
		}
	}

	return dest;
}

/*
	Find the first unused FAT entry
	You must provide a scratch buffer for one sector (SECTOR_SIZE) and a populated VOLINFO
	Returns a FAT32 BAD_CLUSTER value for any error, otherwise the contents of the desired
	FAT entry.
	Returns FAT32 bad_sector (0x0ffffff7) if there is no free cluster available
*/
uint32_t DFS_GetFreeFAT(PVOLINFO volinfo, uint8_t *scratch)
{
	// not relevant
	return 0x0ffffff7;		// Can't find a free cluster
}


/*
	Open a directory for enumeration by DFS_GetNextDirEnt
	You must supply a populated VOLINFO (see DFS_GetVolInfo)
        ** you must also make sure dirinfo->scratch is valid in the dirinfo you pass it** //reza
	The empty string or a string containing only the directory separator are
	considered to be the root directory.
	Returns 0 OK, nonzero for any error.
*/
uint32_t DFS_OpenDir(PVOLINFO volinfo, uint8_t *dirname, PDIRINFO dirinfo)
{
	// TODO
	return DFS_OK;
}

/*
	Get next entry in opened directory structure. Copies fields into the dirent
	structure, updates dirinfo. Note that it is the _caller's_ responsibility to
	handle the '.' and '..' entries.
	A deleted file will be returned as a NULL entry (first char of filename=0)
	by this code. Filenames beginning with 0x05 will be translated to 0xE5
	automatically. Long file name entries will be returned as NULL.
	returns DFS_EOF if there are no more entries, DFS_OK if this entry is valid,
	or DFS_ERRMISC for a media error
*/
uint32_t DFS_GetNext(PVOLINFO volinfo, PDIRINFO dirinfo, PDIRENT dirent)
{
	// TODO
	return DFS_OK;
}

/*
	Open a file for reading or writing. You supply populated VOLINFO, a path to the file,
	mode (DFS_READ or DFS_WRITE) and an empty fileinfo structure. You also need to
	provide a pointer to a sector-sized scratch buffer.
	Returns various DFS_* error states. If the result is DFS_OK, fileinfo can be used
	to access the file from this point on.
*/
uint32_t DFS_OpenFile(PVOLINFO volinfo, uint8_t *path, uint8_t mode, uint8_t *scratch, PFILEINFO fileinfo)
{
	if( SDCARD_Wrapper_getDir() == nil )
		return DFS_ERRMISC; // SD Card directory not specified -> "emulated" SD Card not available

	NSString *fullPath = [[NSString alloc] initWithFormat:@"%@/%s", SDCARD_Wrapper_getDir(), path];
	char fullPathC[1024];
	[fullPath getCString:fullPathC];

//	NSLog(@"Opening '%s'\n", fullPathC);
	FILE *f = fopen(fullPathC, "r");
	if( f == NULL )
		return DFS_NOTFOUND;

	// determine file length and set pointer
	fseek(f, 0 , SEEK_END);
	fileinfo->filelen = ftell(f);
	rewind(f);
	fileinfo->pointer = 0;

//	NSLog(@"Len: %d\n", fileinfo->filelen);

	// mis-use pointer to volinfo to store file reference
	fileinfo->volinfo = (PVOLINFO)f;

	return DFS_OK;
}

/*
	Read an open file
	You must supply a prepopulated FILEINFO as provided by DFS_OpenFile, and a
	pointer to a SECTOR_SIZE scratch buffer.
	Note that returning DFS_EOF is not an error condition. This function updates the
	successcount field with the number of bytes actually read.
*/
uint32_t DFS_ReadFile(PFILEINFO fileinfo, uint8_t *scratch, uint8_t *buffer, uint32_t *successcount, uint32_t len)
{
	*successcount = fread(buffer, 1, len, (FILE *)fileinfo->volinfo);
	fileinfo->pointer += *successcount;

	return *successcount ? DFS_OK : DFS_ERRMISC;
}

/*
	Seek file pointer to a given position
	This function does not return status - refer to the fileinfo->pointer value
	to see where the pointer wound up.
	Requires a SECTOR_SIZE scratch buffer
*/
void DFS_Seek(PFILEINFO fileinfo, uint32_t offset, uint8_t *scratch)
{
	fseek((FILE *)fileinfo->volinfo, offset , SEEK_SET);
	fileinfo->pointer = offset;
	return;
}

/*
	Delete a file
	scratch must point to a sector-sized buffer
*/
uint32_t DFS_UnlinkFile(PVOLINFO volinfo, uint8_t *path, uint8_t *scratch)
{
	// TODO
	return DFS_OK;
}


/*
	Write an open file
	You must supply a prepopulated FILEINFO as provided by DFS_OpenFile, and a
	pointer to a SECTOR_SIZE scratch buffer.
	This function updates the successcount field with the number of bytes actually written.
*/
uint32_t DFS_WriteFile(PFILEINFO fileinfo, uint8_t *scratch, uint8_t *buffer, uint32_t *successcount, uint32_t len)
{
	*successcount = fwrite(buffer, 1, len, (FILE *)fileinfo->volinfo);
	fileinfo->pointer += *successcount;

	return *successcount ? DFS_OK : DFS_ERRMISC;
}


@end
