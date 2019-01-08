/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
//
// FATFS_Wrapper_Dir.c
//
// Unfortunately the DIR structure of FatFS clashes with the Posix DIR structure
// Therefore two help functions for f_opendir and f_readdir are isolated in this file
// We don't include ff.h here!
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

// from ff.h
#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_MASK	0x3F	/* Mask of defined bits */

static DIR *currentDir = NULL;

int f_opendir_hlp(const char *path)
{
    if( currentDir )
        closedir(currentDir);

    currentDir = opendir(path);
    if( currentDir == NULL )
        return -1; // directory not found

    return 0; // no error
}


int f_readdir_hlp(char *fname, unsigned char *fattrib, unsigned *fsize, unsigned short *fdate, unsigned short *ftime)
{
    struct dirent *de;

    de = readdir(currentDir);
    if( de == NULL ) // no more entries
        return -1;

    // convert posix style (long) filename to DOS style 8.3 filename

    // name
    char *fname_cpy = fname;
    int pos;
    for(pos=0; pos<8; ++pos) {
        if( de->d_name[pos] == 0 || de->d_name[pos] == '.' )
            break;
        *fname_cpy++ = de->d_name[pos];
    }

    // extension
    if( de->d_name[pos] != 0 ) {
        *fname_cpy++ = '.';

        if( de->d_name[pos] == '.' )
            ++pos;

        int extpos;
        for(extpos=0; extpos<3; ++extpos) {
            if( de->d_name[pos+extpos] == 0 || de->d_name[pos+extpos] == '.' )
                break;
            *fname_cpy++ = de->d_name[pos+extpos];
        }
    }
    *fname_cpy++ = 0;

    // attribute (currently only DT_DIR supported)
    *fattrib = 0;
    if( de->d_type & DT_DIR )
        *fattrib |= AM_DIR;

    // not supported yet
    *fsize = 0; // TODO
    *fdate = 0; // TODO
    *ftime = 0; // TODO

    return 0; // no error
}
