// $Id: seq_file_bm.h 1205 2011-05-13 22:06:38Z tk $
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

#ifndef _SEQ_FILE_PRESETS_H
#define _SEQ_FILE_PRESETS_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_FILE_PRESETS_Init(u32 mode);
extern s32 SEQ_FILE_PRESETS_Load(void);
extern s32 SEQ_FILE_PRESETS_Unload(void);

extern s32 SEQ_FILE_PRESETS_Valid(void);

extern s32 SEQ_FILE_PRESETS_CreateDefaults(void);

extern s32 SEQ_FILE_PRESETS_TrkLabel_NumGet(void);
extern s32 SEQ_FILE_PRESETS_TrkLabel_Read(u8 preset_ix, char *dst);

extern s32 SEQ_FILE_PRESETS_TrkCategory_NumGet(void);
extern s32 SEQ_FILE_PRESETS_TrkCategory_Read(u8 preset_ix, char *dst);

extern s32 SEQ_FILE_PRESETS_TrkDrum_NumGet(void);
extern s32 SEQ_FILE_PRESETS_TrkDrum_Read(u8 preset_ix, char *dst, u8 *note, u8 init_layer_preset_notes);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_FILE_PRESETS_H */
