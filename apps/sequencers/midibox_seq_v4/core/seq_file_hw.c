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
  unsigned only_once: 1;   // file is only loaded after startup
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

  seq_file_hw_info.only_once = 0;

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

  if( seq_file_hw_info.only_once ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] HW config file not loaded again\n");
#endif
  } else {
    error = SEQ_FILE_HW_Read();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_HW] Tried to open HW config file, status: %d\n", error);
#endif
    seq_file_hw_info.only_once = 1;
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
  char _line_buffer[80];
  char *line_buffer;
  do {
    line_buffer = _line_buffer;
    status=SEQ_FILE_ReadLine(&fi, (u8 *)line_buffer, 80);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE_HW] read: %s", line_buffer);
#endif

      // sscanf consumes too much memory, therefore we parse directly
      while( *line_buffer == ' ' || *line_buffer == '\t' )
	++line_buffer;

      if( *line_buffer == 0 || *line_buffer == '#' )
	continue;
      
      char *space = strchr(line_buffer, ' ');
      if( space != NULL ) {
	// separate line buffer into keyword and value string
	size_t space_pos = space-line_buffer;
	char *value_str = (char *)(line_buffer + space_pos + 1);
	line_buffer[space_pos] = 0;

	// search for keywords
	if( strncmp(line_buffer, "BUTTON_", 7) == 0 ) {
	  line_buffer += 7;

	  long sr, pin;
	  char *next = value_str;
	  sr = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid SR value!", line_buffer);
#endif
	    continue;
	  }

	  value_str = next;
	  pin = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_%s definition: invalid pin value!", line_buffer);
#endif
	    continue;
	  }

	  u8 din_value = ((sr-1)<<3) | pin;
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] Button %s: SR %d Pin %d (DIN: 0x%02x)", line_buffer, sr, pin, din_value);
#endif

	  if( strcmp(line_buffer, "DOWN") == 0 ) {
	    seq_hwcfg_button.down = din_value;
	  } else if( strcmp(line_buffer, "UP") == 0 ) {
	    seq_hwcfg_button.up = din_value;
	  } else if( strcmp(line_buffer, "LEFT") == 0 ) {
	    seq_hwcfg_button.left = din_value;
	  } else if( strcmp(line_buffer, "RIGHT") == 0 ) {
	    seq_hwcfg_button.right = din_value;
	  } else if( strcmp(line_buffer, "SCRUB") == 0 ) {
	    seq_hwcfg_button.scrub = din_value;
	  } else if( strcmp(line_buffer, "METRONOME") == 0 ) {
	    seq_hwcfg_button.metronome = din_value;
	  } else if( strcmp(line_buffer, "STOP") == 0 ) {
	    seq_hwcfg_button.stop = din_value;
	  } else if( strcmp(line_buffer, "PAUSE") == 0 ) {
	    seq_hwcfg_button.pause = din_value;
	  } else if( strcmp(line_buffer, "PLAY") == 0 ) {
	    seq_hwcfg_button.play = din_value;
	  } else if( strcmp(line_buffer, "REW") == 0 ) {
	    seq_hwcfg_button.rew = din_value;
	  } else if( strcmp(line_buffer, "FWD") == 0 ) {
	    seq_hwcfg_button.fwd = din_value;
	  } else if( strcmp(line_buffer, "F1") == 0 ) {
	    seq_hwcfg_button.f1 = din_value;
	  } else if( strcmp(line_buffer, "F2") == 0 ) {
	    seq_hwcfg_button.f2 = din_value;
	  } else if( strcmp(line_buffer, "F3") == 0 ) {
	    seq_hwcfg_button.f3 = din_value;
	  } else if( strcmp(line_buffer, "F4") == 0 ) {
	    seq_hwcfg_button.f4 = din_value;
	  } else if( strcmp(line_buffer, "MENU") == 0 ) {
	    seq_hwcfg_button.menu = din_value;
	  } else if( strcmp(line_buffer, "SELECT") == 0 ) {
	    seq_hwcfg_button.select = din_value;
	  } else if( strcmp(line_buffer, "EXIT") == 0 ) {
	    seq_hwcfg_button.exit = din_value;
	  } else if( strcmp(line_buffer, "TRACK1") == 0 ) {
	    seq_hwcfg_button.track[0] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK2") == 0 ) {
	    seq_hwcfg_button.track[1] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK3") == 0 ) {
	    seq_hwcfg_button.track[2] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK4") == 0 ) {
	    seq_hwcfg_button.track[3] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_A") == 0 ) {
	    seq_hwcfg_button.par_layer[0] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_B") == 0 ) {
	    seq_hwcfg_button.par_layer[1] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_C") == 0 ) {
	    seq_hwcfg_button.par_layer[2] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "EDIT") == 0 ) {
	    seq_hwcfg_button.edit = din_value;
	  } else if( strcmp(line_buffer, "MUTE") == 0 ) {
	    seq_hwcfg_button.mute = din_value;
	  } else if( strcmp(line_buffer, "PATTERN") == 0 ) {
	    seq_hwcfg_button.pattern = din_value;
	  } else if( strcmp(line_buffer, "SONG") == 0 ) {
	    seq_hwcfg_button.song = din_value;
	  } else if( strcmp(line_buffer, "SOLO") == 0 ) {
	    seq_hwcfg_button.solo = din_value;
	  } else if( strcmp(line_buffer, "FAST") == 0 ) {
	    seq_hwcfg_button.fast = din_value;
	  } else if( strcmp(line_buffer, "ALL") == 0 ) {
	    seq_hwcfg_button.all = din_value;
	  } else if( strcmp(line_buffer, "GP1") == 0 ) {
	    seq_hwcfg_button.gp[0] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP2") == 0 ) {
	    seq_hwcfg_button.gp[1] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP3") == 0 ) {
	    seq_hwcfg_button.gp[2] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP4") == 0 ) {
	    seq_hwcfg_button.gp[3] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP5") == 0 ) {
	    seq_hwcfg_button.gp[4] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP6") == 0 ) {
	    seq_hwcfg_button.gp[5] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP7") == 0 ) {
	    seq_hwcfg_button.gp[6] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP8") == 0 ) {
	    seq_hwcfg_button.gp[7] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP9") == 0 ) {
	    seq_hwcfg_button.gp[8] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP10") == 0 ) {
	    seq_hwcfg_button.gp[9] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP11") == 0 ) {
	    seq_hwcfg_button.gp[10] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP12") == 0 ) {
	    seq_hwcfg_button.gp[11] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP13") == 0 ) {
	    seq_hwcfg_button.gp[12] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP14") == 0 ) {
	    seq_hwcfg_button.gp[13] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP15") == 0 ) {
	    seq_hwcfg_button.gp[14] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GP16") == 0 ) {
	    seq_hwcfg_button.gp[15] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP1") == 0 ) {
	    seq_hwcfg_button.group[0] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP2") == 0 ) {
	    seq_hwcfg_button.group[1] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP3") == 0 ) {
	    seq_hwcfg_button.group[2] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP4") == 0 ) {
	    seq_hwcfg_button.group[3] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_A") == 0 ) {
	    seq_hwcfg_button.trg_layer[0] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_B") == 0 ) {
	    seq_hwcfg_button.trg_layer[1] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_C") == 0 ) {
	    seq_hwcfg_button.trg_layer[2] = din_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_button.step_view = din_value;
	  } else if( strcmp(line_buffer, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_button.tap_tempo = din_value;
	  } else if( strcmp(line_buffer, "UTILITY") == 0 ) {
	    seq_hwcfg_button.utility = din_value;
	  } else if( strcmp(line_buffer, "COPY") == 0 ) {
	    seq_hwcfg_button.copy = din_value;
	  } else if( strcmp(line_buffer, "PASTE") == 0 ) {
	    seq_hwcfg_button.paste = din_value;
	  } else if( strcmp(line_buffer, "CLEAR") == 0 ) {
	    seq_hwcfg_button.clear = din_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown button function 'BUTTON_%s'!", line_buffer);
#endif
	  }

	} else if( strncmp(line_buffer, "LED_", 4) == 0 ) {
	  line_buffer += 4;

	  long sr, pin;
	  char *next = value_str;
	  sr = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid SR value!", line_buffer);
#endif
	    continue;
	  }

	  value_str = next;
	  pin = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in LED_%s definition: invalid pin value!", line_buffer);
#endif
	    continue;
	  }

	  u8 dout_value = ((sr-1)<<3) | pin;
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] LED %s: SR %d Pin %d (DOUT: 0x%02x)", line_buffer, sr, pin, dout_value);
#endif

	  if( strcmp(line_buffer, "TRACK1") == 0 ) {
	    seq_hwcfg_led.track[0] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK2") == 0 ) {
	    seq_hwcfg_led.track[1] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK3") == 0 ) {
	    seq_hwcfg_led.track[2] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRACK4") == 0 ) {
	    seq_hwcfg_led.track[3] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_A") == 0 ) {
	    seq_hwcfg_led.par_layer[0] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_B") == 0 ) {
	    seq_hwcfg_led.par_layer[1] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PAR_LAYER_C") == 0 ) {
	    seq_hwcfg_led.par_layer[2] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "BEAT") == 0 ) {
	    seq_hwcfg_led.beat = dout_value;
	  } else if( strcmp(line_buffer, "EDIT") == 0 ) {
	    seq_hwcfg_led.edit = dout_value;
	  } else if( strcmp(line_buffer, "MUTE") == 0 ) {
	    seq_hwcfg_led.mute = dout_value;
	  } else if( strcmp(line_buffer, "PATTERN") == 0 ) {
	    seq_hwcfg_led.pattern = dout_value;
	  } else if( strcmp(line_buffer, "SONG") == 0 ) {
	    seq_hwcfg_led.song = dout_value;
	  } else if( strcmp(line_buffer, "SOLO") == 0 ) {
	    seq_hwcfg_led.solo = dout_value;
	  } else if( strcmp(line_buffer, "FAST") == 0 ) {
	    seq_hwcfg_led.fast = dout_value;
	  } else if( strcmp(line_buffer, "ALL") == 0 ) {
	    seq_hwcfg_led.all = dout_value;
	  } else if( strcmp(line_buffer, "GROUP1") == 0 ) {
	    seq_hwcfg_led.group[0] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP2") == 0 ) {
	    seq_hwcfg_led.group[1] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP3") == 0 ) {
	    seq_hwcfg_led.group[2] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "GROUP4") == 0 ) {
	    seq_hwcfg_led.group[3] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_A") == 0 ) {
	    seq_hwcfg_led.trg_layer[0] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_B") == 0 ) {
	    seq_hwcfg_led.trg_layer[1] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "TRG_LAYER_C") == 0 ) {
	    seq_hwcfg_led.trg_layer[2] = dout_value; // TODO: optimize
	  } else if( strcmp(line_buffer, "PLAY") == 0 ) {
	    seq_hwcfg_led.play = dout_value;
	  } else if( strcmp(line_buffer, "STOP") == 0 ) {
	    seq_hwcfg_led.stop = dout_value;
	  } else if( strcmp(line_buffer, "PAUSE") == 0 ) {
	    seq_hwcfg_led.pause = dout_value;
	  } else if( strcmp(line_buffer, "REW") == 0 ) {
	    seq_hwcfg_led.rew = dout_value;
	  } else if( strcmp(line_buffer, "FWD") == 0 ) {
	    seq_hwcfg_led.fwd = dout_value;
	  } else if( strcmp(line_buffer, "MENU") == 0 ) {
	    seq_hwcfg_led.menu = dout_value;
	  } else if( strcmp(line_buffer, "SCRUB") == 0 ) {
	    seq_hwcfg_led.scrub = dout_value;
	  } else if( strcmp(line_buffer, "METRONOME") == 0 ) {
	    seq_hwcfg_led.metronome = dout_value;
	  } else if( strcmp(line_buffer, "UTILITY") == 0 ) {
	    seq_hwcfg_led.utility = dout_value;
	  } else if( strcmp(line_buffer, "COPY") == 0 ) {
	    seq_hwcfg_led.copy = dout_value;
	  } else if( strcmp(line_buffer, "PASTE") == 0 ) {
	    seq_hwcfg_led.paste = dout_value;
	  } else if( strcmp(line_buffer, "CLEAR") == 0 ) {
	    seq_hwcfg_led.clear = dout_value;
	  } else if( strcmp(line_buffer, "F1") == 0 ) {
	    seq_hwcfg_led.f1 = dout_value;
	  } else if( strcmp(line_buffer, "F2") == 0 ) {
	    seq_hwcfg_led.f2 = dout_value;
	  } else if( strcmp(line_buffer, "F3") == 0 ) {
	    seq_hwcfg_led.f3 = dout_value;
	  } else if( strcmp(line_buffer, "F4") == 0 ) {
	    seq_hwcfg_led.f4 = dout_value;
	  } else if( strcmp(line_buffer, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_led.step_view = dout_value;
	  } else if( strcmp(line_buffer, "DOWN") == 0 ) {
	    seq_hwcfg_led.down = dout_value;
	  } else if( strcmp(line_buffer, "UP") == 0 ) {
	    seq_hwcfg_led.up = dout_value;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown LED function 'LED_%s'!", line_buffer);
#endif
	  }

	} else if( strncmp(line_buffer, "ENC_", 4) == 0 ) {
	  line_buffer += 4;

	  long sr, pin;
	  char *next = value_str;
	  sr = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid SR value!", line_buffer);
#endif
	    continue;
	  }

	  value_str = next;
	  pin = strtol(value_str, &next, 0);
	  if( value_str == next ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid pin value!", line_buffer);
#endif
	    continue;
	  }

	  value_str = next;
#if 0
	  // TODO: check why parser hangs up with this trim()
	  while( *value_str == ' ' || *value_str == '\t' )
	    ++value_str;
#endif

	  mios32_enc_type_t enc_type = DISABLED;
	  if( *value_str == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: missing encoder type!", line_buffer);
#endif
	    continue;
	  } else if( strstr("NON_DETENTED", value_str) != NULL ) {
	    enc_type = NON_DETENTED;
	  } else if( strstr("DETENTED1", value_str) != NULL ) {
	    enc_type = DETENTED1;
	  } else if( strstr("DETENTED2", value_str) != NULL ) {
	    enc_type = DETENTED2;
	  } else if( strstr("DETENTED3", value_str) != NULL ) {
	    enc_type = DETENTED3;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in ENC_%s definition: invalid type '%s'!", line_buffer, next);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] ENC %s: SR %d Pin %d Type %d", line_buffer, sr, pin, enc_type);
#endif

	  mios32_enc_config_t enc_config = { .cfg.type=enc_type, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=sr, .cfg.pos=pin };

	  if( strcmp(line_buffer, "DATAWHEEL") == 0 ) {
	    MIOS32_ENC_ConfigSet(0, enc_config);
	  } else if( strncmp(line_buffer, "GP", 2) == 0 ) {
	    line_buffer += 2;

	    int gp = atoi(line_buffer);
	    if( gp < 1 || gp > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR: invalid ENC_GP number 'ENC_GP%s'!", line_buffer);
#endif
	    } else {
	      MIOS32_ENC_ConfigSet(gp, enc_config);
	    }

	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown ENC name 'ENC_%s'!", line_buffer);
#endif
	  }

	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown setting: %s %s", line_buffer, value_str);
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
