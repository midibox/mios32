// LoopA UI data structures and code

enum LoopAPage
{
   PAGE_MUTE,
   PAGE_CLIP,
   PAGE_NOTES,
   PAGE_TRACK,
   PAGE_DISK,
   PAGE_TEMPO,
   PAGE_ARPECHO,
   PAGE_ROUTER,
   PAGE_MIDIMONITOR,
   PAGE_SETUP
};

enum Command
{
   COMMAND_NONE,
   COMMAND_CLIPLEN, COMMAND_QUANTIZE, COMMAND_TRANSPOSE, COMMAND_SCROLL, COMMAND_STRETCH, COMMAND_FREEZE, // PAGE_CLIP
   COMMAND_POSITION, COMMAND_NOTE_KEY, COMMAND_NOTE_VELOCITY, COMMAND_NOTE_LENGTH, COMMAND_NOTE_DELETE,   // PAGE_NOTES
   COMMAND_TRACK_OUTPORT, COMMAND_TRACK_OUTCHANNEL, COMMAND_TRACK_INPORT, COMMAND_TRACK_INCHANNEL, COMMAND_TRACK_TOGGLE_FORWARD, COMMAND_TRACK_LIVE_TRANSPOSE, // PAGE_TRACK
   COMMAND_DISK_SELECT_SESSION, COMMAND_DISK_SAVE, COMMAND_DISK_LOAD, COMMAND_DISK_NEW,                   // PAGE_DISK
   COMMAND_TEMPO_BPM, COMMAND_TEMPO_BPM_UP, COMMAND_TEMPO_BPM_DOWN, COMMAND_TEMPO_TOGGLE_METRONOME,       // PAGE_TEMPO
   COMMAND_ROUTE_SELECT, COMMAND_ROUTE_IN_PORT, COMMAND_ROUTE_IN_CHANNEL, COMMAND_ROUTE_OUT_PORT, COMMAND_ROUTE_OUT_CHANNEL, // PAGE_ROUTER
   COMMAND_SETUP_SELECT, COMMAND_SETUP_PAR1, COMMAND_SETUP_PAR2, COMMAND_SETUP_PAR3, COMMAND_SETUP_PAR4   // PAGE_SETUP
};

enum KeyIcon
{
   KEYICON_MENU_INVERTED,
   KEYICON_MENU,
   KEYICON_MUTE_INVERTED,
   KEYICON_MUTE,
   KEYICON_CLIP_INVERTED,
   KEYICON_CLIP,
   KEYICON_ARPECHO_INVERTED,
   KEYICON_ARPECHO,
   KEYICON_TRACK_INVERTED,
   KEYICON_TRACK,
   KEYICON_NOTES_INVERTED,
   KEYICON_NOTES,
   KEYICON_DISK_INVERTED,
   KEYICON_DISK,
   KEYICON_METRONOME_INVERTED,
   KEYICON_METRONOME,
   KEYICON_TEMPO_INVERTED,
   KEYICON_TEMPO,
   KEYICON_ROUTER_INVERTED,
   KEYICON_ROUTER,
   KEYICON_SETUP_INVERTED,
   KEYICON_SETUP,
   KEYICON_SHIFT_INVERTED,
   KEYICON_SHIFT,
   KEYICON_HELP_INVERTED,
   KEYICON_HELP,
   KEYICON_ABOUT_INVERTED,
   KEYICON_ABOUT,
   KEYICON_DISPLAY_INVERTED,
   KEYICON_DISPLAY,
   KEYICON_MIDIMONITOR_INVERTED,
   KEYICON_MIDIMONITOR
};

// --- Globals ---
extern u8 routerActiveRoute_;
extern u8 setupActiveItem_;
extern u8 scrubModeActive_;

// --- UI State Changes ---

// Set a different active track
void setActiveTrack(u8 trackNumber);

// Set a new active scene number
void setActiveScene(u8 sceneNumber);

// Set a new active page
void setActivePage(enum LoopAPage page);

// --- LED Handling ---

void updateLED(u8 number, u8 newState);

// Update the LED states (called every 20ms from app.c timer)
void updateLEDs();

// --- UI human interaction ---

// A buttonpress has occured
void loopaButtonPressed(s32 pin);

// A button release has occured
void loopaButtonReleased(s32 pin);

// An encoder has been turned
void loopaEncoderTurned(s32 encoder, s32 incrementer);

// Control the play/stop button function
s32 seqPlayStopButton(void);

// Control the arm button function
s32 seqArmButton(void);

// Fast forward is put on depressed LL encoder as "scrub"
s32 seqFFwdButton(void);

// Rewind is put on depressed LL encoder as "scrub"
s32 seqFRewButton(void);