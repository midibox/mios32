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
  SEQ_EVENT_MODE_Note,
  SEQ_EVENT_MODE_Chord,
  SEQ_EVENT_MODE_CC,
  SEQ_EVENT_MODE_Drum
} seq_event_mode_t;


typedef enum {
  SEQ_LAYER_ControlType_None,
  SEQ_LAYER_ControlType_Note,
  SEQ_LAYER_ControlType_Velocity,
  SEQ_LAYER_ControlType_Chord,
  SEQ_LAYER_ControlType_Chord_Velocity,
  SEQ_LAYER_ControlType_Length,
  SEQ_LAYER_ControlType_CC
} seq_layer_ctrl_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LAYER_Init(u32 mode);

extern s32 SEQ_LAYER_GetEvntOfLayer(u8 track, u8 step, u8 layer, seq_layer_evnt_t *layer_event);
extern seq_layer_ctrl_type_t SEQ_LAYER_GetVControlType(u8 track, u8 par_layer);
extern char *SEQ_LAYER_GetVControlTypeString(u8 track, u8 par_layer);

extern s32 SEQ_LAYER_GetEvents(u8 track, u8 step, seq_layer_evnt_t layer_events[16]);

extern s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers, u8 all_triggers_cleared, u8 drum_with_accent);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_LAYER_H */
