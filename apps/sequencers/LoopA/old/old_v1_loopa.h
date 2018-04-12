// MBLoopa Core Logic
// (c) Hawkeye 2015

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// Current "menu" position in app

// --- Export global variables ---

extern u16 sessionNumber_;       // currently active session number (directories will be auto-created)
extern u8 selectedClipNumber_;   // currently active or last active clip number (1-8)
extern u8 baseView_;             // if not in baseView, we are in single clipView
extern u8 displayMode2d_;        // if not in 2d display mode, we are rendering voxelspace

extern u16 seqPlayEnabledPorts_;
extern u16 seqRecEnabledPorts_;


// First callback from app - render Loopa Startup logo on screen
void loopaStartup();

// (Re)load a session clip. It might have changed after recording If no clip was found, the local clipData will be empty
void loadClip(u8 clipNumber);


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






