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
#include <string.h>

#include <aout.h>
#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_hw.h"

#include "mbcv_hwcfg.h"
#include "mbcv_map.h"
#include "mbcv_enc.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the MBMBCV files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MBCV_FILES_PATH "/"
//#define MBCV_FILES_PATH "/MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid:1;
  unsigned config_locked: 1;   // file is only loaded after startup
} mbcv_file_hw_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mbcv_file_hw_info_t mbcv_file_hw_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Init(u32 mode)
{
  MBCV_FILE_HW_Unload();

  mbcv_file_hw_info.config_locked = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Loads hardware config file
// Called from MBCV_FILE_HWheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Load(void)
{
  s32 error = 0;

  if( mbcv_file_hw_info.config_locked ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] HW config file not loaded again\n");
#endif
  } else {
    error = MBCV_FILE_HW_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] Tried to open HW config file, status: %d\n", error);
#endif
    mbcv_file_hw_info.config_locked = 1;
  }

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads config file
// Called from MBCV_FILE_HWheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Unload(void)
{
  mbcv_file_hw_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config file valid
// Returns 0 if config file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Valid(void)
{
  return mbcv_file_hw_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_ConfigLocked(void)
{
  return mbcv_file_hw_info.config_locked;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if config has been read and is locked (no change anymore)
// Returns 0 if config hasn't been read yet (Button/Encoder/LED functions should be disabled)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_LockConfig(void)
{
  mbcv_file_hw_info.config_locked = 1;

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
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_HW_Read(void)
{
  s32 status = 0;
  mbcv_file_hw_info_t *info = &mbcv_file_hw_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBCV_HW.CV2", MBCV_FILES_PATH);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_HW] Open config file '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_HW] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read config values
  char line_buffer[128];
  do {
    status=FILE_ReadLine((u8 *)line_buffer, 80);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[MBCV_FILE_HW] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
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
	} else if( strncmp(parameter, "BUTTON_", 7) == 0 ) {
	  parameter += 7;

	  char *word = strtok_r(NULL, separators, &brkt);

	  // M1..M16 -> SR 16..31
          s32 sr = -1;
          if( word[0] == 'M' ) {
            ++word;
            s32 m = get_dec(word);

            if( m < 1 || m > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
              DEBUG_MSG("[MBCV_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value 'M%s'!", parameter, word);
#endif
              continue;
            }
            sr = 16 + m;
          } else {
            sr = get_dec(word);
            if( sr < 0 || sr > 32 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
              DEBUG_MSG("[MBCV_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value '%s'!", parameter, word);
#endif
              continue;
            }
          }

	  word = strtok_r(NULL, separators, &brkt);
	  s32 pin = get_dec(word);
	  if( pin < 0 || pin >= 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  u8 din_value = ((sr-1)<<3) | pin;

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[MBCV_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcmp(parameter, "SCS_EXIT") == 0 ) {
	    mbcv_hwcfg_button.scs_exit = din_value;
	  } else if( strcmp(parameter, "SCS_SHIFT") == 0 ) {
	    mbcv_hwcfg_button.scs_shift = din_value;
	  } else if( strcmp(parameter, "SCS_SOFT1") == 0 ) {
	    mbcv_hwcfg_button.scs_soft[0] = din_value;
	  } else if( strcmp(parameter, "SCS_SOFT2") == 0 ) {
	    mbcv_hwcfg_button.scs_soft[1] = din_value;
	  } else if( strcmp(parameter, "SCS_SOFT3") == 0 ) {
	    mbcv_hwcfg_button.scs_soft[2] = din_value;
	  } else if( strcmp(parameter, "SCS_SOFT4") == 0 ) {
	    mbcv_hwcfg_button.scs_soft[3] = din_value;
#if SCS_NUM_MENU_ITEMS != 4
# error "Please adapt the SCS_SOFT* assignments here!"
#endif
	  } else if( strcmp(parameter, "LFO1") == 0 ) {
	    mbcv_hwcfg_button.lfo1 = din_value;
	  } else if( strcmp(parameter, "LFO2") == 0 ) {
	    mbcv_hwcfg_button.lfo2 = din_value;
	  } else if( strcmp(parameter, "ENV1") == 0 ) {
	    mbcv_hwcfg_button.env1 = din_value;
	  } else if( strcmp(parameter, "ENV2") == 0 ) {
	    mbcv_hwcfg_button.env2 = din_value;
	  } else if( strncmp(parameter, "CV", 2) == 0 && // GP%d
		     (hlp=atoi(parameter+2)) >= 1 && hlp <= CV_SE_NUM ) {
	    mbcv_hwcfg_button.cv[hlp-1] = din_value;
	  } else if( strncmp(parameter, "ENCBANK", 7) == 0 && // ENCBANK%d
		     (hlp=atoi(parameter+7)) >= 1 && hlp <= MBCV_LRE_NUM_BANKS ) {
	    mbcv_hwcfg_button.enc_bank[hlp-1] = din_value;
	  } else if( strncmp(parameter, "CV_AND_ENCBANK", 14) == 0 && // CV_AND_ENCBANK%d
		     (hlp=atoi(parameter+14)) >= 1 && hlp <= CV_SE_NUM ) {
	    mbcv_hwcfg_button.cv_and_enc_bank[hlp-1] = din_value;
	  } else if( strncmp(parameter, "SCOPE", 5) == 0 && // SCOPE%d
		     (hlp=atoi(parameter+5)) >= 1 && hlp <= CV_SCOPE_NUM ) {
	    mbcv_hwcfg_button.scope[hlp-1] = din_value;
	  } else if( strncmp(parameter, "KEYBOARD_", 9) == 0 ) {
	    parameter += 9;

#if MBCV_BUTTON_KEYBOARD_KEYS_NUM != 17
# error "Please adapt keys here!"
#endif
	    const char *keys[MBCV_BUTTON_KEYBOARD_KEYS_NUM] = {
	      "C1", "C1S", "D1", "D1S", "E1", "F1", "F1S", "G1", "G1S", "A1", "A1S", "B1",
	      "C2", "C2S", "D2", "D2S", "E2",
	    };

	    u8 key_found = 0;
	    for(int i=0; i<MBCV_BUTTON_KEYBOARD_KEYS_NUM; ++i) {
	      if( strcmp(parameter, keys[i]) == 0 ) {
		key_found = 1;
		mbcv_hwcfg_button.keyboard[i] = din_value;
		break;
	      }
	    }

	    if( !key_found ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown button function 'BUTTON_KEYBOARD_%s'!", parameter);
#endif
	    }
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown button function 'BUTTON_%s'!", parameter);
#endif
	  }

	////////////////////////////////////////////////////////////////////////////////////////////
	// LREx
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "LRE", 3) == 0 ) {
	  s32 lre = parameter[3] - '1'; // user counts from 1, we count from 0
	  if( lre < 0 || lre >= MBCV_LRE_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR in %s definition: invalid LRE ID '%s' (expecting LRE1_ or LRE2_)!", parameter);
#endif
	    continue;
	  }
	  parameter += 4;

	  if( strncmp(parameter, "_ENC", 4) == 0 ) {
	    parameter += 4;
	    s32 enc = get_dec(parameter);
	    if( enc < 1 || enc > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: expecting ENC1..ENC16!!", lre+1, parameter);
#endif
	      continue;
	    }
	    --enc; // counting from 0

	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 sr = get_dec(word);
	    if( sr < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid first value '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) { // should we check for odd pin values (1/3/5/7) as well?
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid pin value '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    mios32_enc_type_t enc_type = DISABLED;
	    if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: missing encoder type!", lre+1, parameter);
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
	    } else if( strcmp(word, "DETENTED4") == 0 ) {
	      enc_type = DETENTED4;
	    } else if( strcmp(word, "DETENTED5") == 0 ) {
	      enc_type = DETENTED5;
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_ENC%s definition: invalid type '%s'!", lre+1, parameter, word);
#endif
	      continue;
	    }

#if DEBUG_VERBOSE_LEVEL >= 3
	    DEBUG_MSG("[MBCV_FILE_HW] LRE%d_ENC%d: SR %d Pin %d Type %d", lre+1, enc+1, sr, pin, enc_type);
#endif

	    // LRE encoders are starting at +1+MBCV_ENC_NUM, since the first encoders are allocated by SCS and GP functions
	    mios32_enc_config_t enc_config;
	    enc_config = MIOS32_ENC_ConfigGet(16*lre + enc+1+MBCV_ENC_NUM);
	    enc_config.cfg.type = enc_type;
	    enc_config.cfg.sr = sr;
	    enc_config.cfg.pos = pin;
	    enc_config.cfg.speed = NORMAL;
	    enc_config.cfg.speed_par = 0;
	    MIOS32_ENC_ConfigSet(16*lre + enc+1+MBCV_ENC_NUM, enc_config);

	  } else {
	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 value = get_dec(word);

	    if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting value!", lre+1, parameter, word);
#endif
	      continue;
	    } else {
	      if( strcmp(parameter, "_ENABLED") == 0 ) {
		if( value > 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..2!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].enabled = value;
	      } else if( strcmp(parameter, "_LEDRING_SELECT_DOUT_SR1") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_select_sr1 = value;
	      } else if( strcmp(parameter, "_LEDRING_SELECT_DOUT_SR2") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_select_sr2 = value;
	      } else if( strcmp(parameter, "_LEDRING_PATTERN_DOUT_SR1") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_pattern_sr1 = value;
	      } else if( strcmp(parameter, "_LEDRING_PATTERN_DOUT_SR2") == 0 ) {
		if( value > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBCV_FILE_HW] ERROR in LRE%d_%s definition: expecting 0..16!", lre+1, parameter, word);
#endif
		  continue;
		}
		mbcv_hwcfg_lre[lre].ledring_pattern_sr2 = value;
	      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown LRE parameter name 'LRE%d%s'!", lre+1, parameter);
#endif
	      }
	    }
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// WS2812
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "WS2812", 6) == 0 ) {
	  parameter += 6;
	  if( strncmp(parameter, "_LED", 4) == 0 ) {
	    parameter += 4;
	    s32 led = get_dec(parameter);
	    if( led < 1 || led > MBCV_RGB_LED_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in WS2812_LED%s definition: expecting LED1..LED%d!", parameter, MBCV_RGB_LED_NUM);
#endif
	      continue;
	    }
	    --led; // counting from 0

	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 pos = get_dec(word);
	    if( pos < 0 || pos > MBCV_RGB_LED_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in WS2812_LED%s definition: invalid position '%s'!", parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    mbcv_rgb_mode_t mode = MBCV_RGB_MODE_DISABLED;
	    if( strcmp(word, "DISABLED") == 0 ) {
	      mode = MBCV_RGB_MODE_DISABLED;
	    } else if( strcmp(word, "CHANNEL_HUE") == 0 ) {
	      mode = MBCV_RGB_MODE_CHANNEL_HUE;
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in WS2812%d_LED%s definition: invalid mode '%s'!", parameter, word);
#endif
	      continue;
	    }

	    mbcv_hwcfg_ws2812_t *ws2812 = &mbcv_hwcfg_ws2812[led];
	    ws2812->pos = pos;
	    ws2812->mode = mode;

	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR unknown config 'WS2812_%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// ENC assignments
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncmp(parameter, "ENC_", 4) == 0 ) {
	  parameter += 4;

	  int enc = -1;
	  if( strcmp(parameter, "SCS") == 0 ) {
	    enc = 0;
	  } else if( strcmp(parameter, "GP1") == 0 ) {
	    enc = 1;
	  } else if( strcmp(parameter, "GP2") == 0 ) {
	    enc = 2;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR unknown encoder function 'ENC_%s'!", parameter);
#endif
	  }

	  if( enc >= 0 ) {
	    char *word = strtok_r(NULL, separators, &brkt);
	    s32 sr = get_dec(word);
	    if( sr < 0 || sr > MIOS32_SRIO_NUM_SR ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in ENC_%s definition: invalid first value '%s'!", parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    s32 pin = get_dec(word);
	    if( pin < 0 || pin >= 8 ) { // should we check for odd pin values (1/3/5/7) as well?
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in ENC_%s definition: invalid pin value '%s'!", parameter, word);
#endif
	      continue;
	    }

	    word = strtok_r(NULL, separators, &brkt);
	    mios32_enc_type_t enc_type = DISABLED;
	    if( word == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in ENC_%s definition: missing encoder type!", parameter);
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
	    } else if( strcmp(word, "DETENTED4") == 0 ) {
	      enc_type = DETENTED4;
	    } else if( strcmp(word, "DETENTED5") == 0 ) {
	      enc_type = DETENTED5;
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBCV_FILE_HW] ERROR in ENC_%s definition: invalid type '%s'!", parameter, word);
#endif
	      continue;
	    }

#if DEBUG_VERBOSE_LEVEL >= 3
	    DEBUG_MSG("[MBCV_FILE_HW] ENC_%s: SR %d Pin %d Type %d", parameter, sr, pin, enc_type);
#endif

	    // GP encoders are starting at +1, since the first encoder is allocated by SCS
	    mios32_enc_config_t enc_config;
	    enc_config = MIOS32_ENC_ConfigGet(enc+1);
	    enc_config.cfg.type = enc_type;
	    enc_config.cfg.sr = sr;
	    enc_config.cfg.pos = pin;
	    enc_config.cfg.speed = NORMAL;
	    enc_config.cfg.speed_par = 0;
	    MIOS32_ENC_ConfigSet(enc+1, enc_config);
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// DOUT assignments
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "GATE_CV_DOUT_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_dout.gate_sr = sr;

	} else if( strcmp(parameter, "CLK_DOUT_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_dout.clk_sr = sr;


	////////////////////////////////////////////////////////////////////////////////////////////
	// Button Matrix
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcmp(parameter, "BM_SELECT_DOUT_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_bm.dout_select_sr = sr;
	} else if( strcmp(parameter, "BM_SCAN_ROW1_DIN_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_bm.din_scan_sr[0] = sr;
#if MBCV_BUTTON_MATRIX_ROWS >= 16
	} else if( strcmp(parameter, "BM_SCAN_ROW2_DIN_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_dec(word);
	  if( sr < 0 || sr > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR: invalid SR number: %s %s", parameter, word);
#endif
	    continue;
	  }
	  mbcv_hwcfg_bm.din_scan_sr[1] = sr;
#endif

#if MBCV_BUTTON_MATRIX_ROWS >= 24
# error "Please add additional BLM_SCAN_ROWx_DIN_SR here!"
#endif


	////////////////////////////////////////////////////////////////////////////////////////////
	// AOUT Interface
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "AOUT_IF") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  int aout_type;
	  for(aout_type=0; aout_type<AOUT_NUM_IF; ++aout_type) {
	    if( strcasecmp(word, MBCV_MAP_IfNameGet((aout_if_t)aout_type)) == 0 )
	      break;
	  }

	  if( aout_type == AOUT_NUM_IF ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBCV_FILE_HW] ERROR invalid AOUT interface name for parameter '%s': %s\n", parameter, word);
#endif
	  } else {
	    MBCV_MAP_IfSet((aout_if_t)aout_type);
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// unknown
	////////////////////////////////////////////////////////////////////////////////////////////
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBCV_FILE_HW] ERROR: unknown parameter: %s", line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBCV_FILE_HW] ERROR: no space separator in following line: %s", line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_HW] ERROR while reading file, status: %d\n", status);
#endif
    return MBCV_FILE_HW_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}
