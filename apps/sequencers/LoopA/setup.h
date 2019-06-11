// LoopA Setup/Config routines

#include "commonIncludes.h"

// --- constants ---
#define configFilePath "/setup.txt"

// --- globals ---
extern u8 configChangesToBeWritten_;

// --- Global config variables ---
extern u8 gcBeatLEDsEnabled_;
extern u8 gcBeatDisplayEnabled_;

// --- Global config settings ---
typedef struct
{
   char name[16];
   char par1Name[8];
   char par2Name[8];
   char par3Name[8];
   char par4Name[8];
} SetupParameter;

#define SETUP_NUM_ITEMS 17

enum SetupParameterEnum
{
   SETUP_MCLK_DIN_IN,
   SETUP_MCLK_DIN_OUT,
   SETUP_MCLK_USB_IN,
   SETUP_MCLK_USB_OUT,
   SETUP_METRONOME_PORT_CHN,
   SETUP_METRONOME_NOTES_MEASURE_BEAT,
   SETUP_BEAT_LEDS_ENABLED,
   SETUP_BEAT_DISPLAY_ENABLED,
   SETUP_TEMPO_UP_DOWN_BPM_SEC,
   SETUP_COMMAND_HELP_ENABLED,
   SETUP_SCREENSAVER_MINUTES,
   SETUP_DEFAULT_TRACK_1_PORT_CHN,
   SETUP_DEFAULT_TRACK_2_PORT_CHN,
   SETUP_DEFAULT_TRACK_3_PORT_CHN,
   SETUP_DEFAULT_TRACK_4_PORT_CHN,
   SETUP_DEFAULT_TRACK_5_PORT_CHN,
   SETUP_DEFAULT_TRACK_6_PORT_CHN
};

extern SetupParameter setupParameters_[SETUP_NUM_ITEMS];

// --- functions ---
// (After a configuration change), write the global setup file to disk
extern void writeSetup();

// Read global setup from disk (usually only at startup time)
extern void readSetup();

// Setup screen: parameter button depressed
extern void setupParameterDepressed(u8 parameterNumber);

// Setup screen: parameter encoder turned
extern void setupParameterEncoderTurned(u8 parameterNumber, s32 incrementer);
