// $Id$
/*
 * Hardware Soft-Config File access functions
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
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

#include <ff.h>
#include <string.h>
#include <aout.h>
#include <blm_cheapo.h>
#include <seq_cv.h>

#include "file.h"
#include "seq_file.h"
#include "seq_file_hw.h"

#include "seq_hwcfg.h"
#include "seq_blm8x8.h"

#include "seq_ui.h"
#include "seq_midi_port.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBSEQ files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH "/"
//#define SEQ_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid:1;
  unsigned config_locked: 1;   // file is only loaded after startup
} seq_file_hw_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_hw_info_t seq_file_hw_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_Init(u32 mode)
{
  SEQ_FILE_HW_Unload();

  seq_file_hw_info.config_locked = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Loads hardware config file
// Called from SEQ_FILE_HWheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_Load(void)
{
  s32 error = 0;

  if( seq_file_hw_info.config_locked ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] HW config file not loaded again\n");
#endif
  } else {
    error = SEQ_FILE_HW_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] Tried to open HW config file, status: %d\n", error);
#endif
    seq_file_hw_info.config_locked = 1;
  }

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from SEQ_FILE_HWheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_Unload(void)
{
  seq_file_hw_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_Valid(void)
{
  return seq_file_hw_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_ConfigLocked(void)
{
  return seq_file_hw_info.config_locked;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_LockConfig(void)
{
  seq_file_hw_info.config_locked = 1;

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a shift register number (allows M1..8 to assign SR24..31)
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_sr(char *word)
{
  if( word == NULL )
    return -1;

  u8 blm8x8_selected = 0;
  if( word[0] == 'M' ) {
    blm8x8_selected = 1;
    ++word;
  }

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  if( blm8x8_selected ) {
    l = 32 + l;
  }

  return l; // value is valid
}


///////////////////////////////////////////////////////////////////////////////
//// help function which returns the MIDI OUT port
//// returns >= 0 if value is valid
//// returns -1 if value is invalid
///////////////////////////////////////////////////////////////////////////////
//static s32 get_port_out(char *word)
//{
//  if( word == NULL )
//    return -1;
//
//  mios32_midi_port_t port = 0xff;
//  int port_ix;
//  for(port_ix=0; port_ix<SEQ_MIDI_PORT_OutNumGet(); ++port_ix) {
//    // terminate port name at first space
//    char port_name[10];
//    strcpy(port_name, SEQ_MIDI_PORT_OutNameGet(port_ix));
//    int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;
//    
//    if( strcmp(word, port_name) == 0 ) {
//      port = SEQ_MIDI_PORT_OutPortGet(port_ix);
//      break;
//    }
//  }
//
//  return (port != 0xff) ? port : get_dec(word);
//}


/////////////////////////////////////////////////////////////////////////////
// reads the hardware config file content
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_HW_Read(void)
{
  s32 status = 0;
  seq_file_hw_info_t *info = &seq_file_hw_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_HW.V4L", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_HW] Open config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 80);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_HW] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;
      int hlp;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	////////////////////////////////////////////////////////////////////////////////////////////
	// ignore comments
	////////////////////////////////////////////////////////////////////////////////////////////
	if( *parameter == '#' ) {


	////////////////////////////////////////////////////////////////////////////////////////////
	// BUTTON_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BUTTON_", 7) == 0 ) {
	  parameter += 7;

	  char *word = strtok_r(NULL, separators, &brkt);

	  // M1..M8 or M1A..M8A -> SR 32..39
	  // M1B..M8B -> SR40..47
	  // M1C..M8C -> SR48..55
	  s32 sr = -1;
	  u8 blm8x8_selected = 0;
	  if( word[0] == 'M' ) {
	    blm8x8_selected = 1;
	    ++word;
	    s32 m = word[0] - '0';
	    s32 blm = word[1] ? (word[1]-'A') : 0;

	    if( m < 1 || m > 8 || blm < 0 || blm >= SEQ_BLM8X8_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value 'M%s'!", parameter, word);
#endif
	      continue;
	    }
	    sr = 32 + blm*8 + m;
	  } else {
	    sr = get_dec(word);
	    if( sr < 0 || sr >= (32 + SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	      continue;
	    }
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u16 din_value = ((sr-1)<<3) | pin;

	  // compatibility with old configurations: if SRIO_NUM_SR hasn't been set to 23 (so that it's still 16)
	  // then map DIN SR 17..24 to 32..39
	  if( !blm8x8_selected && MIOS32_SRIO_ScanNumGet() <= 16 && din_value >= 128 && din_value < 192 ) {
	    din_value += 128;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcasecmp(parameter, "BAR1") == 0 ) {
	    seq_hwcfg_button.bar1 = din_value;
	  } else if( strcasecmp(parameter, "BAR2") == 0 ) {
	    seq_hwcfg_button.bar2 = din_value;
	  } else if( strcasecmp(parameter, "BAR3") == 0 ) {
	    seq_hwcfg_button.bar3 = din_value;
	  } else if( strcasecmp(parameter, "BAR4") == 0 ) {
	    seq_hwcfg_button.bar4 = din_value;
	  } else if( strcasecmp(parameter, "SEQ1") == 0 ) {
	    seq_hwcfg_button.seq1 = din_value;
	  } else if( strcasecmp(parameter, "SEQ2") == 0 ) {
	    seq_hwcfg_button.seq2 = din_value;
	  } else if( strcasecmp(parameter, "LOAD") == 0 ) {
	    seq_hwcfg_button.load = din_value;
	  } else if( strcasecmp(parameter, "SAVE") == 0 ) {
	    seq_hwcfg_button.save = din_value;
	  } else if( strcasecmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_button.copy = din_value;
	  } else if( strcasecmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_button.paste = din_value;
	  } else if( strcasecmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_button.clear = din_value;
	  } else if( strcasecmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_button.undo = din_value;
	  } else if( strcasecmp(parameter, "MASTER") == 0 ) {
	    seq_hwcfg_button.master = din_value;
	  } else if( strcasecmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_button.tap_tempo = din_value;
	  } else if( strcasecmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_button.stop = din_value;
	  } else if( strcasecmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_button.play = din_value;
	  } else if( strcasecmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_button.pause = din_value;
	  } else if( strcasecmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_button.metronome = din_value;
	  } else if( strcasecmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_button.ext_restart = din_value;
	  } else if( strcasecmp(parameter, "TRIGGER") == 0 ) {
	    seq_hwcfg_button.trigger = din_value;
	  } else if( strcasecmp(parameter, "LENGTH") == 0 ) {
	    seq_hwcfg_button.length = din_value;
	  } else if( strcasecmp(parameter, "PROGRESSION") == 0 ) {
	    seq_hwcfg_button.progression = din_value;
	  } else if( strcasecmp(parameter, "GROOVE") == 0 ) {
	    seq_hwcfg_button.groove = din_value;
	  } else if( strcasecmp(parameter, "ECHO") == 0 ) {
	    seq_hwcfg_button.echo = din_value;
	  } else if( strcasecmp(parameter, "HUMANIZER") == 0 ) {
	    seq_hwcfg_button.humanizer = din_value;
	  } else if( strcasecmp(parameter, "LFO") == 0 ) {
	    seq_hwcfg_button.lfo = din_value;
	  } else if( strcasecmp(parameter, "SCALE") == 0 ) {
	    seq_hwcfg_button.scale = din_value;
	  } else if( strcasecmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_button.mute = din_value;
	  } else if( strcasecmp(parameter, "MIDICHN") == 0 ) {
	    seq_hwcfg_button.midichn = din_value;
	  } else if( strcasecmp(parameter, "REC_ARM") == 0 ) {
	    seq_hwcfg_button.rec_arm = din_value;
	  } else if( strcasecmp(parameter, "REC_STEP") == 0 ) {
	    seq_hwcfg_button.rec_step = din_value;
	  } else if( strcasecmp(parameter, "REC_LIVE") == 0 ) {
	    seq_hwcfg_button.rec_live = din_value;
	  } else if( strcasecmp(parameter, "REC_POLY") == 0 ) {
	    seq_hwcfg_button.rec_poly = din_value;
	  } else if( strcasecmp(parameter, "INOUT_FWD") == 0 ) {
	    seq_hwcfg_button.inout_fwd = din_value;
	  } else if( strcasecmp(parameter, "TRANSPOSE") == 0 ) {
	    seq_hwcfg_button.transpose = din_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown button function 'BUTTON_%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// LED_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "LED_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);

	  // M1..M8 or M1A..M8A -> SR 32..39
	  // M1B..M8B -> SR40..47
	  // M1C..M8C -> SR48..55
	  s32 sr = -1;
	  u8 blm8x8_selected = 0;
	  if( word[0] == 'M' ) {
	    blm8x8_selected = 1;
	    ++word;
	    s32 m = word[0] - '0';
	    s32 blm = word[1] ? (word[1]-'A') : 0;

	    if( m < 1 || m > 8 || blm < 0 || blm >= SEQ_BLM8X8_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid SR value 'M%s'!", parameter, word);
#endif
	      continue;
	    }
	    sr = 32 + blm*8 + m;
	  } else {
	    sr = get_dec(word);
	    if( sr < 0 || sr >= (32 + SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	      continue;
	    }
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u16 dout_value = ((sr-1)<<3) | pin;

	  // compatibility with old configurations: if SRIO_NUM_SR hasn't been set to 23 (so that it's still 16)
	  // then map DOUT SR 17..24 to 32..39
	  if( !blm8x8_selected && MIOS32_SRIO_ScanNumGet() <= 16 && dout_value >= 128 && dout_value < 192 ) {
	    dout_value += 128;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] LED %s: SR %d Pin %d (DOUT: 0x%02x)", parameter, sr, pin, dout_value);
#endif

	  if( strcasecmp(parameter, "BAR1") == 0 ) {
	    seq_hwcfg_led.bar1 = dout_value;
	  } else if( strcasecmp(parameter, "BAR2") == 0 ) {
	    seq_hwcfg_led.bar2 = dout_value;
	  } else if( strcasecmp(parameter, "BAR3") == 0 ) {
	    seq_hwcfg_led.bar3 = dout_value;
	  } else if( strcasecmp(parameter, "BAR4") == 0 ) {
	    seq_hwcfg_led.bar4 = dout_value;
	  } else if( strcasecmp(parameter, "SEQ1") == 0 ) {
	    seq_hwcfg_led.seq1 = dout_value;
	  } else if( strcasecmp(parameter, "SEQ2") == 0 ) {
	    seq_hwcfg_led.seq2 = dout_value;
	  } else if( strcasecmp(parameter, "LOAD") == 0 ) {
	    seq_hwcfg_led.load = dout_value;
	  } else if( strcasecmp(parameter, "SAVE") == 0 ) {
	    seq_hwcfg_led.save = dout_value;
	  } else if( strcasecmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_led.copy = dout_value;
	  } else if( strcasecmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_led.paste = dout_value;
	  } else if( strcasecmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_led.clear = dout_value;
	  } else if( strcasecmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_led.undo = dout_value;
	  } else if( strcasecmp(parameter, "MASTER") == 0 ) {
	    seq_hwcfg_led.master = dout_value;
	  } else if( strcasecmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_led.tap_tempo = dout_value;
	  } else if( strcasecmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_led.stop = dout_value;
	  } else if( strcasecmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_led.play = dout_value;
	  } else if( strcasecmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_led.pause = dout_value;
	  } else if( strcasecmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_led.metronome = dout_value;
	  } else if( strcasecmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_led.ext_restart = dout_value;
	  } else if( strcasecmp(parameter, "TRIGGER") == 0 ) {
	    seq_hwcfg_led.trigger = dout_value;
	  } else if( strcasecmp(parameter, "LENGTH") == 0 ) {
	    seq_hwcfg_led.length = dout_value;
	  } else if( strcasecmp(parameter, "PROGRESSION") == 0 ) {
	    seq_hwcfg_led.progression = dout_value;
	  } else if( strcasecmp(parameter, "GROOVE") == 0 ) {
	    seq_hwcfg_led.groove = dout_value;
	  } else if( strcasecmp(parameter, "ECHO") == 0 ) {
	    seq_hwcfg_led.echo = dout_value;
	  } else if( strcasecmp(parameter, "HUMANIZER") == 0 ) {
	    seq_hwcfg_led.humanizer = dout_value;
	  } else if( strcasecmp(parameter, "LFO") == 0 ) {
	    seq_hwcfg_led.lfo = dout_value;
	  } else if( strcasecmp(parameter, "SCALE") == 0 ) {
	    seq_hwcfg_led.scale = dout_value;
	  } else if( strcasecmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_led.mute = dout_value;
	  } else if( strcasecmp(parameter, "MIDICHN") == 0 ) {
	    seq_hwcfg_led.midichn = dout_value;
	  } else if( strcasecmp(parameter, "REC_ARM") == 0 ) {
	    seq_hwcfg_led.rec_arm = dout_value;
	  } else if( strcasecmp(parameter, "REC_STEP") == 0 ) {
	    seq_hwcfg_led.rec_step = dout_value;
	  } else if( strcasecmp(parameter, "REC_LIVE") == 0 ) {
	    seq_hwcfg_led.rec_live = dout_value;
	  } else if( strcasecmp(parameter, "REC_POLY") == 0 ) {
	    seq_hwcfg_led.rec_poly = dout_value;
	  } else if( strcasecmp(parameter, "INOUT_FWD") == 0 ) {
	    seq_hwcfg_led.inout_fwd = dout_value;
	  } else if( strcasecmp(parameter, "TRANSPOSE") == 0 ) {
	    seq_hwcfg_led.transpose = dout_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown LED function 'LED_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// ENC_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "ENC_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid first value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  if( strcasecmp(parameter, "BPM_FAST_SPEED") == 0 ) {
	    seq_hwcfg_enc.bpm_fast_speed = sr;
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) { // should we check for odd pin values (1/3/5/7) as well?
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  mios32_enc_type_t enc_type = DISABLED;
	  if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: missing encoder type!", parameter);
#endif
	    continue;
	  } else if( strcasecmp(word, "NON_DETENTED") == 0 ) {
	    enc_type = NON_DETENTED;
	  } else if( strcasecmp(word, "DETENTED1") == 0 ) {
	    enc_type = DETENTED1;
	  } else if( strcasecmp(word, "DETENTED2") == 0 ) {
	    enc_type = DETENTED2;
	  } else if( strcasecmp(word, "DETENTED3") == 0 ) {
	    enc_type = DETENTED3;
	  } else if( strcasecmp(word, "DETENTED4") == 0 ) {
	    enc_type = DETENTED4;
	  } else if( strcasecmp(word, "DETENTED5") == 0 ) {
	    enc_type = DETENTED5;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid type '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] ENC %s: SR %d Pin %d Type %d", parameter, sr, pin, enc_type);
#endif

	  mios32_enc_config_t enc_config = { .cfg.type=enc_type, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=sr, .cfg.pos=pin };

	  if( strcasecmp(parameter, "BPM") == 0 ) {
	    MIOS32_ENC_ConfigSet(0, enc_config);
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown ENC name 'ENC_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// GP_DIN_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "GP_DIN_L_SR") == 0 || strcasecmp(parameter, "GP_DIN_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcasecmp(parameter, "GP_DIN_L_SR") == 0 ) {
	    seq_hwcfg_button.gp_din_l_sr = sr;
	  } else {
	    seq_hwcfg_button.gp_din_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// SRIO_NUM_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "SRIO_NUM_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 num_sr = get_dec(word);
	  if( num_sr < 1 || num_sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in SRIO_NUM_SR definition: invalid value '%s'!", word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] SRIO_NUM_SR %d", num_sr);
#endif

	  MIOS32_SRIO_ScanNumSet(num_sr);

	  // clear all DOUTs (for the case that the number has been decreased)
	  int i;
	  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i)
	    MIOS32_DOUT_SRSet(i, 0x00);

	////////////////////////////////////////////////////////////////////////////////////////////
	// GP_DOUT_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "GP_DOUT_L_SR") == 0 || strcasecmp(parameter, "GP_DOUT_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcasecmp(parameter, "GP_DOUT_L_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_l_sr = sr;
	  } else {
	    seq_hwcfg_led.gp_dout_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// POS_DOUT_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "POS_DOUT_L_SR") == 0 || strcasecmp(parameter, "POS_DOUT_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcasecmp(parameter, "POS_DOUT_L_SR") == 0 ) {
	    seq_hwcfg_led.pos_dout_l_sr = sr;
	  } else {
	    seq_hwcfg_led.pos_dout_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// BLM8X8_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BLM8X8_", 7) == 0 ) {
	  parameter += 7;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BLM8X8_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] BLM8X8_%s: %d", parameter, value);
#endif

	  int blm = 0; // TODO: multiple BLMs

	  if( strcasecmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_blm8x8.enabled = value;
	  } else if( strcasecmp(parameter, "DOUT_CATHODES_SR") == 0 ) {
	    seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	    config.rowsel_dout_sr = value;
	    SEQ_BLM8X8_ConfigSet(blm, config);
	  } else if( strcasecmp(parameter, "DOUT_CATHODES_INV_MASK") == 0 ) {
	    seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	    config.rowsel_inv_mask = value;
	    SEQ_BLM8X8_ConfigSet(blm, config);
	  } else if( strcasecmp(parameter, "DOUT_ANODES_INV_MASK") == 0 ) {
	    seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	    config.col_inv_mask = value;
	    SEQ_BLM8X8_ConfigSet(blm, config);
	  } else if( strcasecmp(parameter, "DOUT_LED_SR") == 0 ) {
	    seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	    config.led_dout_sr = value;
	    SEQ_BLM8X8_ConfigSet(blm, config);
	  } else if( strcasecmp(parameter, "DOUT_GP_MAPPING") == 0 ) {
	    seq_hwcfg_blm8x8.dout_gp_mapping = value;
	  } else if( strcasecmp(parameter, "DIN_SR") == 0 ) {
	    seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	    config.button_din_sr = value;
	    SEQ_BLM8X8_ConfigSet(blm, config);
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown BLM8X8_* name '%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// BPM_DIGITS_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BPM_DIGITS_", 11) == 0 ) {
	  parameter += 11;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BPM_DIGITS_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] BPM_DIGITS_%s: %d", parameter, value);
#endif

	  if( strcasecmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_bpm_digits.enabled = value;
	  } else if( strcasecmp(parameter, "SEGMENTS_SR") == 0 ) {
	    seq_hwcfg_bpm_digits.segments_sr = value;
	  } else if( strcasecmp(parameter, "COMMON1_PIN") == 0 ||
		     strcasecmp(parameter, "COMMON2_PIN") == 0 ||
		     strcasecmp(parameter, "COMMON3_PIN") == 0 ||
		     strcasecmp(parameter, "COMMON4_PIN") == 0 ) {
	    
	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in BPM_DIGITS_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	      continue;
	    }
	    u16 dout_value = ((value-1)<<3) | pin;

	    if( strcasecmp(parameter, "COMMON1_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common1_pin = dout_value;
	    } else if( strcasecmp(parameter, "COMMON2_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common2_pin = dout_value;
	    } else if( strcasecmp(parameter, "COMMON3_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common3_pin = dout_value;
	    } else if( strcasecmp(parameter, "COMMON4_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common4_pin = dout_value;
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown BPM_DIGITS_* name '%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// STEP_DIGITS_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "STEP_DIGITS_", 12) == 0 ) {
	  parameter += 12;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in STEP_DIGITS_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] STEP_DIGITS_%s: %d", parameter, value);
#endif

	  if( strcasecmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_step_digits.enabled = value;
	  } else if( strcasecmp(parameter, "SEGMENTS_SR") == 0 ) {
	    seq_hwcfg_step_digits.segments_sr = value;
	  } else if( strcasecmp(parameter, "COMMON1_PIN") == 0 ||
		     strcasecmp(parameter, "COMMON2_PIN") == 0 ||
		     strcasecmp(parameter, "COMMON3_PIN") == 0 ) {
	    
	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in STEP_DIGITS_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	      continue;
	    }
	    u16 dout_value = ((value-1)<<3) | pin;

	    if( strcasecmp(parameter, "COMMON1_PIN") == 0 ) {
	      seq_hwcfg_step_digits.common1_pin = dout_value;
	    } else if( strcasecmp(parameter, "COMMON2_PIN") == 0 ) {
	      seq_hwcfg_step_digits.common2_pin = dout_value;
	    } else if( strcasecmp(parameter, "COMMON3_PIN") == 0 ) {
	      seq_hwcfg_step_digits.common3_pin = dout_value;
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown STEP_DIGITS_* name '%s'!", parameter);
#endif
	  }
	////////////////////////////////////////////////////////////////////////////////////////////
	// TPD_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "TPD_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in TPD_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] TPD_%s: %d", parameter, value);
#endif

	  if( strcasecmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_tpd.enabled = value;
	  } else if( strcasecmp(parameter, "COLUMNS_SR") == 0 || strcasecmp(parameter, "COLUMNS_SR_L") == 0 ) {
	    seq_hwcfg_tpd.columns_sr[0] = value;
	  } else if( strcasecmp(parameter, "COLUMNS_SR_R") == 0 ) {
	    seq_hwcfg_tpd.columns_sr[1] = value;
	  } else if( strcasecmp(parameter, "ROWS_SR") == 0 || strcasecmp(parameter, "ROWS_SR_GREEN_L") == 0 ) {
	    seq_hwcfg_tpd.rows_sr_green[0] = value;
	  } else if( strcasecmp(parameter, "ROWS_SR_GREEN_R") == 0 ) {
	    seq_hwcfg_tpd.rows_sr_green[1] = value;
	  } else if( strcasecmp(parameter, "ROWS_SR_RED_L") == 0 ) {
	    seq_hwcfg_tpd.rows_sr_red[0] = value;
	  } else if( strcasecmp(parameter, "ROWS_SR_RED_R") == 0 ) {
	    seq_hwcfg_tpd.rows_sr_red[1] = value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown STEP_TPM_* name '%s'!", parameter);
#endif
	  }	  

	////////////////////////////////////////////////////////////////////////////////////////////
	// misc
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "MIDI_REMOTE_KEY") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "MIDI_REMOTE_CC") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "TRACK_CC_MODE") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "TRACK_CC_PORT") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "TRACK_CC_CHANNEL") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "TRACK_CC_NUMBER") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "RS_OPTIMISATION") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in MBSEQ_GC.V4 file

	} else if( strcasecmp(parameter, "DEBOUNCE_DELAY") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 delay = get_dec(word);
	  if( delay < 0 || delay >= 128 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid delay value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  MIOS32_SRIO_DebounceSet(delay);
	  BLM_CHEAPO_DebounceSet(delay);

	  // SEQ_BLM8X8 based DINs
	  {
	    int blm;

	    for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm) {
	      seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	      config.debounce_delay = delay;
	      SEQ_BLM8X8_ConfigSet(blm, config);
	    }
	  }

	} else if( strcasecmp(parameter, "AOUT_INTERFACE_TYPE") == 0 ) {
	  // only for compatibility reasons - AOUT interface is stored in MBSEQ_GC.V4L now!
	  // can be removed once most users switched to beta28 and later!

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 aout_type = get_dec(word);
	  if( aout_type < 0 || aout_type >= 4 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid AOUT interface type '%s'!", parameter, word);
#endif
	    continue;
	  }

	  aout_config_t config;
	  config = AOUT_ConfigGet();
	  config.if_type = aout_type;
	  config.if_option = (config.if_type == AOUT_IF_74HC595) ? 0xffffffff : 0x00000000; // AOUT_LC: select 8/8 bit configuration
	  config.num_channels = 8;
	  config.chn_inverted = 0;
	  AOUT_ConfigSet(config);
	  AOUT_IF_Init(0);

	} else if( strncasecmp(parameter, "CV_GATE_SR", 10) == 0 && // CV_GATE_SR%d
		     (hlp=atoi(parameter+10)) >= 1 && hlp <= SEQ_HWCFG_NUM_SR_CV_GATES ) {

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	    seq_hwcfg_cv_gate_sr[hlp-1] = sr;

	} else if( strcasecmp(parameter, "CLK_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	    seq_hwcfg_clk_sr = sr;

	} else if( strncasecmp(parameter, "DOUT_GATE_SR", 12) == 0 && // DOUT_GATE_SR%d
		     (hlp=atoi(parameter+12)) >= 1 && hlp <= SEQ_HWCFG_NUM_SR_DOUT_GATES ) {

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	    seq_hwcfg_dout_gate_sr[hlp-1] = sr;

	} else if( strcasecmp(parameter, "J5_ENABLED") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 j5_enabled = get_dec(word);
	  if( j5_enabled < 0 || j5_enabled > 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: expecting 0, 1 or 2!", parameter);
#endif
	    continue;
	  }

	  int i;
	  mios32_board_pin_mode_t pin_mode = MIOS32_BOARD_PIN_MODE_INPUT_PD;
	  if( j5_enabled == 1 )
	    pin_mode = MIOS32_BOARD_PIN_MODE_OUTPUT_PP;
	  if( j5_enabled == 2 )
	    pin_mode = MIOS32_BOARD_PIN_MODE_OUTPUT_OD;

#if defined(MIOS32_FAMILY_STM32F10x)
	  for(i=8; i<12; ++i) {
	    MIOS32_BOARD_J5_PinInit(i, pin_mode);
	    MIOS32_BOARD_J5_PinSet(i, 0);
	  }
#elif defined(MIOS32_FAMILY_LPC17xx)
	  for(i=0; i<4; ++i) {
	    MIOS32_BOARD_J28_PinInit(i, pin_mode);
	    MIOS32_BOARD_J28_PinSet(i, 0);
	  }
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif

	} else if( strcasecmp(parameter, "DIN_SYNC_CLK_PULSEWIDTH") == 0 ) {
	  // only for compatibility reasons - AOUT interface is stored in MBSEQ_GC.V4 now!
	  // can be removed once most users switched to beta28 and later!

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 pulsewidth = get_dec(word);
	  if( pulsewidth < 1 || pulsewidth > 250 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: expecting pulsewidth of 1..250!", parameter);
#endif
	    continue;
	  }

	  SEQ_CV_ClkPulseWidthSet(0, pulsewidth);

	} else if( strcasecmp(parameter, "DOUT_1MS_TRIGGER") == 0 ) {

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 trg_enabled = get_dec(word);
	  if( trg_enabled < 0 || trg_enabled > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: expecting 0 or 1!", parameter);
#endif
	    continue;
	  }

	  seq_hwcfg_dout_gate_1ms = trg_enabled;


	////////////////////////////////////////////////////////////////////////////////////////////
	// unknown
	////////////////////////////////////////////////////////////////////////////////////////////
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown parameter: %s", line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_HW] ERROR: no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_HW] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_HW_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}
