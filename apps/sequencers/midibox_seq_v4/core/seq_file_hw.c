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
#include <seq_cv.h>
#include <blm.h>

#include "file.h"
#include "seq_file.h"
#include "seq_file_hw.h"

#include "seq_hwcfg.h"

#include "seq_blm8x8.h"

#include "seq_ui.h"
#include "seq_ui_pages.h"
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
  sprintf(filepath, "%sMBSEQ_HW.V4", SEQ_FILES_PATH);

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
	// MENU_SHORTCUT
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "MENU_SHORTCUT") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	////////////////////////////////////////////////////////////////////////////////////////////
	// BUTTON_BEH
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BUTTON_BEH_", 11) == 0 ) {
	  parameter += 11;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 flag = get_dec(word);
	  if( flag < 0 || flag > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_BEH_%s definition: expecting 0 or 1, got '%s'!", parameter, word);
#endif
	    continue;
	  }

	  if( strcasecmp(parameter, "FAST2") == 0 ) {
	    seq_hwcfg_button_beh.fast2 = flag;
	  } else if( strcasecmp(parameter, "FAST") == 0 ) {
	    seq_hwcfg_button_beh.fast = flag;
	  } else if( strcasecmp(parameter, "ALL") == 0 ) {
	    seq_hwcfg_button_beh.all = flag;
	  } else if( strcasecmp(parameter, "SOLO") == 0 ) {
	    seq_hwcfg_button_beh.solo = flag;
	  } else if( strcasecmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_button_beh.metronome = flag;
	  } else if( strcasecmp(parameter, "SCRUB") == 0 ) {
	    seq_hwcfg_button_beh.scrub = flag;
	  } else if( strcasecmp(parameter, "LOOP") == 0 ) {
	    seq_hwcfg_button_beh.loop = flag;
	  } else if( strcasecmp(parameter, "FOLLOW") == 0 ) {
	    seq_hwcfg_button_beh.follow = flag;
	  } else if( strcasecmp(parameter, "MENU") == 0 ) {
	    seq_hwcfg_button_beh.menu = flag;
	  } else if( strcasecmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_button_beh.mute = flag;
	  } else if( strcasecmp(parameter, "BOOKMARK") == 0 ) {
	    seq_hwcfg_button_beh.bookmark = flag;
	  } else if( strcasecmp(parameter, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_button_beh.step_view = flag;
	  } else if( strcasecmp(parameter, "TRG_LAYER") == 0 ) {
	    seq_hwcfg_button_beh.trg_layer = flag;
	  } else if( strcasecmp(parameter, "PAR_LAYER") == 0 ) {
	    seq_hwcfg_button_beh.par_layer = flag;
	  } else if( strcasecmp(parameter, "INS_SEL") == 0 ) {
	    seq_hwcfg_button_beh.ins_sel = flag;
	  } else if( strcasecmp(parameter, "TRACK_SEL") == 0 ) {
	    seq_hwcfg_button_beh.track_sel = flag;
	  } else if( strcasecmp(parameter, "TEMPO_PRESET") == 0 ) {
	    seq_hwcfg_button_beh.tempo_preset = flag;
	  } else if( strcasecmp(parameter, "ALL_WITH_TRIGGERS") == 0 ) {
	    seq_hwcfg_button_beh.all_with_triggers = flag;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown button behaviour function 'BUTTON_BEH_%s'!", parameter);
#endif
	  }


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

	  if( strcasecmp(parameter, "DOWN") == 0 ) {
	    seq_hwcfg_button.down = din_value;
	  } else if( strcasecmp(parameter, "UP") == 0 ) {
	    seq_hwcfg_button.up = din_value;
	  } else if( strcasecmp(parameter, "LEFT") == 0 ) {
	    seq_hwcfg_button.left = din_value;
	  } else if( strcasecmp(parameter, "RIGHT") == 0 ) {
	    seq_hwcfg_button.right = din_value;
	  } else if( strcasecmp(parameter, "SCRUB") == 0 ) {
	    seq_hwcfg_button.scrub = din_value;
	  } else if( strcasecmp(parameter, "LOOP") == 0 ) {
	    seq_hwcfg_button.loop = din_value;
	  } else if( strcasecmp(parameter, "FOLLOW") == 0 ) {
	    seq_hwcfg_button.follow = din_value;
	  } else if( strcasecmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_button.metronome = din_value;
	  } else if( strcasecmp(parameter, "RECORD") == 0 ) {
	    seq_hwcfg_button.record = din_value;
	  } else if( strcasecmp(parameter, "JAM_LIVE") == 0 ) {
	    seq_hwcfg_button.jam_live = din_value;
	  } else if( strcasecmp(parameter, "JAM_STEP") == 0 ) {
	    seq_hwcfg_button.jam_step = din_value;
	  } else if( strcasecmp(parameter, "LIVE") == 0 ) {
	    seq_hwcfg_button.live = din_value;
	  } else if( strcasecmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_button.stop = din_value;
	  } else if( strcasecmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_button.pause = din_value;
	  } else if( strcasecmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_button.play = din_value;
	  } else if( strcasecmp(parameter, "REW") == 0 ) {
	    seq_hwcfg_button.rew = din_value;
	  } else if( strcasecmp(parameter, "FWD") == 0 ) {
	    seq_hwcfg_button.fwd = din_value;
	  } else if( strcasecmp(parameter, "MENU") == 0 ) {
	    seq_hwcfg_button.menu = din_value;
	  } else if( strcasecmp(parameter, "BOOKMARK") == 0 ) {
	    seq_hwcfg_button.bookmark = din_value;
	  } else if( strcasecmp(parameter, "SELECT") == 0 ) {
	    seq_hwcfg_button.select = din_value;
	  } else if( strcasecmp(parameter, "EXIT") == 0 ) {
	    seq_hwcfg_button.exit = din_value;
	  } else if( strncasecmp(parameter, "TRACK", 5) == 0 && // TRACK[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_button.track[hlp] = din_value;
	  } else if( strncasecmp(parameter, "PAR_LAYER_", 10) == 0 && // PAR_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_button.par_layer[hlp] = din_value;
	  } else if( strcasecmp(parameter, "EDIT") == 0 ) {
	    seq_hwcfg_button.edit = din_value;
	  } else if( strcasecmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_button.mute = din_value;
	  } else if( strcasecmp(parameter, "PATTERN") == 0 ) {
	    seq_hwcfg_button.pattern = din_value;
	  } else if( strcasecmp(parameter, "SONG") == 0 ) {
	    seq_hwcfg_button.song = din_value;
	  } else if( strcasecmp(parameter, "PHRASE") == 0 ) {
	    seq_hwcfg_button.phrase = din_value;
	  } else if( strcasecmp(parameter, "SOLO") == 0 ) {
	    seq_hwcfg_button.solo = din_value;
	  } else if( strcasecmp(parameter, "FAST2") == 0 ) {
	    seq_hwcfg_button.fast2 = din_value;
	  } else if( strcasecmp(parameter, "FAST") == 0 ) {
	    seq_hwcfg_button.fast = din_value;
	  } else if( strcasecmp(parameter, "ALL") == 0 ) {
	    seq_hwcfg_button.all = din_value;
	  } else if( strncasecmp(parameter, "GP", 2) == 0 && // GP%d
		     (hlp=atoi(parameter+2)) >= 1 && hlp <= 16 ) {
	    seq_hwcfg_button.gp[hlp-1] = din_value;
	  } else if( strncasecmp(parameter, "GROUP", 5) == 0 && // GROUP[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_button.group[hlp] = din_value;
	  } else if( strncasecmp(parameter, "TRG_LAYER_", 10) == 0 && // TRG_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_button.trg_layer[hlp] = din_value;
	  } else if( strcasecmp(parameter, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_button.step_view = din_value;
	  } else if( strcasecmp(parameter, "TRG_LAYER_SEL") == 0 ) {
	    seq_hwcfg_button.trg_layer_sel = din_value;
	  } else if( strcasecmp(parameter, "PAR_LAYER_SEL") == 0 ) {
	    seq_hwcfg_button.par_layer_sel = din_value;
	  } else if( strcasecmp(parameter, "INS_SEL") == 0 ) {
	    seq_hwcfg_button.ins_sel = din_value;
	  } else if( strcasecmp(parameter, "TRACK_SEL") == 0 ) {
	    seq_hwcfg_button.track_sel = din_value;
	  } else if( strcasecmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_button.tap_tempo = din_value;
	  } else if( strcasecmp(parameter, "TEMPO_PRESET") == 0 ) {
	    seq_hwcfg_button.tempo_preset = din_value;
	  } else if( strcasecmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_button.ext_restart = din_value;
	  } else if( strcasecmp(parameter, "UTILITY") == 0 ) {
	    seq_hwcfg_button.utility = din_value;
	  } else if( strcasecmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_button.copy = din_value;
	  } else if( strcasecmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_button.paste = din_value;
	  } else if( strcasecmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_button.clear = din_value;
	  } else if( strcasecmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_button.undo = din_value;
	  } else if( strcasecmp(parameter, "MOVE") == 0 ) {
	    seq_hwcfg_button.move = din_value;
	  } else if( strcasecmp(parameter, "SCROLL") == 0 ) {
	    seq_hwcfg_button.scroll = din_value;
	  } else if( strcasecmp(parameter, "MIXER") == 0 ) {
	    seq_hwcfg_button.mixer = din_value;
	  } else if( strcasecmp(parameter, "SAVE") == 0 ) {
	    seq_hwcfg_button.save = din_value;
	  } else if( strcasecmp(parameter, "SAVE_ALL") == 0 ) {
	    seq_hwcfg_button.save_all = din_value;
	  } else if( strcasecmp(parameter, "TRACK_MODE") == 0 ) {
	    seq_hwcfg_button.track_mode = din_value;
	  } else if( strcasecmp(parameter, "TRACK_GROOVE") == 0 ) {
	    seq_hwcfg_button.track_groove = din_value;
	  } else if( strcasecmp(parameter, "TRACK_LENGTH") == 0 ) {
	    seq_hwcfg_button.track_length = din_value;
	  } else if( strcasecmp(parameter, "TRACK_DIRECTION") == 0 ) {
	    seq_hwcfg_button.track_direction = din_value;
	  } else if( strcasecmp(parameter, "MORPH") == 0 || strcasecmp(parameter, "TRACK_MORPH") == 0 ) {
	    seq_hwcfg_button.track_morph = din_value;
	  } else if( strcasecmp(parameter, "TRANSPOSE") == 0 || strcasecmp(parameter, "TRACK_TRANSPOSE") == 0 ) {
	    seq_hwcfg_button.track_transpose = din_value;
	  } else if( strcasecmp(parameter, "FX") == 0 ) {
	    seq_hwcfg_button.fx = din_value;
	  } else if( strcasecmp(parameter, "FOOTSWITCH") == 0 ) {
	    seq_hwcfg_button.footswitch = din_value;
	  } else if( strcasecmp(parameter, "ENC_BTN_FWD") == 0 ) {
	    seq_hwcfg_button.enc_btn_fwd = din_value;
          } else if( strcasecmp(parameter, "PATTERN_RMX") == 0 ) {
            seq_hwcfg_button.pattern_remix = din_value;
          } else if( strcasecmp(parameter, "MUTE_ALL_TRACKS") == 0 ) {
            seq_hwcfg_button.mute_all_tracks = din_value;
          } else if( strcasecmp(parameter, "MUTE_TRACK_LAYERS") == 0 ) {
            seq_hwcfg_button.mute_track_layers = din_value;
          } else if( strcasecmp(parameter, "MUTE_ALL_TRACKS_AND_LAYERS") == 0 ) {
            seq_hwcfg_button.mute_all_tracks_and_layers = din_value;
          } else if( strcasecmp(parameter, "UNMUTE_ALL_TRACKS") == 0 ) {
            seq_hwcfg_button.unmute_all_tracks = din_value;
          } else if( strcasecmp(parameter, "UNMUTE_TRACK_LAYERS") == 0 ) {
            seq_hwcfg_button.unmute_track_layers = din_value;
          } else if( strcasecmp(parameter, "UNMUTE_ALL_TRACKS_AND_LAYERS") == 0 ) {
            seq_hwcfg_button.unmute_all_tracks_and_layers = din_value;
	  } else if( strncasecmp(parameter, "DIRECT_BOOKMARK", 15) == 0 ) {
	    parameter += 15;

	    s32 bookmark = get_dec(parameter);
	    if( bookmark < 1 || bookmark > SEQ_HWCFG_NUM_DIRECT_BOOKMARK ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_DIRECT_BOOKMARK%s definition: invalid function number '%s'!", parameter, parameter);
#endif
	      continue;
	    }
	    seq_hwcfg_button.direct_bookmark[bookmark-1] = din_value;
	  } else if( strncasecmp(parameter, "ENC", 3) == 0 ) {
	    parameter += 3;

#if SEQ_HWCFG_NUM_ENCODERS != 18
#error "please adapt this code to new encoder assignments"
#endif
	    if( strcasecmp(parameter, "_DATAWHEEL") == 0 ) {
	      seq_hwcfg_button.enc[0] = din_value;
	    } else if( strcasecmp(parameter, "_BPM") == 0 ) {
	      seq_hwcfg_button.enc[17] = din_value;
	    } else {
	      s32 gp = get_dec(parameter);
	      if( gp < 1 || gp > 16 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_ENC%s definition: invalid enc number '%s'!", parameter, parameter);
#endif
		continue;
	      }
	      seq_hwcfg_button.enc[gp] = din_value; // GPs located at index 1..16
	    }
	  } else if( strncasecmp(parameter, "DIRECT_TRACK", 12) == 0 ) {
	    parameter += 12;

	    s32 track = get_dec(parameter);
	    if( track < 1 || track > SEQ_HWCFG_NUM_DIRECT_TRACK ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[SEQ_FILE_HW] ERROR in BUTTON_DIRECT_TRACK%s definition: invalid track number '%s'!", parameter, parameter);
#endif
	      continue;
	    }
	    seq_hwcfg_button.direct_track[track-1] = din_value;
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

	  if( strncasecmp(parameter, "TRACK", 5) == 0 && // TRACK[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_led.track[hlp] = dout_value;
	  } else if( strncasecmp(parameter, "PAR_LAYER_", 10) == 0 && // PAR_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_led.par_layer[hlp] = dout_value;
	  } else if( strcasecmp(parameter, "BEAT") == 0 ) {
	    seq_hwcfg_led.beat = dout_value;
	  } else if( strcasecmp(parameter, "MEASURE") == 0 ) {
	    seq_hwcfg_led.measure = dout_value;
	  } else if( strcasecmp(parameter, "MIDI_IN_COMBINED") == 0 ) {
	    seq_hwcfg_led.midi_in_combined = dout_value;
	  } else if( strcasecmp(parameter, "MIDI_OUT_COMBINED") == 0 ) {
	    seq_hwcfg_led.midi_out_combined = dout_value;
	  } else if( strcasecmp(parameter, "EDIT") == 0 ) {
	    seq_hwcfg_led.edit = dout_value;
	  } else if( strcasecmp(parameter, "MUTE") == 0 ) {
	    seq_hwcfg_led.mute = dout_value;
	  } else if( strcasecmp(parameter, "PATTERN") == 0 ) {
	    seq_hwcfg_led.pattern = dout_value;
	  } else if( strcasecmp(parameter, "SONG") == 0 ) {
	    seq_hwcfg_led.song = dout_value;
	  } else if( strcasecmp(parameter, "PHRASE") == 0 ) {
	    seq_hwcfg_led.phrase = dout_value;
	  } else if( strcasecmp(parameter, "SOLO") == 0 ) {
	    seq_hwcfg_led.solo = dout_value;
	  } else if( strcasecmp(parameter, "FAST2") == 0 ) {
	    seq_hwcfg_led.fast2 = dout_value;
	  } else if( strcasecmp(parameter, "FAST") == 0 ) {
	    seq_hwcfg_led.fast = dout_value;
	  } else if( strcasecmp(parameter, "ALL") == 0 ) {
	    seq_hwcfg_led.all = dout_value;
	  } else if( strncasecmp(parameter, "GROUP", 5) == 0 && // GROUP[1234]
		     (hlp=parameter[5]-'1') >= 0 && hlp < 4 ) {
	    seq_hwcfg_led.group[hlp] = dout_value;
	  } else if( strncasecmp(parameter, "TRG_LAYER_", 10) == 0 && // TRG_LAYER_[ABC]
		     (hlp=parameter[10]-'A') >= 0 && hlp < 3 ) {
	    seq_hwcfg_led.trg_layer[hlp] = dout_value;
	  } else if( strcasecmp(parameter, "PLAY") == 0 ) {
	    seq_hwcfg_led.play = dout_value;
	  } else if( strcasecmp(parameter, "STOP") == 0 ) {
	    seq_hwcfg_led.stop = dout_value;
	  } else if( strcasecmp(parameter, "PAUSE") == 0 ) {
	    seq_hwcfg_led.pause = dout_value;
	  } else if( strcasecmp(parameter, "REW") == 0 ) {
	    seq_hwcfg_led.rew = dout_value;
	  } else if( strcasecmp(parameter, "FWD") == 0 ) {
	    seq_hwcfg_led.fwd = dout_value;
	  } else if( strcasecmp(parameter, "EXIT") == 0 ) {
	    seq_hwcfg_led.exit = dout_value;
	  } else if( strcasecmp(parameter, "SELECT") == 0 ) {
	    seq_hwcfg_led.select = dout_value;
	  } else if( strcasecmp(parameter, "MENU") == 0 ) {
	    seq_hwcfg_led.menu = dout_value;
	  } else if( strcasecmp(parameter, "BOOKMARK") == 0 ) {
	    seq_hwcfg_led.bookmark = dout_value;
	  } else if( strcasecmp(parameter, "SCRUB") == 0 ) {
	    seq_hwcfg_led.scrub = dout_value;
	  } else if( strcasecmp(parameter, "LOOP") == 0 ) {
	    seq_hwcfg_led.loop = dout_value;
	  } else if( strcasecmp(parameter, "FOLLOW") == 0 ) {
	    seq_hwcfg_led.follow = dout_value;
	  } else if( strcasecmp(parameter, "METRONOME") == 0 ) {
	    seq_hwcfg_led.metronome = dout_value;
	  } else if( strcasecmp(parameter, "RECORD") == 0 ) {
	    seq_hwcfg_led.record = dout_value;
	  } else if( strcasecmp(parameter, "JAM_LIVE") == 0 ) {
	    seq_hwcfg_led.jam_live = dout_value;
	  } else if( strcasecmp(parameter, "JAM_STEP") == 0 ) {
	    seq_hwcfg_led.jam_step = dout_value;
	  } else if( strcasecmp(parameter, "LIVE") == 0 ) {
	    seq_hwcfg_led.live = dout_value;
	  } else if( strcasecmp(parameter, "UTILITY") == 0 ) {
	    seq_hwcfg_led.utility = dout_value;
	  } else if( strcasecmp(parameter, "COPY") == 0 ) {
	    seq_hwcfg_led.copy = dout_value;
	  } else if( strcasecmp(parameter, "PASTE") == 0 ) {
	    seq_hwcfg_led.paste = dout_value;
	  } else if( strcasecmp(parameter, "CLEAR") == 0 ) {
	    seq_hwcfg_led.clear = dout_value;
	  } else if( strcasecmp(parameter, "UNDO") == 0 ) {
	    seq_hwcfg_led.undo = dout_value;
	  } else if( strcasecmp(parameter, "MOVE") == 0 ) {
	    seq_hwcfg_led.move = dout_value;
	  } else if( strcasecmp(parameter, "SCROLL") == 0 ) {
	    seq_hwcfg_led.scroll = dout_value;
	  } else if( strcasecmp(parameter, "STEP_VIEW") == 0 ) {
	    seq_hwcfg_led.step_view = dout_value;
	  } else if( strcasecmp(parameter, "TRG_LAYER_SEL") == 0 ) {
	    seq_hwcfg_led.trg_layer_sel = dout_value;
	  } else if( strcasecmp(parameter, "PAR_LAYER_SEL") == 0 ) {
	    seq_hwcfg_led.par_layer_sel = dout_value;
	  } else if( strcasecmp(parameter, "INS_SEL") == 0 ) {
	    seq_hwcfg_led.ins_sel = dout_value;
	  } else if( strcasecmp(parameter, "TRACK_SEL") == 0 ) {
	    seq_hwcfg_led.track_sel = dout_value;
	  } else if( strcasecmp(parameter, "TAP_TEMPO") == 0 ) {
	    seq_hwcfg_led.tap_tempo = dout_value;
	  } else if( strcasecmp(parameter, "TEMPO_PRESET") == 0 ) {
	    seq_hwcfg_led.tempo_preset = dout_value;
	  } else if( strcasecmp(parameter, "EXT_RESTART") == 0 ) {
	    seq_hwcfg_led.ext_restart = dout_value;
	  } else if( strcasecmp(parameter, "DOWN") == 0 ) {
	    seq_hwcfg_led.down = dout_value;
	  } else if( strcasecmp(parameter, "UP") == 0 ) {
	    seq_hwcfg_led.up = dout_value;
	  } else if( strcasecmp(parameter, "MIXER") == 0 ) {
	    seq_hwcfg_led.mixer = dout_value;
	  } else if( strcasecmp(parameter, "TRACK_MODE") == 0 ) {
	    seq_hwcfg_led.track_mode = dout_value;
	  } else if( strcasecmp(parameter, "TRACK_GROOVE") == 0 ) {
	    seq_hwcfg_led.track_groove = dout_value;
	  } else if( strcasecmp(parameter, "TRACK_LENGTH") == 0 ) {
	    seq_hwcfg_led.track_length = dout_value;
	  } else if( strcasecmp(parameter, "TRACK_DIRECTION") == 0 ) {
	    seq_hwcfg_led.track_direction = dout_value;
	  } else if( strcasecmp(parameter, "MORPH") == 0 || strcasecmp(parameter, "TRACK_MORPH") == 0 ) {
	    seq_hwcfg_led.track_morph = dout_value;
	  } else if( strcasecmp(parameter, "TRANSPOSE") == 0 || strcasecmp(parameter, "TRACK_TRANSPOSE") == 0 ) {
	    seq_hwcfg_led.track_transpose = dout_value;
	  } else if( strcasecmp(parameter, "FX") == 0 ) {
	    seq_hwcfg_led.fx = dout_value;
          } else if( strcasecmp(parameter, "MUTE_ALL_TRACKS") == 0 ) {
            seq_hwcfg_led.mute_all_tracks = dout_value;
          } else if( strcasecmp(parameter, "MUTE_TRACK_LAYERS") == 0 ) {
            seq_hwcfg_led.mute_track_layers = dout_value;
          } else if( strcasecmp(parameter, "MUTE_ALL_TRACKS_AND_LAYERS") == 0 ) {
            seq_hwcfg_led.mute_all_tracks_and_layers = dout_value;
          } else if( strcasecmp(parameter, "UNMUTE_ALL_TRACKS") == 0 ) {
            seq_hwcfg_led.unmute_all_tracks = dout_value;
          } else if( strcasecmp(parameter, "UNMUTE_TRACK_LAYERS") == 0 ) {
            seq_hwcfg_led.unmute_track_layers = dout_value;
          } else if( strcasecmp(parameter, "UNMUTE_ALL_TRACKS_AND_LAYERS") == 0 ) {
            seq_hwcfg_led.unmute_all_tracks_and_layers = dout_value;
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

	  if( strcasecmp(parameter, "DATAWHEEL_FAST_SPEED") == 0 ) {
	    seq_hwcfg_enc.datawheel_fast_speed = sr;
	    continue;
	  }
	  if( strcasecmp(parameter, "BPM_FAST_SPEED") == 0 ) {
	    seq_hwcfg_enc.bpm_fast_speed = sr;
	    continue;
	  }
	  if( strcasecmp(parameter, "GP_FAST_SPEED") == 0 ) {
	    seq_hwcfg_enc.gp_fast_speed = sr;
	    continue;
	  }
	  if( strcasecmp(parameter, "AUTO_FAST") == 0 ) {
	    seq_hwcfg_enc.auto_fast = sr;
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

	  if( strcasecmp(parameter, "DATAWHEEL") == 0 ) {
	    MIOS32_ENC_ConfigSet(0, enc_config);
	  } else if( strcasecmp(parameter, "BPM") == 0 ) {
	    MIOS32_ENC_ConfigSet(17, enc_config);
	  } else if( strncasecmp(parameter, "GP", 2) == 0 ) {
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
	// TRACKS_DOUT_[LR]_SR
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strcasecmp(parameter, "TRACKS_DOUT_L_SR") == 0 || strcasecmp(parameter, "TRACKS_DOUT_R_SR") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }
	  if( strcasecmp(parameter, "TRACKS_DOUT_L_SR") == 0 ) {
	    seq_hwcfg_led.tracks_dout_l_sr = sr;
	  } else {
	    seq_hwcfg_led.tracks_dout_r_sr = sr;
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
	// GP_DOUT_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "GP_DOUT_", 8) == 0 ) {
	  parameter += 8;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 sr = get_sr(word);
	  if( sr < 0 || sr > (32+SEQ_BLM8X8_NUM*8) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in GP_DOUT_%s definition: invalid SR value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] GP_DOUT_%s: SR %d", parameter, sr);
#endif

	  if( strcasecmp(parameter, "L_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_l_sr = sr;
	  } else if( strcasecmp(parameter, "R_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_r_sr = sr;
	  } else if( strcasecmp(parameter, "L2_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_l2_sr = sr;
	  } else if( strcasecmp(parameter, "R2_SR") == 0 ) {
	    seq_hwcfg_led.gp_dout_r2_sr = sr;
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown GP_DOUT_* name '%s'!", parameter);
#endif
	  }


	////////////////////////////////////////////////////////////////////////////////////////////
	// BLM_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BLM_", 4) == 0 ) {
	  parameter += 4;

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 value = get_dec(word);
	  if( value < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in BLM_%s definition: invalid value '%s'!", parameter, word);
#endif
	    continue;
	  }

#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_FILE_HW] BLM_%s: %d", parameter, value);
#endif

	  if( strcasecmp(parameter, "ENABLED") == 0 ) {
	    seq_hwcfg_blm.enabled = value;
	  } else if( strcasecmp(parameter, "DOUT_L1_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_l1_sr = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_R1_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_r1_sr = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_CATHODES_SR1") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_cathodes_sr1 = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_CATHODES_SR2") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_cathodes_sr2 = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_CATHODES_INV_MASK") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.cathodes_inv_mask = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_DUOCOLOUR") == 0 ) {
	    seq_hwcfg_blm.dout_duocolour = value;
	  } else if( strcasecmp(parameter, "DOUT_L2_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_l2_sr = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DOUT_R2_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.dout_r2_sr = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "BUTTONS_ENABLED") == 0 ) {
	    seq_hwcfg_blm.buttons_enabled = value;
	  } else if( strcasecmp(parameter, "BUTTONS_NO_UI") == 0 ) {
	    seq_hwcfg_blm.buttons_no_ui = value;
	  } else if( strcasecmp(parameter, "GP_ALWAYS_SELECT_MENU_PAGE") == 0 ) {
	    seq_hwcfg_blm.gp_always_select_menu_page = value;
	  } else if( strcasecmp(parameter, "DIN_L_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.din_l_sr = value;
	    BLM_ConfigSet(config);
	  } else if( strcasecmp(parameter, "DIN_R_SR") == 0 ) {
	    blm_config_t config = BLM_ConfigGet();
	    config.din_r_sr = value;
	    BLM_ConfigSet(config);

	  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR: unknown BLM_* name '%s'!", parameter);
#endif
	  }

#if !defined(SEQ_DONT_USE_BLM8X8)
	////////////////////////////////////////////////////////////////////////////////////////////
	// BLM8X8_
	////////////////////////////////////////////////////////////////////////////////////////////
	} else if( strncasecmp(parameter, "BLM8X8_", 7) == 0 ||
		   strncasecmp(parameter, "BLM8X8A_", 8) == 0 ||
		   strncasecmp(parameter, "BLM8X8B_", 8) == 0 ||
		   strncasecmp(parameter, "BLM8X8C_", 8) == 0 ) {
	  int blm = 0;
	  if( parameter[6] >= 'A' && parameter[6] <= 'C' ) {
	    blm = parameter[6] - 'A';
	    parameter += 8;
	  } else {
	    parameter += 7;
	  }


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

	  if( blm == 0 && strcasecmp(parameter, "ENABLED") == 0 ) {
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
	  } else if( blm == 0 && strcasecmp(parameter, "DOUT_GP_MAPPING") == 0 ) {
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
#endif

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
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "MIDI_REMOTE_CC") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "TRACK_CC_MODE") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "TRACK_CC_PORT") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "TRACK_CC_CHANNEL") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "TRACK_CC_NUMBER") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "RS_OPTIMISATION") == 0 ) {
	  // obsolete -- ignore
	  // now configurable in Options page

	} else if( strcasecmp(parameter, "DEBOUNCE_DELAY") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 delay = get_dec(word);
	  if( delay < 0 || delay >= 128 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid delay value '%s'!", parameter, word);
#endif
	    continue;
	  }

	  // common DINs
	  MIOS32_SRIO_DebounceSet(delay);
#if !defined(SEQ_DONT_USE_BLM8X8)
	  // SEQ_BLM8X8 based DINs
	  {
	    int blm;

	    for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm) {
	      seq_blm8x8_config_t config = SEQ_BLM8X8_ConfigGet(blm);
	      config.debounce_delay = delay;
	      SEQ_BLM8X8_ConfigSet(blm, config);
	    }
	  }
#endif
#if !defined(MIOS32_DONT_USE_AOUT)
	} else if( strcasecmp(parameter, "AOUT_INTERFACE_TYPE") == 0 ) {
	  // only for compatibility reasons - AOUT interface is stored in MBSEQ_GC.V4 now!
	  // can be removed once most users switched to beta28 and later!

	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 aout_type = get_dec(word);
	  if( aout_type < 0 || aout_type >= 4 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: invalid AOUT interface type '%s'!", parameter, word);
#endif
	    continue;
	  }

	  SEQ_CV_IfSet(aout_type);
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
#endif
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

#if !defined(MIOS32_DONT_USE_BOARD_J5)
	} else if( strcasecmp(parameter, "J5_ENABLED") == 0 ) {
	  char *word = strtok_r(NULL, separators, &brkt);
	  s32 j5_enabled = get_dec(word);
	  if( j5_enabled < 0 || j5_enabled > 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[SEQ_FILE_HW] ERROR in %s definition: expecting 0, 1 or 2!", parameter);
#endif
	    continue;
	  }

	  // copy to global variable (used in seq_cv)
	  seq_hwcfg_j5_enabled = j5_enabled;

	  if( j5_enabled ) {
	    int i;
	    mios32_board_pin_mode_t pin_mode = MIOS32_BOARD_PIN_MODE_INPUT_PD;
	    if( j5_enabled == 1 )
	      pin_mode = MIOS32_BOARD_PIN_MODE_OUTPUT_PP;
	    if( j5_enabled == 2 )
	      pin_mode = MIOS32_BOARD_PIN_MODE_OUTPUT_OD;

	    for(i=0; i<6; ++i) {
	      MIOS32_BOARD_J5_PinInit(i, pin_mode);
	      MIOS32_BOARD_J5_PinSet(i, 0);
	    }

#if defined(MIOS32_FAMILY_STM32F10x)
	    // pin J5.A6 and J5.A7 used for UART2 (-> MIDI OUT3)
	    for(i=8; i<12; ++i) {
	      MIOS32_BOARD_J5_PinInit(i, pin_mode);
	      MIOS32_BOARD_J5_PinSet(i, 0);
	    }
#elif defined(MIOS32_FAMILY_STM32F4xx)
	    // pin J5.A6 and J5.A7 used as gates
	    for(i=6; i<8; ++i) {
	      MIOS32_BOARD_J5_PinInit(i, pin_mode);
	      MIOS32_BOARD_J5_PinSet(i, 0);
	    }
#if !defined(MIOS32_DONT_USE_BOARD_J10)
	    // and J10B for additional outputs
	    for(i=8; i<16; ++i) {
	      MIOS32_BOARD_J10_PinInit(i, pin_mode);
	      MIOS32_BOARD_J10_PinSet(i, 0);
	    }
#endif
#elif defined(MIOS32_FAMILY_LPC17xx)
#if !defined(MIOS32_DONT_USE_BOARD_J28)
	    // and pin J28 for additional outputs
	    for(i=0; i<4; ++i) {
	      MIOS32_BOARD_J28_PinInit(i, pin_mode);
	      MIOS32_BOARD_J28_PinSet(i, 0);
	    }
#endif
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
	  }
#endif

#if !defined(MIOS32_DONT_USE_AOUT)
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
#endif
	} else if( strcasecmp(parameter, "DOUT_1MS_TRIGGER") == 0 ) {

	  // obsolete - now configured in CV menu - ignore!

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
