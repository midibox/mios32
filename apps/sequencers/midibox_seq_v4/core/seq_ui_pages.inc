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
// since beta40 the numbers shouldn't be changed anymore, since they are stored in bookmark file!
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
  SEQ_UI_PAGE_TRKMODE = 23,
  SEQ_UI_PAGE_TRKDIR = 24,
  SEQ_UI_PAGE_TRKDIV = 25,
  SEQ_UI_PAGE_TRKLEN = 26,
  SEQ_UI_PAGE_TRKTRAN = 27,
  SEQ_UI_PAGE_TRKGRV = 28,
  SEQ_UI_PAGE_TRGASG = 29,
  SEQ_UI_PAGE_TRKMORPH = 30,
  SEQ_UI_PAGE_TRKRND = 31,
  SEQ_UI_PAGE_TRKREC = 32,
  SEQ_UI_PAGE_MANUAL = 33,
  SEQ_UI_PAGE_FX_ECHO = 34,
  SEQ_UI_PAGE_FX_HUMANIZE = 35,
  SEQ_UI_PAGE_FX_LIMIT = 36,
  SEQ_UI_PAGE_FX_LFO = 37,
  SEQ_UI_PAGE_FX_LOOP = 38,
  SEQ_UI_PAGE_FX_SCALE = 39,
  SEQ_UI_PAGE_UTIL = 40,
  SEQ_UI_PAGE_BPM = 41,
  SEQ_UI_PAGE_OPT = 42,
  SEQ_UI_PAGE_SAVE = 43,
  SEQ_UI_PAGE_METRONOME = 44,
  SEQ_UI_PAGE_MIDI = 45,
  SEQ_UI_PAGE_MIDIMON = 46,
  SEQ_UI_PAGE_SYSEX = 47,
  SEQ_UI_PAGE_CV = 48,
  SEQ_UI_PAGE_DISK = 49,
  SEQ_UI_PAGE_ETH = 50,
  SEQ_UI_PAGE_BOOKMARKS = 51,
  SEQ_UI_PAGE_INFO = 52,
  SEQ_UI_PAGE_TRKLIVE = 53,
  SEQ_UI_PAGE_PATTERN_RMX = 54,
  SEQ_UI_PAGE_TRKEUCLID = 55,
  SEQ_UI_PAGE_FX_DUPL = 56,
} seq_ui_page_t;

#define SEQ_UI_PAGES (SEQ_UI_PAGE_FX_DUPL + 1)
#define SEQ_UI_FIRST_MENU_SELECTION_PAGE SEQ_UI_PAGE_EDIT
#define SEQ_UI_NUM_MENU_PAGES (SEQ_UI_PAGES-SEQ_UI_FIRST_MENU_SELECTION_PAGE)


// Prototypes for all UI pages are burried here
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
extern s32 SEQ_UI_TRKMODE_Init(u32 mode);
extern s32 SEQ_UI_TRKDIR_Init(u32 mode);
extern s32 SEQ_UI_TRKDIV_Init(u32 mode);
extern s32 SEQ_UI_TRKLEN_Init(u32 mode);
extern s32 SEQ_UI_TRKTRAN_Init(u32 mode);
extern s32 SEQ_UI_TRKGRV_Init(u32 mode);
extern s32 SEQ_UI_TRGASG_Init(u32 mode);
extern s32 SEQ_UI_TRKMORPH_Init(u32 mode);
extern s32 SEQ_UI_TRKRND_Init(u32 mode);
extern s32 SEQ_UI_TRKREC_Init(u32 mode);
extern s32 SEQ_UI_MANUAL_Init(u32 mode);
extern s32 SEQ_UI_GROOVE_Init(u32 mode);
extern s32 SEQ_UI_FX_ECHO_Init(u32 mode);
extern s32 SEQ_UI_FX_HUMANIZE_Init(u32 mode);
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
extern s32 SEQ_UI_PATTERN_RMX_Init(u32 mode);
extern s32 SEQ_UI_TRKEUCLID_Init(u32 mode);


typedef struct {
  char name[19]; 
  s32 (*init_callback)(u32 mode);
} seq_ui_page_entry_t;


#ifdef SEQ_UI_PAGES_INC_LOCAL_VARS

static const seq_ui_page_entry_t ui_menu_pages[SEQ_UI_PAGES] = {
  { "TODO page         ", (void *)&SEQ_UI_TODO_Init },
  { "Page Menu         ", (void *)&SEQ_UI_MENU_Init },
  { "Fx Selection      ", (void *)&SEQ_UI_FX_Init },
  { "Step Selection    ", (void *)&SEQ_UI_STEPSEL_Init },
  { "Trigger Selection ", (void *)&SEQ_UI_TRGSEL_Init },
  { "Param. Selection  ", (void *)&SEQ_UI_PARSEL_Init },
  { "Track Selection   ", (void *)&SEQ_UI_TRACKSEL_Init },
  { "BPM Presets       ", (void *)&SEQ_UI_BPM_PRESETS_Init },

  { "Reserved page #1  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #2  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #3  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #4  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #5  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #6  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #7  ", (void *)&SEQ_UI_TODO_Init },
  { "Reserved page #8  ", (void *)&SEQ_UI_TODO_Init },

  // menu selection starts here
  { "Edit              ", (void *)&SEQ_UI_EDIT_Init },
  { "Mute Tracks       ", (void *)&SEQ_UI_MUTE_Init },
  { "Mute Ports        ", (void *)&SEQ_UI_PMUTE_Init },
  { "Patterns          ", (void *)&SEQ_UI_PATTERN_Init },
  { "Song              ", (void *)&SEQ_UI_SONG_Init },
  { "Mixer             ", (void *)&SEQ_UI_MIXER_Init },
  { "Track Events      ", (void *)&SEQ_UI_TRKEVNT_Init },
  { "Track Mode        ", (void *)&SEQ_UI_TRKMODE_Init },
  { "Track Direction   ", (void *)&SEQ_UI_TRKDIR_Init },
  { "Track ClockDivider", (void *)&SEQ_UI_TRKDIV_Init },
  { "Track Length      ", (void *)&SEQ_UI_TRKLEN_Init },
  { "Track Transpose   ", (void *)&SEQ_UI_TRKTRAN_Init },
  { "Track Groove      ", (void *)&SEQ_UI_TRKGRV_Init },
  { "Track Triggers    ", (void *)&SEQ_UI_TRGASG_Init },
  { "Track Morphing    ", (void *)&SEQ_UI_TRKMORPH_Init },
  { "Random Generator  ", (void *)&SEQ_UI_TRKRND_Init },
  { "Record            ", (void *)&SEQ_UI_TRKREC_Init },
  { "Manual Trigger    ", (void *)&SEQ_UI_MANUAL_Init },
  { "Track Fx: Echo    ", (void *)&SEQ_UI_FX_ECHO_Init },
  { "Track Fx: Humanize", (void *)&SEQ_UI_FX_HUMANIZE_Init },
  { "Track Fx: Limit   ", (void *)&SEQ_UI_FX_LIMIT_Init },
  { "Track Fx: LFO     ", (void *)&SEQ_UI_FX_LFO_Init },
  { "Global Fx: Loop   ", (void *)&SEQ_UI_FX_LOOP_Init },
  { "Global Fx: Scale  ", (void *)&SEQ_UI_FX_SCALE_Init },
  { "Utilities         ", (void *)&SEQ_UI_UTIL_Init },
  { "BPM Selection     ", (void *)&SEQ_UI_BPM_Init },
  { "Options           ", (void *)&SEQ_UI_OPT_Init },
  { "Save Pattern      ", (void *)&SEQ_UI_SAVE_Init },
  { "Metronome         ", (void *)&SEQ_UI_METRONOME_Init },
  { "MIDI Configuration", (void *)&SEQ_UI_MIDI_Init },
  { "MIDI Monitor      ", (void *)&SEQ_UI_MIDIMON_Init },
  { "SysEx             ", (void *)&SEQ_UI_SYSEX_Init },
  { "CV Configuration  ", (void *)&SEQ_UI_CV_Init },
  { "Disk (SD Card)    ", (void *)&SEQ_UI_DISK_Init },
  { "Ethernet (OSC)    ", (void *)&SEQ_UI_ETH_Init },
  { "Bookmarks         ", (void *)&SEQ_UI_BOOKMARKS_Init },
  { "About this MIDIbox", (void *)&SEQ_UI_INFO_Init },
  { "Live Play         ", (void *)&SEQ_UI_TRKLIVE_Init },
  { "Pattern Remix     ", (void *)&SEQ_UI_PATTERN_RMX_Init },
  { "Track Euclid Gen. ", (void *)&SEQ_UI_TRKEUCLID_Init },
  { "Track Fx: Duplict.", (void *)&SEQ_UI_FX_DUPL_Init }, // TK: time to add a mapping table for the menu page order
};

// following pages are directly accessible with the GP buttons when MENU button is pressed
#define UI_SHORTCUT_STR "Mix  Evnt Mode Dir. Div. Len. Trn. Grv. Trg.  Fx  Man. Mrp. BPM  Save MIDI SysEx"
static const seq_ui_page_t ui_shortcut_menu_pages[16] = {
  SEQ_UI_PAGE_MIXER,       // GP1
  SEQ_UI_PAGE_TRKEVNT,     // GP2
  SEQ_UI_PAGE_TRKMODE,     // GP3
  SEQ_UI_PAGE_TRKDIR,      // GP4
  SEQ_UI_PAGE_TRKDIV,      // GP5
  SEQ_UI_PAGE_TRKLEN,      // GP6
  SEQ_UI_PAGE_TRKTRAN,     // GP7
  SEQ_UI_PAGE_TRKGRV,      // GP8
  SEQ_UI_PAGE_TRGASG,      // GP9
  SEQ_UI_PAGE_FX,          // GP10
  SEQ_UI_PAGE_MANUAL,      // GP11
  SEQ_UI_PAGE_TRKMORPH,    // GP12
  SEQ_UI_PAGE_BPM,         // GP13
  SEQ_UI_PAGE_SAVE,        // GP14
  SEQ_UI_PAGE_MIDI,        // GP15
  SEQ_UI_PAGE_SYSEX        // GP16
};

#endif

