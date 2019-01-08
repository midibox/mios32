/*
 *  IIC_SpeakJet.h
 *  kII
 *
 *  Created by audiocommander on 03.05.06.
 *  Based on a C-Skeleton of Thorsten Klose (c) 2006
 *    http://www.midibox.org
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
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


/*
 * ********* NOTES ******************** 
 * 
 * MIOS Application 	© 2004 by Thorsten Klose, tk@midibox.org
 * kII					© 2006 by Michael Markert, dev@audiocommander.de
 * contains additional code submitted by by Sebastian KŸhn, rio@rattenrudel.de
 *
 *
 * see readme.txt
 * for version info
 * and description
 * 
 */





// ********* DEFINES ****************** //

#ifndef _IIC_SPEAKJET_H
#define _IIC_SPEAKJET_H


// include definition listings
#include "ACMidiDefines.h"
#include "IIC_SpeakJetDefines.h"
#include "ACToolbox.h"


// allowed IDs: 0x20, 0x22, 0x24 or 0x26 
// (selectable with J2 of the MBHP_IIC_SPEAKJET module)
#define SPEAKJET_SLAVE_ID					0x20

// If Pin D2/Buffer Half Full is connected to the Core Module:
// if the SpeakJet's buffer is half full, PORTDbits.RD4 get logic high
// all IIC transmissions will be locked to prevent an overflows of the SJ's input buffer
// that may result in ugly pings or pistol shots. (default: 1)
// On the other hand, messages get lost this way...
#define SPEAKJET_BUFFER_CHECK				1

// TK: macro to check pin state
#define SPEAKJET_GET_D2_PIN() (MIOS32_SYS_LPC_PINGET(SPEAKJET_PORT_D2, SPEAKJET_PIN_D2))

// pitch is 0-8192-16383
// SJ-pitch is 0-128-255 with default 88
// sets 88 as center by subtracting 40
#define SPEAKJET_DEFAULT_PITCH_CORRECTION	1

// if random consonants are used the phoneme maps (based on jaw and tongue position)
// are ignored for the advantage of improved consonant enunciation; default & recommended: 1
#define SPEAKJET_USE_RANDOM_CONSONANTS		1

// if startup messages are enabled or not, default: 0
// recommended only if the SJ has been equipped with own sentences!
// also see:
// ACSyncronizer.h:	CTR_MEASURE_MAX			// triggers startup msg each x measures
#define SPEAKJET_STARTUP_MESSAGES			KII_WELCOME_ENABLED
// play phrases from 0 to *
#define SPEAKJET_STARTUP_MESSAGES_MAX		8
// if startup messages are cycled, all messages from 0 to STARTUP_MESSAGES_MAX are triggered
// one after another, else a random phrase is triggered (0 to STARTUP_MSG_MAX)
#define SPEAKJET_CYCLE_STARTUP_MESSAGES		1
// Send Panic (all channels) right before startup message
#define SPEAKJET_STARTUP_PANIC				1

#if KII_RECORDER_ENA
	// default recording buffer is 64 bytes (aka input buffer SpeakJet)
	#define RECBUFFER_MAX					63
#endif /* KII_RECORDER_ENA */



// ********* TYPEDEFS ***************** //

typedef struct {
	unsigned	manualTransmission	: 1;			// overrides transmitStart() and Stop()
	unsigned	iic_lock			: 1;			// locked if buffer_half_full (J14)
	unsigned	unused				: 5;
	unsigned	enveloped_synthesis : 1;			// 0: OFF (Manual), 1: BASE 
} sj_control_t;

typedef union {
	struct {
		unsigned	ALL				: 8;			// read as unsigned char to transmit
	};
	struct {
		unsigned	enveloped_ignore: 6;
		unsigned	enveloped_osc	: 2;			// used to toggle envelope setting for OSC groups
	};
	struct {
		unsigned	waveshape		: 2;			// 00:SAW, 01:SINE, 10:TRIANGLE, 11:SQUARE
		unsigned	unused			: 4;			// 
		unsigned	enveloped_osc_a	: 1;			// if OSCs 1, 2 and 3 are enveloped
		unsigned	enveloped_osc_b	: 1;			// if OSCs 4 and 5 are half enveloped
	};
} sj_env_control_t;

#if KII_RECORDER_ENA
typedef struct {
	unsigned	recMode				: 1;			// 1 if in record-mode
	unsigned	recPause			: 1;			// 1 if record paused
	unsigned	phrase				: 4;			// current phrase num 0..15
	unsigned	scpMode				: 1;			// 1 if scp mode entered but not yet exited
	unsigned	unused				: 1;			//
} sj_stepseq_t;
#endif /* KII_RECORDER_ENA */


// ********* EXPORT GLOBALS *********** //

extern volatile sj_control_t		sj_control;		// sj control settings
extern volatile sj_env_control_t	sj_env_control;	// sj envelope controls

#if KII_RECORDER_ENA
extern volatile	sj_stepseq_t		sj_stepseq;		// sj related settings (see typedef)
#endif /* KII_RECORDER_ENA */

extern volatile unsigned char		jaw;			// 0..14
extern volatile unsigned char		tongue;			// 0..4

extern volatile unsigned char		oscsynth_waveshape;	// harmonic osc synth waveform type


// ********* PROTOTYPES *************** //

#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Core Functions
#endif

// Core Functions
extern void			 IIC_SPEAKJET_Init(void);

extern unsigned char IIC_SPEAKJET_TransmitStart(unsigned char _slave);
extern unsigned char IIC_SPEAKJET_TransmitByte(unsigned char value);
extern unsigned char IIC_SPEAKJET_TransmitNumber(unsigned int value);
extern void			 IIC_SPEAKJET_TransmitStop(void);

extern void			 IIC_SPEAKJET_ClearBuffer(void);						// clears the input buffer only
extern void			 IIC_SPEAKJET_Reset(unsigned char hardreset);	// resets MSA, clears buffer (& resets)

extern void			 IIC_SPEAKJET_StartupMessage(void);						// enunciate startup message


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark MSA Control
#endif

// MSA Controls
extern void	IIC_SPEAKJET_MSA_Stop(void);									// stops enunciating and clears buffer
extern void IIC_SPEAKJET_MSA_Phrase(unsigned char phrase);			// 0..16
extern void IIC_SPEAKJET_MSA_Ctrl(unsigned char msaCtrl);			// see SpeakJetDefines!
extern void IIC_SPEAKJET_MSA_Soundcode(unsigned char soundcode);	// 0..127 => 128..254
extern void IIC_SPEAKJET_MSA_Allophone(unsigned char soundcode);	// 0..127 => 128..199 (72)
extern void IIC_SPEAKJET_MSA_FX(unsigned char soundcode);			// 0..127 => 200..254 (55)
extern void IIC_SPEAKJET_MSA_Percussive(unsigned char soundcode);	// 0..127 => 165..199 + 253 (35)
extern void IIC_SPEAKJET_MSA_Pauses(unsigned char value);			// 0..127 => 0..64, 0:short <-> 127:long

extern void IIC_SPEAKJET_MSA_Volume(unsigned char value);			// 0..127
extern void IIC_SPEAKJET_MSA_Speed(unsigned char value);			// 0..127
extern void IIC_SPEAKJET_MSA_Pitch(unsigned int value, unsigned char valueIs14bit); // 0..16383 or/=> 0..127 (36..95)
extern void IIC_SPEAKJET_MSA_Bend(unsigned char value);			// 0..127 => 0..15
extern void IIC_SPEAKJET_Direct_Bend(unsigned char value);			// 0..15


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Articulation Control
#endif

// Articulation Controls
// see SpeakJetDefines! (JAW/TONGUE)
extern void IIC_SPEAKJET_Articulate(unsigned char articulationType, unsigned char value); // 0..127
// see SpeakjetDefines! Trigger Phonemes (use Articulate to change articulation!)
extern unsigned char IIC_SPEAKJET_Enunciate(unsigned char phonemeType, unsigned char value); // 0..127


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark SCP Controls
#endif

// SCP Controls
extern void IIC_SPEAKJET_SCP_Enter(unsigned char sj_id, unsigned char ctrl_type);
																			// SCP_CTRLTYPE_REALTIME or _REGISTER
extern void IIC_SPEAKJET_SCP_Exit(unsigned char write);			// write after _REGISTER

extern void IIC_SPEAKJET_OSCSynthesis_ToggleWaveshape(void);				// toggles next subtr. synthesis waveshape
extern void IIC_SPEAKJET_OSCSynthesis(unsigned char value);		// additive synthesis for OSCs, 0..127
extern void IIC_SPEAKJET_OSCPolyphone(unsigned char evnt1, unsigned char evnt2); // 
extern void IIC_SPEAKJET_OSCFreq(unsigned char osc, unsigned char value);	// OSC 1..5, 0..127	
extern void IIC_SPEAKJET_OSCLvl(unsigned char osc, unsigned char value);	// OSC 1..5, 0..127 => 0..31
extern void IIC_SPEAKJET_ENV_ToggleWaveshape(void);							// toggles next ENV waveshape
extern void IIC_SPEAKJET_ENV_Toggle(void);									// toggles next ENV ON/OFF setting
extern void IIC_SPEAKJET_ENV_Waveshape(unsigned char value);		// 0..127 => 0..3
extern void IIC_SPEAKJET_ENVCtrl(void);										// transmits ENV settings
extern void IIC_SPEAKJET_ENVFreq(unsigned char value);				// 0..127
extern void IIC_SPEAKJET_Distortion(unsigned char value);			// 0..127 => 0..255


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Recorder
#endif

#if KII_RECORDER_ENA
// Phrases
extern void IIC_SPEAKJET_Play(unsigned char start);				// 1: start, 0: stop

extern void IIC_SPEAKJET_RecordPhrase(unsigned char start);		// 1: rec, 0: exit & save
extern void IIC_SPEAKJET_RecordPhrasePause(unsigned char resume);	// 0: pause, 1: resume
extern void IIC_SPEAKJET_RecordPhraseAbort(void);
extern void IIC_SPEAKJET_RecordPhraseUndo(void);							// removes last added value from recBuffer
extern void IIC_SPEAKJET_RecordPhrasePreview(void);							// prelisten current buffer-content
#endif /* KII_RECORDER_ENA */


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Helpers
#endif

// SCP Helpers
extern void IIC_SPEAKJET_NoteToFreqChars(unsigned char note);		// eg. sends note 69 as '4','4','0' Hz


#endif /* _IIC_SPEAKJET_H */
