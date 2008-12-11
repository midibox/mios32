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
  u16                   len;
} seq_layer_evnt_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LAYER_Init(u32 mode);

extern char *SEQ_LAYER_VTypeStr(u8 event_mode, u8 p_layer);
extern char *SEQ_LAYER_CTypeStr(u8 event_mode, u8 c_num);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const void *seq_layer_getevnt_func[SEQ_LAYER_EVNTMODE_NUM];


#endif /* _SEQ_LAYER_H */
