// MBLoopa Core Logic
// (c) Hawkeye 2015, 2017

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#define TRACKS 6
#define SCENES 6

// Current "menu" position in app

// --- Consts ---
#define MAXNOTES 256 // per clip, * TRACKS * SCENES for total note storage

#define PAGES 6
enum LoopaPage
{
   PAGE_TRACK,
   PAGE_EDIT,
   PAGE_NOTES,
   PAGE_MIDI,
   PAGE_DISK,
   PAGE_BPM
};

enum Command
{
   COMMAND_NONE,
   COMMAND_CLIPLEN, COMMAND_QUANTIZE, COMMAND_TRANSPOSE, COMMAND_SCROLL, COMMAND_STRETCH, COMMAND_CLEAR,  // PAGE_EDIT
   COMMAND_POSITION, COMMAND_NOTE, COMMAND_VELOCITY, COMMAND_LENGTH, COMMAND_DELETENOTE, // PAGE_NOTES
   COMMAND_PORT, COMMAND_CHANNEL,                                         // PAGE_MIDI
   COMMAND_SAVE, COMMAND_LOAD, COMMAND_NEW,                               // PAGE_DISK
   COMMAND_BPM, COMMAND_BPMFLASH                                          // PAGE_BPM
};

// --- Data structures ---

typedef struct
{
   u16 tick;
   u16 length;
   u8 note;
   u8 velocity;
} NoteData;


// --- Export global variables ---

extern u32 tick_;                // global seq tick
extern u16 bpm_;                 // bpm
extern u16 sessionNumber_;       // currently active session number (directories will be auto-created)
extern u8 sessionExistsOnDisk_;  // is the currently selected session number already saved on disk/sd card
extern enum LoopaPage page_;     // currently active page/view
extern enum Command command_;    // currently active command

extern u8 activeTrack_;          // currently active track number (0-5)
extern u8 activeScene_;          // currently active scene number (0-15)
extern u16 activeNote_;          // currently active edited note number, when in noteroll editor
extern u16 beatLoopSteps_;       // number of steps for one beatloop (adjustable)
extern u8 isRecording_;          // set, if currently recording to the selected clip
extern u8 oledBeatFlashState_;   // 0: don't flash, 1: flash slightly (normal 1/4th note), 2: flash intensively (after four 1/4th notes or 16 steps)
extern u16 seqPlayEnabledPorts_;
extern u16 seqRecEnabledPorts_;

extern u8 trackMute_[TRACKS];    // mute state of each track
extern u8 trackMidiPort_[TRACKS];
extern u8 trackMidiChannel_[TRACKS];

extern u16 clipSteps_[TRACKS][SCENES];     // number of steps for each clip
extern u32 clipQuantize_[TRACKS][SCENES];  // brings all clip notes close to the specified timing, e.g. qunatize = 4 - close to 4th notes, 16th = quantize to step, ...
extern s8 clipTranspose_[TRACKS][SCENES];
extern s16 clipScroll_[TRACKS][SCENES];
extern u8 clipStretch_[TRACKS][SCENES];    // 1: compress to 1/16th, 2: compress to 1/8th ... 16: no stretch, 32: expand 2x, 64: expand 4x, 128: expand 8x

NoteData clipNotes_[TRACKS][SCENES][MAXNOTES]; // clip note data storage of active scene (chained list, note start time and length/velocity)
extern u16 clipNotesSize_[TRACKS][SCENES];     // Active number of notes in use for that clip

// --- Secondary clip data (not on disk) ---
extern u8 trackMuteToggleRequested_[TRACKS]; // 1: perform a mute/unmute toggle of the clip at the next beatstep (synced mute/unmute)
extern u8 sceneChangeRequested_;             // If != activeScene_, this will be the scene we are changing to
extern u16 clipActiveNote_[TRACKS][SCENES];  // currently active edited note number, when in noteroll editor

// Help function: convert tick number to step number
u16 tickToStep(u32 tick);

// Help function: convert step number to tick number
u32 stepToTick(u16 step);

// Bound tick to clip steps
u32 boundTickToClipSteps(u32 tick, u8 clip);

// Quantize a tick time event
u32 quantize(u32 tick, u32 quantizeMeasure, u32 clipLengthInTicks);

// Transform (stretch and scroll) and then quantize a note in a clip
s32 quantizeTransform(u8 clip, u16 noteNumber);

// Get the clip length in ticks
u32 getClipLengthInTicks(u8 clip);

// Update the six general purpose LED states
void updateGPLeds();

// First callback from app - render Loopa Startup logo on screen
void loopaStartup();

// Initialize Loopa SEQ
s32 seqInit();

// This main sequencer handler is called periodically to poll the clock/current tick from BPM generator
s32 seqHandler(void);

// SD Card Available, initialize
void loopaSDCardAvailable();

// A buttonpress has occured
void loopaButtonPressed(s32 pin);

// An encoder has been turned
void loopaEncoderTurned(s32 encoder, s32 incrementer);

// Record midi event
void loopaRecord(mios32_midi_port_t port, mios32_midi_package_t midi_package);





