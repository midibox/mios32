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

#include "seq_ui_pages.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// cursor flash with following waveform:
// 0..449 mS: cursor on
// 450..499 mS: cursor off
#define SEQ_UI_CURSOR_FLASH_CTR_MAX     500  // mS
#define SEQ_UI_CURSOR_FLASH_CTR_LED_OFF 450  // mS
#define SEQ_UI_CURSOR_FLASH_CTR_LED_OFF_EDIT_PAGE 300  // mS in edit page

#define SEQ_UI_BOOKMARKS_NUM 16

#define UI_QUICKSEL_NUM_PRESETS 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u32 ALL;
  };
  struct {
    u8 PAGE_CHANGE_BUTTON_FLAGS:7;
  };
  struct {
    // these button functions will change a page (special "radio button" handling required)
    u32 MENU_PRESSED:1;
    u32 STEP_VIEW:1;
    u32 PAR_LAYER_SEL:1;
    u32 TRG_LAYER_SEL:1;
    u32 TRACK_SEL:1;
    u32 TEMPO_PRESET:1;
    u32 BOOKMARK:1;

    // remaining functions
    u32 MENU_FIRST_PAGE_SELECTED:1;
    u32 CHANGE_ALL_STEPS:1;
    u32 CHANGE_ALL_STEPS_SAME_VALUE:1;
    u32 SELECT_PRESSED:1;
    u32 EDIT_PRESSED:1;
    u32 MUTE_PRESSED:1;
    u32 PATTERN_PRESSED:1;
    u32 SONG_PRESSED:1;
    u32 FAST_ENCODERS:1;
    u32 FAST2_ENCODERS:1;
    u32 SOLO:1;
    u32 SCRUB:1;
    u32 REW:1;
    u32 FWD:1;
    u32 COPY:1;
    u32 PASTE:1;
    u32 CLEAR:1;
    u32 UNDO:1;
    u32 TAP_TEMPO:1;
    u32 UP:1;
    u32 DOWN:1;
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
  SEQ_UI_BUTTON_Down,
  SEQ_UI_BUTTON_Edit  // only used in seq_ui_edit.c
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
  SEQ_UI_MSG_DELAYED_ACTION,
  SEQ_UI_MSG_DELAYED_ACTION_R
} seq_ui_msg_type_t;


typedef enum {
  SEQ_UI_REMOTE_MODE_AUTO=0,
  SEQ_UI_REMOTE_MODE_SERVER,
  SEQ_UI_REMOTE_MODE_CLIENT
} seq_ui_remote_mode_t;

// numbers should not be changed, as it's also used by the bookmark function
typedef enum {
  SEQ_UI_EDIT_VIEW_STEPS = 0,
  SEQ_UI_EDIT_VIEW_TRG = 1,
  SEQ_UI_EDIT_VIEW_LAYERS = 2,
  SEQ_UI_EDIT_VIEW_303 = 3,
  SEQ_UI_EDIT_VIEW_STEPSEL = 8,
} seq_ui_edit_view_t;

typedef enum {
  SEQ_UI_EDIT_DATAWHEEL_MODE_SCROLL_CURSOR = 0,
  SEQ_UI_EDIT_DATAWHEEL_MODE_SCROLL_VIEW,
  SEQ_UI_EDIT_DATAWHEEL_MODE_CHANGE_VALUE,
  SEQ_UI_EDIT_DATAWHEEL_MODE_CHANGE_PARLAYER,
  SEQ_UI_EDIT_DATAWHEEL_MODE_CHANGE_TRGLAYER
} seq_ui_edit_datawheel_mode_t;
#define SEQ_UI_EDIT_DATAWHEEL_MODE_NUM 5

typedef union {
  u8 ALL;

  struct {
    u8 LOCKED:1;
    u8 SOLO:1;
    u8 CHANGE_ALL_STEPS:1;
    u8 FAST:1;
    u8 METRONOME:1;
    u8 LOOP:1;
    u8 FOLLOW:1;
  };
} seq_ui_bookmark_flags_t;

typedef union {
  u32 ALL;

  struct {
    u32 SOLO:1;
    u32 CHANGE_ALL_STEPS:1;
    u32 FAST:1;
    u32 METRONOME:1;
    u32 LOOP:1;
    u32 FOLLOW:1;

    u32 PAGE:1;
    u32 GROUP:1;
    u32 PAR_LAYER:1;
    u32 TRG_LAYER:1;
    u32 INSTRUMENT:1;
    u32 STEP_VIEW:1;
    u32 STEP:1;
    u32 EDIT_VIEW:1;
    u32 MUTES:1;
    u32 TRACKS:1;
  };
} seq_ui_bookmark_enable_t;

typedef struct seq_ui_bookmark_t {
  seq_ui_bookmark_enable_t enable;
  seq_ui_bookmark_flags_t flags;
  char name[6]; // 5 chars used
  u8  page;
  u8  group;
  u8  par_layer;
  u8  trg_layer;
  u8  instrument;
  u8  step_view;
  u8  step;
  u8  edit_view;
  u16 tracks;
  u16 mutes;

  // note: pattern and song number shouldn't be stored via bookmark
  // since the "song phrases" feature is intended for such a purpose.
  // storing mutes is already an exception, because they can be controlled via phrases as well.
} seq_ui_bookmark_t;


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

extern s32 SEQ_UI_Button_Play(s32 depressed);
extern s32 SEQ_UI_Button_Stop(s32 depressed);

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
extern s32 SEQ_UI_InstallExitCallback(void *callback);

extern s32 SEQ_UI_InstallDelayedActionCallback(void *callback, u16 delay_mS, u32 parameter);
extern s32 SEQ_UI_UnInstallDelayedActionCallback(void *callback);

extern s32 SEQ_UI_InstallMIDIINCallback(void *callback);
extern s32 SEQ_UI_NotifyMIDIINCallback(mios32_midi_port_t port, mios32_midi_package_t p);

extern s32 SEQ_UI_LCD_Update(void);

extern s32 SEQ_UI_CheckSelections(void);

extern u8  SEQ_UI_VisibleTrackGet(void);
extern s32 SEQ_UI_IsSelectedTrack(u8 track);

extern s32 SEQ_UI_SelectedStepSet(u8 step);

extern s32 SEQ_UI_GxTyInc(s32 incrementer);
extern s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_Var8_Inc(u8 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_CC_SendParameter(u8 track, u8 cc);
extern s32 SEQ_UI_CC_Inc(u8 cc, u8 min, u8 max, s32 incrementer);
extern s32 SEQ_UI_CC_Set(u8 cc, u8 value);
extern s32 SEQ_UI_CC_SetFlags(u8 cc, u8 flag_mask, u8 value);

extern s32 SEQ_UI_SelectListItem(s32 incrementer, u8 num_items, u8 max_items_on_screen, u8 *selected_item_on_screen, u8 *view_offset);

extern s32 SEQ_UI_KeyPad_Init(void);
extern s32 SEQ_UI_KeyPad_Handler(seq_ui_encoder_t encoder, s32 incrementer, char *edit_str, u8 len);
extern s32 SEQ_UI_KeyPad_LCD_Msg(void);


/////////////////////////////////////////////////////////////////////////////
// Prototypes for functions implemented in seq_ui_*.c
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_TODO_Init(u32 mode);
extern s32 SEQ_UI_MENU_Init(u32 mode);
extern s32 SEQ_UI_PMUTE_Init(u32 mode);
extern s32 SEQ_UI_FX_Init(u32 mode);
extern s32 SEQ_UI_STEPSEL_Init(u32 mode);
extern s32 SEQ_UI_TRGSEL_Init(u32 mode);
extern s32 SEQ_UI_PARSEL_Init(u32 mode);
extern s32 SEQ_UI_TRACKSEL_Init(u32 mode);
extern s32 SEQ_UI_BPM_PRESETS_Init(u32 mode);
extern s32 SEQ_UI_EDIT_Init(u32 mode);
extern s32 SEQ_UI_MUTE_Init(u32 mode);
extern s32 SEQ_UI_PATTERN_Init(u32 mode);
extern s32 SEQ_UI_SONG_Init(u32 mode);
extern s32 SEQ_UI_MIXER_Init(u32 mode);
extern s32 SEQ_UI_TRKEVNT_Init(u32 mode);
extern s32 SEQ_UI_TRKINST_Init(u32 mode);
extern s32 SEQ_UI_TRKMODE_Init(u32 mode);
extern s32 SEQ_UI_TRKDIR_Init(u32 mode);
extern s32 SEQ_UI_TRKDIV_Init(u32 mode);
extern s32 SEQ_UI_TRKLEN_Init(u32 mode);
extern s32 SEQ_UI_TRKTRAN_Init(u32 mode);
extern s32 SEQ_UI_TRKGRV_Init(u32 mode);
extern s32 SEQ_UI_TRGASG_Init(u32 mode);
extern s32 SEQ_UI_TRKMORPH_Init(u32 mode);
extern s32 SEQ_UI_TRKRND_Init(u32 mode);
extern s32 SEQ_UI_TRKJAM_Init(u32 mode);
extern s32 SEQ_UI_MANUAL_Init(u32 mode);
extern s32 SEQ_UI_GROOVE_Init(u32 mode);
extern s32 SEQ_UI_FX_ECHO_Init(u32 mode);
extern s32 SEQ_UI_FX_HUMANIZE_Init(u32 mode);
extern s32 SEQ_UI_FX_ROBOTIZE_Init(u32 mode);
extern s32 SEQ_UI_FX_LIMIT_Init(u32 mode);
extern s32 SEQ_UI_FX_DUPL_Init(u32 mode);
extern s32 SEQ_UI_FX_LFO_Init(u32 mode);
extern s32 SEQ_UI_FX_LOOP_Init(u32 mode);
extern s32 SEQ_UI_FX_SCALE_Init(u32 mode);
extern s32 SEQ_UI_UTIL_Init(u32 mode);
extern s32 SEQ_UI_BPM_Init(u32 mode);
extern s32 SEQ_UI_OPT_Init(u32 mode);
extern s32 SEQ_UI_SAVE_Init(u32 mode);
extern s32 SEQ_UI_METRONOME_Init(u32 mode);
extern s32 SEQ_UI_MIDI_Init(u32 mode);
extern s32 SEQ_UI_MIDPLY_Init(u32 mode);
extern s32 SEQ_UI_MIDIMON_Init(u32 mode);
extern s32 SEQ_UI_SYSEX_Init(u32 mode);
extern s32 SEQ_UI_CV_Init(u32 mode);
extern s32 SEQ_UI_DISK_Init(u32 mode);
extern s32 SEQ_UI_ETH_Init(u32 mode);
extern s32 SEQ_UI_BOOKMARKS_Init(u32 mode);
extern s32 SEQ_UI_INFO_Init(u32 mode);
extern s32 SEQ_UI_TRKLIVE_Init(u32 mode);
extern s32 SEQ_UI_TRKREPEAT_Init(u32 mode);
extern s32 SEQ_UI_PATTERN_RMX_Init(u32 mode);
extern s32 SEQ_UI_TRKEUCLID_Init(u32 mode);


extern s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds);
extern s32 SEQ_UI_EDIT_Button_Handler(seq_ui_button_t button, s32 depressed);
extern s32 SEQ_UI_EDIT_LCD_Handler(u8 high_prio, seq_ui_edit_mode_t edit_mode);

extern s32 SEQ_UI_UTIL_CopyButton(s32 depressed);
extern s32 SEQ_UI_UTIL_PasteButton(s32 depressed);
extern s32 SEQ_UI_UTIL_ClearButton(s32 depressed);
extern s32 SEQ_UI_UTIL_UndoButton(s32 depressed);

extern s32 SEQ_UI_UTIL_CopyLivePattern(void);
extern s32 SEQ_UI_UTIL_PasteLivePattern(void);
extern s32 SEQ_UI_UTIL_ClearLivePattern(void);

extern s32 SEQ_UI_MIXER_Copy(void);
extern s32 SEQ_UI_MIXER_Paste(void);
extern s32 SEQ_UI_MIXER_Clear(void);
extern s32 SEQ_UI_MIXER_Undo(void);
extern s32 SEQ_UI_MIXER_UndoUpdate(void);

extern s32 SEQ_UI_SONG_Copy(void);
extern s32 SEQ_UI_SONG_Paste(void);
extern s32 SEQ_UI_SONG_Clear(void);
extern s32 SEQ_UI_SONG_Insert(void);
extern s32 SEQ_UI_SONG_Delete(void);
extern s32 SEQ_UI_SONG_EditPosSet(u8 new_edit_pos);

extern s32 SEQ_UI_BPM_TapTempo(void);

extern s32 SEQ_UI_UTIL_UndoUpdate(u8 track);

extern u8 SEQ_UI_UTIL_CopyPasteBeginGet(void);
extern u8 SEQ_UI_UTIL_CopyPasteEndGet(void);

extern s32 SEQ_UI_TRKJAM_PatternRecordSelected(void);

extern s32 SEQ_UI_Msg(seq_ui_msg_type_t msg_type, u16 delay, char *line1, char *line2);
extern s32 SEQ_UI_MsgStop(void);
extern s32 SEQ_UI_SDCardErrMsg(u16 delay, s32 status);
extern s32 SEQ_UI_MIDILearnMessage(seq_ui_msg_type_t msg_type, u8 on_off);

extern s32 SEQ_UI_Bookmark_Store(u8 bookmark);
extern s32 SEQ_UI_Bookmark_Restore(u8 bookmark);

extern s32 SEQ_UI_BOOKMARKS_Button_Handler(seq_ui_button_t button, s32 depressed);


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
extern u16 ui_selected_gp_buttons;

extern u16 ui_hold_msg_ctr;

extern seq_ui_page_t ui_page;
extern seq_ui_page_t ui_selected_page;
extern seq_ui_page_t ui_stepview_prev_page;
extern seq_ui_page_t ui_trglayer_prev_page;
extern seq_ui_page_t ui_parlayer_prev_page;

extern volatile u8 ui_cursor_flash;
extern volatile u8 ui_cursor_flash_overrun_ctr;
extern u16 ui_cursor_flash_ctr;

extern u8 ui_edit_name_cursor;
extern u8 ui_edit_preset_num_category;
extern u8 ui_edit_preset_num_label;
extern u8 ui_edit_preset_num_drum;

extern u8 ui_seq_pause;

extern u8 ui_song_edit_pos;

extern u8 ui_store_file_required;

extern u8 ui_quicksel_length[UI_QUICKSEL_NUM_PRESETS];
extern u8 ui_quicksel_loop_length[UI_QUICKSEL_NUM_PRESETS];
extern u8 ui_quicksel_loop_loop[UI_QUICKSEL_NUM_PRESETS];

extern u8 seq_ui_backup_req;
extern u8 seq_ui_format_req;
extern u8 seq_ui_saveall_req;

extern char ui_global_dir_list[80];

extern seq_ui_edit_view_t seq_ui_edit_view;
extern seq_ui_edit_datawheel_mode_t seq_ui_edit_datawheel_mode;

extern seq_ui_bookmark_t seq_ui_bookmarks[SEQ_UI_BOOKMARKS_NUM];

extern mios32_sys_time_t seq_play_timer;

#endif /* _SEQ_UI_H */
