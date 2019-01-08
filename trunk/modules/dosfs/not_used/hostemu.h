/*
	hostemu.h
	DOSFS Embedded FAT-Compatible Filesystem
	Host-Side Emulation Code	
	(C) 2005 Lewin A.R.W. Edwards (sysadm@zws.com)
*/

#ifndef _HOSTEMU_H
#define _HOSTEMU_H

#include <sys/types.h>

// Override media sector functions
#define DFS_ReadSector(unit,buffer,sector,count) DFS_HostReadSector(buffer,sector,count)
#define DFS_WriteSector(unit,buffer,sector,count) DFS_HostWriteSector(buffer,sector,count)


/*
	Attach emulation to a host-side disk image file
	Returns 0 OK, nonzero for any error
*/
int DFS_HostAttach(char *imagefile);

/*
	Read sector from image
	Returns 0 OK, nonzero for any error
*/
int DFS_HostReadSector(uint8_t *buffer, uint32_t sector, uint32_t count);

/*
	Write sector to image
	Returns 0 OK, nonzero for any error
*/
int DFS_HostWriteSector(uint8_t *buffer, uint32_t sector, uint32_t count);


#endif // _HOSTEMU_H
