// $Id$
/*
 * Page table
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
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// must be kept in sync with ui_menu_pages table in seq_ui_pages.c
typedef enum {
  SEQ_UI_PAGE_NONE = 0,
  SEQ_UI_PAGE_MENU = 1,
  SEQ_UI_PAGE_FX = 2,
  SEQ_UI_PAGE_STEPSEL = 3,
  SEQ_UI_PAGE_TRGSEL = 4,
  SEQ_UI_PAGE_PARSEL = 5,
  SEQ_UI_PAGE_TRACKSEL = 6,
  SEQ_UI_PAGE_BPM_PRESETS = 7,

  SEQ_UI_PAGE_DIRECT_RESERVED1 = 8,
  SEQ_UI_PAGE_DIRECT_RESERVED2 = 9,
  SEQ_UI_PAGE_DIRECT_RESERVED3 = 10,
  SEQ_UI_PAGE_DIRECT_RESERVED4 = 11,
  SEQ_UI_PAGE_DIRECT_RESERVED5 = 12,
  SEQ_UI_PAGE_DIRECT_RESERVED6 = 13,
  SEQ_UI_PAGE_DIRECT_RESERVED7 = 14,
  SEQ_UI_PAGE_DIRECT_RESERVED8 = 15,

  SEQ_UI_PAGE_EDIT = 16,
  SEQ_UI_PAGE_MUTE = 17,
  SEQ_UI_PAGE_PMUTE = 18,
  SEQ_UI_PAGE_PATTERN = 19,
  SEQ_UI_PAGE_SONG = 20,
  SEQ_UI_PAGE_MIXER = 21,
  SEQ_UI_PAGE_TRKEVNT = 22,
  SEQ_UI_PAGE_TRKINST = 23,
  SEQ_UI_PAGE_TRKMODE = 24,
  SEQ_UI_PAGE_TRKDIR = 25,
  SEQ_UI_PAGE_TRKDIV = 26,
  SEQ_UI_PAGE_TRKLEN = 27,
  SEQ_UI_PAGE_TRKTRAN = 28,
  SEQ_UI_PAGE_TRKGRV = 29,
  SEQ_UI_PAGE_TRGASG = 30,
  SEQ_UI_PAGE_TRKMORPH = 31,
  SEQ_UI_PAGE_TRKRND = 32,
  SEQ_UI_PAGE_TRKEUCLID = 33,
  SEQ_UI_PAGE_TRKREC = 34,
  SEQ_UI_PAGE_MANUAL = 35,
  SEQ_UI_PAGE_FX_ECHO = 36,
  SEQ_UI_PAGE_FX_HUMANIZE = 37,
  SEQ_UI_PAGE_FX_LIMIT = 38,
  SEQ_UI_PAGE_FX_LFO = 39,
  SEQ_UI_PAGE_FX_DUPL = 40,
  SEQ_UI_PAGE_FX_LOOP = 41,
  SEQ_UI_PAGE_FX_SCALE = 42,
  SEQ_UI_PAGE_UTIL = 43,
  SEQ_UI_PAGE_BPM = 44,
  SEQ_UI_PAGE_OPT = 45,
  SEQ_UI_PAGE_SAVE = 46,
  SEQ_UI_PAGE_METRONOME = 47,
  SEQ_UI_PAGE_MIDI = 48,
  SEQ_UI_PAGE_MIDIMON = 49,
  SEQ_UI_PAGE_SYSEX = 50,
  SEQ_UI_PAGE_CV = 51,
  SEQ_UI_PAGE_DISK = 52,
  SEQ_UI_PAGE_ETH = 53,
  SEQ_UI_PAGE_TRKLIVE = 54,
  SEQ_UI_PAGE_PATTERN_RMX = 55,
  SEQ_UI_PAGE_BOOKMARKS = 56,
  SEQ_UI_PAGE_INFO = 57,
  SEQ_UI_PAGE_LAST__UNIMPLEMENTED = 58
} seq_ui_page_t;

#define SEQ_UI_PAGES (SEQ_UI_PAGE_LAST__UNIMPLEMENTED)
#define SEQ_UI_FIRST_MENU_SELECTION_PAGE SEQ_UI_PAGE_EDIT
#define SEQ_UI_NUM_MENU_PAGES (SEQ_UI_PAGES-SEQ_UI_FIRST_MENU_SELECTION_PAGE)


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_UI_PAGES_CallInit(seq_ui_page_t page);
extern const char *SEQ_UI_PAGES_PageNameGet(seq_ui_page_t page);
extern const char *SEQ_UI_PAGES_CfgNameGet(seq_ui_page_t page);
extern seq_ui_page_t SEQ_UI_PAGES_CfgNameSearch(const char *name);
extern seq_ui_page_t SEQ_UI_PAGES_OldBmIndexSearch(u32 bm_index);

extern seq_ui_page_t SEQ_UI_PAGES_MenuShortcutPageGet(u8 pos);
extern s32 SEQ_UI_PAGES_MenuShortcutPageSet(u8 pos, seq_ui_page_t page);
extern const char *SEQ_UI_PAGES_MenuShortcutNameGet(u8 pos);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_UI_PAGES_H */

