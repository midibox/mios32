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

#include "seq_ui_pages.inc"


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
    unsigned ALL:32;
  };
  struct {
    unsigned PAGE_CHANGE_BUTTON_FLAGS:6;
  };
  struct {
    // these button functions will change a page (special "radio button" handling required)
    unsigned MENU_PRESSED:1;
    unsigned STEP_VIEW:1;
    unsigned PAR_LAYER_SEL:1;
    unsigned TRG_LAYER_SEL:1;
    unsigned TRACK_SEL:1;
    unsigned TEMPO_PRESET:1;

    // remaining functions
    unsigned MENU_FIRST_PAGE_SELECTED:1;
    unsigned CHANGE_ALL_STEPS:1;
    unsigned CHANGE_ALL_STEPS_SAME_VALUE:1;
    unsigned SELECT_PRESSED:1;
    unsigned EXIT_PRESSED:1;
    unsigned EDIT_PRESSED:1;
    unsigned MUTE_PRESSED:1;
    unsigned PATTERN_PRESSED:1;
    unsigned SONG_PRESSED:1;
    unsigned FAST_ENCODERS:1;
    unsigned SOLO:1;
    unsigned SCRUB:1;
    unsigned REW:1;
    unsigned FWD:1;
    unsigned COPY:1;
    unsigned PASTE:1;
    unsigned CLEAR:1;
    unsigned TAP_TEMPO:1;
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
  SEQ_UI_EDIT_MODE_RECORD,
  SEQ_UI_EDIT_MODE_MANUAL
} seq_ui_edit_mode_t;


typedef enum {
  SEQ_UI_MSG_USER,
  SEQ_UI_MSG_USER_R,
  SEQ_UI_MSG_SDCARD,
  SEQ_UI_MSG_DELAYED_ACTION
} seq_ui_msg_type_t;


typedef enum {
  SEQ_UI_REMOTE_MODE_AUTO=0,
  SEQ_UI_REMOTE_MODE_SERVER,
  SEQ_UI_REMOTE_MODE_CLIENT
} seq_ui_remote_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_Init(u32 mode);

extern s32 SEQ_UI_InitEncSpeed(u32 auto_config);

extern s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value);
extern s32 SEQ_UI_BLM_Button_Handler(u32 row, u32 pin, u32 pin_value);
extern s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer);
extern s32 SEQ_UI_REMOTE_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 SEQ_UI_REMOTE_MIDI_Keyboard(u8 key, u8 depressed);

extern s32 SEQ_UI_LED_Handler(void);
extern s32 SEQ_UI_LED_Handler_Periodic();
extern s32 SEQ_UI_LCD_Handler(void);
extern s32 SEQ_UI_MENU_Handler_Periodic();

extern s32 SEQ_UI_PageSet(seq_ui_page_t page);
extern char *SEQ_UI_PageNameGet(seq_ui_page_t page);

extern s32 SEQ_UI_InstallInitCallback(void *callback);
extern s32 SEQ_UI_InstallButtonCallback(void *callback);
extern s32 SEQ_UI_InstallEncoderCallback(void *callback);
extern s32 SEQ_UI_InstallLEDCallback(void *callback);
extern s32 SEQ_UI_InstallLCDCallback(void *callback);
extern s32 SEQ_UI_InstallExitCallback(void *callback);
extern s32 SEQ_UI_InstallDelayedActionCallback(void *callback, u16 ctr, char *msg);
extern s32 SEQ_UI_UnInstallDelayedActionCallback(void *callback);

extern s32 SEQ_UI_LCD_Update(void);

extern s32 SEQ_UI_CheckSelections(void);

extern u8  SEQ_UI_VisibleTrackGet(void);
extern s32 SEQ_UI_IsSelectedTrack(u8 track);

extern s32 SEQ_UI_SelectedStepSet(u8 step);

extern s32 SEQ_UI_GxTyInc(s32 incrementer);
extern s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_Var8_Inc(u8 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_CC_Inc(u8 cc, u8 min, u8 max, s32 incrementer);
extern s32 SEQ_UI_CC_Set(u8 cc, u8 value);
extern s32 SEQ_UI_CC_SetFlags(u8 cc, u8 flag_mask, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Prototypes for functions implemented in seq_ui_*.c
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_EDIT_LCD_Handler(u8 high_prio, seq_ui_edit_mode_t edit_mode);
extern s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds);

extern s32 SEQ_UI_MENU_StopwatchInit(void);
extern s32 SEQ_UI_MENU_StopwatchReset(void);
extern s32 SEQ_UI_MENU_StopwatchCapture(void);

extern s32 SEQ_UI_MENU_Idle(void);

extern s32 SEQ_UI_UTIL_CopyButton(s32 depressed);
extern s32 SEQ_UI_UTIL_PasteButton(s32 depressed);
extern s32 SEQ_UI_UTIL_ClearButton(s32 depressed);
extern s32 SEQ_UI_UTIL_UndoButton(s32 depressed);

extern s32 SEQ_UI_MIXER_Copy(void);
extern s32 SEQ_UI_MIXER_Paste(void);
extern s32 SEQ_UI_MIXER_Clear(void);
extern s32 SEQ_UI_MIXER_Undo(u8 mixer_map);
extern s32 SEQ_UI_MIXER_UndoUpdate(void);

extern s32 SEQ_UI_SONG_Copy(void);
extern s32 SEQ_UI_SONG_Paste(void);
extern s32 SEQ_UI_SONG_Clear(void);
extern s32 SEQ_UI_SONG_Insert(void);
extern s32 SEQ_UI_SONG_Delete(void);

extern s32 SEQ_UI_UTIL_UndoUpdate(u8 track);

extern u8 SEQ_UI_UTIL_CopyPasteBeginGet(void);
extern u8 SEQ_UI_UTIL_CopyPasteEndGet(void);

extern s32 SEQ_UI_Msg(seq_ui_msg_type_t msg_type, u16 delay, char *line1, char *line2);
extern s32 SEQ_UI_MsgStop(void);
extern s32 SEQ_UI_SDCardErrMsg(u16 delay, s32 status);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_ui_display_update_req;
extern u8 seq_ui_display_init_req;

extern seq_ui_button_state_t seq_ui_button_state;

extern u8 ui_selected_group;
extern u16 ui_selected_tracks;
extern u8 ui_selected_par_layer;
extern u8 ui_selected_trg_layer;
extern u8 ui_selected_instrument;
extern u8 ui_selected_step_view;
extern u8 ui_selected_step;
extern u8 ui_selected_item;

extern u16 ui_hold_msg_ctr;

extern seq_ui_page_t ui_page;
extern seq_ui_page_t ui_selected_page;
extern seq_ui_page_t ui_stepview_prev_page;
extern seq_ui_page_t ui_trglayer_prev_page;
extern seq_ui_page_t ui_parlayer_prev_page;

extern volatile u8 ui_cursor_flash;
extern volatile u8 ui_cursor_flash_overrun_ctr;

extern u8 ui_edit_name_cursor;
extern u8 ui_edit_preset_num_category;
extern u8 ui_edit_preset_num_label;
extern u8 ui_edit_preset_num_drum;

extern u8 ui_seq_pause;

extern seq_ui_remote_mode_t seq_ui_remote_mode;
extern seq_ui_remote_mode_t seq_ui_remote_active_mode;
extern mios32_midi_port_t seq_ui_remote_port;
extern mios32_midi_port_t seq_ui_remote_active_port;
extern u8 seq_ui_remote_id;
extern u16 seq_ui_remote_client_timeout_ctr;
extern u8 seq_ui_remote_force_lcd_update;
extern u8 seq_ui_remote_force_led_update;

extern u8 seq_ui_backup_req;
extern u8 seq_ui_format_req;

#endif /* _SEQ_UI_H */
