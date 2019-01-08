/*
 *  IIC_SpeakJetMidiDefines.h
 *  kII
 *
 *  Created by Michael Markert, audiocommander.de on 26.08.06
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


#ifndef _IIC_SPEAKJETMIDIDEFINES_H
#define _IIC_SPEAKJETMIDIDEFINES_H


// include standard MIDI names
// you can then use MIDI_CC_VOLUME instead of 0x7
#include "ACMidiDefines.h"


// ********* SPEAKJET MIDI ASSIGNMENTS * //
// MIDI ASSIGNMENTS
// optimized for KORG microKONTROL
//
// just assign the ControlChange or Channel Numbers to the functions
// eg: CC 107 on CH 1 controls the SPEED of the SpeakJet Allophones (SJCC_SPEED)
// eg: Notes  on CH 2 trigger allophones (all phonemes) (SJCH_ALLOPHONES)
//
// some midi-controls are hardcoded (can't be assigned here)
// channel control messages like PITCH-WHEEL, AFTERTOUCH or PANIC and
// system realtime messages like START/STOP/CONTINUE




// ********** CHANNELS **********
// channel assignments for NOTE_ONs 0x90 and NOTE_OFFs 0x80 functions
// MIDI CHANNELS for NOTE_ONs
#define SJCH_SOUNDCODES			1	//	MSA Soundcodes (Allophones + FX)						default 1
#define SJCH_ALLOPHONES			2	//	MSA Soundcodes (Allophones)								default 2
#define SJCH_FX					3	//  MSA Soundcodes (FX)										default 3
#define SJCH_PITCH				4	//	MSA Pitch (Note2Freq) only! No sound output!			default 4
#define SJCH_VOWELS_DIPHTONGS	5	//  MSA Pitched Vowels incl. Diphtongs 						default 5
#define SJCH_CONSONANTS			6	//	MSA Pitched Consonants  (depending on jaw & tongue)*	default 6
#define SJCH_VOWELS_CONSONANTS	7	//	MSA Pitched Vowels & Consonants (mixed)					default 7
#define SJCH_CONSONANTS_OPEN	8	//  MSA Pitched Consonants produced by mouth opening		default 8
#define SJCH_CONSONANTS_CLOSE	9	//	MSA Pitched Consonants produced by mouth closing		default 9
#define	SJCH_PERCUSSIVE			10	//	MSA Soundcodes (Percussive Consonants)					default 10
#define SJCH_OSC1				11	//	OSC 1 (Note2Freq, monophon)								default 11
#define SJCH_OSC2				12	//	OSC 2 (Note2Freq, monophon)								default 12
#define SJCH_OSC3				13	//	OSC 3 (Note2Freq, monophon)								default 13
#define SJCH_OSC4				14	//	OSC 4 (Note2Freq, monophon)								default 14
#define SJCH_OSC5				15	//	OSC 5 (Note2Freq, monophon)								default 15
#define SJCH_OSC_SYN			16	//	OSCs 1 to 5 (Note2Freq, harmonic polymode)				default 16 

// the following channel types can also be used, but make sure only 16 channel nums are defined!
#define SJCH_VOWELS				255	//	MSA Pitched Vowels only (depending on jaw & tongue)*	default 0 (n/a)
#define SJCH_DIPHTONGS			254	//	MSA Pitched Diphtongs only (depending on jaw & tongue)*	default 0 (n/a)



// ********** CONTROL CHANGES **********
// general
#define SJCC_CHANNEL_ASSIGNMENT_TOGGLE	17	//  switch to next channel assignment, CH1=>CH12 (channelAssignment, main.h)
#define SJCC_CHANNEL_ASSIGNMENT			18	//  set channel assignment, 0..127 => 1..16 (channelAssignment, main.h)
#define SJCC_PHRASE				9

// pauses: pause 0(0ms) / 1(100ms) / 2(200ms) / 3(700ms)
//		   pause 4(30ms)/ 5(60ms)  / 6(90ms)
#define SJCC_PAUSE0				255	//   0 ms	// inactive!
#define SJCC_PAUSE1				22	// 100 ms
#define SJCC_PAUSE2				23	// 200 ms
#define SJCC_PAUSE3				24	// 700 ms
#define SJCC_PAUSE4				254	//  30 ms	// inactive!
#define SJCC_PAUSE5				21	//  60 ms
#define SJCC_PAUSE6				253	//  90 ms	// inactive!
// modifiers: slow/low/high/fast
#define SJCC_NEXT_SLOW			25	// play next phrase ...
#define SJCC_NEXT_LOW			26
#define SJCC_NEXT_HIGH			27
#define SJCC_NEXT_FAST			28
// phrases: call phrase
#define SJCC_PHRASE0			29	// play internal phrase ...
#define SJCC_PHRASE1			30
#define SJCC_PHRASE2			31
#define SJCC_PHRASE3			32
// step phrase record (not activated by default!)
#define SJCC_PHRASE_REC_UNDO	33	// delete last recorded command
#define SJCC_PHRASE_REC_ABORT	34	// stop recording, delete buffer
#define SJCC_PHRASE_REC			35	// start recording
#define SJCC_PHRASE_REC_PREVIEW	36	// preview buffer
// harmony control (also see ACMidiProtocol.h!)
#define SJCC_HARMONY_LISTEN		37	// 64-127: listen for next note-on, 0-64: exit listen-mode
#define SJCC_HARMONY_SCALE		38	// 0-SCALE_MAX: set scale, 127: next scale
#define SJCC_HARMONY_BASE		39	// set new base note
// articulation controls
#define SJCC_MOUTH_JAW			40	// jaw position (low/mid/high)
#define SJCC_MOUTH_TONGUE		41	// tongue position (front/center/back)
#define SJCC_MOUTH_HEIGHT		42	// pitch, note-height (send only! use notes on  CH# SJCH_PITCH to receive!)
#define SJCC_MOUTH_STRESS		43	// same as bend
#define SJCC_MOUTH_SPEED		44	// same as speed
#define SJCC_MOUTH_PAUSES		45	// inserts pauses (long/short) 
#define SJCC_MOUTH_PHONEME		46	// last phoneme (send only! use notes on CH# SJCH_ALLOPHONES to receive!)
// OSCx Frequency
#define SJCC_OSC1_FREQ			101
#define SJCC_OSC2_FREQ			102
#define SJCC_OSC3_FREQ			103
#define SJCC_OSC4_FREQ			104
#define SJCC_OSC5_FREQ			105
// OSCx Level
#define SJCC_OSC1_LVL			111
#define SJCC_OSC2_LVL			112
#define SJCC_OSC3_LVL			113
#define SJCC_OSC4_LVL			114
#define SJCC_OSC5_LVL			115
// OSC controls
#define SJCC_OSC_WAVESHAPE		50	// switch to next osc additive synth waveshape
#define SJCC_DISTORTION			118	// distortion level (mix) for OSCs 4 & 5
// ENV controls
#define SJCC_ENV_OSC_A			53	// ENV on/off for OSCs 1 to 3
#define SJCC_ENV_OSC_B			54	// ENV on/off for OSCs 4 and 5 (half enveloped only)
#define SJCC_ENV_OSC_TGL		52	// toggle ENV on/off for OSC groups (00, 01, 10, 11)
#define SJCC_ENV_WAVESHAPE		116	// set ENV Waveshape 1:0..31, 2:32..63, 3:64..95, 4:96..127
#define SJCC_ENV_WAVESHAPE_TGL	51	// switch to next envelope waveshape, also see SJCC_ENV_TYPE
#define SJCC_ENV_FREQ			106
#define SJCC_SYNTH_ENV			56	// ENV on/off for Additive Synthesis
// speech params
#define SJCC_SPEED				107	// same as SJCC_MOUTH_SPEED
#define SJCC_BEND				117
#define SJCC_MASTER_VOL			108

// Joystick X/Y: Pitch / Env Freq
#define SJCC_PITCH				MIDI_PITCH
#define SJCC_FREQ				MIDI_POLY_AFTER		// same as bend




#endif /* _IIC_SPEAKJETMIDIDEFINES_H */

