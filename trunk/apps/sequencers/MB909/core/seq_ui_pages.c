// $Id: seq_ui_pages.c 2164 2015-04-20 20:40:45Z tk $
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

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "seq_ui.h"
#include "seq_ui_pages.h"
#include "seq_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// The big page table
// Order of entries must be kept in sync with the seq_ui_page_t enum in seq_ui_pages.h
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char *cfg_name;           // used in MBSEQ_HW.V4 file to config the page
  const char short_name[6];       // used in MENU page, up to 5 chars
  const char long_name[19];       // used in main page, up to 18 chars
  u8    old_bm_index;             // for compatibility of bookmark files, never change these numbers!
  s32 (*init_callback)(u32 mode); // will be called to initialize the page
} seq_ui_page_entry_t;

static const seq_ui_page_entry_t ui_menu_pages[SEQ_UI_PAGES] = {
  { "NONE",         "TODO ", "TODO page         ",  0, (void *)&SEQ_UI_TODO_Init },
  { "MENU",         "Menu ", "Page Menu         ",  1, (void *)&SEQ_UI_MENU_Init },
  { "FXSEL",        " Fx  ", "Fx Selection      ",  2, (void *)&SEQ_UI_FX_Init },
  { "STEPSEL",      "StpS.", "Step Selection    ",  3, (void *)&SEQ_UI_STEPSEL_Init },
  { "TRGSEL",       "TrgS.", "Trigger Selection ",  4, (void *)&SEQ_UI_TRGSEL_Init },
  { "PARSEL",       "ParS.", "Param. Selection  ",  5, (void *)&SEQ_UI_PARSEL_Init },
  { "TRACKSEL",     "TrkS.", "Track Selection   ",  6, (void *)&SEQ_UI_TRACKSEL_Init },
  { "BPM_PRESETS",  "BpmP.", "BPM Presets       ",  7, (void *)&SEQ_UI_BPM_PRESETS_Init },

  { "RES1",         "Res1 ", "Reserved page #1  ",  8, (void *)&SEQ_UI_TODO_Init },
  { "RES2",         "Res2 ", "Reserved page #2  ",  9, (void *)&SEQ_UI_TODO_Init },
  { "RES3",         "Res3 ", "Reserved page #3  ", 10, (void *)&SEQ_UI_TODO_Init },
  { "RES4",         "Res4 ", "Reserved page #4  ", 11, (void *)&SEQ_UI_TODO_Init },
  { "RES5",         "Res5 ", "Reserved page #5  ", 12, (void *)&SEQ_UI_TODO_Init },
  { "RES6",         "Res6 ", "Reserved page #6  ", 13, (void *)&SEQ_UI_TODO_Init },
  { "RES7",         "Res7 ", "Reserved page #7  ", 14, (void *)&SEQ_UI_TODO_Init },
  { "RES8",         "Res8 ", "Reserved page #8  ", 15, (void *)&SEQ_UI_TODO_Init },

  // menu selection  starts here
  { "EDIT",         "Edit ", "Edit              ", 16, (void *)&SEQ_UI_EDIT_Init },
  { "MUTE",         "Mute ", "Mute Tracks       ", 17, (void *)&SEQ_UI_MUTE_Init },
  { "MUTE_PORTS",   "PMute", "Mute Ports        ", 18, (void *)&SEQ_UI_PMUTE_Init },
  { "PATTERNS",     "Pat. ", "Patterns          ", 19, (void *)&SEQ_UI_PATTERN_Init },
  { "SONG",         "Song ", "Song              ", 20, (void *)&SEQ_UI_SONG_Init },
  { "MIXER",        "Mix  ", "Mixer             ", 21, (void *)&SEQ_UI_MIXER_Init },
  { "EVENTS",       "Evnt ", "Track Events      ", 22, (void *)&SEQ_UI_TRKEVNT_Init },
  { "INSTRUMENT",   "Inst ", "Track Instrument  ",  0, (void *)&SEQ_UI_TRKINST_Init },
  { "MODE",         "Mode ", "Track Mode        ", 23, (void *)&SEQ_UI_TRKMODE_Init },
  { "DIRECTION",    "Dir. ", "Track Direction   ", 24, (void *)&SEQ_UI_TRKDIR_Init },
  { "DIVIDER",      "Div. ", "Track ClockDivider", 25, (void *)&SEQ_UI_TRKDIV_Init },
  { "LENGTH",       "Len. ", "Track Length      ", 26, (void *)&SEQ_UI_TRKLEN_Init },
  { "TRANSPOSE",    "Trn. ", "Track Transpose   ", 27, (void *)&SEQ_UI_TRKTRAN_Init },
  { "GROOVE",       "Grv. ", "Track Groove      ", 28, (void *)&SEQ_UI_TRKGRV_Init },
  { "TRG_ASSIGN",   "Trg. ", "Track Triggers    ", 29, (void *)&SEQ_UI_TRGASG_Init },
  { "MORPH",        "Mrp. ", "Track Morphing    ", 30, (void *)&SEQ_UI_TRKMORPH_Init },
  { "RANDOM",       "Rnd. ", "Random Generator  ", 31, (void *)&SEQ_UI_TRKRND_Init },
  { "EUCLID",       "Eucl.", "Track Euclid Gen. ", 55, (void *)&SEQ_UI_TRKEUCLID_Init },
  { "JAM",          "Jam  ", "Jam (Recording)   ", 32, (void *)&SEQ_UI_TRKJAM_Init },
  { "MANUAL",       "Man. ", "Manual Trigger    ", 33, (void *)&SEQ_UI_MANUAL_Init },
  { "FX_ECHO",      "Echo ", "Track Fx: Echo    ", 34, (void *)&SEQ_UI_FX_ECHO_Init },
  { "FX_HUMANIZER", "Hum. ", "Track Fx: Humanize", 35, (void *)&SEQ_UI_FX_HUMANIZE_Init },
  { "FX_ROBOTIZER", "Rob. ", "Track Fx: Robotize", 57, (void *)&SEQ_UI_FX_ROBOTIZE_Init },
  { "FX_LIMIT",     "Limit", "Track Fx: Limit   ", 36, (void *)&SEQ_UI_FX_LIMIT_Init },
  { "FX_LFO",       "LFO  ", "Track Fx: LFO     ", 37, (void *)&SEQ_UI_FX_LFO_Init },
  { "FX_DUPLICATE", "Dupl.", "Track Fx: Duplict.", 56, (void *)&SEQ_UI_FX_DUPL_Init },
  { "FX_LOOP",      "Loop ", "Global Fx: Loop   ", 38, (void *)&SEQ_UI_FX_LOOP_Init },
  { "FX_SCALE",     "Scale", "Global Fx: Scale  ", 39, (void *)&SEQ_UI_FX_SCALE_Init },
  { "UTIL",         "Util ", "Utilities         ", 40, (void *)&SEQ_UI_UTIL_Init },
  { "BPM",          "BPM  ", "BPM Selection     ", 41, (void *)&SEQ_UI_BPM_Init },
  { "OPTIONS",      "Opt. ", "Options           ", 42, (void *)&SEQ_UI_OPT_Init },
  { "SAVE",         "Save ", "Save Pattern      ", 43, (void *)&SEQ_UI_SAVE_Init },
  { "METRONOME",    "Metr.", "Metronome         ", 44, (void *)&SEQ_UI_METRONOME_Init },
  { "MIDI",         "MIDI ", "MIDI Configuration", 45, (void *)&SEQ_UI_MIDI_Init },
  { "MIDIMON",      "MMon ", "MIDI Monitor      ", 46, (void *)&SEQ_UI_MIDIMON_Init },
  { "SYSEX",        "SysEx", "SysEx             ", 47, (void *)&SEQ_UI_SYSEX_Init },
  { "CVCFG",        "CVCfg", "CV Configuration  ", 48, (void *)&SEQ_UI_CV_Init },
  { "DISK",         "Disk ", "Disk (SD Card)    ", 49, (void *)&SEQ_UI_DISK_Init },
  { "ETH_OSC",      "Eth. ", "Ethernet (OSC)    ", 50, (void *)&SEQ_UI_ETH_Init },
  { "LIVE",         "Live ", "Live Play         ", 53, (void *)&SEQ_UI_TRKLIVE_Init },
  { "REMIX",        "Remix", "Pattern Remix     ", 54, (void *)&SEQ_UI_PATTERN_RMX_Init },
  { "BOOKMARKS",    "BookM", "Bookmarks         ", 51, (void *)&SEQ_UI_BOOKMARKS_Init },
  { "ABOUT",        "About", "About this MIDIbox", 52, (void *)&SEQ_UI_INFO_Init },
  { "CONTRAST",     "Contr", "LCD contrast setup", 53, (void *)&SEQ_UI_LCD_Init },
};


/////////////////////////////////////////////////////////////////////////////
// Access functions to the table
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PAGES_CallInit(seq_ui_page_t page)
{
  if( page >= SEQ_UI_PAGES )
    return -1; // invalid page

  if( ui_menu_pages[ui_page].init_callback != NULL )
    return ui_menu_pages[ui_page].init_callback(0); // mode

  return -2; // no init function available
}

const char *SEQ_UI_PAGES_PageNameGet(seq_ui_page_t page)
{
  if( page >= SEQ_UI_PAGES )
    return "** Invalid Page **";

  return (const char *)ui_menu_pages[page].long_name;
}

const char *SEQ_UI_PAGES_CfgNameGet(seq_ui_page_t page)
{
  if( page >= SEQ_UI_PAGES )
    return "INVALID";

  return ui_menu_pages[page].cfg_name;
}

seq_ui_page_t SEQ_UI_PAGES_CfgNameSearch(const char *name)
{
  int i;

  seq_ui_page_entry_t *page_ptr = (seq_ui_page_entry_t *)&ui_menu_pages[0];
  for(i=0; i<SEQ_UI_PAGES; ++i, ++page_ptr) {
    if( strcasecmp(name, page_ptr->cfg_name) == 0 )
      return i;
  }

  return SEQ_UI_PAGE_NONE;
}

seq_ui_page_t SEQ_UI_PAGES_OldBmIndexSearch(u32 bm_index)
{
  int i;

  seq_ui_page_entry_t *page_ptr = (seq_ui_page_entry_t *)&ui_menu_pages[0];
  for(i=0; i<SEQ_UI_PAGES; ++i, ++page_ptr) {
    if( page_ptr->old_bm_index == bm_index )
      return i;
  }

  return SEQ_UI_PAGE_NONE;
}


/////////////////////////////////////////////////////////////////////////////
// Menu Shortcuts
/////////////////////////////////////////////////////////////////////////////

// following pages are directly accessible with the GP buttons when MENU button is pressed
static seq_ui_page_t ui_shortcut_menu_pages[16] = {
  SEQ_UI_PAGE_MIXER,       // GP1
  SEQ_UI_PAGE_TRKEVNT,     // GP2
  SEQ_UI_PAGE_TRKDIR,     // GP3
  SEQ_UI_PAGE_TRKLEN,      // GP4
  SEQ_UI_PAGE_TRKGRV,      // GP5
  SEQ_UI_PAGE_TRGASG,      // GP6
  SEQ_UI_PAGE_MANUAL,     // GP7
  SEQ_UI_PAGE_UTIL,      // GP8
  SEQ_UI_PAGE_OPT,      // GP9
  SEQ_UI_PAGE_TRKDIV,          // GP10
  SEQ_UI_PAGE_TRKLIVE,      // GP11
  SEQ_UI_PAGE_TRKJAM,    // GP12
  SEQ_UI_PAGE_BPM,         // GP13
  SEQ_UI_PAGE_SAVE,        // GP14
  SEQ_UI_PAGE_MIDI,        // GP15
  SEQ_UI_PAGE_SYSEX        // GP16  
  };

seq_ui_page_t SEQ_UI_PAGES_MenuShortcutPageGet(u8 pos)
{
  if( pos >= 16 )
    return SEQ_UI_PAGE_NONE;

  return ui_shortcut_menu_pages[pos];
}

s32 SEQ_UI_PAGES_MenuShortcutPageSet(u8 pos, seq_ui_page_t page)
{
  if( page >= SEQ_UI_PAGES )
    return -1; // invalid page

  if( pos >= 16 )
    return -2; // invalid pos

  ui_shortcut_menu_pages[pos] = page;

  return 0; // no error
}


const char *SEQ_UI_PAGES_MenuShortcutNameGet(u8 pos)
{
  if( pos >= 16 )
    return "?????";

  return ui_menu_pages[ui_shortcut_menu_pages[pos]].short_name;
}
