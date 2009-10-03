/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - config and definition *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _DEFS_H
#define _DEFS_H

// Compiler switches for the debug console

// #define ENV_VERBOSE 
// #define TRIGGER_VERBOSE 
// #define ENGINE_VERBOSE 
// #define ENGINE_VERBOSE_MAX
// #define DRUM_VERBOSE 
// #define SYSEX_VERBOSE 
// #define APP_VERBOSE 
// #define ROUTING_VERBOSE
// #define FILTER_VERBOSE

// new routing matrix
#define	ROUTE_SOURCES 16
#define	ROUTE_TARGETS 16

// routing signal sources
#define	RS_OUT			0  // output value of this row
#define	RS_LFO1_OUT		1
#define	RS_LFO2_OUT		2
#define	RS_LFO3_OUT		3  // dummy
#define	RS_ENV1_OUT		4
#define	RS_ENV2_OUT		5
#define	RS_ENV3_OUT		6  // dummy
#define	RS_OSC1_OUT		7
#define	RS_OSC2_OUT		8
#define	RS_OSC3_OUT		9  // dummy
#define	RS_AFTERTOUCH 	10
#define	RS_MODWHEEL		11
#define	RS_CONSTANT		12

// routing signal target 
#define	RT_FILTER_CUTOFF	0
#define	RT_FILTER_RESONANCE 1
#define	RT_VOLUME           2
#define	RT_OSC1_VOLUME      3
#define	RT_OSC2_VOLUME      4
#define	RT_OSC3_VOLUME      5 // dummy
#define	RT_OSC1_PITCH       6
#define	RT_OSC2_PITCH       7
#define	RT_OSC3_PITCH       8

// at 48kHz sample frequency and two channels, the sample buffer has to be 
// refilled at a rate of 48Khz / SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE 32  
#define CHANNELS 2

#define ENVELOPE_RESOLUTION 		100 // divider for the envelope clock (48kHz/X)
#define LFO_RESOLUTION 				10  // divider for the lfo clock (48kHz/X)

#define ENGINE_SYNTH				0
#define ENGINE_DRUM					1

#define	NO_STEAL					0
#define STEAL						1

#define PB_UP 						0
#define PB_DOWN 					1

#define NOTESTACK_SIZE 				16

#define 	OSC_COUNT				2

#define		TRIANGLE				0
#define		SAW     				1
#define		RAMP    				2
#define		SINE    				3
#define		SQUARE  				4
#define		PULSE   				5
#define		WHITE_NOISE				6
#define		PINK_NOISE				7

#define		ROUTING_TARGETS			6
#define		T_CUTOFF				0
#define		T_MASTER_VOLUME			1
#define		T_OSC1_PITCH        	2
#define		T_OSC2_PITCH   			3
#define		T_OVERDRIVE   			4
#define		T_SAMPLERATE  			5

#define 	PORTA_NONE				0
#define		PORTA_GLIDE				1

#define		ROUTING_SOURCES			5
#define		S_NONE					0			
#define 	S_LFO1					1
#define 	S_LFO2					2
#define 	S_ENVELOPE1				3
#define 	S_ENVELOPE2				4

#define 	ATTACK 					0
#define 	DECAY 					1
#define 	SUSTAIN 				2
#define 	RELEASE 				3

#define     FILTER_TYPES        	7
#define     FILTER_NONE				0
#define     FILTER_LP				1
#define     FILTER_RES_LP       	2
#define     FILTER_MOOG_LP      	3
#define     FILTER_SVF_LOWPASS      4
#define     FILTER_SVF_BANDPASS	    5		
#define     FILTER_SVF_HIGHPASS		6	

#define		TRIGGER_NOTEON			0
#define		TRIGGER_NOTEOFF			1
#define 	TRIGGER_LFO_PERIOD		2
#define 	TRIGGER_LFO1_PERIOD		2
#define 	TRIGGER_LFO2_PERIOD		3
#define 	TRIGGER_ENV1_SUSTAIN	4
#define 	TRIGGER_ENV2_SUSTAIN	5

#endif
