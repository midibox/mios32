// LoopA Setup/Config routines

#include "commonIncludes.h"

// --- constants ---
#define CONFIG_FILE_PATH "/setup.txt"

// --- globals ---
extern u8 configChangesToBeWritten_;
extern char line_buffer_[128];  // single global line buffer for reading/writing from/to files

// --- Global config variables ---
extern s16 gcLastUsedSessionNumber_;
extern s8 gcNumberOfActiveUserInstruments_;

extern s8 gcFontType_;
extern s8 gcInvertOLED_;
extern s8 gcBeatLEDsEnabled_;
extern s8 gcBeatDisplayEnabled_;
extern u8 gcScreensaverAfterMinutes_;
extern mios32_midi_port_t gcMetronomePort_;
extern u8 gcMetronomeChannel_;
extern u8 gcMetronomeNoteM_;
extern u8 gcMetronomeNoteB_;
extern enum TempodeltaTypeEnum gcTempodeltaType_;
extern u8 gcFootswitchesInversionmask_; // binary encoded: fsw1 = rightmost bit, fsw2 = second bit, ...
extern enum FootswitchActionEnum gcFootswitch1Action_;
extern enum FootswitchActionEnum gcFootswitch2Action_;
extern s8 gcInvertMuteLEDs_;
extern enum TrackswitchTypeEnum gcTrackswitchType_;
extern enum FollowtrackTypeEnum gcFollowtrackType_;
extern s8 gcLEDNotes_;

// --- Global config settings ---
typedef struct
{
   const char* name;
   const char* par1Name;
   const char* par2Name;
   const char* par3Name;
   const char* par4Name;
} SetupParameter;

#define SETUP_NUM_ITEMS 18

enum SetupParameterEnum
{
   SETUP_FONT_TYPE,
   SETUP_BEAT_LEDS_ENABLED,
   SETUP_BEAT_DISPLAY_ENABLED,
   SETUP_SCREENSAVER_MINUTES,
   SETUP_INVERT_OLED,
   SETUP_METRONOME,
   SETUP_TEMPODELTA,
   SETUP_INVERT_FOOTSWITCHES,
   SETUP_FOOTSWITCH1_ACTION,
   SETUP_FOOTSWITCH2_ACTION,
   SETUP_INVERT_MUTE_LEDS,
   SETUP_TRACK_SWITCH_TYPE,
   SETUP_FOLLOW_TRACK_TYPE,
   SETUP_LED_NOTES,

   SETUP_MCLK_DIN_IN,
   SETUP_MCLK_DIN_OUT,
   SETUP_MCLK_USB_IN,
   SETUP_MCLK_USB_OUT,
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

// --- Tempodelta type ---

typedef struct
{
   const char* configname;
   const char* name;
   float speed;
} TempodeltaDescription;

#define SETUP_NUM_TEMPODELTATYPES 5
extern TempodeltaDescription tempodeltaDescriptions_[SETUP_NUM_TEMPODELTATYPES];

enum TempodeltaTypeEnum
{
   TEMPODELTA_SLOWER,
   TEMPODELTA_SLOW,
   TEMPODELTA_NORMAL,
   TEMPODELTA_FAST,
   TEMPODELTA_FASTER
};

// --- Footswitch actions ---

typedef struct
{
   const char* configname;
   const char* name;
} FootswitchActionDescription;

#define SETUP_NUM_FOOTSWITCHACTIONS 11
extern FootswitchActionDescription footswitchActionDescriptions_[SETUP_NUM_FOOTSWITCHACTIONS];

enum FootswitchActionEnum
{
   FOOTSWITCH_CURSORERASE,
   FOOTSWITCH_RUNSTOP,
   FOOTSWITCH_ARM,
   FOOTSWITCH_CLEARCLIP,
   FOOTSWITCH_JUMPTOSTART,
   FOOTSWITCH_JUMPTOPRECOUNT,
   FOOTSWITCH_METRONOME,
   FOOTSWITCH_PREVSCENE,
   FOOTSWITCH_NEXTSCENE,
   FOOTSWITCH_PREVTRACK,
   FOOTSWITCH_NEXTTRACK
};

// --- Track switch type ---

typedef struct
{
   const char* configname;
   const char* name;
} TrackswitchDescription;

#define SETUP_NUM_TRACKSWITCHTYPES 3
extern TrackswitchDescription trackswitchDescriptions_[SETUP_NUM_TRACKSWITCHTYPES];

enum TrackswitchTypeEnum
{
   TRACKSWITCH_NORMAL,              // Track switching only with the select encoder
   TRACKSWITCH_HOLD_MUTE_KEY_FAST,  // Track switching by pushing and holding a mute key in the mute screen (keep pressed for at least 0.2 sec and release afterwards)
   TRACKSWITCH_HOLD_MUTE_KEY,       // Track switching by pushing and holding a top-row key in the mute screen (keep pressed for at least 0.4 sec and release afterwards)
};

// --- Follow Track type ---

typedef struct
{
   const char* configname;
   const char* name;
} FollowtrackDescription;

#define SETUP_NUM_FOLLOWTRACKTYPES 3
extern FollowtrackDescription followtrackDescriptions_[SETUP_NUM_FOLLOWTRACKTYPES];

enum FollowtrackTypeEnum
{
   FOLLOWTRACK_DISABLED,              // No automatic track switching/following
   FOLLOWTRACK_ON_UNMUTE,             // When a track is unmuted, active track follows this action
   FOLLOWTRACK_ON_MUTE_AND_UNMUTE     // When a track is muted or unmuted, active track follows this action
};


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
