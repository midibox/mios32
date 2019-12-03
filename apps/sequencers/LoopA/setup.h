// LoopA Setup/Config routines

#include "commonIncludes.h"

// --- constants ---
#define CONFIG_FILE_PATH "/setup.txt"

// --- globals ---
extern u8 configChangesToBeWritten_;
extern char line_buffer_[128];  // single global line buffer for reading/writing from/to files

// --- Global config variables ---
extern s16 gcLastUsedSessionNumber_;
extern s8 gcFontType_;
extern s8 gcInvertOLED_;
extern s8 gcBeatLEDsEnabled_;
extern s8 gcBeatDisplayEnabled_;
extern s8 gcNumberOfActiveUserInstruments_;

extern mios32_midi_port_t gcMetronomePort_;
extern u8 gcMetronomeChannel_;
extern u8 gcMetronomeNoteM_;
extern u8 gcMetronomeNoteB_;
extern u8 gcScreensaverAfterMinutes_;

// --- Global config settings ---
typedef struct
{
   char name[16];
   char par1Name[8];
   char par2Name[8];
   char par3Name[8];
   char par4Name[8];
} SetupParameter;

#define SETUP_NUM_ITEMS 10

enum SetupParameterEnum
{
   SETUP_FONT_TYPE,
   SETUP_BEAT_LEDS_ENABLED,
   SETUP_BEAT_DISPLAY_ENABLED,
   // SETUP_COMMAND_HELP_ENABLED,
   SETUP_SCREENSAVER_MINUTES,
   SETUP_INVERT_OLED,
   SETUP_METRONOME,
   // SETUP_TEMPO_UP_DOWN_BPM_SEC,

   SETUP_MCLK_DIN_IN,
   SETUP_MCLK_DIN_OUT,
   SETUP_MCLK_USB_IN,
   SETUP_MCLK_USB_OUT,

   /* SETUP_DEFAULT_TRACK_1_PORT_CHN_LEN_FWD,
   SETUP_DEFAULT_TRACK_2_PORT_CHN_LEN_FWD,
   SETUP_DEFAULT_TRACK_3_PORT_CHN_LEN_FWD,
   SETUP_DEFAULT_TRACK_4_PORT_CHN_LEN_FWD,
   SETUP_DEFAULT_TRACK_5_PORT_CHN_LEN_FWD,
   SETUP_DEFAULT_TRACK_6_PORT_CHN_LEN_FWD */
};

extern SetupParameter setupParameters_[SETUP_NUM_ITEMS];

// --- User defined instruments ---
typedef struct
{
   char name[9];
   mios32_midi_port_t port;
   u8 channel;
} UserInstrument;

#define SETUP_NUM_USERINSTRUMENTS 32

extern UserInstrument userInstruments_[SETUP_NUM_USERINSTRUMENTS];



// --- functions ---
// (After a configuration change), write the global setup file to disk
extern void writeSetup();

// Read global setup from disk (usually only at startup time)
extern void readSetup();

// Setup screen: parameter button depressed
extern void setupParameterDepressed(u8 parameterNumber);

// Setup screen: parameter encoder turned
extern void setupParameterEncoderTurned(u8 parameterNumber, s32 incrementer);


// --- user instruments & MIDI ports handling ---
// Adjust internal LoopA port number by incrementer change (negative LoopA port numbers are user instruments, positve are mios ports)
extern s8 adjustLoopAPortNumber(s8 loopaPortNumber, s32 incrementer);

// Return true, if loopaPortNumber is a user defined instrument (don't print channel number then)
extern u8 isInstrument(s8 loopaPortNumber);

// Get verbal port or instrument name from loopaPortNumber
extern char* getPortOrInstrumentNameFromLoopAPortNumber(s8 loopaPortNumber);

// Get numeric mios port id from loopaPortNumber
extern mios32_midi_port_t getMIOSPortNumberFromLoopAPortNumber(s8 loopaPortNumber);

// Get channel number from loopaPortNumber
extern u8 getInstrumentChannelNumberFromLoopAPortNumber(s8 loopaPortNumber);
