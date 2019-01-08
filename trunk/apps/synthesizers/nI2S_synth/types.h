/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - type definitions      *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _TYPES_H
#define _TYPES_H

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

#define 	TR_ROWS					9
#define 	TR_COLS					11

// waveform structure ********************************************************
typedef union {
	struct {
		unsigned all:8;
	};
	struct {
		unsigned triangle:1;
		unsigned saw:1;
		unsigned ramp:1;
		unsigned sine:1;
		unsigned square:1;
		unsigned pulse:1;
		unsigned white_noise:1;
		unsigned pink_noise:1;
	};
} waveform_t;

// pitchbend *****************************************************************
typedef struct {
	u8 upRange;				// range for pitchbend up (in semitones)
	u8 downRange;			// range for pitchbend down (in semitones)
	s16 value;				// normed value -128..127
} pitchbend_t;

// oscillator ****************************************************************
typedef struct {
	waveform_t waveforms;
	
	u16 pulsewidth;			// pulsewidth for "pulse" waveform
	s32 triangle;			// 8 samples for the individual waveforms
	s32 saw;
	s32 ramp;
	s32 sine;
	s32 square;
	s32 pulse;
	s32 white_noise;
	s32 pink_noise;
	u16 accumulator;		// holds the phase accumulator
	u16 subAccumulator;		// sub octave accumulator
	s32 sample;				// oscillator output
	s32 subSample;			// sub oscillator output
	u16 accumValue;			// holds the value to be added each sample (unoffset) 
	u16 accumValuePUp;		// -"- (offset up by pitchbend)
	u16 accumValuePDown;	// -"- (offset down by pitchbend)
	u16 pitchedAccumValue;  // holds the combined pitched accum value
	u8 accumNote;			// note that is being played 
	
	u8 waveformCount;		// number of waveforms selected simultaneously
	
	u8 velocity;			// velocity of note
	u16 volume;				// overall oscillator volume
	u16 subOscVolume;	    // sub octave oscillator volume

	float detune;			// detune
	s8 transpose;			// transpose
	s8 finetune;			// finetune
	pitchbend_t pitchbend;	// pitchbend structure
	
	u16 portaRate;			// portamento time
	u8 portaMode;			// 0 = off, 1 = glide
	u16 portaStart;
	u32 portaTick;
} oscillator_t;

// lfo ***********************************************************************
typedef struct lfo {
	waveform_t waveforms;
	u8 waveformCount;

	u16 frequency;
	u16 accumulator;
	u16 accumValue;

	u16 triangle;			// 8 samples for the individual waveforms
	u16 saw;
	u16 ramp;
	u16 sine;
	u16 square;
	u16 pulse;
	u16 white_noise;
	u16 pink_noise;
	
	u16 pulsewidth;
	u16 depth;				// fixme: bad relic, needs to be removed
	u16 out;
} lfo_t;

// envelope ******************************************************************
typedef struct {
	u16 envelopeDepth;
	u16 envelopeStage;
	u16 out;
	struct {
		u16 attack;
		u16 decay;
		u16 sustain;
		u16 release;
	} stages;

	u16 accumulator;
	u16 attackAccumValue;
	u16 decayAccumValue;
	u16 releaseAccumValue;

	u8 gate:1;
} envelope_t;

// voice *********************************************************************
typedef struct {
	u16 masterVolume;
	u16 overdrive; 			// overdrive amount
	u8 bitcrush; 			// bitcrush amount
	u8 downsample;			// downsampling
	u16 xor; 				// XOR value
	u16 mask; 				// bitmask
	
	u16 delayTime;			// delay time 0..16382
	u16 delayFeedback;      // delay feedback
	u8 delayDownsample;		// downsampling for delay

	u16 chorusTime;			// chorus time 0..8190
	u16 chorusFeedback;     // chorus feedback

	s16 lastSample;			// last sample
} voice_t;

// note **********************************************************************
typedef struct {
	u8 note;
	u8 velocity;
} note_t;

// filter ********************************************************************
typedef struct {
	u16 cutoff;
	u16 resonance;
	u8 mode;
	u16 cutoffMod;
	u16 resonaneMod;
	u8 filterType;
} filter_t;

// engine flags **************************************************************
typedef union {
	struct {
	unsigned all:16;
	};
	struct {
		unsigned reattackOnSteal:1;
		unsigned interpolate:1;
		unsigned syncOsc2:1;
		unsigned overdrive:1;
		unsigned empty0:1;  // !
		unsigned empty1:1;  // !
		unsigned dcf:1;
		unsigned ringmod:1;
		unsigned delay:1;
		unsigned chorus:1;
		unsigned emptyA:1;  // !
		unsigned emptyB:1;
		unsigned emptyC:1;
		unsigned emptyD:1;
		unsigned emptyE:1;
		unsigned emptyF:1;
	};
} engineflags_t;

// internal engine flags *****************************************************
typedef struct {
} engineflags2_t;

// routing target ************************************************************
typedef struct {
	u8 source;
	u16 depth;
} routing_t;

// trigger matrix col ********************************************************
typedef union {
	struct {
		unsigned all:16;
	};
	struct {
		unsigned lfo1_reset:1;
		unsigned lfo2_reset:1;
		unsigned env1_attack:1;
		unsigned env1_decay:1;
		unsigned env1_sustain:1;
		unsigned env1_release:1;
		unsigned env2_attack:1;
		unsigned env2_decay:1;
		unsigned env2_sustain:1;
		unsigned env2_release:1;
		unsigned :7;
	};
} trigger_col_t;

typedef struct {

} patch_inner_t;

typedef union {
	u8 					all[512];
	struct {
		char			name[33];
		lfo_t 			lfos[2];
		envelope_t 		envelopes[2];
		oscillator_t 	oscillators[2];
		engineflags_t 	engineFlags;
		engineflags2_t 	engineFlags2;
		filter_t 		filter;
		voice_t			voice;
		trigger_col_t   trigger_matrix[TR_COLS];
		u8				engine;
		routing_t		routing[ROUTING_TARGETS];
	} d;
} patch_t;

typedef union {
	struct {
		u8 				all[512];
	};
	struct {
		u32				header;
	};
} config_t;

// new routing matrix //////////////////////////////////////////////////////////

// routing element
typedef struct {
	s16 offset;			// offset (comes after depth)
	s16 depth; 			// depth  (before offset)
	s16 out; 			// output = (in * depth / 65536) + offset;
} routing_element_t;

typedef union {
	s16 s16;
	u16 u16;
} su16;

// modulation path type
typedef struct {
	s16  depth[ROUTE_INPUTS_PER_PATH];   	// output = ((input * depth) / 65536) + offset
	s16  offset[ROUTE_INPUTS_PER_PATH];  	// -"-
	s16  output[ROUTE_INPUTS_PER_PATH];  	// -"-
	s16  rdepth;
	s16  roffset;
	u8   inputid[ROUTE_INPUTS_PER_PATH];	// id of input (index for route_ins)
	u8   rdsourceid;
	u8   outputid;							// signal output index
} route_t;

#endif
