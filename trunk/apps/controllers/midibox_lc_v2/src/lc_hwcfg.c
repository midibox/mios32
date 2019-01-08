// $Id$
/*
 * MIDIbox LC V2
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
#include "lc_hwcfg.h"

/////////////////////////////////////////////////////////////////////////////
// Verbose Level (can be changed with "set debug <on|off>"
// 0: off
// 1: print errors (recommended default)
// 2: print infos
/////////////////////////////////////////////////////////////////////////////
u8 lc_hwcfg_verbose_level = 1;


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////

// define the encoder speed modes and dividers here
#define ENC_VPOT_SPEED_MODE        FAST // SLOW/NORMAL/FAST
#define ENC_VPOT_SPEED_PAR         1    // see description about MIOS32_ENC_ConfigSet

#define ENC_JOGWHEEL_SPEED_MODE    FAST // SLOW/NORMAL/FAST
#define ENC_JOGWHEEL_SPEED_PAR     1    // see description about MIOS32_ENC_ConfigSet

// (only relevant for MIDIbox SEQ hardware option)
#define ENC_VPOTFADER_SPEED_MODE   FAST // SLOW/NORMAL/FAST
#define ENC_VPOTFADER_SPEED_PAR    2    // see description about MIOS32_ENC_ConfigSet

mios32_enc_config_t lc_hwcfg_encoders[LC_HWCFG_ENC_NUM] = {
#if MBSEQ_HARDWARE_OPTION
  { .cfg.type=DETENTED2, .cfg.speed=ENC_JOGWHEEL_SPEED_MODE, .cfg.speed_par=ENC_JOGWHEEL_SPEED_PAR, .cfg.sr=1, .cfg.pos=0 },

  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=5, .cfg.pos=6 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=6, .cfg.pos=6 },

  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=8, .cfg.pos=6 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=0 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=2 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=4 },
  { .cfg.type=DETENTED3, .cfg.speed=ENC_VPOTFADER_SPEED_MODE, .cfg.speed_par=ENC_VPOTFADER_SPEED_PAR, .cfg.sr=9, .cfg.pos=6 },

#else
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_JOGWHEEL_SPEED_MODE, .cfg.speed_par=ENC_JOGWHEEL_SPEED_PAR, .cfg.sr=15, .cfg.pos=0 },

  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=13, .cfg.pos=6 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=ENC_VPOT_SPEED_MODE, .cfg.speed_par=ENC_VPOT_SPEED_PAR, .cfg.sr=14, .cfg.pos=6 },
#endif
};


/////////////////////////////////////////////////////////////////////////////
// use 0x10 for Logic Control
//     0x11 for Logic Control XT
//     0x14 for Mackie Control
//     0x15 for Mackie Control XT (?)
//     0x80: Auto ID LC/MC
//     0x81: Auto ID LC/MC XT
/////////////////////////////////////////////////////////////////////////////
u8 lc_hwcfg_emulation_id = 0x80;


/////////////////////////////////////////////////////////////////////////////
// Display Config
/////////////////////////////////////////////////////////////////////////////
lc_hwcfg_display_cfg_t lc_hwcfg_display_cfg = {
  .initial_page_clcd = 0, // initial display page after startup (choose your favourite one: 0-3)
  .initial_page_glcd = 3, // initial display page after startup (choose your favourite one: 0-3)
};


/////////////////////////////////////////////////////////////////////////////
// used by lc_dio.c
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_lc_ledrings_meters.pdf
// NOTE: the shift registers are counted from 1 here, means: 1 is the first shift register, 2 the second...
// 0 disables the shift register
// NOTE2: it's possible to display the meter values with the LEDrings by using ID_MBLC_*LEDMETER* buttons!
// this feature saves you from adding additional LEDs to your MIDIbox
/////////////////////////////////////////////////////////////////////////////
lc_hwcfg_ledrings_t lc_hwcfg_ledrings = {
  .cathodes_ledrings_sr =  9, // shift register with cathodes of the 8 LED rings
  .cathodes_meters_sr   = 10, // shift register with cathodes of the 8 meters
  .anodes_sr1           = 11, // first shift register with anodes of the 8 LED rings (and 8 meters)
  .anodes_sr2           = 12, // second shift register with anodes of the 8 LED rings (and 8 meters)
};


/////////////////////////////////////////////////////////////////////////////
// used by lc_leddigits.c
// define the two shift registers to which the status digits are connected here.
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_lc_leddigits.pdf
// NOTE: the shift registers are counted from 1 here, means: 1 is the first shift register, 2 the second...
// 0 disables the shift register
// NOTE2: in principle this driver supports up to 16 LED digits, but only 12 of them are used
/////////////////////////////////////////////////////////////////////////////
lc_hwcfg_leddigits_t lc_hwcfg_leddigits = {
  .common_anode = 1,  // 1: common anode, 0: common cathode
  .segments_sr1 = 13, // shift register which drives the segments of digit 7-0 (right side)
  .select_sr1   = 14, // shift register which selects the digits 7-0
  .segments_sr2 = 15, // shift register which drives the segments of digit 15-8 (left side)
  .select_sr2   = 16, // shift register which selects the digits 15-8
};


/////////////////////////////////////////////////////////////////////////////
// the GPC (General Purpose Controller) feature can be activated by using ID_MBLC_*GPC* buttons
// up to 128 midi events are defined in mios_wrapper/mios_tables.inc
// the lables are defined in lc_gpc_lables.c
// optionally a MIDI event will be sent on any SFB action which notifies if GPC is enabled or not
/////////////////////////////////////////////////////////////////////////////
lc_hwcfg_gpc_t lc_hwcfg_gpc = {
  .enabled = 0,
  .on_evnt = {0x9f, 0x3c, 0x7f},
  .off_evnt = {0x9f, 0x3c, 0x00}
};


/////////////////////////////////////////////////////////////////////////////
// This function initializes the HW config layer
/////////////////////////////////////////////////////////////////////////////
s32 LC_HWCFG_Init(u32 mode)
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
s32 LC_HWCFG_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  set debug <on|off>:               enables/disables debug mode (not stored in EEPROM)");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 LC_HWCFG_TerminalParseLine(char *input, void *_output_function)
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
	  lc_hwcfg_verbose_level = on_off ? 2 : 1;

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
s32 LC_HWCFG_TerminalPrintConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Debug Messages: %s", (lc_hwcfg_verbose_level >= 2) ? "on" : "off");

  return 0; // no error
}
