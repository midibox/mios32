/*
 *  ACHandTracker.h
 *  kII
 *
 *  Created by audiocommander on 14.12.06.
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 *	
 *	This sensor matrix fetches the readings of 4 GP2D-Sharp GP2D120X distance sensors
 *	to calculate additional vars. The matrix is set up by 2 top and 2 bottom sensors
 *  If you put your hand in this space to simulate a hand-talking puppet following
 *	infos are accessible
 *	TL			top left	 sensor 0 value										0..20 cm
 *	TR			top right	 sensor 1 value										0..20 cm
 *	BL			bottom left	 sensor 2 value										0..20 cm
 *	BR			bottom right sensor 3 value										0..20 cm
 *	FINGER		position of fingers (height top)								0..20 cm
 *	THUMB		position of thumb (height bottom)								0..20 cm
 *	OPEN		opening distance, space between fingers & thumb					0..14 cm
 *	HEIGHT		heigt of whole hand												15..95 Hz
 *	ROLL		angle of hand, roll left (0..1) or right (3..4), default 2		0..5  (2)
 *	//BEND		  not yet available --- (0..15, default 5)						0..15 (5)
 *
 *	If a hand-opening is detected, phonemes are triggered based on the phonetic topic
 *	tables of the IIC_SpeakJet classes.
 *
 *	To make it less abstract:
 *  A Cocoa (Objective-C) demonstration program for Mac OS X (maybe it works with GNU-Step
 *  on Linux too, who know ;) This program emulates the sensors and calculates the virtual
 *  open beam. All number-chrunching is visualized there. Frankly, I think the gesture
 *  implementation so far is quite easy to grasp if you see what's going on at this demo.
 *
 *
 */

/*
 * Released under GNU General Public License
 * http://www.gnu.org/licenses/gpl.html
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation
 *
 * YOU ARE ALLOWED TO COPY AND CHANGE 
 * BUT YOU MUST RELEASE THE SOURCE TOO (UNDER GNU GPL) IF YOU RELEASE YOUR PRODUCT 
 * YOU ARE NOT ALLOWED TO USE IT WITHIN PROPRIETARY CLOSED-SOURCE PROJECTS
 *
 */


#ifndef _ACHANDTRACKER_H
#define _ACHANDTRACKER_H


#include "ACToolbox.h"



// ***** DEFINES *****

// general options
// if SYNC_ENABLED == 1 ACHandTracker_Update() is polled by notelength from the Syncronizer
// if SYNC_ENABLED == 0 ACHandTracker_Update() is polled each TRACKER_THRESHOLD times by AIN_NotifyChange()
#define SYNC_ENABLED				KII_SYNC_ENABLED	// defined in main.h

// additional OSC and ENV frequencies will be sent
// based on main.h: unsigned char channelAssignment
#define EXTENDED_HT_MODE			1		// default: 1, see IIC_SpeakJetMidiDefines.h

// do not change this:
#define AIN_MUXED					1		// 32 AINs via AIN-module
#define AIN_UNMUXED					0		// 8 AINs via J5, default setting
// ...instead select here the MUX-Type:
#define AIN_MUX						AIN_UNMUXED

#define AIN_NUM						4		// number of Analog Inputs, default: 4
#define SENSOR_NUM					AIN_NUM
#define AIN_DEADBAND				7		// 10-bit: 0 // 9-bit:1 // 8-bit:3 // 7-bit:7 // 6-bit:15	

// sensor matrix hardware
#define HT_TL						0		// AIN pin num of top left sensor
#define HT_TR						1		// AIN pin num of top right sensor
#define HT_BL						2		// AIN pin num of bottom left sensor
#define HT_BR						3		// AIN pin num of bottom right sensor

// sensor config							// use 16bit division math; best linearized results
#define USE_COMPLEX_LINEARIZING		1		// if disabled, uses lookup table (not recommended, only GP2D120X supported!)
#define GP2D120X					0x30	//  4..30cm Sharp Distance Sensor
#define GP2D12J0					0x80	// 10..80cm Sharp Distance Sensor
#define HT_SENSORTYPE				GP2D120X	// which sensor type is used
// note that a sensorType other than GP2D120X has never been tested; 
// it's probably better to leave GP2D120X even when working with a GP2D12J0, 
// because all calculations are optimized for 0..26 cm; as the voltage is relative, this should be no problem...
#if HT_SENSORTYPE == GP2D120X
	#define COMPLEX_LINEAR_K		2914	// constant k for linearizing formula for GP2D120X sensor ( 4-30 cm)
#else
	#define COMPLEX_LINEAR_K		6787	// constant k for linearizing formula for GP2D12J0 sensor (10-80 cm)
#endif

#if KIII_MODE
	#define SMIN					38		// minimum value (10bit) for reading <default: 46>
	#define SMAX					655		// maximum value (10bit) for reading <default: 655>
#else
	#define SMIN					117		// minimum value (10bit) for reading <default: 117>
	#define SMAX					655		// maximum value (10bit) for reading <default: 655>
#endif
#define SENSOR_SLOWDOWN				4		// every nth signal will be processed (per sensor), only if not quantized

// gesture detection
#define GESTURE_SLOWDOWN			4		// every nth update will be processed (unrelevant if SYNC_ENABLED!)
#define TRACKER_THRESHOLD			10		// every nth update will be requested (unrelevant if SYNC_ENABLED!)
#define SMEAR_VOWEL					126		// used for enunciate, see SJ_MidiDefines for SJCH_xx typedefs

// midi related
#define SENSORIZED_NOTE_VELOCITY	0		// note velocity for sensorized NOTE_ONs; 0 for lastPhoneme (default: 100)
#define HANDTRACKER_CC_OUTPUT		1		// if tracked values should be sent by midi
#define HANDTRACKER_CC_OUTPUT_VERBOSE 0		// if additional values like AIN values should be sent by midi/CC

// don't change
#define LAST_QUEUE_MAX				2		// number of history states (eg lastOpen[x])


// ***** TYPEDEFS *****

typedef union {
	struct {
		unsigned ALL				:8;
	};
	struct {
		unsigned GESTURE_UPDATE_REQ	:1;		// update gesture only if AIN changed
		unsigned UPDATE_GESTURE		:1;		// true if gesture detected
		unsigned UPDATE_BEND		:1;		// true if bend detected
		unsigned UPDATE_SPEED		:1;		// true if open speed difference detected
		unsigned free				:4;
	};
} ht_state_t;								// used in update() & ain_notify()
	
	
typedef struct {							// range_
	unsigned char	FINGER;					//  0..20
	unsigned char	THUMB;					//  0..20
	unsigned char	OPEN;					//  0..14
	unsigned char	HEIGHT;					// 15..95
	unsigned char	ROLL;					//  0..4
	unsigned char	SPEED;					// 80..127
	unsigned char	BEND;					//  0..15
} ht_t;



#if KII_AIN_ENABLED


// ***** GLOBALS *****

extern volatile ht_state_t		ht_state;				// handTracker settings
extern volatile ht_t			ht;						// handTracker status vars

extern volatile unsigned char	lastAinPin;				// last pin
extern volatile unsigned char	lastValue[SENSOR_NUM];	// last 7bit values (per sensor)
extern volatile unsigned char	lastOpen[(LAST_QUEUE_MAX + 1)];	// last open state, to detect gestures

#ifdef _DEBUG_C
extern volatile unsigned char	lastRoll;				// last roll value
extern volatile unsigned char	lastEnunciate;			// last enunciation type, eg SJCH_CONSONANTS_OPEN
extern volatile unsigned char	lastPhonemeType;		// contains last spoken phoneme type, eg PHONEME_VOWEL
extern volatile unsigned char	lastPhoneme;			// contains last spoken phoneme, eg MSAPH_AX
#endif

// ***** PROTOTYPES *****

extern void ACHandTracker_Init(void);
extern void ACHandTracker_Update(void);					// updates all values incl. gesture detection
extern void ACHandTracker_Mute(void);
extern void ACHandTracker_AIN_NotifyChange(unsigned char pin, unsigned int pin_value);


#endif /* KII_AIN_ENABLED */

#endif /* _ACHANDTRACKER_H */

