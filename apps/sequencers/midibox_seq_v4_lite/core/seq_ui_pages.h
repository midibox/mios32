// $Id$
/*
 * UI Menu Pages
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_UI_PAGES_H
#define _SEQ_UI_PAGES_H


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_UI_PAGE_NONE = 0,
  SEQ_UI_PAGE_LOAD,
  SEQ_UI_PAGE_SAVE,
  SEQ_UI_PAGE_TRIGGER,
  SEQ_UI_PAGE_LENGTH,
  SEQ_UI_PAGE_PROGRESSION,
  SEQ_UI_PAGE_GROOVE,
  SEQ_UI_PAGE_ECHO,
  SEQ_UI_PAGE_HUMANIZER,
  SEQ_UI_PAGE_LFO,
  SEQ_UI_PAGE_SCALE,
  SEQ_UI_PAGE_MUTE,
  SEQ_UI_PAGE_MIDICHN,
  SEQ_UI_PAGE_REC_ARM,
  SEQ_UI_PAGE_REC_STEP,
  SEQ_UI_PAGE_REC_LIVE,
} seq_ui_page_t;

#define SEQ_UI_PAGES (SEQ_UI_PAGE_SAVE+1)



typedef struct {
  u8 repeats;
  u8 delay;
  u8 velocity;
  u8 fb_velocity;
  u8 fb_note;
  u8 fb_gatelength;
  u8 fb_ticks;
} seq_ui_pages_echo_presets_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_PAGES_Set(seq_ui_page_t page);

extern u16 SEQ_UI_PAGES_GP_LED_Handler(void);
extern s32 SEQ_UI_PAGES_GP_Button_Handler(u8 button, u8 depressed);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_ui_page_t ui_page;

extern seq_ui_pages_echo_presets_t seq_ui_pages_echo_presets[16];

#endif /* _SEQ_PAGES_H */

