/*
	hostemu.c
	DOSFS Embedded FAT-Compatible Filesystem
	Host-Side Emulation Code	
	(C) 2005 Lewin A.R.W. Edwards (sysadm@zws.com)
*/

#include <stdio.h>
#include <stdlib.h>

#include "dosfs.h"
#include "hostemu.h"

//===================================================================
// Globals
FILE *hostfile;			// references host-side image file

/*
	Attach emulation to a host-side disk image file
	Returns 0 OK, nonzero for any error
*/
int DFS_HostAttach(char *imagefile)
{
	hostfile = fopen(imagefile, "r+b");
	if (hostfile == NULL)
		return -1;

	return 0;	// OK
}

/*
	Read sector from image
	Returns 0 OK, nonzero for any error
*/
int DFS_HostReadSector(uint8_t *buffer, uint32_t sector, uint32_t count)
{
	if (fseek(hostfile, sector * SECTOR_SIZE, SEEK_SET))
		return -1;

	fread(buffer, SECTOR_SIZE, count, hostfile);
	return 0;
}

/*
	Write sector to image
	Returns 0 OK, nonzero for any error
*/
int DFS_HostWriteSector(uint8_t *buffer, uint32_t sector, uint32_t count)
{
	if (fseek(hostfile, sector * SECTOR_SIZE, SEEK_SET))
		return -1;

	fwrite(buffer, SECTOR_SIZE, count, hostfile);
	fflush(hostfile);
	return 0;
}
