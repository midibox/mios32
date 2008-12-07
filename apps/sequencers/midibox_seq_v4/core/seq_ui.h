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
// Menu Page definitions
/////////////////////////////////////////////////////////////////////////////

// must be kept in sync with ui_init_callback list in seq_ui.c!
#define SEQ_UI_PAGES 3

typedef enum {
  SEQ_UI_PAGE_NONE,
  SEQ_UI_PAGE_EDIT,
  SEQ_UI_PAGE_TRKDIR
} seq_ui_page_t;


// Prototypes for all UI pages are burried here
extern s32 SEQ_UI_TODO_Init(u32 mode);
extern s32 SEQ_UI_EDIT_Init(u32 mode);
extern s32 SEQ_UI_TRKDIR_Init(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned MENU_PRESSED:1;
  };
} seq_ui_button_state_t;



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_Init(u32 mode);

extern s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value);
extern s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer);
extern s32 SEQ_UI_LED_Handler(void);
extern s32 SEQ_UI_LED_Handler_Periodic();

extern s32 SEQ_UI_PageSet(seq_ui_page_t page);

extern s32 SEQ_UI_InstallInitCallback(void *callback);
extern s32 SEQ_UI_InstallGPButtonCallback(void *callback);
extern s32 SEQ_UI_InstallGPEncoderCallback(void *callback);
extern s32 SEQ_UI_InstallGPLEDCallback(void *callback);
extern s32 SEQ_UI_InstallGPLCDCallback(void *callback);

extern u8 SEQ_UI_VisibleTrackGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_ui_display_update_req;
extern u8 seq_ui_display_init_req;

extern seq_ui_button_state_t seq_ui_button_state;

extern u8 ui_selected_group;
extern u8 ui_selected_tracks;
extern u8 ui_selected_par_layer;
extern u8 ui_selected_trg_layer;
extern u8 ui_selected_step_view;
extern u8 ui_selected_step;

#endif /* _SEQ_UI_H */
