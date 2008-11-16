/*
	Test program for DOSFS
	Lewin A.R.W. Edwards (sysadm@zws.com)
*/

#include <stdio.h>
#include <sys/types.h>

#include "dosfs.h"


	uint8_t sector[SECTOR_SIZE], sector2[SECTOR_SIZE];
	VOLINFO vi;
	DIRINFO di;
	DIRENT de;
	FILEINFO fi;

int main(int _argc, char *_argv[])
{
	uint32_t pstart, psize, i;
	uint8_t pactive, ptype;
	uint32_t cache;
	uint8_t *p;

	if (_argc < 2) {
		printf("Usage: %s [image_file]\n", _argv[0]);
		return -1;
	}	

	// Attach user-specified image file
	if (DFS_HostAttach(_argv[1])) {
		printf("Cannot attach image file '%s'\n", _argv[1]);
		return -1;
	}

	// Obtain pointer to first partition on first (only) unit
	pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
	if (pstart == 0xffffffff) {
		printf("Cannot find first partition\n");
		return -1;
	}

	printf("Partition 0 start sector 0x%-08.8lX active %-02.2hX type %-02.2hX size %-08.8lX\n", pstart, pactive, ptype, psize);

	if (DFS_GetVolInfo(0, sector, pstart, &vi)) {
		printf("Error getting volume information\n");
		return -1;
	}
	printf("Volume label '%-11.11s'\n", vi.label);
	printf("%d sector/s per cluster, %d reserved sector/s, volume total %d sectors.\n", vi.secperclus, vi.reservedsecs, vi.numsecs);
	printf("%d sectors per FAT, first FAT at sector #%d, root dir at #%d.\n",vi.secperfat,vi.fat1,vi.rootdir);
	printf("(For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n");
	printf("%d root dir entries, data area commences at sector #%d.\n",vi.rootentries,vi.dataarea);
	printf("%d clusters (%d bytes) in data area, filesystem IDd as ", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE);
	if (vi.filesystem == FAT12)
		printf("FAT12.\n");
	else if (vi.filesystem == FAT16)
		printf("FAT16.\n");
	else if (vi.filesystem == FAT32)
		printf("FAT32.\n");
	else
		printf("[unknown]\n");

//------------------------------------------------------------
// Directory enumeration test
	di.scratch = sector;
//	if (DFS_OpenDir(&vi, "", &di)) {
//		printf("Error opening root directory\n");
//		return -1;
//	}
	if (DFS_OpenDir(&vi, "MYDIR1", &di)) {
		printf("error opening subdirectory\n");
		return -1;
	}
	while (!DFS_GetNext(&vi, &di, &de)) {
		if (de.name[0])
			printf("file: '%-11.11s'\n", de.name);
	}


//------------------------------------------------------------
// Unlink test
//	cache = 0;
//	printf("*** FAT BEFORE ***\n");
//  	for (i=0;i<vi.numclusters;i++) {
//		printf("entry %-08.8x, %-08.8X\n", i, DFS_GetFAT(&vi, sector, &cache, i));
//	}
//	if (DFS_UnlinkFile(&vi, "MYDIR1/SUBDIR1.2/README.TXT", sector)) {
//		printf("error unlinking file\n");
//	}
//	printf("*** FAT AFTER ***\n");
//  	for (i=0;i<vi.numclusters;i++) {
//		printf("entry %-08.8x, %-08.8X\n", i, DFS_GetFAT(&vi, sector, &cache, i));
//	}

//------------------------------------------------------------
// File write test
	if (DFS_OpenFile(&vi, "MYDIR1/WRTEST.TXT", DFS_WRITE, sector, &fi)) {
		printf("error opening file\n");
		return -1;
	}
	for (i=0;i<18;i++) {
		memset(sector2, 128+i, SECTOR_SIZE);
		DFS_WriteFile(&fi, sector, sector2, &cache, SECTOR_SIZE/2);
		memset(sector2+256, 255-i, SECTOR_SIZE/2);
		DFS_WriteFile(&fi, sector, sector2+256, &cache, SECTOR_SIZE/2);
	}
	sprintf(sector2, "test string at the end...");
	DFS_WriteFile(&fi, sector, sector2, &cache, strlen(sector2));

//------------------------------------------------------------
// File read test
	printf("Readback test\n");
	if (DFS_OpenFile(&vi, "MYDIR1/WRTEST.TXT", DFS_READ, sector, &fi)) {
		printf("error opening file\n");
		return -1;
	}
	p = (void *) malloc(fi.filelen+512);
	memset(p, 0xaa, fi.filelen+512);

	DFS_ReadFile(&fi, sector, p, &i, fi.filelen);
printf("read complete %d bytes (expected %d) pointer %d\n", i, fi.filelen, fi.pointer);

	{
	FILE *fp;
	fp=fopen("test.txt","wb");
	fwrite(p, fi.filelen+512, 1, fp);
	fclose(fp);
	}

	return 0;
}
