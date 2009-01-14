// $Id$
/*
 * This include file contains references to all available menu pages
 *
 * If "SEQ_UI_PAGES_INC_LOCAL_VARS" is defined (only set inside seq_ui.c), variables
 * will be declared in addition
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

// must be kept in sync with ui_init_callback list!
typedef enum {
  SEQ_UI_PAGE_NONE,
  SEQ_UI_PAGE_MENU,
  SEQ_UI_PAGE_SHORTCUT,
  SEQ_UI_PAGE_FX,
  SEQ_UI_PAGE_STEPSEL,
  SEQ_UI_PAGE_EDIT,
  SEQ_UI_PAGE_MUTE,
  SEQ_UI_PAGE_PATTERN,
  SEQ_UI_PAGE_TRKEVNT,
  SEQ_UI_PAGE_TRKMODE,
  SEQ_UI_PAGE_TRKDIR,
  SEQ_UI_PAGE_TRKDIV,
  SEQ_UI_PAGE_TRKLEN,
  SEQ_UI_PAGE_TRKTRAN,
  SEQ_UI_PAGE_TRGASG,
  SEQ_UI_PAGE_TRKRND,
  SEQ_UI_PAGE_FX_ECHO,
  SEQ_UI_PAGE_UTIL,
  SEQ_UI_PAGE_BPM,
  SEQ_UI_PAGE_OPT,
  SEQ_UI_PAGE_SAVE,
  SEQ_UI_PAGE_MIXER
} seq_ui_page_t;

	
#define SEQ_UI_PAGES (SEQ_UI_PAGE_MIXER + 1)
#define SEQ_UI_FIRST_MENU_SELECTION_PAGE SEQ_UI_PAGE_EDIT


// Prototypes for all UI pages are burried here
extern s32 SEQ_UI_TODO_Init(u32 mode);
extern s32 SEQ_UI_MENU_Init(u32 mode);
extern s32 SEQ_UI_SHORTCUT_Init(u32 mode);
extern s32 SEQ_UI_FX_Init(u32 mode);
extern s32 SEQ_UI_STEPSEL_Init(u32 mode);
extern s32 SEQ_UI_EDIT_Init(u32 mode);
extern s32 SEQ_UI_MUTE_Init(u32 mode);
extern s32 SEQ_UI_PATTERN_Init(u32 mode);
extern s32 SEQ_UI_TRKEVNT_Init(u32 mode);
extern s32 SEQ_UI_TRKMODE_Init(u32 mode);
extern s32 SEQ_UI_TRKDIR_Init(u32 mode);
extern s32 SEQ_UI_TRKDIV_Init(u32 mode);
extern s32 SEQ_UI_TRKLEN_Init(u32 mode);
extern s32 SEQ_UI_TRKTRAN_Init(u32 mode);
extern s32 SEQ_UI_TRGASG_Init(u32 mode);
extern s32 SEQ_UI_TRKRND_Init(u32 mode);
extern s32 SEQ_UI_FX_ECHO_Init(u32 mode);
extern s32 SEQ_UI_UTIL_Init(u32 mode);
extern s32 SEQ_UI_BPM_Init(u32 mode);
extern s32 SEQ_UI_OPT_Init(u32 mode);
extern s32 SEQ_UI_SAVE_Init(u32 mode);
extern s32 SEQ_UI_MIXER_Init(u32 mode);


typedef struct {
  char name[19]; 
  s32 (*init_callback)(u32 mode);
} seq_ui_page_entry_t;


#ifdef SEQ_UI_PAGES_INC_LOCAL_VARS

static const seq_ui_page_entry_t ui_menu_pages[SEQ_UI_PAGES] = {
  { "TODO page         ", (void *)&SEQ_UI_TODO_Init },
  { "Page Menu         ", (void *)&SEQ_UI_MENU_Init },
  { "Page Shortcuts    ", (void *)&SEQ_UI_SHORTCUT_Init },
  { "Fx Selection      ", (void *)&SEQ_UI_FX_Init },
  { "Step Selection    ", (void *)&SEQ_UI_STEPSEL_Init },

  // menu selection starts here
  { "Edit              ", (void *)&SEQ_UI_EDIT_Init },
  { "Mute Tracks       ", (void *)&SEQ_UI_MUTE_Init },
  { "Patterns          ", (void *)&SEQ_UI_PATTERN_Init },
  { "Track Events      ", (void *)&SEQ_UI_TRKEVNT_Init },
  { "Track Mode        ", (void *)&SEQ_UI_TRKMODE_Init },
  { "Track Direction   ", (void *)&SEQ_UI_TRKDIR_Init },
  { "Track ClockDivider", (void *)&SEQ_UI_TRKDIV_Init },
  { "Track Length      ", (void *)&SEQ_UI_TRKLEN_Init },
  { "Track Transpose   ", (void *)&SEQ_UI_TRKTRAN_Init },
  { "Track Triggers    ", (void *)&SEQ_UI_TRGASG_Init },
  { "Random Generator  ", (void *)&SEQ_UI_TRKRND_Init },
  { "Track Fx: Echo    ", (void *)&SEQ_UI_FX_ECHO_Init },
  { "Utilities         ", (void *)&SEQ_UI_UTIL_Init },
  { "BPM Selection     ", (void *)&SEQ_UI_BPM_Init },
  { "Options           ", (void *)&SEQ_UI_OPT_Init },
  { "Save Pattern      ", (void *)&SEQ_UI_SAVE_Init },
  { "Mixer             ", (void *)&SEQ_UI_MIXER_Init }
};
#endif

