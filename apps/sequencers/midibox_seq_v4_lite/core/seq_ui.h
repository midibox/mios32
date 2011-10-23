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

// cursor flash with following waveform:
// 0..449 mS: cursor on
// 450..499 mS: cursor off
#define SEQ_UI_CURSOR_FLASH_CTR_MAX     500  // mS
#define SEQ_UI_CURSOR_FLASH_CTR_LED_OFF 450  // mS

#define SEQ_UI_BOOKMARKS_NUM 16

#define UI_QUICKSEL_NUM_PRESETS 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// !!!IMPORTANT!!!
// must be kept in sync with the original seq_ui_button_state_t in MBSEQ V4
// TODO: remove dependencies
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

    u32 SCALE_PRESSED:1;
  };
} seq_ui_button_state_t;

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
extern s32 SEQ_UI_Encoder_Handler(u32 encoder, s32 incrementer);
extern s32 SEQ_UI_REMOTE_MIDI_Keyboard(u8 key, u8 depressed);
extern s32 SEQ_UI_REMOTE_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);

extern s32 SEQ_UI_EDIT_Button_Handler(u8 button, u8 depressed);

extern s32 SEQ_UI_Button_Play(s32 depressed);
extern s32 SEQ_UI_Button_Stop(s32 depressed);

extern s32 SEQ_UI_LED_Handler(void);
extern s32 SEQ_UI_LED_Handler_Periodic();
extern s32 SEQ_UI_MENU_Handler_Periodic();

extern s32 SEQ_UI_CheckSelections(void);

extern u8  SEQ_UI_VisibleTrackGet(void);
extern s32 SEQ_UI_IsSelectedTrack(u8 track);

extern u8  SEQ_UI_VisibleLayerGet(void);
extern s32 SEQ_UI_IsSelectedLayer(u8 track);

extern s32 SEQ_UI_SelectedStepSet(u8 step);

extern s32 SEQ_UI_CC_Set(u8 cc, u8 value);
extern s32 SEQ_UI_CC_SetFlags(u8 cc, u8 flag_mask, u8 value);

extern s32 SEQ_UI_SDCardErrMsg(u16 delay, s32 status);

extern s32 SEQ_UI_Var16_Inc(u16 *value, u16 min, u16 max, s32 incrementer);
extern s32 SEQ_UI_Var8_Inc(u8 *value, u16 min, u16 max, s32 incrementer);

extern s32 SEQ_UI_Bookmark_Store(u8 bookmark);
extern s32 SEQ_UI_Bookmark_Restore(u8 bookmark);


/////////////////////////////////////////////////////////////////////////////
// Prototypes for functions implemented in seq_ui_*.c
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_UTIL_Copy(void);
extern s32 SEQ_UI_UTIL_Paste(void);
extern s32 SEQ_UI_UTIL_Clear(void);
extern s32 SEQ_UI_UTIL_Undo(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_ui_button_state_t seq_ui_button_state;

extern u8 ui_selected_group;
extern u16 ui_selected_tracks;
extern u8 ui_selected_par_layer;
extern u8 ui_selected_trg_layer;
extern u8 ui_selected_instrument;
extern u8 ui_selected_step;
extern u8 ui_selected_step_view;
extern u16 ui_selected_gp_buttons;

extern volatile u8 ui_cursor_flash;
extern volatile u8 ui_cursor_flash_overrun_ctr;
extern u16 ui_cursor_flash_ctr;

extern u8 seq_ui_display_update_req;

extern u8 ui_seq_pause;

extern u8 ui_quicksel_length[UI_QUICKSEL_NUM_PRESETS];
extern u8 ui_quicksel_loop_length[UI_QUICKSEL_NUM_PRESETS];
extern u8 ui_quicksel_loop_loop[UI_QUICKSEL_NUM_PRESETS];

extern seq_ui_bookmark_t seq_ui_bookmarks[SEQ_UI_BOOKMARKS_NUM];

#endif /* _SEQ_UI_H */
