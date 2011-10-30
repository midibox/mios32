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
#include <blm_x.h>
#include <seq_cv.h>

#include "file.h"
#include "seq_file.h"
#include "seq_file_hw.h"

#include "seq_hwcfg.h"

#include "seq_ui.h"


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

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	////////////////////////////////////////////////////////////////////////////////////////////
	// ignore comments
	////////////////////////////////////////////////////////////////////////////////////////////
	if( *parameter == '#' ) {


	////////////////////////////////////////////////////////////////////////////////////////////
	// BUTTON_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "BUTTON_", 7) == 0 ) {
	  parameter += 7;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 32 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u8 din_value = ((sr-1)<<3) | pin;

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcmp(parameter, "BAR1") == 0 ) {
	    seq_hwcfg_button.bar1 = din_value;
	  } else if( strcmp(parameter, "BAR2") == 0 ) {
	    seq_hwcfg_button.bar2 = din_value;
	  } else if( strcmp(parameter, "BAR3") == 0 ) {
	    seq_hwcfg_button.bar3 = din_value;
	  } else if( strcmp(parameter, "BAR4") == 0 ) {
	    seq_hwcfg_button.bar4 = din_value;
	  } else if( strcmp(parameter, "SEQ1") == 0 ) {
	    seq_hwcfg_button.seq1 = din_value;
	  } else if( strcmp(parameter, "SEQ2") == 0 ) {
	    seq_hwcfg_button.seq2 = din_value;
	  } else if( strcmp(parameter, "LOAD") == 0 ) {
	    seq_hwcfg_button.load = din_value;
	  } else if( strcmp(parameter, "SAVE") == 0 ) {
	    seq_hwcfg_button.save = din_value;
	  } else if( strcmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_button.copy = din_value;
	  } else if( strcmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_button.paste = din_value;
	  } else if( strcmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_button.clear = din_value;
	  } else if( strcmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_button.undo = din_value;
	  } else if( strcmp(parameter, "MASTER") == 0 ) {
	    seq_hwcfg_button.master = din_value;
	  } else if( strcmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_button.tap_tempo = din_value;
	  } else if( strcmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_button.stop = din_value;
	  } else if( strcmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_button.play = din_value;
	  } else if( strcmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_button.pause = din_value;
	  } else if( strcmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_button.metronome = din_value;
	  } else if( strcmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_button.ext_restart = din_value;
	  } else if( strcmp(parameter, "TRIGGER") == 0 ) {
	    seq_hwcfg_button.trigger = din_value;
	  } else if( strcmp(parameter, "LENGTH") == 0 ) {
	    seq_hwcfg_button.length = din_value;
	  } else if( strcmp(parameter, "PROGRESSION") == 0 ) {
	    seq_hwcfg_button.progression = din_value;
	  } else if( strcmp(parameter, "GROOVE") == 0 ) {
	    seq_hwcfg_button.groove = din_value;
	  } else if( strcmp(parameter, "ECHO") == 0 ) {
	    seq_hwcfg_button.echo = din_value;
	  } else if( strcmp(parameter, "HUMANIZER") == 0 ) {
	    seq_hwcfg_button.humanizer = din_value;
	  } else if( strcmp(parameter, "LFO") == 0 ) {
	    seq_hwcfg_button.lfo = din_value;
	  } else if( strcmp(parameter, "SCALE") == 0 ) {
	    seq_hwcfg_button.scale = din_value;
	  } else if( strcmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_button.mute = din_value;
	  } else if( strcmp(parameter, "MIDICHN") == 0 ) {
	    seq_hwcfg_button.midichn = din_value;
	  } else if( strcmp(parameter, "REC_ARM") == 0 ) {
	    seq_hwcfg_button.rec_arm = din_value;
	  } else if( strcmp(parameter, "REC_STEP") == 0 ) {
	    seq_hwcfg_button.rec_step = din_value;
	  } else if( strcmp(parameter, "REC_LIVE") == 0 ) {
	    seq_hwcfg_button.rec_live = din_value;
	  } else if( strcmp(parameter, "REC_POLY") == 0 ) {
	    seq_hwcfg_button.rec_poly = din_value;
	  } else if( strcmp(parameter, "INOUT_FWD") == 0 ) {
	    seq_hwcfg_button.inout_fwd = din_value;
	  } else if( strcmp(parameter, "TRANSPOSE") == 0 ) {
	    seq_hwcfg_button.transpose = din_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown button function 'BUTTON_%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// LED_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "LED_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 32 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u8 dout_value = ((sr-1)<<3) | pin;
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] LED %s: SR %d Pin %d (DOUT: 0x%02x)", parameter, sr, pin, dout_value);
#endif

	  if( strcmp(parameter, "BAR1") == 0 ) {
	    seq_hwcfg_led.bar1 = dout_value;
	  } else if( strcmp(parameter, "BAR2") == 0 ) {
	    seq_hwcfg_led.bar2 = dout_value;
	  } else if( strcmp(parameter, "BAR3") == 0 ) {
	    seq_hwcfg_led.bar3 = dout_value;
	  } else if( strcmp(parameter, "BAR4") == 0 ) {
	    seq_hwcfg_led.bar4 = dout_value;
	  } else if( strcmp(parameter, "SEQ1") == 0 ) {
	    seq_hwcfg_led.seq1 = dout_value;
	  } else if( strcmp(parameter, "SEQ2") == 0 ) {
	    seq_hwcfg_led.seq2 = dout_value;
	  } else if( strcmp(parameter, "LOAD") == 0 ) {
	    seq_hwcfg_led.load = dout_value;
	  } else if( strcmp(parameter, "SAVE") == 0 ) {
	    seq_hwcfg_led.save = dout_value;
	  } else if( strcmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_led.copy = dout_value;
	  } else if( strcmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_led.paste = dout_value;
	  } else if( strcmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_led.clear = dout_value;
	  } else if( strcmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_led.undo = dout_value;
	  } else if( strcmp(parameter, "MASTER") == 0 ) {
	    seq_hwcfg_led.master = dout_value;
	  } else if( strcmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_led.tap_tempo = dout_value;
	  } else if( strcmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_led.stop = dout_value;
	  } else if( strcmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_led.play = dout_value;
	  } else if( strcmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_led.pause = dout_value;
	  } else if( strcmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_led.metronome = dout_value;
	  } else if( strcmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_led.ext_restart = dout_value;
	  } else if( strcmp(parameter, "TRIGGER") == 0 ) {
	    seq_hwcfg_led.trigger = dout_value;
	  } else if( strcmp(parameter, "LENGTH") == 0 ) {
	    seq_hwcfg_led.length = dout_value;
	  } else if( strcmp(parameter, "PROGRESSION") == 0 ) {
	    seq_hwcfg_led.progression = dout_value;
	  } else if( strcmp(parameter, "GROOVE") == 0 ) {
	    seq_hwcfg_led.groove = dout_value;
	  } else if( strcmp(parameter, "ECHO") == 0 ) {
	    seq_hwcfg_led.echo = dout_value;
	  } else if( strcmp(parameter, "HUMANIZER") == 0 ) {
	    seq_hwcfg_led.humanizer = dout_value;
	  } else if( strcmp(parameter, "LFO") == 0 ) {
	    seq_hwcfg_led.lfo = dout_value;
	  } else if( strcmp(parameter, "SCALE") == 0 ) {
	    seq_hwcfg_led.scale = dout_value;
	  } else if( strcmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_led.mute = dout_value;
	  } else if( strcmp(parameter, "MIDICHN") == 0 ) {
	    seq_hwcfg_led.midichn = dout_value;
	  } else if( strcmp(parameter, "REC_ARM") == 0 ) {
	    seq_hwcfg_led.rec_arm = dout_value;
	  } else if( strcmp(parameter, "REC_STEP") == 0 ) {
	    seq_hwcfg_led.rec_step = dout_value;
	  } else if( strcmp(parameter, "REC_LIVE") == 0 ) {
	    seq_hwcfg_led.rec_live = dout_value;
	  } else if( strcmp(parameter, "REC_POLY") == 0 ) {
	    seq_hwcfg_led.rec_poly = dout_value;
	  } else if( strcmp(parameter, "INOUT_FWD") == 0 ) {
	    seq_hwcfg_led.inout_fwd = dout_value;
	  } else if( strcmp(parameter, "TRANSPOSE") == 0 ) {
	    seq_hwcfg_led.transpose = dout_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown LED function 'LED_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// ENC_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "ENC_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid first value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  if( strcmp(parameter, "BPM_FAST_SPEED") == 0 ) {
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
	  } else if( strcmp(word, "NON_DETENTED") == 0 ) {
	    enc_type = NON_DETENTED;
	  } else if( strcmp(word, "DETENTED1") == 0 ) {
	    enc_type = DETENTED1;
	  } else if( strcmp(word, "DETENTED2") == 0 ) {
	    enc_type = DETENTED2;
	  } else if( strcmp(word, "DETENTED3") == 0 ) {
	    enc_type = DETENTED3;
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

	  if( strcmp(parameter, "BPM") == 0 ) {
	    MIOS32_ENC_ConfigSet(0, enc_config);
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown ENC name 'ENC_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// GP_DIN_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "GP_DIN_L_SR") == 0 || strcmp(parameter, "GP_DIN_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcmp(parameter, "GP_DIN_L_SR") == 0 ) {
	    seq_hwcfg_button.gp_din_l_sr = sr;
	  } else {
	    seq_hwcfg_button.gp_din_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// GP_DOUT_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "GP_DOUT_L_SR") == 0 || strcmp(parameter, "GP_DOUT_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcmp(parameter, "GP_DOUT_L_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_l_sr = sr;
	  } else {
	    seq_hwcfg_led.gp_dout_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// POS_DOUT_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "POS_DOUT_L_SR") == 0 || strcmp(parameter, "POS_DOUT_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcmp(parameter, "POS_DOUT_L_SR") == 0 ) {
	    seq_hwcfg_led.pos_dout_l_sr = sr;
	  } else {
	    seq_hwcfg_led.pos_dout_r_sr = sr;
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// BLM8X8_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "BLM8X8_", 7) == 0 ) {
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

	  if( strcmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_blm8x8.enabled = value;
	  } else if( strcmp(parameter, "DOUT_CATHODES_SR") == 0 ) {
	    blm_x_config_t config = BLM_X_ConfigGet();
	    config.rowsel_dout_sr = value;
	    BLM_X_ConfigSet(config);
	  } else if( strcmp(parameter, "DOUT_CATHODES_INV_MASK") == 0 ) {
	    blm_x_config_t config = BLM_X_ConfigGet();
	    config.rowsel_inv_mask = value;
	    BLM_X_ConfigSet(config);
	  } else if( strcmp(parameter, "DOUT_LED_SR") == 0 ) {
	    blm_x_config_t config = BLM_X_ConfigGet();
	    config.led_first_dout_sr = value;
	    BLM_X_ConfigSet(config);
	  } else if( strcmp(parameter, "DOUT_GP_MAPPING") == 0 ) {
	    seq_hwcfg_blm8x8.dout_gp_mapping = value;
	  } else if( strcmp(parameter, "DIN_SR") == 0 ) {
	    blm_x_config_t config = BLM_X_ConfigGet();
	    config.btn_first_din_sr = value;
	    BLM_X_ConfigSet(config);
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown BLM8X8_* name '%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// BPM_DIGITS_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "BPM_DIGITS_", 11) == 0 ) {
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

	  if( strcmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_bpm_digits.enabled = value;
	  } else if( strcmp(parameter, "SEGMENTS_SR") == 0 ) {
	    seq_hwcfg_bpm_digits.segments_sr = value;
	  } else if( strcmp(parameter, "COMMON1_PIN") == 0 ||
		     strcmp(parameter, "COMMON2_PIN") == 0 ||
		     strcmp(parameter, "COMMON3_PIN") == 0 ||
		     strcmp(parameter, "COMMON4_PIN") == 0 ) {
	    
	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_DIGITS_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	      continue;
	    }
	    u8 dout_value = ((value-1)<<3) | pin;

	    if( strcmp(parameter, "COMMON1_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common1_pin = dout_value;
	    } else if( strcmp(parameter, "COMMON2_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common2_pin = dout_value;
	    } else if( strcmp(parameter, "COMMON3_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common3_pin = dout_value;
	    } else if( strcmp(parameter, "COMMON4_PIN") == 0 ) {
	      seq_hwcfg_bpm_digits.common4_pin = dout_value;
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown LED_DIGITS_* name '%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// STEP_DIGITS_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "STEP_DIGITS_", 12) == 0 ) {
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

	  if( strcmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_step_digits.enabled = value;
	  } else if( strcmp(parameter, "SEGMENTS_SR") == 0 ) {
	    seq_hwcfg_step_digits.segments_sr = value;
	  } else if( strcmp(parameter, "COMMON1_PIN") == 0 ||
		     strcmp(parameter, "COMMON2_PIN") == 0 ||
		     strcmp(parameter, "COMMON3_PIN") == 0 ) {
	    
	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in STEP_DIGITS_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	      continue;
	    }
	    u8 dout_value = ((value-1)<<3) | pin;

	    if( strcmp(parameter, "COMMON1_PIN") == 0 ) {
	      seq_hwcfg_step_digits.common1_pin = dout_value;
	    } else if( strcmp(parameter, "COMMON2_PIN") == 0 ) {
	      seq_hwcfg_step_digits.common2_pin = dout_value;
	    } else if( strcmp(parameter, "COMMON3_PIN") == 0 ) {
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
	} else if( strncmp(parameter, "TPD_", 4) == 0 ) {
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

	  if( strcmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_tpd.enabled = value;
	  } else if( strcmp(parameter, "COLUMNS_SR") == 0 ) {
	    seq_hwcfg_tpd.columns_sr = value;
	  } else if( strcmp(parameter, "ROWS_SR") == 0 ) {
	    seq_hwcfg_tpd.rows_sr = value;	    
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown STEP_TPM_* name '%s'!", parameter);
#endif
	  }	  

	////////////////////////////////////////////////////////////////////////////////////////////
	// misc
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "MIDI_REMOTE_KEY") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 key = get_dec(word);
	  if( key < 0 || key >= 128 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid remote key '%s'!", parameter, word);
#endif
	    continue;
	  }

	  seq_hwcfg_midi_remote.key = key;

	} else if( strcmp(parameter, "MIDI_REMOTE_CC") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 cc = get_dec(word);
	  if( cc < 0 || cc >= 128 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid remote CC '%s'!", parameter, word);
#endif
	    continue;
	  }

	  seq_hwcfg_midi_remote.cc = cc;

	} else if( strcmp(parameter, "RS_OPTIMISATION") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 port = get_dec(word);

	  if( port < 0 || port >= 0x100 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid port number '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 enable = get_dec(word);
	  if( enable != 0 && enable != 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid enable flag '%s', expecting 1 or 0!", parameter, word);
#endif
	    continue;
	  }

	  s32 status;
	  if( (status=MIOS32_MIDI_RS_OptimisationSet(port, enable)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    if( port != 0x23 ) // this port is only available for LPC17, not for STM32
	      DEBUG_MSG("[SEQ_FILE_HW] RS_OPTIMISATION 0x%02x %d failed with status %d!", port, enable, status);
#endif
	  }

	} else if( strcmp(parameter, "DEBOUNCE_DELAY") == 0 ) {
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

	} else if( strcmp(parameter, "AOUT_INTERFACE_TYPE") == 0 ) {
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

	} else if( strcmp(parameter, "J5_ENABLED") == 0 ) {
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

	} else if( strcmp(parameter, "DIN_SYNC_CLK_PULSEWIDTH") == 0 ) {
	  // only for compatibility reasons - AOUT interface is stored in MBSEQ_GC.V4L now!
	  // can be removed once most users switched to beta28 and later!

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 pulsewidth = get_dec(word);
	  if( pulsewidth < 1 || pulsewidth > 250 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: expecting pulsewidth of 1..250!", parameter);
#endif
	    continue;
	  }

	  SEQ_CV_ClkPulseWidthSet(pulsewidth);


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
