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

#if MBSEQ_HARDWARE_OPTION
# define LCD_USE_GLCD    0              // 0: CLCD, 1: GLCD
#else
# define LCD_USE_GLCD    1              // 0: CLCD, 1: GLCD
#endif
#define LCD_EMU_COL     55		// number of emulated columns (characters per line)
					//    o one 2x40 LCD: use 40   (unfortunately..)
					//    o two 2x40 LCDs: use 55  (like a Logic Control)

#define INITIAL_DISPLAY_PAGE	3	// initial display page after startup (choose your favourite one: 0-3)


// debounce counter (see the function description of MIOS_SRIO_DebounceSet)
// Use 0 for high-quality buttons, use higher values for low-quality buttons
#define SRIO_DEBOUNCE_CTR	0

// motordriver settings
#if MBSEQ_HARDWARE_OPTION
#  define ENABLE_MOTORDRIVER	0	// if 0, MF module will not be enabled
#  define NUMBER_MOTORFADERS	0	// number of faders
#else
#  define ENABLE_MOTORDRIVER	1	// if 0, MF module will not be enabled
#  define NUMBER_MOTORFADERS	8	// number of faders
#endif

#define MOTORFADERS_DEADBAND	3	// so called "deadband" of motorfaders
#define AIN_DEADBAND		3	// so called "deadband" of ADC
#define MOTORFADERS_PWM_DUTY_UP	  0x01	// PWM duty cycle for upward moves   (see http://www.ucapps.de/mbhp_mf.html)
#define MOTORFADERS_PWM_DUTY_DOWN 0x01  // PWM duty cycle for downward moves (see http://www.ucapps.de/mbhp_mf.html)
#define MOTORFADERS_PWM_PERIOD    0x03	// PWM period                        (see http://www.ucapps.de/mbhp_mf.html)

#define MOTORFADER0_IS_MASTERFADER 0	// if set, the first motorfader acts as master fader

// following setting configures the touch sensor behaviour. The touch sensors of the motorfaders
// have to be assigned to the DIN pins in lc_dio_tables.c - the appr. IDs are:
// ID_FADER_TOUCH_CHN1, ID_FADER_TOUCH_CHN2, ... ID_FADER_TOUCH_CHN8 (8 sensors)
// If the master fader option is used, the ID is ID_FADER_TOUCH_MASTER
// 
// TOUCH_SENSOR_MODE 0: touch sensor events (pressed/depressed) are only forwarded to the host program
//                      like specified in the logic control specification (MIDI events)
// TOUCH_SENSOR_MODE 1: like mode 0, but additionaly the motors are suspended by MIOS, so that they
//                      are not moved so long as the touch sensor is pressed
// TOUCH_SENSOR_MODE 2: like mode 0+1, additionally no fader move will be reported to the host
//                      application (no MIDI event) so long the touch sensor is *not* pressed
// 
// Mode "1" selected by default to avoid circular troubleshooting requests in the MIDIbox forum from people
// who don't read this information before starting the application.
// Mode "2" should be the prefered setting if your touch sensors are working properly
#if MBSEQ_HARDWARE_OPTION
#  define TOUCH_SENSOR_MODE	0
#else
#  define TOUCH_SENSOR_MODE	0 // TODO: 1
#endif

#define NUMBER_SHIFTREGISTERS	16	// length of shift register chain (max value: 16 == 128 DIN/DOUT pins)
#define SRIO_UPDATE_FREQUENCY	1	// we are using rotary encoders: use 1 ms!
#define TOUCHSENSOR_SENSITIVITY 3	// for the motorfader touchsensors


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
// NOTE: it's possible to display the meter values with the LEDrings by using ID_MBLC_*LEDMETER* buttons!
// this feature saves you from adding additional LEDs to your MIDIbox
#if MBSEQ_HARDWARE_OPTION
#  define LEDRINGS_ENABLED	0	// if 1, ledrings are enabled
#  define METERS_ENABLED	0	// if 1, meters are enabled
#else
#  define LEDRINGS_ENABLED	1	// if 1, ledrings are enabled
#  define METERS_ENABLED	0	// if 1, meters are enabled
#endif

// NOTE2: the shift registers are counted from zero here, means: 0 is the first shift register, 1 the second...
#define LEDRINGS_SR_CATHODES	8	// shift register with cathodes of the 8 LED rings
#define METERS_SR_CATHODES	9	// shift register with cathodes of the 8 meters
#define LEDRINGS_METERS_SR_ANODES_1 10	// first shift register with anodes of the 8 LED rings (and 8 meters)
#define LEDRINGS_METERS_SR_ANODES_2 11	// second shift register with anodes of the 8 LED rings (and 8 meters)

// used by lc_leddigits.c
// define the two shift registers to which the status digits are connected here.
// the schematic can be found under http://www.ucapps.de/midibox_lc/midibox_lc_leddigits.pdf
// NOTE: in principle this driver supports up to 16 LED digits, but only 12 of them are used
#define LEDDIGITS_ENABLED		0	// if 1, leddigits are enabled
#define LEDDIGITS_SR_SEGMENTS_1		12	// shift register which drives the segments of digit 7-0 (right side)
#define LEDDIGITS_SR_SELECT_1		13	// shift register which selects the digits 7-0
#define LEDDIGITS_SR_SEGMENTS_2		14	// shift register which drives the segments of digit 15-8 (left side)
#define LEDDIGITS_SR_SELECT_2		15	// shift register which selects the digits 15-8


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
