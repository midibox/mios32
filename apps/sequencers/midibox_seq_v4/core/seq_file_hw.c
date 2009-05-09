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

#include <dosfs.h>
#include <string.h>
#include <aout.h>

#include "seq_file.h"
#include "seq_file_hw.h"

#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBSEQ files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH ""
//#define SEQ_FILES_PATH "MySongs/"


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
  FILEINFO fi;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_HW.V4", SEQ_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_HW] Open config file '%s'\n", filepath);
#endif

  if( (status=SEQ_FILE_ReadOpen(&fi, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // change to file header
  if( (status=SEQ_FILE_Seek(&fi, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] failed to change offset in file, status: %d\n", status);
#endif
    return SEQ_FILE_HW_ERR_READ;
  }

  // read config values
  char line_buffer[80];
  do {
    status=SEQ_FILE_ReadLine(&fi, (u8 *)line_buffer, 80);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_HW] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t";
      char *brkt;
      char *parameter;
      int hlp;

      if( parameter = strtok_r(line_buffer, separators, &brkt) ) {

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
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value 's'!", parameter, word);
#endif
	    continue;
	  }

	  u8 din_value = ((sr-1)<<3) | pin;

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcmp(parameter, "DOWN") == 0 ) {
	    seq_hwcfg_button.down = din_value;
	  } else if( strcmp(parameter, "UP") == 0 ) {
	    seq_hwcfg_button.up = din_value;
	  } else if( strcmp(parameter, "LEFT") == 0 ) {
	    seq_hwcfg_button.left = din_value;
	  } else if( strcmp(parameter, "RIGHT") == 0 ) {
	    seq_hwcfg_button.right = din_value;
	  } else if( strcmp(parameter, "SCRUB") == 0 ) {
	    seq_hwcfg_button.scrub = din_value;
	  } else if( strcmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_button.metronome = din_value;
	  } else if( strcmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_button.stop = din_value;
	  } else if( strcmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_button.pause = din_value;
	  } else if( strcmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_button.play = din_value;
	  } else if( strcmp(parameter, "REW") == 0 ) {
	    seq_hwcfg_button.rew = din_value;
	  } else if( strcmp(parameter, "FWD") == 0 ) {
	    seq_hwcfg_button.fwd = din_value;
	  } else if( strcmp(parameter, "F1") == 0 ) {
	    seq_hwcfg_button.f1 = din_value;
	  } else if( strcmp(parameter, "F2") == 0 ) {
	    seq_hwcfg_button.f2 = din_value;
	  } else if( strcmp(parameter, "F3") == 0 ) {
	    seq_hwcfg_button.f3 = din_value;
	  } else if( strcmp(parameter, "F4") == 0 ) {
	    seq_hwcfg_button.f4 = din_value;
	  } else if( strcmp(parameter, "MENU") == 0 ) {
	    seq_hwcfg_button.menu = din_value;
	  } else if( strcmp(parameter, "SELECT") == 0 ) {
	    seq_hwcfg_button.select = din_value;
	  } else if( strcmp(parameter, "EXIT") == 0 ) {
	    seq_hwcfg_button.exit = din_value;
	  } else if( strncmp(parameter, "TRACK", 5) == 0 && // TRACK[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_button.track[hlp] = din_value;
	  } else if( strncmp(parameter, "PAR_LAYER_", 10) == 0 && // PAR_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_button.par_layer[hlp] = din_value;
	  } else if( strcmp(parameter, "EDIT") == 0 ) {
	    seq_hwcfg_button.edit = din_value;
	  } else if( strcmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_button.mute = din_value;
	  } else if( strcmp(parameter, "PATTERN") == 0 ) {
	    seq_hwcfg_button.pattern = din_value;
	  } else if( strcmp(parameter, "SONG") == 0 ) {
	    seq_hwcfg_button.song = din_value;
	  } else if( strcmp(parameter, "SOLO") == 0 ) {
	    seq_hwcfg_button.solo = din_value;
	  } else if( strcmp(parameter, "FAST") == 0 ) {
	    seq_hwcfg_button.fast = din_value;
	  } else if( strcmp(parameter, "ALL") == 0 ) {
	    seq_hwcfg_button.all = din_value;
	  } else if( strncmp(parameter, "GP", 2) == 0 && // GP%d
		     (hlp=atoi(parameter+2)) >= 1 && hlp <= 16 ) {
	    seq_hwcfg_button.gp[hlp-1] = din_value;
	  } else if( strncmp(parameter, "GROUP", 5) == 0 && // GROUP[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_button.group[hlp] = din_value;
	  } else if( strncmp(parameter, "TRG_LAYER_", 10) == 0 && // TRG_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_button.trg_layer[hlp] = din_value;
	  } else if( strcmp(parameter, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_button.step_view = din_value;
	  } else if( strcmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_button.tap_tempo = din_value;
	  } else if( strcmp(parameter, "UTILITY") == 0 ) {
	    seq_hwcfg_button.utility = din_value;
	  } else if( strcmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_button.copy = din_value;
	  } else if( strcmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_button.paste = din_value;
	  } else if( strcmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_button.clear = din_value;
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
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid pin value 's'!", parameter, word);
#endif
	    continue;
	  }

	  u8 dout_value = ((sr-1)<<3) | pin;
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] LED %s: SR %d Pin %d (DOUT: 0x%02x)", parameter, sr, pin, dout_value);
#endif

	  if( strncmp(parameter, "TRACK", 5) == 0 && // TRACK[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_led.track[hlp] = dout_value;
	  } else if( strncmp(parameter, "PAR_LAYER_", 10) == 0 && // PAR_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_led.par_layer[hlp] = dout_value;
	  } else if( strcmp(parameter, "BEAT") == 0 ) {
	    seq_hwcfg_led.beat = dout_value;
	  } else if( strcmp(parameter, "EDIT") == 0 ) {
	    seq_hwcfg_led.edit = dout_value;
	  } else if( strcmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_led.mute = dout_value;
	  } else if( strcmp(parameter, "PATTERN") == 0 ) {
	    seq_hwcfg_led.pattern = dout_value;
	  } else if( strcmp(parameter, "SONG") == 0 ) {
	    seq_hwcfg_led.song = dout_value;
	  } else if( strcmp(parameter, "SOLO") == 0 ) {
	    seq_hwcfg_led.solo = dout_value;
	  } else if( strcmp(parameter, "FAST") == 0 ) {
	    seq_hwcfg_led.fast = dout_value;
	  } else if( strcmp(parameter, "ALL") == 0 ) {
	    seq_hwcfg_led.all = dout_value;
	  } else if( strncmp(parameter, "GROUP", 5) == 0 && // GROUP[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_led.group[hlp] = dout_value;
	  } else if( strncmp(parameter, "TRG_LAYER_", 10) == 0 && // TRG_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_led.trg_layer[hlp] = dout_value;
	  } else if( strcmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_led.play = dout_value;
	  } else if( strcmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_led.stop = dout_value;
	  } else if( strcmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_led.pause = dout_value;
	  } else if( strcmp(parameter, "REW") == 0 ) {
	    seq_hwcfg_led.rew = dout_value;
	  } else if( strcmp(parameter, "FWD") == 0 ) {
	    seq_hwcfg_led.fwd = dout_value;
	  } else if( strcmp(parameter, "MENU") == 0 ) {
	    seq_hwcfg_led.menu = dout_value;
	  } else if( strcmp(parameter, "SCRUB") == 0 ) {
	    seq_hwcfg_led.scrub = dout_value;
	  } else if( strcmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_led.metronome = dout_value;
	  } else if( strcmp(parameter, "UTILITY") == 0 ) {
	    seq_hwcfg_led.utility = dout_value;
	  } else if( strcmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_led.copy = dout_value;
	  } else if( strcmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_led.paste = dout_value;
	  } else if( strcmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_led.clear = dout_value;
	  } else if( strcmp(parameter, "F1") == 0 ) {
	    seq_hwcfg_led.f1 = dout_value;
	  } else if( strcmp(parameter, "F2") == 0 ) {
	    seq_hwcfg_led.f2 = dout_value;
	  } else if( strcmp(parameter, "F3") == 0 ) {
	    seq_hwcfg_led.f3 = dout_value;
	  } else if( strcmp(parameter, "F4") == 0 ) {
	    seq_hwcfg_led.f4 = dout_value;
	  } else if( strcmp(parameter, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_led.step_view = dout_value;
	  } else if( strcmp(parameter, "DOWN") == 0 ) {
	    seq_hwcfg_led.down = dout_value;
	  } else if( strcmp(parameter, "UP") == 0 ) {
	    seq_hwcfg_led.up = dout_value;
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
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) { // should we check for odd pin values (1/3/5/7) as well?
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid pin value 's'!", parameter, word);
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

	  if( strcmp(parameter, "DATAWHEEL") == 0 ) {
	    MIOS32_ENC_ConfigSet(0, enc_config);
	  } else if( strncmp(parameter, "GP", 2) == 0 ) {
	    parameter += 2;

	    int gp = atoi(parameter);
	    if( gp < 1 || gp > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR: invalid ENC_GP number 'ENC_GP%s'!", parameter);
#endif
	    } else {
	      MIOS32_ENC_ConfigSet(gp, enc_config);
	    }

	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown ENC name 'ENC_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// GP_DOUT_SR_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "GP_DOUT_SR_", 11) == 0 ) {
	  parameter += 11;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in GP_DOUT_SR_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] GP_DOUT_SR_%s: SR %d", parameter, sr);
#endif

	  if( strcmp(parameter, "L") == 0 ) {
	    seq_hwcfg_led.gp_dout_sr_l = sr;
	  } else if( strcmp(parameter, "R") == 0 ) {
	    seq_hwcfg_led.gp_dout_sr_r = sr;
	  } else if( strcmp(parameter, "L2") == 0 ) {
	    seq_hwcfg_led.gp_dout_sr_l2 = sr;
	  } else if( strcmp(parameter, "R2") == 0 ) {
	    seq_hwcfg_led.gp_dout_sr_r2 = sr;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown GP_DOUT_SR_ name '%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// SRM_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "SRM_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in SRM_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] SRM_%s: %d", parameter, value);
#endif

	  if( strcmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_srm.enabled = value;
	  } else if( strcmp(parameter, "DOUT_L1") == 0 ) {
	    seq_hwcfg_srm.dout_l1 = value;
	  } else if( strcmp(parameter, "DOUT_R1") == 0 ) {
	    seq_hwcfg_srm.dout_r1 = value;
	  } else if( strcmp(parameter, "DOUT_M") == 0 ) {
	    seq_hwcfg_srm.dout_m = value;
	  } else if( strcmp(parameter, "DOUT_CATHODES1") == 0 ) {
	    seq_hwcfg_srm.dout_cathodes1 = value;
	  } else if( strcmp(parameter, "DOUT_CATHODES2") == 0 ) {
	    seq_hwcfg_srm.dout_cathodes2 = value;
	  } else if( strcmp(parameter, "DOUT_CATHODESM") == 0 ) {
	    seq_hwcfg_srm.dout_cathodesm = value;
	  } else if( strcmp(parameter, "DOUT_M_MAPPING") == 0 ) {
	    seq_hwcfg_srm.dout_m_mapping = value;
	  } else if( strcmp(parameter, "CATHODES_INV_MASK") == 0 ) {
	    seq_hwcfg_srm.cathodes_inv_mask = value;
	  } else if( strcmp(parameter, "CATHODES_INV_MASK_M") == 0 ) {
	    seq_hwcfg_srm.cathodes_inv_mask_m = value;
	  } else if( strcmp(parameter, "DOUT_DUOCOLOUR") == 0 ) {
	    seq_hwcfg_srm.dout_duocolour = value;
	  } else if( strcmp(parameter, "DOUT_L2") == 0 ) {
	    seq_hwcfg_srm.dout_l2 = value;
	  } else if( strcmp(parameter, "DOUT_R2") == 0 ) {
	    seq_hwcfg_srm.dout_r2 = value;
	  } else if( strcmp(parameter, "BUTTONS_ENABLED") == 0 ) {
	    seq_hwcfg_srm.buttons_enabled = value;
	  } else if( strcmp(parameter, "BUTTONS_NO_UI") == 0 ) {
	    seq_hwcfg_srm.buttons_no_ui = value;
	  } else if( strcmp(parameter, "DIN_L") == 0 ) {
	    seq_hwcfg_srm.din_l = value;
	  } else if( strcmp(parameter, "DIN_R") == 0 ) {
	    seq_hwcfg_srm.din_r = value;
	  } else if( strcmp(parameter, "DIN_M") == 0 ) {
	    seq_hwcfg_srm.din_m = value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown SRM_* name '%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// misc
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "AOUT_INTERFACE_TYPE") == 0 ) {
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

	} else if( strncmp(parameter, "DOUT_GATE_SR", 12) == 0 && // DOUT_GATE_SR%d
		     (hlp=atoi(parameter+12)) >= 1 && hlp <= SEQ_HWCFG_NUM_SR_DOUT_GATES ) {

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

	    seq_hwcfg_dout_gate_sr[hlp-1] = sr;

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

	  for(i=0; i<12; ++i)
	    MIOS32_BOARD_J5_PinInit(i, pin_mode);

	} else if( strcmp(parameter, "DOUT_1MS_TRIGGER") == 0 ) {

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
