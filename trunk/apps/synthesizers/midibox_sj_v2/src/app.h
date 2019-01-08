// $Id$
/*
 * Header file of application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Michael Markert (http://www.audiocommander.de) & Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _APP_H
#define _APP_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// general defines
#define FALSE						0
#define TRUE						1


// application defines
#define KII_AIN_ENABLED				0	// if AIN (sensorMatrix) enabled, default: 1
#define KII_LCD_ENABLED				1	// if LCD is attached, default: 1
#define KII_SYNC_ENABLED			1	// if SYNC features (MASTER/SLAVE) are enabled
#define KII_WELCOME_ENABLED			1	// enunciates welcome message on startup, default: 1
// note: KII_RECORDER_ENA is an experimental mode. Do not set this to 1 unless you are aware it might damage your SpeakJet!
#define KII_RECORDER_ENA			0	// if recorder module (experimental) active, default: 0

#define KIII_MODE					0	// for use with Mac OS kIII software only.

// include MIDI Assignments
#include "IIC_SpeakJetMidiDefines.h"

// LCD 
#define KII_LCD_TYPE				28	// currently supported: 216 & 28 <default>

// LCD special chars
#define LCD_SPECIALCHAR_HAND_OPEN	0xC2
#define LCD_SPECIALCHAR_HAND_CLOSE	0xAF


// ********* TYPEDEFS ***************** //

typedef struct {
	unsigned	displayNeedsInit	: 1;	// init is called next LCD_Display_Tick()
	unsigned	displayNeedsUpdate	: 1;	// scheduled for next LCD_Display_Tick()
	unsigned	display_IIC_Active  : 1;	// display IIC transmission
	unsigned	display_Harmony		: 1;
	unsigned	display_Gesture		: 1;
	unsigned	display_Waveform	: 1;
	unsigned	empty				: 2;	//
} acapp_t;



// ********* EXPORT GLOBALS *********** //

extern acapp_t acapp;
extern volatile unsigned char	channelAssignment;	// if incoming channel should be mapped to {value}, 0..15


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _APP_H */
