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
// 0..399 mS: cursor on
// 400..499 mS: cursor off
#define SEQ_UI_CURSOR_FLASH_CTR_MAX     500  // mS
#define SEQ_UI_CURSOR_FLASH_CTR_LED_OFF 400  // mS


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u32 ALL;
  };
  struct {
    u32 TRIGGER:1;
    u32 MUTE:1;
    u32 SOLO:1;
    u32 COPY:1;
    u32 PASTE:1;
    u32 CLEAR:1;
    u32 UNDO:1;
    u32 TAP_TEMPO:1;
    u32 EXT_RESTART:1;
  };
} seq_ui_button_state_t;



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_Init(u32 mode);

extern s32 SEQ_UI_InitEncSpeed(u32 auto_config);

extern s32 SEQ_UI_Button_Handler(u32 pin, u32 pin_value);
extern s32 SEQ_UI_REMOTE_MIDI_Keyboard(u8 key, u8 depressed);
extern s32 SEQ_UI_REMOTE_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);

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

extern u16 ui_selected_tracks;
extern u8 ui_selected_step;
extern u8 ui_selected_step_view;

extern volatile u8 ui_cursor_flash;
extern volatile u8 ui_cursor_flash_overrun_ctr;
extern u16 ui_cursor_flash_ctr;

extern u8 seq_ui_display_update_req;

extern u8 ui_seq_pause;

#endif /* _SEQ_UI_H */
