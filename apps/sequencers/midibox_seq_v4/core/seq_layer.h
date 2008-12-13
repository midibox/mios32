// $Id$
/*
 * Header file for parameter layer routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LAYER_H
#define _SEQ_LAYER_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of available event modes
#define SEQ_LAYER_EVNTMODE_NUM 11


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_package_t midi_package;
  s16                   len;
} seq_layer_evnt_t;


typedef enum {
  SEQ_LAYER_ControlType_None,
  SEQ_LAYER_ControlType_Note,
  SEQ_LAYER_ControlType_Velocity,
  SEQ_LAYER_ControlType_Chord1,
  SEQ_LAYER_ControlType_Chord2,
  SEQ_LAYER_ControlType_Chord1_Velocity,
  SEQ_LAYER_ControlType_Chord2_Velocity,
  SEQ_LAYER_ControlType_Length,
  SEQ_LAYER_ControlType_CC,
  SEQ_LAYER_ControlType_CMEM_T
} seq_layer_ctrl_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LAYER_Init(u32 mode);

extern char *SEQ_LAYER_VTypeStr(u8 event_mode, u8 par_layer);
extern char *SEQ_LAYER_CTypeStr(u8 event_mode, u8 const_num);

extern s32 SEQ_LAYER_GetEvntOfParLayer(u8 track, u8 step, u8 par_layer, seq_layer_evnt_t *layer_event);
extern seq_layer_ctrl_type_t SEQ_LAYER_GetVControlType(u8 track, u8 par_layer);
extern seq_layer_ctrl_type_t SEQ_LAYER_GetCControlType(u8 track, u8 const_ix);

extern s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const void *seq_layer_getevnt_func[SEQ_LAYER_EVNTMODE_NUM];


#endif /* _SEQ_LAYER_H */
