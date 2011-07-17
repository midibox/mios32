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

#ifndef _FILE_H
#define _FILE_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// error codes
// NOTE: FILE_SendErrorMessage() should be extended whenever new codes have been added!

#define FILE_ERR_SD_CARD           -1 // failed to access SD card
#define FILE_ERR_NO_PARTITION      -2 // DFS_GetPtnStart failed to find partition
#define FILE_ERR_NO_VOLUME         -3 // DFS_GetVolInfo failed to find volume information
#define FILE_ERR_UNKNOWN_FS        -4 // unknown filesystem (only FAT12/16/32 supported)
#define FILE_ERR_OPEN_READ         -5 // DFS_OpenFile(..DFS_READ..) failed, e.g. file not found
#define FILE_ERR_OPEN_READ_WITHOUT_CLOSE -6 // FILE_ReadOpen() has been called while previous file hasn't been closed via FILE_ReadClose()
#define FILE_ERR_READ              -7 // DFS_ReadFile failed
#define FILE_ERR_READCOUNT         -8 // less bytes read than expected
#define FILE_ERR_READCLOSE         -9 // DFS_ReadClose aborted due to previous error
#define FILE_ERR_WRITE_MALLOC     -10 // FILE_WriteOpen failed to allocate memory for write buffer
#define FILE_ERR_OPEN_WRITE       -11 // DFS_OpenFile(..DFS_WRITE..) failed
#define FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE -12 // FILE_WriteOpen() has been called while previous file hasn't been closed via FILE_WriteClose()
#define FILE_ERR_WRITE            -13 // DFS_WriteFile failed
#define FILE_ERR_WRITECOUNT       -14 // less bytes written than expected
#define FILE_ERR_WRITECLOSE       -15 // DFS_WriteClose aborted due to previous error
#define FILE_ERR_SEEK             -16 // FILE_Seek() failed
#define FILE_ERR_OPEN_DIR         -17 // DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found
#define FILE_ERR_COPY             -18 // FILE_Copy() failed
#define FILE_ERR_COPY_NO_FILE     -19 // source file doesn't exist
#define FILE_ERR_NO_DIR           -20 // FILE_GetDirs() or FILE_GetFiles() failed because of missing directory
#define FILE_ERR_NO_FILE          -21 // FILE_GetFiles() failed because of missing directory
#define FILE_ERR_SYSEX_READ       -22 // error while reading .syx file
#define FILE_ERR_MKDIR            -23 // FILE_MakeDir() failed
#define FILE_ERR_INVALID_SESSION_NAME -24 // FILE_LoadSessionName()
#define FILE_ERR_UPDATE_FREE      -25 // FILE_UpdateFreeBytes()

// used by file_p.c
#define FILE_P_ERR_INVALID_BANK    -128 // invalid bank number
#define FILE_P_ERR_INVALID_GROUP   -129 // invalid group number
#define FILE_P_ERR_INVALID_PATCH   -130 // invalid patch number
#define FILE_P_ERR_FORMAT          -131 // invalid bank file format
#define FILE_P_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define FILE_P_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define FILE_P_ERR_NO_FILE         -134 // no or invalid bank file
#define FILE_P_ERR_P_TOO_LARGE     -135 // during patch write: patch too large for slot in bank


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
} file_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 FILE_Init(u32 mode);

extern s32 FILE_CheckSDCard(void);

extern s32 FILE_SDCardAvailable(void);
extern s32 FILE_VolumeAvailable(void);
extern u32 FILE_VolumeBytesFree(void);
extern u32 FILE_VolumeBytesTotal(void);
extern char *FILE_VolumeLabel(void);
extern s32 FILE_UpdateFreeBytes(void);

extern s32 FILE_ReadOpen(file_t* file, char *filepath);
extern s32 FILE_ReadReOpen(file_t* file);
extern s32 FILE_ReadClose(file_t* file);
extern s32 FILE_ReadSeek(u32 offset);
extern s32 FILE_ReadBuffer(u8 *buffer, u32 len);
extern s32 FILE_ReadLine(u8 *buffer, u32 max_len);
extern s32 FILE_ReadByte(u8 *byte);
extern s32 FILE_ReadHWord(u16 *hword);
extern s32 FILE_ReadWord(u32 *word);

extern s32 FILE_WriteOpen(char *filepath, u8 create);
extern s32 FILE_WriteClose(void);
extern s32 FILE_WriteSeek(u32 offset);
extern u32 FILE_WriteGetCurrentSize(void);
extern s32 FILE_WriteBuffer(u8 *buffer, u32 len);
extern s32 FILE_WriteByte(u8 byte);
extern s32 FILE_WriteHWord(u16 hword);
extern s32 FILE_WriteWord(u32 word);

extern s32 FILE_Copy(char *src_file, char *dst_file);

extern s32 FILE_MakeDir(char *path);

extern s32 FILE_FileExists(char *filepath);
extern s32 FILE_DirExists(char *path);

extern s32 FILE_GetDirs(char *path, char *dir_list, u8 num_of_items, u8 dir_offset);
extern s32 FILE_GetFiles(char *path, char *ext_filter, char *dir_list, u8 num_of_items, u8 dir_offset);

extern s32 FILE_FindNextFile(char *path, char *filename, char *ext_filter, char *next_filename);
extern s32 FILE_FindPreviousFile(char *path, char *filename, char *ext_filter, char *prev_filename);

extern s32 FILE_SendSyxDump(char *path, mios32_midi_port_t port);

extern s32 FILE_PrintSDCardInfos(void);

extern s32 FILE_SendErrorMessage(s32 error_status);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// last error status returned by DFS
// can be used as additional debugging help if FILE_*ERR returned by function
extern u32 file_dfs_errno;

// for percentage display during copy operations
extern u8 file_copy_percentage;

#endif /* _FILE_H */
