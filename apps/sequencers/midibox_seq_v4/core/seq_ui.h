// $Id$
/*
 * Header file for user interface routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_UI_H
#define _SEQ_UI_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_Init(u32 mode);

extern s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value);
extern s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer);
extern s32 SEQ_UI_LED_Handler(void);
extern s32 SEQ_UI_LED_Handler_Periodic();

extern u8 SEQ_UI_VisibleTrackGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_ui_display_update_req;
extern u8 seq_ui_display_init_req;

extern u8 ui_selected_group;
extern u8 ui_selected_tracks;
extern u8 ui_selected_par_layer;
extern u8 ui_selected_trg_layer;
extern u8 ui_selected_step_view;
extern u8 ui_selected_step;

#endif /* _SEQ_UI_H */
