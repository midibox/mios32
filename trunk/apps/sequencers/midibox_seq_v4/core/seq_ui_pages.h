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
  SEQ_UI_PAGE_INSSEL = 6,
  SEQ_UI_PAGE_TRACKSEL = 7,
  SEQ_UI_PAGE_BPM_PRESETS = 8,

  SEQ_UI_PAGE_DIRECT_RESERVED2 = 9,
  SEQ_UI_PAGE_DIRECT_RESERVED3 = 10,
  SEQ_UI_PAGE_DIRECT_RESERVED4 = 11,
  SEQ_UI_PAGE_DIRECT_RESERVED5 = 12,
  SEQ_UI_PAGE_DIRECT_RESERVED6 = 13,
  SEQ_UI_PAGE_DIRECT_RESERVED7 = 14,
  SEQ_UI_PAGE_DIRECT_RESERVED8 = 15,

  SEQ_UI_PAGE_EDIT = 16,
  SEQ_UI_PAGE_MUTE,
  SEQ_UI_PAGE_PMUTE,
  SEQ_UI_PAGE_PATTERN,
  SEQ_UI_PAGE_SONG,
  SEQ_UI_PAGE_MIXER,
  SEQ_UI_PAGE_TRKEVNT,
  SEQ_UI_PAGE_TRKINST,
  SEQ_UI_PAGE_TRKMODE,
  SEQ_UI_PAGE_TRKDIR,
  SEQ_UI_PAGE_TRKDIV,
  SEQ_UI_PAGE_TRKLEN,
  SEQ_UI_PAGE_TRKTRAN,
  SEQ_UI_PAGE_TRKGRV,
  SEQ_UI_PAGE_TRGASG,
  SEQ_UI_PAGE_TRKMORPH,
  SEQ_UI_PAGE_TRKRND,
  SEQ_UI_PAGE_TRKEUCLID,
  SEQ_UI_PAGE_TRKJAM,
  SEQ_UI_PAGE_MANUAL,
  SEQ_UI_PAGE_FX_ECHO,
  SEQ_UI_PAGE_FX_HUMANIZE,
  SEQ_UI_PAGE_FX_ROBOTIZE,
  SEQ_UI_PAGE_FX_LIMIT,
  SEQ_UI_PAGE_FX_LFO,
  SEQ_UI_PAGE_FX_DUPL,
  SEQ_UI_PAGE_FX_LOOP,
  SEQ_UI_PAGE_FX_SCALE,
  SEQ_UI_PAGE_UTIL,
  SEQ_UI_PAGE_BPM,
  SEQ_UI_PAGE_OPT,
  SEQ_UI_PAGE_SAVE,
  SEQ_UI_PAGE_MIDI,
  SEQ_UI_PAGE_MIDIMON,
  SEQ_UI_PAGE_SYSEX,
  SEQ_UI_PAGE_CV,
  SEQ_UI_PAGE_DISK,
  SEQ_UI_PAGE_ETH,
  SEQ_UI_PAGE_TRKLIVE,
  SEQ_UI_PAGE_PATTERN_RMX,
  SEQ_UI_PAGE_BOOKMARKS,
  SEQ_UI_PAGE_INFO,
  SEQ_UI_PAGE_LAST__UNIMPLEMENTED
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

