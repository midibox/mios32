// $Id$
/*
 * Header for file functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO_FILE_H
#define _MIDIO_FILE_H

// for compatibility with DOSFS
// TODO: change
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are session directories located?
#define MIDIO_FILE_SESSION_PATH "/SESSIONS"


// error codes
// NOTE: MIDIO_FILE_SendErrorMessage() should be extended whenever new codes have been added!

#define MIDIO_FILE_ERR_SD_CARD           -1 // failed to access SD card
#define MIDIO_FILE_ERR_NO_PARTITION      -2 // DFS_GetPtnStart failed to find partition
#define MIDIO_FILE_ERR_NO_VOLUME         -3 // DFS_GetVolInfo failed to find volume information
#define MIDIO_FILE_ERR_UNKNOWN_FS        -4 // unknown filesystem (only FAT12/16/32 supported)
#define MIDIO_FILE_ERR_OPEN_READ         -5 // DFS_OpenFile(..DFS_READ..) failed, e.g. file not found
#define MIDIO_FILE_ERR_OPEN_READ_WITHOUT_CLOSE -6 // MIDIO_FILE_ReadOpen() has been called while previous file hasn't been closed via MIDIO_FILE_ReadClose()
#define MIDIO_FILE_ERR_READ              -7 // DFS_ReadFile failed
#define MIDIO_FILE_ERR_READCOUNT         -8 // less bytes read than expected
#define MIDIO_FILE_ERR_READCLOSE         -9 // DFS_ReadClose aborted due to previous error
#define MIDIO_FILE_ERR_WRITE_MALLOC     -10 // MIDIO_FILE_WriteOpen failed to allocate memory for write buffer
#define MIDIO_FILE_ERR_OPEN_WRITE       -11 // DFS_OpenFile(..DFS_WRITE..) failed
#define MIDIO_FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE -12 // MIDIO_FILE_WriteOpen() has been called while previous file hasn't been closed via MIDIO_FILE_WriteClose()
#define MIDIO_FILE_ERR_WRITE            -13 // DFS_WriteFile failed
#define MIDIO_FILE_ERR_WRITECOUNT       -14 // less bytes written than expected
#define MIDIO_FILE_ERR_WRITECLOSE       -15 // DFS_WriteClose aborted due to previous error
#define MIDIO_FILE_ERR_SEEK             -16 // MIDIO_FILE_Seek() failed
#define MIDIO_FILE_ERR_OPEN_DIR         -17 // DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found
#define MIDIO_FILE_ERR_COPY             -18 // MIDIO_FILE_Copy() failed
#define MIDIO_FILE_ERR_COPY_NO_FILE     -19 // source file doesn't exist
#define MIDIO_FILE_ERR_NO_DIR           -20 // MIDIO_FILE_GetDirs() or MIDIO_FILE_GetFiles() failed because of missing directory
#define MIDIO_FILE_ERR_NO_FILE          -21 // MIDIO_FILE_GetFiles() failed because of missing directory
#define MIDIO_FILE_ERR_SYSEX_READ       -22 // error while reading .syx file
#define MIDIO_FILE_ERR_MKDIR            -23 // MIDIO_FILE_MakeDir() failed
#define MIDIO_FILE_ERR_INVALID_SESSION_NAME -24 // MIDIO_FILE_LoadSessionName()
#define MIDIO_FILE_ERR_UPDATE_FREE      -25 // MIDIO_FILE_UpdateFreeBytes()

// used by midio_file_p.c
#define MIDIO_FILE_P_ERR_INVALID_BANK    -128 // invalid bank number
#define MIDIO_FILE_P_ERR_INVALID_GROUP   -129 // invalid group number
#define MIDIO_FILE_P_ERR_INVALID_PATCH   -130 // invalid patch number
#define MIDIO_FILE_P_ERR_FORMAT          -131 // invalid bank file format
#define MIDIO_FILE_P_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define MIDIO_FILE_P_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define MIDIO_FILE_P_ERR_NO_FILE         -134 // no or invalid bank file
#define MIDIO_FILE_P_ERR_P_TOO_LARGE     -135 // during patch write: patch too large for slot in bank


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// simplified file reference, part of FIL structure of FatFs
typedef struct {
  u8  flag;  // file status flag
  u8  csect; // sector address in cluster
  u32 fptr;  // file r/w pointer
  u32 fsize; // file size
  u32 org_clust; // file start cluster
  u32 curr_clust; // current cluster
  u32 dsect; // current data sector;
  u32 dir_sect; // sector containing the directory entry
  u8 *dir_ptr; // pointer to the directory entry in the window
} midio_file_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIO_FILE_Init(u32 mode);

extern s32 MIDIO_FILE_CheckSDCard(void);

extern s32 MIDIO_FILE_SDCardAvailable(void);
extern s32 MIDIO_FILE_VolumeAvailable(void);
extern u32 MIDIO_FILE_VolumeBytesFree(void);
extern u32 MIDIO_FILE_VolumeBytesTotal(void);
extern char *MIDIO_FILE_VolumeLabel(void);
extern s32 MIDIO_FILE_UpdateFreeBytes(void);

extern s32 MIDIO_FILE_LoadAllFiles(u8 including_hw);
extern s32 MIDIO_FILE_UnloadAllFiles(void);

extern s32 MIDIO_FILE_StoreSessionName(void);
extern s32 MIDIO_FILE_LoadSessionName(void);

extern s32 MIDIO_FILE_ReadOpen(midio_file_t* file, char *filepath);
extern s32 MIDIO_FILE_ReadReOpen(midio_file_t* file);
extern s32 MIDIO_FILE_ReadClose(midio_file_t* file);
extern s32 MIDIO_FILE_ReadSeek(u32 offset);
extern s32 MIDIO_FILE_ReadBuffer(u8 *buffer, u32 len);
extern s32 MIDIO_FILE_ReadLine(u8 *buffer, u32 max_len);
extern s32 MIDIO_FILE_ReadByte(u8 *byte);
extern s32 MIDIO_FILE_ReadHWord(u16 *hword);
extern s32 MIDIO_FILE_ReadWord(u32 *word);

extern s32 MIDIO_FILE_WriteOpen(char *filepath, u8 create);
extern s32 MIDIO_FILE_WriteClose(void);
extern s32 MIDIO_FILE_WriteSeek(u32 offset);
extern u32 MIDIO_FILE_WriteGetCurrentSize(void);
extern s32 MIDIO_FILE_WriteBuffer(u8 *buffer, u32 len);
extern s32 MIDIO_FILE_WriteByte(u8 byte);
extern s32 MIDIO_FILE_WriteHWord(u16 hword);
extern s32 MIDIO_FILE_WriteWord(u32 word);

extern s32 MIDIO_FILE_MakeDir(char *path);

extern s32 MIDIO_FILE_FileExists(char *filepath);
extern s32 MIDIO_FILE_DirExists(char *path);

extern s32 MIDIO_FILE_FormattingRequired(void);
extern s32 MIDIO_FILE_Format(void);

extern s32 MIDIO_FILE_SendErrorMessage(s32 error_status);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// name/directory of session
extern char midio_file_session_name[13];
extern char midio_file_new_session_name[13];

// last error status returned by DFS
// can be used as additional debugging help if MIDIO_FILE_*ERR returned by function
extern u32 midio_file_dfs_errno;

extern char *midio_file_backup_notification;

extern u8 midio_file_copy_percentage;
extern u8 midio_file_backup_percentage;

#endif /* _MIDIO_FILE_H */
