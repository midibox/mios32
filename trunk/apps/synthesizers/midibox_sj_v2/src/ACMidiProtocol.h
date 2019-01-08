/*
 *  ACMidiProtocol.h
 *  mX series
 *
 *  Created by audiocommander on 08.03.2007
 *  Copyright 2006-2007 Michael Markert, http://www.audiocommander.de
 *
 *  v0.0.1	08.03.07	first version as one protocol document
 *  v0.0.2	29.03.07	added 4-char scale identifiers
 *	v0.0.3	01.05.07	added ACMidiProcessing <NSProtocol>
 *
 *
 *	description:
 *		MIDI Definition Listing for AC- & ZS-Applications
 *		Protocol Definitions for MIDI Controller Numbers
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
 */


#ifdef __OBJC__
	#pragma mark Cocoa ACMidiProtocol
	
#import <Foundation/Foundation.h>

@protocol ACMidiProcessing <NSCoding>

-(unsigned int)processMIDIMessage:(Byte*)message
						 ofLength:(unsigned int)length
						   atTime:(unsigned int)timeStamp;

@end

#endif /* __OBJC__ */



#ifndef _ACMIDIPROTOCOL_H
#define _ACMIDIPROTOCOL_H



// Protocol Version Number for future compatibily
#define	ACMIDICTRLPROTOCOL_VERSION		0x03




// ***** CONTROLLER CHANGES *****
// The CC's use unused entries from the official Midi Tables (ACMidiDefines.h)
// Controllers 14..31 undefined & general purpose  & undefined and
// Controllers 80..90 general purpose & undefined
// Controllers 102..119	undefined
#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark ++ CONTROLLER ++
#endif

// ** SENSORS / AIN **
// These are the default CC numbers for sensorial measures
// send: 	YES
// receive: NO
// forward:	NO
#ifdef _DEBUG_C
	#pragma mark Sensors / AIN
#endif

#define MIDI_CC_SENSOR_A						20	// 0x14
#define MIDI_CC_SENSOR_B						21	// 0x15
#define MIDI_CC_SENSOR_C						22	// 0x16
#define MIDI_CC_SENSOR_D						23	// 0x17
#define MIDI_CC_SENSOR_E						24	// 0x18
#define MIDI_CC_SENSOR_F						25	// 0x19
#define MIDI_CC_SENSOR_G						26	// 0x20
#define MIDI_CC_SENSOR_H						27	// 0x21

// ** HARMONY **
// harmony & scale setting methods
// send:	YES if input by HUI
// receive:	YES
// forward:	NO
#ifdef _DEBUG_C
	#pragma mark Harmony
#endif

#define MIDI_CC_HARMONY_BASE					80	// 0x50, 0-11, see MIDI_NOTE_x
#define MIDI_CC_HARMONY_SCALE					81	// 0x51, 0:none, 1-127:scaleNum
#define MIDI_CC_HARMONY_BASE_LISTEN				82	// 0x52, 0-63:OFF,  64-127:ON
#define MIDI_CC_HARMONY_SCALE_RELATIVE_PREV		83	// 0x53, 0-63:OFF,  64-127:ON
#define MIDI_CC_HARMONY_SCALE_RELATIVE_NEXT		84	// 0x54, 0-63:OFF,	64-127:ON
#define MIDI_CC_HARMONY_RANDOM					85	// 0x55, 0x63:OFF,  64-127:ON
#define MIDI_CC_HARMONY_RANDOM_RELATIVE			86	// 0x56, 0x63:OFF,  64-127:ON

#define MIDI_CC_HARMONY_DETECTION				79	// 0x49, 0x63:OFF,  64-127:ON

// ** SCALES **
// scale names
#ifdef _DEBUG_C
	#pragma mark Scales
#endif

#define SCALE_NONE								@"    "
#define SCALE_AEOLIAN							@"AEO "
#define SCALE_AHABA_RABBA						@"AHAB"
#define SCALE_ALGERIAN							@"ALG "
#define SCALE_ARABIAN							@"arab"
#define SCALE_AUGMENTED							@"AUGM"
#define SCALE_BLUES								@"Blue"
#define SCALE_BLUES_MAJOR						@"BLUE"
#define SCALE_BLUES_MINOR						@"blue"
#define SCALE_BYZANTINE							@"BYZ "
#define SCALE_CHROMATIC							@"chro"
#define SCALE_DIATONIC							@"DIA "
#define SCALE_DORIAN							@"DOR "
#define SCALE_FULLTONE							@"FULL"
#define SCALE_GIPSY_MINOR						@"gyps"
#define SCALE_HARMONIC_DOUBLE					@"hrmD"
#define SCALE_HARMONIC_MINOR					@"harm"
#define SCALE_HUNGARIAN_MAJOR					@"HUNG"
#define SCALE_HUNGARIAN_MINOR					@"hung"
#define SCALE_HUNGARIAN_PERSIAN					@"PERH"
#define SCALE_JAPANESE							@"JAP "
#define SCALE_JAPANESE_DIMINISHED				@"JAPd"
#define SCALE_KUMOI								@"KUMO"
#define SCALE_LOCRIAN							@"LOC "
#define SCALE_LYDIAN							@"LYD "
#define SCALE_MAJOR								@"MAJ "
#define SCALE_MELODIC_MAJOR						@"MELO"
#define SCALE_MELODIC_MINOR						@"melo"
#define SCALE_MINOR								@"min "
#define SCALE_MI_SHEBERACH						@"MISH"
#define SCALE_MIXOLYDIAN						@"MIX "
#define SCALE_MOHAMMEDAN						@"MOHA"
#define SCALE_NEOPOLITAN_MAJOR					@"NEO "
#define SCALE_NEOPOLITAN_MINOR					@"neo "
#define SCALE_ORIENTAL							@"ORNT"
#define SCALE_OVERTONE							@"OVER"
#define SCALE_PELOG								@"PLOG"
#define SCALE_PENTATONIC						@"PENT"
#define SCALE_PENTATONIC_BLUES					@"PBLU"
#define SCALE_PENTATONIC_MAJOR					@"PMAJ"
#define SCALE_PENTATONIC_MINOR					@"Pmin"
#define SCALE_PERSIAN							@"PERS"
#define SCALE_PHRYGIAN							@"PHRY"
#define SCALE_SPANISH							@"SPAN"

// indian scales
#define SCALE_INDIAN_
#define SCALE_INDIAN_
#define SCALE_INDIAN_BHAVAPRIYA					@"BAVA"
#define SCALE_INDIAN_DHATUVARDHANI				@"DATU"
#define SCALE_INDIAN_HEMAVITI					@"HEMA"
#define SCALE_INDIAN_JHANKARADHVANI				@"JHAN"
#define SCALE_INDIAN_KAMAVARDHANI				@"KAMA"
#define SCALE_INDIAN_NAVANITAM					@"NAVA"
#define SCALE_INDIAN_NITIMATI					@"NITI"
#define SCALE_INDIAN_RAGHUPRIYA					@"RAGU"
#define SCALE_INDIAN_RAMAPRIYA					@"RAMA"
#define SCALE_INDIAN_SALAGAM					@"SALA"
#define SCALE_INDIAN_SANMUKHAPRIYA				@"SAMU"
#define SCALE_INDIAN_SUBHAPANTUVARALI			@"SUBH"
#define SCALE_INDIAN_SUVARNANGI					@"SUVA"


// scale-ID's
#define SCALE_ID_NONE							0
#define SCALE_ID_MAJOR							1
#define SCALE_ID_MINOR							2
#define SCALE_ID_HARMONIC_MINOR					3
#define SCALE_ID_BLUES_MINOR					4
#define SCALE_ID_BLUES_MAJOR					5
#define SCALE_ID_DORIAN							6
#define SCALE_ID_JAPANESE						7
#define SCALE_ID_JAPANESE_DIMINISHED			8
#define SCALE_ID_KUMOI							9
#define SCALE_ID_LOCRIAN						10
#define SCALE_ID_LYDIAN							11
#define SCALE_ID_MI_SHEBERACH					12
#define SCALE_ID_MIXOLYDIAN						13
#define SCALE_ID_PELOG							14
#define SCALE_ID_PENTATONIC						15
#define SCALE_ID_PENTATONIC_BLUES				16
#define SCALE_ID_PENTATONIC_MAJOR				17
#define SCALE_ID_PENTATONIC_MINOR				18
#define SCALE_ID_PHRYGIAN						19
#define SCALE_ID_SPANISH						20


// ** QUANTIZERS **
#ifdef _DEBUG_C
	#pragma mark Quantizers
#endif

#define QUANTIZE_BPM							87	// 0: 60bpm to 127: 187bpm
#define QUANTIZE_TOGGLE_NEXT					89
#define QUANTIZE_SET							90

#define SYNC_STOP								62	// like System Realtime Stop
#define SYNC_START								63	// like System Realtime Start



#endif /* _ACMIDIPROTOCOL_H */

