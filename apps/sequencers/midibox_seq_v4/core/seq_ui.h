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
#define SEQ_UI_PAGES 18

typedef enum {
  SEQ_UI_PAGE_NONE,
  SEQ_UI_PAGE_SHORTCUT,
  SEQ_UI_PAGE_EDIT,
  SEQ_UI_PAGE_MUTE,
  SEQ_UI_PAGE_PATTERN,
  SEQ_UI_PAGE_TRKEVNT,
  SEQ_UI_PAGE_TRKMODE,
  SEQ_UI_PAGE_TRKDIR,
  SEQ_UI_PAGE_TRKDIV,
  SEQ_UI_PAGE_TRKLEN,
  SEQ_UI_PAGE_TRKTRAN,
  SEQ_UI_PAGE_TRKRND,
  SEQ_UI_PAGE_TRGASG,
  SEQ_UI_PAGE_FX,
  SEQ_UI_PAGE_FX_ECHO,
  SEQ_UI_PAGE_UTIL,
  SEQ_UI_PAGE_BPM,
  SEQ_UI_PAGE_OPT
} seq_ui_page_t;


// Prototypes for all UI pages are burried here
extern s32 SEQ_UI_TODO_Init(u32 mode);
extern s32 SEQ_UI_SHORTCUT_Init(u32 mode);
extern s32 SEQ_UI_EDIT_Init(u32 mode);
extern s32 SEQ_UI_MUTE_Init(u32 mode);
extern s32 SEQ_UI_PATTERN_Init(u32 mode);
extern s32 SEQ_UI_TRKEVNT_Init(u32 mode);
extern s32 SEQ_UI_TRKMODE_Init(u32 mode);
extern s32 SEQ_UI_TRKDIR_Init(u32 mode);
extern s32 SEQ_UI_TRKDIV_Init(u32 mode);
extern s32 SEQ_UI_TRKLEN_Init(u32 mode);
extern s32 SEQ_UI_TRKTRAN_Init(u32 mode);
extern s32 SEQ_UI_TRKRND_Init(u32 mode);
extern s32 SEQ_UI_TRGASG_Init(u32 mode);
extern s32 SEQ_UI_FX_Init(u32 mode);
extern s32 SEQ_UI_FX_ECHO_Init(u32 mode);
extern s32 SEQ_UI_UTIL_Init(u32 mode);
extern s32 SEQ_UI_BPM_Init(u32 mode);
extern s32 SEQ_UI_OPT_Init(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// cursor flash with following waveform:
// 0..399 mS: cursor on
// 400..499 mS: cursor off
#define SEQ_UI_CURSOR_FLASH_CTR_MAX     500  // mS
#define SEQ_UI_CURSOR_FLASH_CTR_LED_OFF 400  // mS


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:16;
  };
  struct {
    unsigned MENU_PRESSED:1;
    unsigned CHANGE_ALL_STEPS:1;
    unsigned CHANGE_ALL_STEPS_SAME_VALUE:1;
    unsigned FAST_ENCODERS:1;
    unsigned SOLO:1;
    unsigned METRONOME:1;
    unsigned SCRUB:1;
    unsigned REW:1;
    unsigned FWD:1;
    unsigned COPY:1;
    unsigned PASTE:1;
    unsigned CLEAR:1;
    unsigned F1:1;
    unsigned F2:1;
    unsigned F3:1;
    unsigned F4:1;
    unsigned UP:1;
    unsigned DOWN:1;
  };
} seq_ui_button_state_t;


typedef enum {
  SEQ_UI_BUTTON_GP1,
  SEQ_UI_BUTTON_GP2,
  SEQ_UI_BUTTON_GP3,
  SEQ_UI_BUTTON_GP4,
  SEQ_UI_BUTTON_GP5,
  SEQ_UI_BUTTON_GP6,
  SEQ_UI_BUTTON_GP7,
  SEQ_UI_BUTTON_GP8,
  SEQ_UI_BUTTON_GP9,
  SEQ_UI_BUTTON_GP10,
  SEQ_UI_BUTTON_GP11,
  SEQ_UI_BUTTON_GP12,
  SEQ_UI_BUTTON_GP13,
  SEQ_UI_BUTTON_GP14,
  SEQ_UI_BUTTON_GP15,
  SEQ_UI_BUTTON_GP16,
  SEQ_UI_BUTTON_Exit,
  SEQ_UI_BUTTON_Select,
  SEQ_UI_BUTTON_Left,
  SEQ_UI_BUTTON_Right,
  SEQ_UI_BUTTON_Up,
  SEQ_UI_BUTTON_Down
} seq_ui_button_t;


typedef enum {
  SEQ_UI_ENCODER_GP1,
  SEQ_UI_ENCODER_GP2,
  SEQ_UI_ENCODER_GP3,
  SEQ_UI_ENCODER_GP4,
  SEQ_UI_ENCODER_GP5,
  SEQ_UI_ENCODER_GP6,
  SEQ_UI_ENCODER_GP7,
  SEQ_UI_ENCODER_GP8,
  SEQ_UI_ENCODER_GP9,
  SEQ_UI_ENCODER_GP10,
  SEQ_UI_ENCODER_GP11,
  SEQ_UI_ENCODER_GP12,
  SEQ_UI_ENCODER_GP13,
  SEQ_UI_ENCODER_GP14,
  SEQ_UI_ENCODER_GP15,
  SEQ_UI_ENCODER_GP16,
  SEQ_UI_ENCODER_Datawheel
} seq_ui_encoder_t;


typedef enum {
  SEQ_UI_EDIT_MODE_NORMAL,
  SEQ_UI_EDIT_MODE_COPY,
  SEQ_UI_EDIT_MODE_PASTE,
  SEQ_UI_EDIT_MODE_CLEAR,
  SEQ_UI_EDIT_MODE_MOVE,
  SEQ_UI_EDIT_MODE_SCROLL,
  SEQ_UI_EDIT_MODE_RANDOM,
  SEQ_UI_EDIT_MODE_RECORD
} seq_ui_edit_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_Init(u32 mode);

extern s32 SEQ_UI_InitEncSpeed(u32 auto_config);

extern s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value);
extern s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer);
extern s32 SEQ_UI_LED_Handler(void);
extern s32 SEQ_UI_LED_Handler_Periodic();
extern s32 SEQ_UI_LCD_Handler(void);
extern s32 SEQ_UI_MENU_Handler_Periodic();

extern s32 SEQ_UI_PageSet(seq_ui_page_t page);

extern s32 SEQ_UI_InstallInitCallback(void *callback);
extern s32 SEQ_UI_InstallButtonCallback(void *callback);
extern s32 SEQ_UI_InstallEncoderCallback(void *callback);
extern s32 SEQ_UI_InstallLEDCallback(void *callback);
extern s32 SEQ_UI_InstallLCDCallback(void *callback);

extern u8  SEQ_UI_VisibleTrackGet(void);
extern s32 SEQ_UI_IsSelectedTrack(u8 track);

extern s32 SEQ_UI_SelectedStepSet(u8 step);

extern s32 SEQ_UI_GxTyInc(s32 incrementer);
extern s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_Var8_Inc(u8 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_CC_Inc(u8 cc, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_CC_Set(u8 cc, u16 value);
extern s32 SEQ_UI_CC_SetFlags(u8 cc, u16 flag_mask, u16 value);


/////////////////////////////////////////////////////////////////////////////
// Prototypes for functions implemented in seq_ui_*.c
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_EDIT_LCD_Handler(u8 high_prio, seq_ui_edit_mode_t edit_mode);
extern s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds);

extern s32 SEQ_UI_UTIL_CopyButton(s32 depressed);
extern s32 SEQ_UI_UTIL_PasteButton(s32 depressed);
extern s32 SEQ_UI_UTIL_ClearButton(s32 depressed);
extern s32 SEQ_UI_UTIL_UndoButton(s32 depressed);

extern s32 SEQ_UI_UTIL_UndoUpdate(u8 track);

extern u8 SEQ_UI_UTIL_CopyPasteBeginGet(void);
extern u8 SEQ_UI_UTIL_CopyPasteEndGet(void);


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
extern u8 ui_selected_item;

extern u16 ui_hold_msg_ctr;

extern seq_ui_page_t ui_page;
extern seq_ui_page_t ui_shortcut_prev_page;

extern volatile u8 ui_cursor_flash;

extern u8 ui_seq_pause;

#endif /* _SEQ_UI_H */
