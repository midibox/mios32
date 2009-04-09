/****************************************************************************
 * nI2S Digital Toy Synth - SYNTH ENGINE                                    *
 *                                                                          *
 * Very simple approach w/o any oversampling, anti-aliasing, ...            *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

/* TODO *********************************************************************
 * trigger matrix (mainly envelopes)
 * extended wave blending/mixing
 * new mod targets/sources
 * lfo depth offset before multiply
 * mod behaviour not cool
 * ...
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <math.h>
#include <FreeRTOS.h>
#include <portmacro.h>

#include "engine.h"
#include "lfo.h"
#include "filter.h"
#include "drum.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

       u32 sample_buffer[SAMPLE_BUFFER_SIZE]; 	// sample buffer Tx

static note_t noteStack[NOTESTACK_SIZE];		// last played notes
static u8 noteStackIndex;						// index of last note
static s8 noteStackLen;							// number of notes playing

static trigger_col_t    trigger_matrix[TR_COLS];// trigger matrix
	   voice_t			voice;					// voice struct
static oscillator_t		oscillators[OSC_COUNT];	// the oscillators
	   lfo_t			lfos[2];				// the LFOs
	   envelope_t		envelopes[2];			// the envelopes
	   filter_t			filter;					// the filter
       engineflags_t	engineFlags;			// flag set for the engine
       engineflags2_t	engineFlags2;			// flag set for the engine (internals)

// routing sources per target
	   routing_t		routing[ROUTING_TARGETS];

	   u16 envelopeTime;	// divider for the 48kHz sample clock that clocks the env
       u16 lfoTime;			// divider for the 48kHz sample clock that clocks the lfof

static u16 dead = 1;   	    // debug variable for sample calc overlap / timing

static u16 delayIndex;		// write index of delayBuffer
	   u16 tempValues[2];	// temporary values for whatever 

	   u8 engine;			// selects the sound engine
static u8 downsampled;	    // counter for downsampling
static u8 delaysampled;	    // counter for downsampling of delay

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

void ENGINE_ReloadSampleBuffer(u32 state);

/////////////////////////////////////////////////////////////////////////////
// returns the modulation source value for a given ID
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_getModulator(u8 src) {
	switch (src) {
		case S_LFO1: 			return lfos[0].out;
		case S_LFO2: 			return lfos[1].out;
		case S_ENVELOPE1:		return envelopes[0].out;
		case S_ENVELOPE2:		return envelopes[1].out;
	}
}

/////////////////////////////////////////////////////////////////////////////
// a trigger point has been reached, take appropriate action
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_trigger(u8 trigger) {
	if (trigger_matrix[trigger].lfo1_reset) { 
		lfos[0].accumulator = 0;	// reset lfo accumulator
		
		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 1 Reset", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].lfo2_reset) { 
		lfos[1].accumulator = 0;	// reset lfo accumulator

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 2 Reset", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env1_attack) { 
		envelopes[0].accumulator = 0;		 // reset attack accumulator
		envelopes[0].gate = 1;		 		 // open gate
		envelopes[0].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Attack", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env1_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		envelopes[0].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Decay", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env1_sustain) { 
		// set sustain level to accum and change stage
		envelopes[0].accumulator = envelopes[0].stages.sustain;
		envelopes[0].envelopeStage = SUSTAIN;
		
		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Sustain", trigger);
		#endif		

		// we reach sustain stage, hence a new trigger
		ENGINE_trigger(TRIGGER_ENV1_SUSTAIN);
	}

	if (trigger_matrix[trigger].env1_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		envelopes[0].envelopeStage = RELEASE;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Release", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env2_attack) { 
		envelopes[1].accumulator = 0;		 // reset attack accumulator
		envelopes[1].gate = 1;		 		 // open gate
		envelopes[1].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Attack", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env2_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		envelopes[1].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Decay", trigger);
		#endif		
	}

	if (trigger_matrix[trigger].env2_sustain) { 
		// set sustain level to accum and change statge
		envelopes[1].accumulator = envelopes[1].stages.sustain;
		envelopes[1].envelopeStage = SUSTAIN;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Sustain", trigger);
		#endif		

		ENGINE_trigger(TRIGGER_ENV2_SUSTAIN);
	}

	if (trigger_matrix[trigger].env2_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		envelopes[1].envelopeStage = RELEASE;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Release", trigger);
		#endif		
	}	
}

/////////////////////////////////////////////////////////////////////////////
// applies a range limit to a signal
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_scale(u16 input, u16 depth) {
	s32 sin;
	u32 out;
  
	sin = input - 32768;
	// out = in * (depth / 65535)
	
	sin *= depth;
	sin /= 65536;
	sin += 32768;
	
	return (u16) sin;
}

/////////////////////////////////////////////////////////////////////////////
// modulates the input with modulator depending on depth
/////////////////////////////////////////////////////////////////////////////
s16 ENGINE_modulateS(s16 input, u16 modulator, u16 depth) {
	// in = in + (mod * depth)
	// mod in 0..-MAX_IN
	depth /= 2;
	u32 mod = depth * modulator;
	mod /= 65536;
	s32 in32 = (input * mod) / 65536;

	return in32;
}

u16 ENGINE_modulateU(u16 input, u16 modulator, u16 depth) {
	// in = in + (mod * depth)
	// mod in 0..-MAX_IN
	u32 mod = depth * modulator;
	mod /= 65536;
	mod *= input;
	mod /= 65536;

	return mod;
}

/////////////////////////////////////////////////////////////////////////////
// initializes the synth
/////////////////////////////////////////////////////////////////////////////
void ENGINE_init(void) {
	u8 n;
	
	noteStackLen = 0;
	for (n=0; n<NOTESTACK_SIZE; n++) {
		noteStack[n].note = 0;
	}
	
	engine = ENGINE_SYNTH;
	
	oscillators[0].pulsewidth = 16384;
	oscillators[1].pulsewidth = 16384;
	
	oscillators[0].transpose = 0;
	oscillators[1].transpose = -12;
	
	oscillators[0].volume = 65535;
	oscillators[1].volume = 65535;

	oscillators[0].pitchbend.upRange = 2;
	oscillators[0].pitchbend.downRange = 12;

	oscillators[1].pitchbend.upRange = 2;
	oscillators[1].pitchbend.downRange = 24;

	ENGINE_setOscWaveform(0, 0x08);
	ENGINE_setOscWaveform(0, 0x10);

	voice.masterVolume = 65535;

	filter.filterType = FILTER_RES_LP;
	filter.cutoff = 65535;
	filter.resonance = 0;

	ENV_setAttack(0, 0);
	ENV_setDecay(0, 0);
	ENV_setSustain(0, 65535);
	ENV_setRelease(0, 0);

	ENV_setAttack(1, 1235);
	ENV_setDecay(1, 32767);
	ENV_setSustain(1, 50000);
	ENV_setRelease(1, 32767);

	routing[T_CUTOFF].source = S_NONE;
	routing[T_MASTER_VOLUME].source = S_NONE;
	routing[T_OSC1_PITCH].source = S_NONE;
	routing[T_OSC2_PITCH].source = S_NONE;

	routing[T_CUTOFF].depth = 0;
	routing[T_MASTER_VOLUME].depth = 0;
	routing[T_OSC1_PITCH].depth = 0;
	routing[T_OSC2_PITCH].depth = 0;
	
	engineFlags.reattackOnSteal = 0;
	engineFlags.interpolate = 1;
	engineFlags.syncOsc2 = 0;
	engineFlags.dcf = 0;
	
	oscillators[0].portaMode = PORTA_NONE;
	oscillators[1].portaMode = PORTA_NONE;
	
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("Done booting.");
    // debug: initialize the stopwatch for 100 uS resolution
    MIOS32_STOPWATCH_Init(1);
	#endif

	// start I2S DMA transfers
	MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &ENGINE_ReloadSampleBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// This function removes a note from the stack
/////////////////////////////////////////////////////////////////////////////
void ENGINE_noteOff(u8 note) {
	u8 n, m, j, p;
	
	noteStackLen--;

	if (noteStackLen < 0)
	  noteStackLen = 0;

   	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("noteStackLen = %d", noteStackLen);
	#endif
	
	// find the note in the stack
	for (n=0; n<NOTESTACK_SIZE; n++) {
		// get the note
		m = noteStack[n].note;
		
		if (m == note) {
			// found the note, let's see what we do with it
			
			// zero it
			noteStack[n].note = 0;
			
			// is it the last note that was played?
			if (n == noteStackIndex) {
				// yes, it's the note that was played last, change to a prior one 
				// if there's another key still pressed
				for (j=n+NOTESTACK_SIZE; j>n; j--) {
					// we're walking backwards from n to n - NOTESTACK_SIZE (offsetted), 
					p = j & (NOTESTACK_SIZE - 1);
					
					if (noteStack[p].note > 0) {
						// switch to that note
						noteStackIndex = (p-1) & (NOTESTACK_SIZE - 1);
						ENGINE_noteOn(noteStack[p].note, noteStack[p].velocity, 1);
						
						// kthxbye
						return;
					}
				}
				
				// if we get here we have not found a playable note 
				// kill the output by going to envelope decay
				#ifdef TRIGGER_VERBOSE
				MIOS32_MIDI_SendDebugMessage("ENGINE_noteOff(): triggering NOTE OFF");
				#endif		

				ENGINE_trigger(TRIGGER_NOTEOFF);
			} 
			
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// This function sets a new note
/////////////////////////////////////////////////////////////////////////////
void ENGINE_noteOn(u8 note, u8 vel, u8 steal)
{
	u16 pA1, pA2;
	pA1 = oscillators[0].portaStart;
	pA2 = oscillators[1].portaStart;

	u8 reattack;
	
	// fixme: split into two depending on channel
	
	oscillators[0].accumNote = note;
	oscillators[1].accumNote = note;

	oscillators[0].velocity = vel;
	oscillators[1].velocity = vel;

	oscillators[0].accumValue = accumValueByNote[note + oscillators[0].transpose];
	oscillators[1].accumValue = accumValueByNote[note + oscillators[1].transpose];

	noteStackIndex = (noteStackIndex + 1) & (NOTESTACK_SIZE - 1);
	noteStack[noteStackIndex].note = note;
	noteStack[noteStackIndex].velocity = vel;

	// fixme: reset envelope if desired should be from trigger / same for porta
	if ((noteStackLen == 0) || engineFlags.reattackOnSteal) {
		// trigger matrix NOTE_ON
		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(): triggering NOTE ON");
		#endif

		ENGINE_trigger(TRIGGER_NOTEON); 
	}

	if (!steal)
		noteStackLen++;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("noteStackLen = %d", noteStackLen);
	#endif

	ENGINE_setPitchbend(2, oscillators[0].pitchbend.value);

	reattack = (noteStackLen > 1) || steal;
	
	if ((oscillators[0].portaMode) && reattack) {
		oscillators[0].portaStart = pA1;
	} else {
		oscillators[0].portaStart = oscillators[0].pitchedAccumValue;
	}
	
	if ((oscillators[1].portaMode) && reattack) {
		oscillators[1].portaStart = pA2;
	} else {
		oscillators[1].portaStart = oscillators[1].pitchedAccumValue;
	}
	
}

/////////////////////////////////////////////////////////////////////////////
// Fills the buffer with nicey sample sounds ;D
/////////////////////////////////////////////////////////////////////////////
void ENGINE_ReloadSampleBuffer(u32 state)
{
	// transfer new samples to the lower/upper sample buffer range
	int i;
	u16 out;
	s32 tout, tout2;
	u32 utout, utout2;
	u32 ac;
	u32 *buffer = (u32	*)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/CHANNELS) : 0];

	// debug: measure time it takes for 8 samples
	// decrease counter
	dead--;
	MIOS32_STOPWATCH_Reset();

	// generate new samples to output
	for(i=0; i<SAMPLE_BUFFER_SIZE/CHANNELS; ++i) {
		u8 osc;
		oscillator_t *o;

		// tick the envelopes
		envelopeTime++;
		
		if (envelopeTime >= ENVELOPE_RESOLUTION) {
			envelopeTime = 0;
			ENV_tick();
		}

		// tick the lfos
		lfoTime++;
		
		if (lfoTime >= LFO_RESOLUTION) {
			lfoTime = 0;
			LFO_tick();
		}
		
		/* OSCILLATOR ACCUMULATORS *******************************************/
		// calculate oscillator accumulators individually
		o = &oscillators[0];
		utout2 = o->accumulator;
		ac = o->pitchedAccumValue;

		// porta mode?
		if (o->portaStart != o->pitchedAccumValue) {
			ac = o->portaStart;
			o->portaTick += o->portaRate;
			
			if (o->portaTick >= 0xFFFFF) {
				o->portaTick = 0;
				
				// porta time up
				if (o->portaStart < o->pitchedAccumValue)
					o->portaStart += 1;
				else
					o->portaStart -= 1;
			}
		}

		if (routing[T_OSC1_PITCH].source) {
			// there's a source assigned
			ac = ENGINE_modulateU(ac, ENGINE_getModulator(routing[T_OSC1_PITCH].source), routing[T_OSC1_PITCH].depth);
			// ac += (ENGINE_getModulator(routing[T_OSC1_PITCH].source) >> 3);
			// ac >>= 1;
			// fixme: blend it in!
		} 
		
		ac += o->finetune;
		o->accumulator += ac;
		ac >>= 1;
		o->subAccumulator += ac;
		
		// oscillator 2
		o = &oscillators[1];
		
		if ((engineFlags.syncOsc2) && (oscillators[0].accumulator < utout2)) 
			o->accumulator = 0;
		else {
			// T_OSC2_PITCH is right here
			utout2 = o->pitchedAccumValue;
			
			// porta mode?
			if (o->portaStart != o->pitchedAccumValue) {
				utout2 = o->portaStart;
				o->portaTick += o->portaRate;
				
				if (o->portaTick >= 0xFFFFF) {
					o->portaTick = 0;
					
					// porta time up
					if (o->portaStart < o->pitchedAccumValue)
						o->portaStart += 1;
					else
						o->portaStart -= 1;
				}
			}
			
			if (routing[T_OSC2_PITCH].source) {
				// there's a source assigned
				utout2 = ENGINE_modulateU(utout2, ENGINE_getModulator(routing[T_OSC2_PITCH].source), routing[T_OSC2_PITCH].depth);
				// fixme: blend it in!
			} 

			utout2 += utout2 + o->finetune;
			o->accumulator += utout2;
			utout2 >>= 1;
			o->subAccumulator += utout2;
		}
	
		// downsampling ***********************************************************
		// T_SAMPLERATE is right here
		utout = voice.downsample;
		if (routing[T_SAMPLERATE].source) {
			// there's a source assigned
			utout = ENGINE_modulateU(utout, ENGINE_getModulator(routing[T_SAMPLERATE].source), routing[T_SAMPLERATE].depth);
			if (downsampled > utout)
				downsampled = utout;
		}
		
		if (utout != downsampled) {
			out = voice.lastSample;
			*buffer++ = out << 16 | out;
			downsampled++;
			continue;
		} else
			downsampled = 0;

		/***************************************************************
		 * calculate the oscillators                                   *
		 ***************************************************************/
		for (osc=0; osc<OSC_COUNT; osc++) {
			u16 acc;
			s32 acc32;
			o = &oscillators[osc];

			/***********************************************************
			 * calculate sub oscillator                                *
			 ***********************************************************/
			acc = o->subAccumulator;
			// triangle
			if (acc < 32768) o->subSample = (acc * 2) - 32768;
			else 		  	 o->subSample = 32767 - ((acc - 32768) * 2);
 
            /********************************************************** 
			 * get raw waveforms                                      *
             **********************************************************/
			acc = o->accumulator;

			// triangle
			if (acc < 32768) o->triangle = (acc * 2) - 32768;
			else 		  	 o->triangle = 32767 - ((acc - 32768) * 2);
			// saw
			o->saw = acc - 32768;
			// ramp
			o->ramp = (32768 - acc);
			// sine
			o->sine = ssineTable512[(acc >> 7)];
			// square

			o->square = (acc > 32768) ? 32767 : -32768;
			// pulse
			o->pulse = (acc > o->pulsewidth) ? 32767 : -32768;
			// white noise
			o->white_noise = sineTable512[acc >> 6] * acc - acc;
			// "pink" noise
			o->pink_noise = o->white_noise;

			// we got all the bare waveforms now
            /********************************************************** 
			 * mix/blend waveforms                                    *
             **********************************************************/
			// fixme: mush em all together, missing mix blend and so on
			acc32 = 0;
			if (o->waveforms.triangle)		acc32 += o->triangle;  	
			if (o->waveforms.saw)			acc32 += o->saw;
			if (o->waveforms.ramp)			acc32 += o->ramp;
			if (o->waveforms.sine)			acc32 += o->sine;
			if (o->waveforms.square)		acc32 += o->square;
			if (o->waveforms.pulse)			acc32 += o->pulse;
			if (o->waveforms.white_noise)	acc32 += o->white_noise;
			if (o->waveforms.pink_noise)	acc32 += o->pink_noise;
			if (!o->waveformCount)                  
				// no waveforms... mute
				acc32 = 0;

			// merge with sub osc
			acc32 += (o->subSample * o->subOscVolume) / 65536;
			acc32 /= 2;

			// fixme vel curve
			// acc = (o->velocity > 0x40) ? o->velocity : 0x40; 
			// set velocity
			acc32 *= o->velocity;
			acc32 /= 128;

			// do more magic here
			// ...
			// end of magic
			
			o->sample = acc32;
		} // individual oscillators

		// merge the two oscialltors into one stream
		tout = oscillators[0].sample;
		tout *= oscillators[0].volume;
		tout /= 32768;

		tout2 = oscillators[1].sample;
		tout2 *= oscillators[1].volume;
		tout2 /= 32768;
		
		tout += tout2;
		tout /= 4;

		// hand over merged sample to ENGINE_postProcess for fx
		tout = ENGINE_postProcess(tout);
		
		// save last sample
		voice.lastSample = tout;
		
		// write sample to output buffer 
		out = tout;

		*buffer++ = out << 16 | out;
	}
	
	// debug: measure time for 8 samples
	#ifdef ENGINE_VERBOSE
	if (!dead) {
		// send execution time via MIDI interface
		u32 delay = MIOS32_STOPWATCH_ValueGet();
		delay *= 1000;
		delay /= 333;
		MIOS32_MIDI_SendDebugMessage("%d.%d%%", delay/10, delay % 10);	

		// reset timer to measure every 6000th iteration (1Hz)
		dead = 6000;
	}
	#endif
}

// sets the waveform flags for an oscillator
void ENGINE_setOscWaveform(u8 osc, u16 flags) {
	u8 c, n = 0;
	
	if (osc > 1) return;
	
	// count number of set bits == number of waveforms
	for (c=0; c<8; c++) {
		if ((flags >> c) & 0x01)
			n++;
	}
	
	// fixme setting both in parallel
	oscillators[osc].waveforms.all = flags;
	oscillators[osc].waveformCount = n;
}

void ENGINE_setOscVolume(u8 osc, u16 vol) {
	if (osc > 1) return;
	
	oscillators[osc].volume = vol;
}

void ENGINE_setOscFinetune(u8 osc, s8 ft) {
	oscillators[osc].finetune = (ft < 0) ? -(ft * ft) / 128 : (ft * ft) / 128;
}

void ENGINE_setPitchbend(u8 osc, s16 pb) {
	if (osc == 2) {
		// set both
		ENGINE_setPitchbend(1, pb);
		osc = 0;
	}
	
	oscillator_t *o = &oscillators[osc];
	
	// save pb value
	o->pitchbend.value = pb;

	// calculate new accumulators
	o->accumValuePDown = 
		o->accumValue - 
		accumValueByNote[(o->accumNote - o->pitchbend.downRange + o->transpose)];
	o->accumValuePUp = 
		accumValueByNote[(o->accumNote + o->pitchbend.upRange + o->transpose)] - 
		o->accumValue;
	
	if ((pb > -2) && (pb < 2)) {
		// no pitchbend, pitchedAccumValue == accumValue
		o->pitchedAccumValue = o->accumValue;
	} else
	if (pb > 1) {
		// pitchbend up
		u32 new;
		
		new = pb * o->accumValuePUp;
		new /= 8191;
		o->pitchedAccumValue = 
			o->accumValue + new;
	} else {
		// pitchbend down
		u32 new;
		
		new = (-pb) * o->accumValuePDown;
		new /= 8191;
		o->pitchedAccumValue = 
			o->accumValue - new;
	}

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("pitchbend for %d: %d", osc, pb);
	#endif
}

void ENGINE_setMasterVolume(u16 vol) {
	voice.masterVolume = vol;
}

void ENGINE_setEngineFlags(u8 f) {
	engineFlags.all = f;
}

void ENGINE_setOscTranspose(u8 osc, s8 trans) {
	oscillators[osc].transpose = trans;

	// update accum values
	oscillators[osc].accumValue = accumValueByNote[noteStack[noteStackIndex].note + oscillators[osc].transpose];

	ENGINE_setPitchbend(osc, oscillators[osc].pitchbend.value);
}

void ENGINE_setOscPW(u8 osc, u16 pw) {
	// fixme: "both" mode
	oscillators[osc].pulsewidth = pw;
}

void ENGINE_setRouteDepth(u8 trgt, u16 d) {
	routing[trgt].depth = d;
}

void ENGINE_setRoute(u8 trgt, u8 src) {
	// abort on invalid target
	if (trgt >= ROUTING_TARGETS) return;

	// abort on invalid source
	if (src >= ROUTING_SOURCES) return;

	routing[trgt].source = src;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("target[%d].source = %d;", trgt, src);
	#endif
}

void ENGINE_setOverdrive(u16 od) {
	voice.overdrive = od;
}

void ENGINE_setXOR(u16 xor) {
	voice.xor = xor;
}

void ENGINE_setPitchbendUpRange(u8 osc, u8 pb) {
	oscillators[osc].pitchbend.upRange = pb;
	ENGINE_setPitchbend(0, oscillators[osc].pitchbend.value);
}

void ENGINE_setPitchbendDownRange(u8 osc, u8 pb) {
	oscillators[osc].pitchbend.downRange = pb;
	ENGINE_setPitchbend(0, oscillators[osc].pitchbend.value);
}

void ENGINE_setPortamentoRate(u8 osc, u16 portamento) {
	if (portamento < 65000)
		oscillators[osc].portaRate = portamento;
	else
		oscillators[osc].portaRate = 65000;
}

void ENGINE_setPortamentoMode(u8 osc, u8 mode) {
	oscillators[osc].portaMode = mode;
}

void ENGINE_setBitcrush(u8 resolution) {
	if (resolution < 15) 
		voice.bitcrush = resolution;
}

void ENGINE_setDelayTime(u16 time) {
	if (time < 16381)
		voice.delayTime = time;
}

void ENGINE_setDelayFeedback(u16 feedback) {
	voice.delayFeedback = feedback;
}

void ENGINE_setTriggerColumn(u8 row, u16 value) {
	if (row < TR_ROWS)
		trigger_matrix[row].all = value;
		
	#ifdef TRIGGER_VERBOSE
	MIOS32_MIDI_SendDebugMessage("trigger: column %d = %x", row, value);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].lfo1_reset, 
		trigger_matrix[1].lfo1_reset, 
		trigger_matrix[2].lfo1_reset, 
		trigger_matrix[3].lfo1_reset);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].lfo2_reset, 
		trigger_matrix[1].lfo2_reset, 
		trigger_matrix[2].lfo2_reset, 
		trigger_matrix[3].lfo2_reset);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env1_attack, 
		trigger_matrix[1].env1_attack, 
		trigger_matrix[2].env1_attack, 
		trigger_matrix[3].env1_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env1_decay, 
		trigger_matrix[1].env1_decay, 
		trigger_matrix[2].env1_decay, 
		trigger_matrix[3].env1_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env1_sustain, 
		trigger_matrix[1].env1_sustain, 
		trigger_matrix[2].env1_sustain, 
		trigger_matrix[3].env1_sustain);

		MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env1_release, 
		trigger_matrix[1].env1_release, 
		trigger_matrix[2].env1_release, 
		trigger_matrix[3].env1_release);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env2_attack, 
		trigger_matrix[1].env2_attack, 
		trigger_matrix[2].env2_attack, 
		trigger_matrix[3].env2_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env2_decay, 
		trigger_matrix[1].env2_decay, 
		trigger_matrix[2].env2_decay, 
		trigger_matrix[3].env2_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env2_sustain, 
		trigger_matrix[1].env2_sustain, 
		trigger_matrix[2].env2_sustain, 
		trigger_matrix[3].env2_sustain);

		MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		trigger_matrix[0].env2_release, 
		trigger_matrix[1].env2_release, 
		trigger_matrix[2].env2_release, 
		trigger_matrix[3].env2_release);
	#endif
}

void ENGINE_setSubOscVolume(u8 osc, u16 vol) {
	oscillators[osc].subOscVolume = vol;
}

void ENGINE_setEngine(u8 e) {
	engine = e;
	dead = 20; // debug
	
	switch (e) {
		case ENGINE_SYNTH:
			// set timer to generate samples (skips on buffer full)
			MIOS32_I2S_Stop();

			// init the synth engine
			ENGINE_init();
			
			MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &ENGINE_ReloadSampleBuffer);

			#ifdef ENGINE_VERBOSE
			MIOS32_MIDI_SendDebugMessage("LEAD engine");
			#endif
			break;
		case ENGINE_DRUM:
			// set timer to generate samples (skips on buffer full)
			MIOS32_I2S_Stop();

			DRUM_init();
			
			MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &DRUM_ReloadSampleBuffer);

			#ifdef ENGINE_VERBOSE
			MIOS32_MIDI_SendDebugMessage("DRUM engine");
			#endif
			break;
	}
}

s16 ENGINE_postProcess(s16 sample) {
	u32 uval;
	s32 tout2;
	s32 tout = sample;

	if (engineFlags.overdrive) {
		s32 od = tout;
		u16 d;
		u32 drive = voice.overdrive;

		// T_OVERDRIVE is right here
		if (routing[T_OVERDRIVE].source) {
			// there's a source assigned
			drive = ENGINE_modulateU(
						ENGINE_getModulator(routing[T_OVERDRIVE].source), 
						routing[T_OVERDRIVE].depth, drive);
		}
		
		drive *= drive;
		drive /= 65536;
		
		// unity gain in 0..2048 range
		if (drive < 2048)	
			drive = 2048;

		od *= drive;
		od /= 2048;

		if (od < -32768) od = -32768;
		if (od > 32767)  od = 32767;

		tout = od;
	}

	if (engineFlags.dcf) {
		// routing target T_CUTOFF is right here
		if (routing[T_CUTOFF].source) {
			// there's a source assigned
			// fixme: there's oddness
			tout2 = ENGINE_modulateU(filter.cutoff, ENGINE_getModulator(routing[T_CUTOFF].source), routing[T_CUTOFF].depth);
		} else {
			// not an assigned target, use manual control only
			tout2 = filter.cutoff;
		}
		
		// fixme: filters still operate on 0..65535
		tout = FILTER_filter((u16) (tout + 32768), tout2) - 32768;
	}

	// routing target T_MASTER_VOLUME is right here
	if (routing[T_MASTER_VOLUME].source) {
		// there's a source assigned
		uval = ENGINE_getModulator(routing[T_MASTER_VOLUME].source) * routing[T_MASTER_VOLUME].depth;
		uval /= 65536;
		tout *= uval;
		tout /= 65536;
	} else {
		// not an assigned target, use constant "full power"
		// nothing to see here move along
	}

	// bitcrush
	tout >>= voice.bitcrush;
	tout <<= voice.bitcrush;		
	
	// XOR
	tout ^= voice.xor;

	// set volume
	tout *= voice.masterVolume; 
	tout /= 65536;

	// add delay
	tout2 = delayBuffer[(delayIndex - voice.delayTime) & 0x3FFF];
	tout2 *= voice.delayFeedback;
	tout2 /= 65536;
	tout += tout2;
	tout >>= 1;

	// save to delay buffer
	if (delaysampled == 0) {
		delayBuffer[delayIndex & 0x3FFF] = tout;
		delayIndex++;
		delaysampled = voice.delayDownsample;
	} else
		delaysampled--;

	// median with last sample
	if (engineFlags.interpolate)
		tout = (tout + voice.lastSample) / 2;
		
	return tout;
}

void ENGINE_setDownsampling(u8 rate) {
	voice.downsample = rate;
}

void ENGINE_setDelayDownsample(u8 downsample) {
	voice.delayDownsample = downsample;
}

/////////////////////////////////////////////////////////////////////////////
// Temporary functions
/////////////////////////////////////////////////////////////////////////////

void ENGINE_setTempValue(u8 index, u16 value) {
	tempValues[index] = value;
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("%d %d", tempValues[0], tempValues[1]);
	#endif
}

void tempMute(void) {
	voice.masterVolume = 0;
}

void tempSetPulsewidth(u16 c) {
	oscillators[0].pulsewidth = c * 512;
	oscillators[1].pulsewidth = c * 512;
}
