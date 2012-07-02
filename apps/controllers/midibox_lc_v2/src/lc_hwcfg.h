// $Id$
/*
 * Header File for Hardware Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LC_HWCFG_H
#define _LC_HWCFG_H


/////////////////////////////////////////////////////////////////////////////
// This defition is normaly specified in make.bat to switch between 
// the common "MIDIbox LC" hardware, and the "MIDIbox SEQ" hardware
/////////////////////////////////////////////////////////////////////////////
#ifndef MBSEQ_HARDWARE_OPTION
#define MBSEQ_HARDWARE_OPTION 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define LC_EMULATION_ID	0x80		// use 0x10 for Logic Control
					//     0x11 for Logic Control XT
					//     0x14 for Mackie Control
					//     0x15 for Mackie Control XT (?)
                                        //     0x80: Auto ID LC/MC
                                        //     0x81: Auto ID LC/MC XT

// note: if ID >= 0x80, MIDIbox automatically sets the ID to the same like the
// first incoming LC/MC sysex command
// The taken ID can only be reset with the MIOS reset function or with power-off/power-on

#define LCD_EMU_COL     55		// number of emulated columns (characters per line)
					//    o one 2x40 LCD: use 40   (unfortunately..)
					//    o two 2x40 LCDs: use 55  (like a Logic Control)

#define INITIAL_DISPLAY_PAGE_CLCD  0	// initial display page after startup (choose your favourite one: 0-3)
#define INITIAL_DISPLAY_PAGE_GLCD  3	// initial display page after startup (choose your favourite one: 0-3)


// debounce counter (see the function description of MIOS_SRIO_DebounceSet)
// Use 0 for high-quality buttons, use higher values for low-quality buttons
#define SRIO_DEBOUNCE_CTR	20

#define NUMBER_SHIFTREGISTERS	16	// length of shift register chain (max value: 16 == 128 DIN/DOUT pins)
#define SRIO_UPDATE_FREQUENCY	1	// we are using rotary encoders: use 1 ms!


// define number of encoders
#if MBSEQ_HARDWARE_OPTION
# define ENC_NUM 17
#else
# define ENC_NUM 9
#endif

// define the encoder speed modes and dividers here
#define ENC_VPOT_SPEED_MODE        FAST // SLOW/NORMAL/FAST
#define ENC_VPOT_SPEED_PAR         1    // see description about MIOS32_ENC_ConfigSet

#define ENC_JOGWHEEL_SPEED_MODE    FAST // SLOW/NORMAL/FAST
#define ENC_JOGWHEEL_SPEED_PAR     1    // see description about MIOS32_ENC_ConfigSet

// (only relevant for MIDIbox SEQ hardware option)
#define ENC_VPOTFADER_SPEED_MODE   FAST // SLOW/NORMAL/FAST
#define ENC_VPOTFADER_SPEED_PAR    2    // see description about MIOS32_ENC_ConfigSet


// used by lc_dio.c
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_lc_ledrings_meters.pdf
// NOTE: the shift registers are counted from 1 here, means: 1 is the first shift register, 2 the second...
// 0 disables the shift register
// NOTE2: it's possible to display the meter values with the LEDrings by using ID_MBLC_*LEDMETER* buttons!
// this feature saves you from adding additional LEDs to your MIDIbox
#define LEDRINGS_CATHODES_SR	    9	// shift register with cathodes of the 8 LED rings
#define METERS_CATHODES_SR	   10	// shift register with cathodes of the 8 meters
#define LEDRINGS_METERS_ANODES_SR1 11	// first shift register with anodes of the 8 LED rings (and 8 meters)
#define LEDRINGS_METERS_ANODES_SR2 12	// second shift register with anodes of the 8 LED rings (and 8 meters)


// used by lc_leddigits.c
// define the two shift registers to which the status digits are connected here.
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_lc_leddigits.pdf
// NOTE: the shift registers are counted from 1 here, means: 1 is the first shift register, 2 the second...
// 0 disables the shift register
// NOTE2: in principle this driver supports up to 16 LED digits, but only 12 of them are used
#define LEDDIGITS_SEGMENTS_SR1		13	// shift register which drives the segments of digit 7-0 (right side)
#define LEDDIGITS_SELECT_SR1		14	// shift register which selects the digits 7-0
#define LEDDIGITS_SEGMENTS_SR2		15	// shift register which drives the segments of digit 15-8 (left side)
#define LEDDIGITS_SELECT_SR2		16	// shift register which selects the digits 15-8

// common anode or cathode digits?
#define LEDDIGITS_COMMON_ANODE           1       // 1: common anode, 0: common cathode


// the GPC (General Purpose Controller) feature can be activated by using ID_MBLC_*GPC* buttons
// up to 128 midi events are defined in mios_wrapper/mios_tables.inc
// the lables are defined in lc_gpc_lables.c
// optionally a MIDI event will be sent on any SFB action which notifies if GPC is enabled or not
#define GPC_BUTTON_SENDS_MIDI_EVENT	0	// enables the "send GPC status" feature

#define GPC_BUTTON_ON_EVNT0		0x9f	// on event, byte #0
#define GPC_BUTTON_ON_EVNT1		0x3c	// on event, byte #1
#define GPC_BUTTON_ON_EVNT2		0x7f	// on event, byte #2

#define GPC_BUTTON_OFF_EVNT0		0x9f	// off event, byte #0
#define GPC_BUTTON_OFF_EVNT1		0x3c	// off event, byte #1
#define GPC_BUTTON_OFF_EVNT2		0x00	// off event, byte #2


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_HWCFG_Init(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

extern const mios32_enc_config_t lc_hwcfg_encoders[ENC_NUM];


#endif /* _LC_HWCFG_H */
