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

#ifndef _SEQ_FILE_H
#define _SEQ_FILE_H

#include <dosfs.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// error codes
#define SEQ_FILE_ERR_SD_CARD           -1 // failed to access SD card
#define SEQ_FILE_ERR_NO_PARTITION      -2 // DFS_GetPtnStart failed to find partition
#define SEQ_FILE_ERR_NO_VOLUME         -3 // DFS_GetVolInfo failed to find volume information
#define SEQ_FILE_ERR_UNKNOWN_FS        -4 // unknown filesystem (only FAT12/16/32 supported)
#define SEQ_FILE_ERR_OPEN_READ         -5 // DFS_OpenFile(..DFS_READ..) failed, e.g. file not found
#define SEQ_FILE_ERR_READ              -6 // DFS_ReadFile failed
#define SEQ_FILE_ERR_READCOUNT         -7 // less bytes read than expected
#define SEQ_FILE_ERR_WRITE_MALLOC      -8 // SEQ_FILE_WriteOpen failed to allocate memory for write buffer
#define SEQ_FILE_ERR_OPEN_WRITE        -9 // DFS_OpenFile(..DFS_WRITE..) failed
#define SEQ_FILE_ERR_WRITE            -10 // DFS_WriteFile failed
#define SEQ_FILE_ERR_WRITECOUNT       -11 // less bytes written than expected
#define SEQ_FILE_ERR_SEEK             -12 // SEQ_FILE_Seek() failed
#define SEQ_FILE_ERR_OPEN_DIR         -13 // DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found
#define SEQ_FILE_ERR_NO_BACKUP_DIR    -14 // SEQ_FILE_CreateBackup() failed because of missing backup directory
#define SEQ_FILE_ERR_NO_BACKUP_SUBDIR -15 // SEQ_FILE_CreateBackup() failed because of missing backup subdirectory
#define SEQ_FILE_ERR_NEED_MORE_BACKUP_SUBDIRS -16 // SEQ_FILE_CreateBackup() failed because we need more backup subdirs!
#define SEQ_FILE_ERR_COPY             -17 // SEQ_FILE_Copy() failed
#define SEQ_FILE_ERR_COPY_NO_FILE     -18 // source file doesn't exist
#define SEQ_FILE_ERR_NO_DIR           -19 // SEQ_FILE_GetDirs() or SEQ_FILE_GetFiles() failed because of missing directory
#define SEQ_FILE_ERR_NO_FILE          -20 // SEQ_FILE_GetFiles() failed because of missing directory
#define SEQ_FILE_ERR_SYSEX_READ       -21 // error while reading .syx file

// used by seq_file_b.c
#define SEQ_FILE_B_ERR_INVALID_BANK    -128 // invalid bank number
#define SEQ_FILE_B_ERR_INVALID_GROUP   -129 // invalid group number
#define SEQ_FILE_B_ERR_INVALID_PATTERN -130 // invalid pattern number
#define SEQ_FILE_B_ERR_FORMAT          -131 // invalid bank file format
#define SEQ_FILE_B_ERR_READ            -132 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_B_ERR_WRITE           -133 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_B_ERR_NO_FILE         -134 // no or invalid bank file
#define SEQ_FILE_B_ERR_P_TOO_LARGE     -135 // during pattern write: pattern too large for slot in bank

// used by seq_file_m.c
#define SEQ_FILE_M_ERR_INVALID_BANK    -144 // invalid bank number
#define SEQ_FILE_M_ERR_INVALID_MAP     -146 // invalid map number
#define SEQ_FILE_M_ERR_FORMAT          -147 // invalid bank file format
#define SEQ_FILE_M_ERR_READ            -148 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_M_ERR_WRITE           -149 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_M_ERR_NO_FILE         -150 // no or invalid bank file
#define SEQ_FILE_M_ERR_M_TOO_LARGE     -151 // during map write: map too large for slot in bank

// used by seq_file_s.c
#define SEQ_FILE_S_ERR_INVALID_BANK    -160 // invalid bank number
#define SEQ_FILE_S_ERR_INVALID_SONG    -161 // invalid song number
#define SEQ_FILE_S_ERR_FORMAT          -162 // invalid bank file format
#define SEQ_FILE_S_ERR_READ            -163 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_S_ERR_WRITE           -164 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_S_ERR_NO_FILE         -165 // no or invalid bank file
#define SEQ_FILE_S_ERR_S_TOO_LARGE     -166 // during song write: song too large for slot in bank

// used by seq_file_c.c
#define SEQ_FILE_C_ERR_FORMAT          -176 // invalid config file format
#define SEQ_FILE_C_ERR_READ            -177 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_C_ERR_WRITE           -178 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_C_ERR_NO_FILE         -179 // no or invalid config file

// used by seq_file_g.c
#define SEQ_FILE_G_ERR_FORMAT          -192 // invalid config file format
#define SEQ_FILE_G_ERR_READ            -193 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_G_ERR_WRITE           -194 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_G_ERR_NO_FILE         -195 // no or invalid config file

// used by seq_file_hw.c
#define SEQ_FILE_HW_ERR_FORMAT         -208 // invalid config file format
#define SEQ_FILE_HW_ERR_READ           -209 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_HW_ERR_WRITE          -210 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_HW_ERR_NO_FILE        -211 // no or invalid config file


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_Init(u32 mode);

extern s32 SEQ_FILE_CheckSDCard(void);

extern s32 SEQ_FILE_SDCardAvailable(void);
extern s32 SEQ_FILE_VolumeAvailable(void);
extern u32 SEQ_FILE_VolumeBytesFree(void);
extern u32 SEQ_FILE_VolumeBytesTotal(void);
extern char *SEQ_FILE_VolumeLabel(void);
extern s32 SEQ_FILE_UpdateFreeBytes(void);

extern s32 SEQ_FILE_ReadOpen(PFILEINFO fileinfo, char *filepath);
extern s32 SEQ_FILE_ReadBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len);
extern s32 SEQ_FILE_ReadLine(PFILEINFO fileinfo, u8 *buffer, u32 max_len);
extern s32 SEQ_FILE_ReadByte(PFILEINFO fileinfo, u8 *byte);
extern s32 SEQ_FILE_ReadHWord(PFILEINFO fileinfo, u16 *hword);
extern s32 SEQ_FILE_ReadWord(PFILEINFO fileinfo, u32 *word);
extern s32 SEQ_FILE_ReadClose(PFILEINFO fileinfo);

extern s32 SEQ_FILE_WriteOpen(PFILEINFO fileinfo, char *filepath, u8 create);
extern s32 SEQ_FILE_WriteBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len);
extern s32 SEQ_FILE_WriteByte(PFILEINFO fileinfo, u8 byte);
extern s32 SEQ_FILE_WriteHWord(PFILEINFO fileinfo, u16 hword);
extern s32 SEQ_FILE_WriteWord(PFILEINFO fileinfo, u32 word);
extern s32 SEQ_FILE_WriteClose(PFILEINFO fileinfo);

extern s32 SEQ_FILE_Seek(PFILEINFO fileinfo, u32 offset);

extern s32 SEQ_FILE_FileExists(char *filepath);
extern s32 SEQ_FILE_DirExists(char *path);

extern s32 SEQ_FILE_PrintSDCardInfos(void);

extern s32 SEQ_FILE_FormattingRequired(void);
extern s32 SEQ_FILE_Format(void);

extern s32 SEQ_FILE_Copy(char *src_file, char *dst_file, u8 *write_buffer);
extern s32 SEQ_FILE_CreateBackup(void);

extern s32 SEQ_FILE_GetDirs(char *path, char *dir_list, u8 num_of_items, u8 dir_offset);
extern s32 SEQ_FILE_GetFiles(char *path, char *ext_filter, char *dir_list, u8 num_of_items, u8 dir_offset);
extern s32 SEQ_FILE_SendSyxDump(char *path, mios32_midi_port_t port);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// last error status returned by DFS
// can be used as additional debugging help if SEQ_FILE_*ERR returned by function
extern u32 seq_file_dfs_errno;

extern char *seq_file_backup_notification;

extern u8 seq_file_copy_percentage;
extern u8 seq_file_backup_percentage;

#endif /* _SEQ_FILE_H */
