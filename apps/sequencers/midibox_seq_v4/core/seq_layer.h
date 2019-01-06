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


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_package_t midi_package;
  s16                   len;
  u8                    layer_tag;
} seq_layer_evnt_t;


typedef enum {
  SEQ_EVENT_MODE_Note,
  SEQ_EVENT_MODE_Chord,
  SEQ_EVENT_MODE_CC,
  SEQ_EVENT_MODE_Drum,
  SEQ_EVENT_MODE_Combined,
} seq_event_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LAYER_Init(u32 mode);

extern s32 SEQ_LAYER_ResetLatchedValues(void);

extern s32 SEQ_LAYER_ResetTrackPCBankLatchedValues(void);
extern s32 SEQ_LAYER_SendPCBankValues(u8 track, u8 force, u8 send_now);

extern s32 SEQ_LAYER_PresetDrumNoteSet(u8 num, u8 note);
extern s32 SEQ_LAYER_PresetDrumNoteGet(u8 num);

extern const char *SEQ_LAYER_GetEvntModeName(seq_event_mode_t event_mode);

extern s32 SEQ_LAYER_GetEvntOfLayer(u8 track, u16 step, u8 layer, u8 instrument, seq_layer_evnt_t *layer_event);

#ifdef MBSEQV4P
extern s32 SEQ_LAYER_GetEventsPlus(u8 track, u16 step, seq_layer_evnt_t layer_events[80], u8 insert_empty_notes);
#else
extern s32 SEQ_LAYER_GetEvents(u8 track, u16 step, seq_layer_evnt_t layer_events[16], u8 insert_empty_notes);
#endif

extern s32 SEQ_LAYER_RecEvent(u8 track, u16 step, seq_layer_evnt_t layer_event);

extern s32 SEQ_LAYER_DirectSendEvent(u8 track, u8 par_layer);

extern s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers, u8 all_triggers_cleared, u8 init_assignments);
extern s32 SEQ_LAYER_CopyParLayerPreset(u8 track, u8 par_layer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// to display activity of selected track in trigger/parameter selection page
extern u8 seq_layer_vu_meter[16];

#ifdef MBSEQV4P
// Drum CC configuration
extern u8 seq_layer_drum_cc[16][4]; // up to 4 parameter layers per drum
#endif


#endif /* _SEQ_LAYER_H */
