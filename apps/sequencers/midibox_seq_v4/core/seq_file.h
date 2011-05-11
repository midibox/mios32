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

// for compatibility with DOSFS
// TODO: change
#define MAX_PATH 100

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are session directories located?
#define SEQ_FILE_SESSION_PATH "/SESSIONS"


// error codes
// NOTE: SEQ_FILE_SendErrorMessage() should be extended whenever new codes have been added!

#define SEQ_FILE_ERR_SD_CARD           -1 // failed to access SD card
#define SEQ_FILE_ERR_NO_PARTITION      -2 // DFS_GetPtnStart failed to find partition
#define SEQ_FILE_ERR_NO_VOLUME         -3 // DFS_GetVolInfo failed to find volume information
#define SEQ_FILE_ERR_UNKNOWN_FS        -4 // unknown filesystem (only FAT12/16/32 supported)
#define SEQ_FILE_ERR_OPEN_READ         -5 // DFS_OpenFile(..DFS_READ..) failed, e.g. file not found
#define SEQ_FILE_ERR_OPEN_READ_WITHOUT_CLOSE -6 // SEQ_FILE_ReadOpen() has been called while previous file hasn't been closed via SEQ_FILE_ReadClose()
#define SEQ_FILE_ERR_READ              -7 // DFS_ReadFile failed
#define SEQ_FILE_ERR_READCOUNT         -8 // less bytes read than expected
#define SEQ_FILE_ERR_READCLOSE         -9 // DFS_ReadClose aborted due to previous error
#define SEQ_FILE_ERR_WRITE_MALLOC     -10 // SEQ_FILE_WriteOpen failed to allocate memory for write buffer
#define SEQ_FILE_ERR_OPEN_WRITE       -11 // DFS_OpenFile(..DFS_WRITE..) failed
#define SEQ_FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE -12 // SEQ_FILE_WriteOpen() has been called while previous file hasn't been closed via SEQ_FILE_WriteClose()
#define SEQ_FILE_ERR_WRITE            -13 // DFS_WriteFile failed
#define SEQ_FILE_ERR_WRITECOUNT       -14 // less bytes written than expected
#define SEQ_FILE_ERR_WRITECLOSE       -15 // DFS_WriteClose aborted due to previous error
#define SEQ_FILE_ERR_SEEK             -16 // SEQ_FILE_Seek() failed
#define SEQ_FILE_ERR_OPEN_DIR         -17 // DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found
#define SEQ_FILE_ERR_COPY             -18 // SEQ_FILE_Copy() failed
#define SEQ_FILE_ERR_COPY_NO_FILE     -19 // source file doesn't exist
#define SEQ_FILE_ERR_NO_DIR           -20 // SEQ_FILE_GetDirs() or SEQ_FILE_GetFiles() failed because of missing directory
#define SEQ_FILE_ERR_NO_FILE          -21 // SEQ_FILE_GetFiles() failed because of missing directory
#define SEQ_FILE_ERR_SYSEX_READ       -22 // error while reading .syx file
#define SEQ_FILE_ERR_MKDIR            -23 // SEQ_FILE_MakeDir() failed
#define SEQ_FILE_ERR_INVALID_SESSION_NAME -24 // SEQ_FILE_LoadSessionName()
#define SEQ_FILE_ERR_UPDATE_FREE      -25 // SEQ_FILE_UpdateFreeBytes()

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

// used by seq_file_t.c
#define SEQ_FILE_T_ERR_FORMAT          -224 // invalid file format
#define SEQ_FILE_T_ERR_READ            -225 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_T_ERR_WRITE           -226 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_T_ERR_NO_FILE         -227 // no or invalid file
#define SEQ_FILE_T_ERR_TRACK           -228 // invalid track number

// used by seq_file_gc.c
#define SEQ_FILE_GC_ERR_FORMAT         -140 // invalid config file format
#define SEQ_FILE_GC_ERR_READ           -141 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_WRITE          -142 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_GC_ERR_NO_FILE        -143 // no or invalid config file

// used by seq_file_bm.c
#define SEQ_FILE_BM_ERR_FORMAT         -156 // invalid config file format
#define SEQ_FILE_BM_ERR_READ           -157 // error while reading file (exact error status cannot be determined anymore)
#define SEQ_FILE_BM_ERR_WRITE          -158 // error while writing file (exact error status cannot be determined anymore)
#define SEQ_FILE_BM_ERR_NO_FILE        -159 // no or invalid config file


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
} seq_file_t;


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

extern s32 SEQ_FILE_LoadAllFiles(u8 including_hw);
extern s32 SEQ_FILE_UnloadAllFiles(void);

extern s32 SEQ_FILE_StoreSessionName(void);
extern s32 SEQ_FILE_LoadSessionName(void);

extern s32 SEQ_FILE_ReadOpen(seq_file_t* file, char *filepath);
extern s32 SEQ_FILE_ReadReOpen(seq_file_t* file);
extern s32 SEQ_FILE_ReadClose(seq_file_t* file);
extern s32 SEQ_FILE_ReadSeek(u32 offset);
extern s32 SEQ_FILE_ReadBuffer(u8 *buffer, u32 len);
extern s32 SEQ_FILE_ReadLine(u8 *buffer, u32 max_len);
extern s32 SEQ_FILE_ReadByte(u8 *byte);
extern s32 SEQ_FILE_ReadHWord(u16 *hword);
extern s32 SEQ_FILE_ReadWord(u32 *word);

extern s32 SEQ_FILE_WriteOpen(char *filepath, u8 create);
extern s32 SEQ_FILE_WriteClose(void);
extern s32 SEQ_FILE_WriteSeek(u32 offset);
extern u32 SEQ_FILE_WriteGetCurrentSize(void);
extern s32 SEQ_FILE_WriteBuffer(u8 *buffer, u32 len);
extern s32 SEQ_FILE_WriteByte(u8 byte);
extern s32 SEQ_FILE_WriteHWord(u16 hword);
extern s32 SEQ_FILE_WriteWord(u32 word);

extern s32 SEQ_FILE_MakeDir(char *path);

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

extern s32 SEQ_FILE_SendErrorMessage(s32 error_status);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// name/directory of session
extern char seq_file_session_name[13];
extern char seq_file_new_session_name[13];

// last error status returned by DFS
// can be used as additional debugging help if SEQ_FILE_*ERR returned by function
extern u32 seq_file_dfs_errno;

extern char *seq_file_backup_notification;

extern u8 seq_file_copy_percentage;
extern u8 seq_file_backup_percentage;

#endif /* _SEQ_FILE_H */
