// --- consts, enumerations and defines ---

#define PAGES 6 // TODO: old, drop this
enum LoopaPage
{
   PAGE_MUTE,
   PAGE_CLIP,
   PAGE_NOTES,
   PAGE_TRACK,
   PAGE_DISK,
   PAGE_TEMPO,
   PAGE_FX,
   PAGE_ROUTER,
   PAGE_MIDIMONITOR,
   PAGE_SETUP
};

enum Command
{
   COMMAND_NONE,
   COMMAND_CLIPLEN, COMMAND_QUANTIZE, COMMAND_TRANSPOSE, COMMAND_SCROLL, COMMAND_STRETCH, COMMAND_FREEZE,  // PAGE_CLIP
   COMMAND_POSITION, COMMAND_NOTE, COMMAND_VELOCITY, COMMAND_LENGTH, COMMAND_DELETENOTE, // PAGE_NOTES
   COMMAND_PORT, COMMAND_CHANNEL,                                         // PAGE_TRACK
   COMMAND_SAVE, COMMAND_LOAD, COMMAND_NEW,                               // PAGE_DISK
   COMMAND_BPM, COMMAND_BPMFLASH,                                         // PAGE_TEMPO
   COMMAND_ROUTE_SELECT, COMMAND_ROUTE_IN_PORT, COMMAND_ROUTE_IN_CHANNEL, COMMAND_ROUTE_OUT_PORT, COMMAND_ROUTE_OUT_CHANNEL // PAGE_ROUTER
};

enum KeyIcon
{
   KEYICON_MENU_INVERTED,
   KEYICON_MENU,
   KEYICON_MUTE_INVERTED,
   KEYICON_MUTE,
   KEYICON_CLIP_INVERTED,
   KEYICON_CLIP,
   KEYICON_FX_INVERTED,
   KEYICON_FX,
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

// --- UI State Changes ---

// Set a different active track
void setActiveTrack(u8 trackNumber);

// Set a new active scene number
void setActiveScene(u8 sceneNumber);

// Set a new active page
void setActivePage(enum LoopaPage page);

// --- LED Handling ---

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