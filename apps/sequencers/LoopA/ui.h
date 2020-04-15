// LoopA UI data structures and code

enum LoopAPage
{
   PAGE_SONG,
   PAGE_MIDIMONITOR,
   PAGE_TEMPO,
   PAGE_MUTE,
   PAGE_NOTES,
   PAGE_LIVEFX,

   PAGE_SETUP,
   PAGE_ROUTER,
   PAGE_DISK,
   PAGE_CLIP,
   PAGE_ARPECHO,
   PAGE_TRACK
};

enum Command
{
   COMMAND_NONE,
   COMMAND_TEMPO_BPM, COMMAND_TEMPO_BPM_UP, COMMAND_TEMPO_BPM_DOWN, COMMAND_TEMPO_TOGGLE_METRONOME, COMMAND_TEMPO_MEASURE_LENGTH,
   COMMAND_NOTE_POSITION, COMMAND_NOTE_KEY, COMMAND_NOTE_VELOCITY, COMMAND_NOTE_LENGTH, COMMAND_NOTE_DELETE,
   COMMAND_SETUP_SELECT, COMMAND_SETUP_PAR1, COMMAND_SETUP_PAR2, COMMAND_SETUP_PAR3, COMMAND_SETUP_PAR4,
   COMMAND_ROUTE_SELECT, COMMAND_ROUTE_IN_PORT, COMMAND_ROUTE_IN_CHANNEL, COMMAND_ROUTE_OUT_PORT, COMMAND_ROUTE_OUT_CHANNEL,
   COMMAND_DISK_SELECT_SESSION, COMMAND_DISK_SAVE, COMMAND_DISK_LOAD, COMMAND_DISK_NEW,
   COMMAND_CLIP_LEN, COMMAND_CLIP_TRANSPOSE, COMMAND_CLIP_SCROLL, COMMAND_CLIP_STRETCH, COMMAND_CLIP_FREEZE,
   COMMAND_ARPECHO_MODE, COMMAND_ARPECHO_PATTERN, COMMAND_ARPECHO_DELAY, COMMAND_ARPECHO_REPEATS, COMMAND_ARPECHO_NOTELENGTH,
   COMMAND_TRACK_OUTPORT, COMMAND_TRACK_OUTCHANNEL, COMMAND_TRACK_INPORT, COMMAND_TRACK_INCHANNEL, COMMAND_TRACK_TOGGLE_FORWARD, COMMAND_TRACK_LIVE_TRANSPOSE,
   COMMAND_LIVEFX_QUANTIZE, COMMAND_LIVEFX_SWING, COMMAND_LIVEFX_PROBABILITY, COMMAND_LIVEFX_FTS_MODE, COMMAND_LIVEFX_FTS_NOTE
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
   KEYICON_SONG_INVERTED,
   KEYICON_SONG,
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
   KEYICON_MIDIMONITOR,
   KEYICON_TRACKMUTE_INVERTED,
   KEYICON_TRACKMUTE,
   KEYICON_TRACKUNMUTE_INVERTED,
   KEYICON_TRACKUNMUTE,
   KEYICON_LIVEFX_INVERTED,
   KEYICON_LIVEFX
};

// --- Globals ---
extern u8 routerActiveRoute_;
extern u8 setupActiveItem_;
extern u8 scrubModeActive_;
extern s8 showShiftAbout_;
extern s8 showShiftHelp_;
extern u32 trackswitchKeydownTime_; // timestamp of last "mute" key down if trackswitching via mute keys is enabled. 0 if no mute key is held.
extern s8 trackswitchKeydownTrack_; // if trackswitching via mute keys is enabled, track that will be switched to if held long enough. 0 if no key is held.


// --- UI State Changes ---

// Set a different active track
void setActiveTrack(u8 trackNumber);

// Switch to previous active track
void switchToPreviousTrack();

// Switch to next active track
void switchToNextTrack();

// Set a new active scene number
void setActiveScene(u8 sceneNumber);

// Jump to previous Scene
void jumpToPreviousScene();

// Jump to next scene
void jumpToNextScene();

// Set a new active page
void setActivePage(enum LoopAPage page);

// Return 1, if user currently holds a track mute/unmute gp key
u8 isShiftTrackMuteToggleKeyPressed(u8 track);

// --- LED Handling ---

void updateSwitchLED(u8 number, u8 newState);

// Update the LED states of the general purpose switches (called every 20ms from app.c timer)
void updateSwitchLEDs();

// Update beat LEDs and clip positions
void updateBeatLEDsAndClipPositions(u32 bpm_tick);

// Update live LEDs (upper right encoder section)
void updateLiveLEDs();

// --- Exported Commands (triggered externally, e.g. via footswitch) ---

// Clear active clip
void clipClear();

// Check mute key track switching (called from app 1ms low priority scheduluer)
void checkMuteKeyTrackswitch();

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
