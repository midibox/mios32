// $Id$
/*
 * MIDIbox MM V3
 * Hardware Configuration Layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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

#include "app.h"
#include "mm_hwcfg.h"

/////////////////////////////////////////////////////////////////////////////
// Verbose Level (can be changed with "set debug <on|off>"
// 0: off
// 1: print errors (recommended default)
// 2: print infos
/////////////////////////////////////////////////////////////////////////////
u8 mm_hwcfg_verbose_level = 1;


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////

// define the encoder speed modes and dividers here
#define ENC_VPOT_SPEED_MODE        FAST // SLOW/NORMAL/FAST
#define ENC_VPOT_SPEED_PAR         1    // see description about MIOS32_ENC_ConfigSet

#define ENC_JOGWHEEL_SPEED_MODE    FAST // SLOW/NORMAL/FAST
#define ENC_JOGWHEEL_SPEED_PAR     1    // see description about MIOS32_ENC_ConfigSet

mios32_enc_config_t mm_hwcfg_encoders[MM_HWCFG_ENC_NUM] = {
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_JOGWHEEL_SPEED_MODE, .cfg.speed_par=ENC_JOGWHEEL_SPEED_PAR, .cfg.sr=15, .cfg.pos=0 },

  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=6 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=6 },
};


/////////////////////////////////////////////////////////////////////////////
// define the MIDI channel to which this MM listens and transmits here:
// (Allowed values: 1-16)
/////////////////////////////////////////////////////////////////////////////
u8 mm_hwcfg_midi_channel = 1;


/////////////////////////////////////////////////////////////////////////////
// used by mm_dio.c
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_mm_ledrings_meters.pdf
// NOTE: the shift registers are counted from 1 here, means: 1 is the first shift register, 2 the second...
// 0 disables the shift register
// NOTE2: the MIDIbox MM protocol doesn't support meters!
/////////////////////////////////////////////////////////////////////////////
mm_hwcfg_ledrings_t mm_hwcfg_ledrings = {
  .cathodes_ledrings_sr =  9, // shift register with cathodes of the 8 LED rings
  .cathodes_meters_sr   =  0, // shift register with cathodes of the 8 meters
  .anodes_sr1           = 11, // first shift register with anodes of the 8 LED rings (and 8 meters)
  .anodes_sr2           = 12, // second shift register with anodes of the 8 LED rings (and 8 meters)
};


/////////////////////////////////////////////////////////////////////////////
// used by mm_leddigits.c
// define the two shift registers to which the status digits are connected here.
// the wiring is similar to http://www.ucapps.de/midibox_lc/midibox_lc_leddigits.pdf
// BUT: there are no select lines! The anode has to be connected directly to +5V
/////////////////////////////////////////////////////////////////////////////
mm_hwcfg_leddigits_t mm_hwcfg_leddigits = {
  .segment_a_sr = 13, // the shift register of the first digit
  .segment_b_sr = 14, // the shift register of the second digit
};


/////////////////////////////////////////////////////////////////////////////
// the GPC (General Purpose Controller) feature can be activated by using ID_MBMM_*GPC* buttons
// up to 128 midi events are defined in mios_wrapper/mios_tables.inc
// the lables are defined in mm_gpc_lables.c
// optionally a MIDI event will be sent on any SFB action which notifies if GPC is enabled or not
/////////////////////////////////////////////////////////////////////////////
mm_hwcfg_gpc_t mm_hwcfg_gpc = {
  .enabled = 0,
  .on_evnt = {0x9f, 0x3c, 0x7f},
  .off_evnt = {0x9f, 0x3c, 0x00}
};


/////////////////////////////////////////////////////////////////////////////
// This function initializes the HW config layer
/////////////////////////////////////////////////////////////////////////////
s32 MM_HWCFG_Init(u32 mode)
{
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
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 || strcmp(word, "1") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 || strcmp(word, "0") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 MM_HWCFG_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  set debug <on|off>:               enables/disables debug mode (not stored in EEPROM)");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 MM_HWCFG_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  int input_len = strlen(input);

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "set") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Missing parameter after 'set'!");
	return 1; // command taken
      }

      if( strcmp(parameter, "xxx") == 0 ) {
	s32 sr = -1;
	if( (parameter = strtok_r(NULL, separators, &brkt)) )
	  sr = get_dec(parameter);

	if( sr < 1 || sr > MIOS32_SRIO_ScanNumGet() ) {
	  out("Expecting SR number from 1..%d", MIOS32_SRIO_ScanNumGet());
	  return 1; // command taken
	}

	return 1; // command taken

      } else if( strcmp(parameter, "debug") == 0 ) {
	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Please specify on or off (alternatively 1 or 0)");
	  return 1; // command taken
	}

	int on_off = get_on_off(parameter);

	if( on_off < 0 ) {
	  out("Expecting 'on' or 'off' (alternatively 1 or 0)!");
	} else {
	  mm_hwcfg_verbose_level = on_off ? 2 : 1;

	  out("Debug mode %s", on_off ? "enabled" : "disabled");
	}
	return 1; // command taken
      } else {
	// out("Unknown command - type 'help' to list available commands!");
      }
    }
  }

  // restore input line (replace NUL characters by spaces)
  int i;
  char *input_ptr = input;
  for(i=0; i<input_len; ++i, ++input_ptr)
    if( !*input_ptr )
      *input_ptr = ' ';

  return 0; // command not taken
}


/////////////////////////////////////////////////////////////////////////////
// Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 MM_HWCFG_TerminalPrintConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Debug Messages: %s", (mm_hwcfg_verbose_level >= 2) ? "on" : "off");

  return 0; // no error
}
