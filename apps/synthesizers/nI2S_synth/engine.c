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
	
/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <math.h>
#include <FreeRTOS.h>
#include <portmacro.h>

#include "defs.h"
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

// routing sources per target
	   u16 envelopeTime;	// divider for the 48kHz sample clock that clocks the env
       u16 lfoTime;			// divider for the 48kHz sample clock that clocks the lfof

static u16 dead = 1;   	    // debug variable for sample calc overlap / timing

static u16 delayIndex;		// write index of delayBuffer
static u16 chorusIndex;		// write index of chorusBuffer
static u16 chorusAccum;		// chorus "lfo" accumulator

	   u16 tempValues[2];	// temporary values for whatever 

	   u8 engine;			// selects the sound engine
static u8 downsampled;	    // counter for downsampling
static u8 delaysampled;	    // counter for downsampling of delay

patch_t   p;				// the actual patch

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

void ENGINE_ReloadSampleBuffer(u32 state);

/////////////////////////////////////////////////////////////////////////////
// returns the modulation source value for a given ID
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_getModulator(u8 src) {
	switch (src) {
		case S_LFO1: 			return p.lfos[0].out;
		case S_LFO2: 			return p.lfos[1].out;
		case S_ENVELOPE1:		return p.envelopes[0].out;
		case S_ENVELOPE2:		return p.envelopes[1].out;
	}
}

/////////////////////////////////////////////////////////////////////////////
// a trigger point has been reached, take appropriate action
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_trigger(u8 trigger) {
	if (p.trigger_matrix[trigger].lfo1_reset) { 
		p.lfos[0].accumulator = 0;	// reset lfo accumulator
		
		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 1 Reset", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].lfo2_reset) { 
		p.lfos[1].accumulator = 0;	// reset lfo accumulator

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 2 Reset", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env1_attack) { 
		p.envelopes[0].accumulator = 0;		 // reset attack accumulator
		p.envelopes[0].gate = 1;		 		 // open gate
		p.envelopes[0].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Attack", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env1_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		// edit: on second thought, it does... release still doesn't though
		p.envelopes[0].accumulator = 65535;
		p.envelopes[0].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Decay", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env1_sustain) { 
		// set sustain level to accum and change stage
		p.envelopes[0].accumulator = p.envelopes[0].stages.sustain;
		p.envelopes[0].envelopeStage = SUSTAIN;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Sustain", trigger);
		#endif		

		// we reach sustain stage, hence a new trigger
		ENGINE_trigger(TRIGGER_ENV1_SUSTAIN);
	}

	if (p.trigger_matrix[trigger].env1_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		p.envelopes[0].envelopeStage = RELEASE;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Release", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env2_attack) { 
		p.envelopes[1].accumulator = 0;		 // reset attack accumulator
		p.envelopes[1].gate = 1;		 		 // open gate
		p.envelopes[1].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Attack", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env2_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		// edit: on second thought, it does... release still doesn't though
		p.envelopes[1].accumulator = 65535;
		p.envelopes[1].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Decay", trigger);
		#endif		
	}

	if (p.trigger_matrix[trigger].env2_sustain) { 
		// set sustain level to accum and change statge
		p.envelopes[1].accumulator = p.envelopes[1].stages.sustain;
		p.envelopes[1].envelopeStage = SUSTAIN;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Sustain", trigger);
		#endif		

		ENGINE_trigger(TRIGGER_ENV2_SUSTAIN);
	}

	if (p.trigger_matrix[trigger].env2_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		p.envelopes[1].envelopeStage = RELEASE;

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

	for (n=0; n<32; n++)
		p.name[n] = 'A' + n % 26;

	noteStackLen = 0;
	for (n=0; n<NOTESTACK_SIZE; n++) {
		noteStack[n].note = 0;
	}
	
	engine = ENGINE_SYNTH;
	
	p.oscillators[0].pulsewidth = 16384;
	p.oscillators[1].pulsewidth = 16384;
	
	p.oscillators[0].transpose = 0;
	p.oscillators[1].transpose = -12;
	
	p.oscillators[0].volume = 65535;
	p.oscillators[1].volume = 65535;

	p.oscillators[0].pitchbend.upRange = 2;
	p.oscillators[0].pitchbend.downRange = 12;

	p.oscillators[1].pitchbend.upRange = 2;
	p.oscillators[1].pitchbend.downRange = 24;

	ENGINE_setOscWaveform(0, 0x08);
	ENGINE_setOscWaveform(0, 0x10);

	p.voice.masterVolume = 65535;
	p.voice.delayDownsample = 0;
	delaysampled = 0;

	p.filter.filterType = FILTER_RES_LP;
	p.filter.cutoff = 65535;
	p.filter.resonance = 0;

	ENV_setAttack(0, 0);
	ENV_setDecay(0, 0);
	ENV_setSustain(0, 65535);
	ENV_setRelease(0, 0);

	ENV_setAttack(1, 1235);
	ENV_setDecay(1, 32767);
	ENV_setSustain(1, 50000);
	ENV_setRelease(1, 32767);

	p.routing[T_CUTOFF].source = S_NONE;
	p.routing[T_MASTER_VOLUME].source = S_NONE;
	p.routing[T_OSC1_PITCH].source = S_NONE;
	p.routing[T_OSC2_PITCH].source = S_NONE;

	p.routing[T_CUTOFF].depth = 0;
	p.routing[T_MASTER_VOLUME].depth = 0;
	p.routing[T_OSC1_PITCH].depth = 0;
	p.routing[T_OSC2_PITCH].depth = 0;
	
	p.engineFlags.reattackOnSteal = 0;
	p.engineFlags.interpolate = 1;
	p.engineFlags.syncOsc2 = 0;
	p.engineFlags.dcf = 0;
	
	p.oscillators[0].portaMode = PORTA_NONE;
	p.oscillators[1].portaMode = PORTA_NONE;

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
						ENGINE_noteOn(noteStack[p].note, noteStack[p].velocity, STEAL);
						
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
	pA1 = p.oscillators[0].portaStart;
	pA2 = p.oscillators[1].portaStart;

	u8 reattack;
	
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(%d, %d, %d)", note, vel, steal);
	#endif

	#ifdef TRIGGER_VERBOSE
	if (note || vel) {
		MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(): real note call");
	} else {
		MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(): transpose call");
	} 
	#endif

	// fixme: split into two depending on channel

	// is this not just a transpose rushing through?
	if (note || vel) {
		// it's a real note, set the stuff
		p.oscillators[0].accumNote = note;
		p.oscillators[1].accumNote = note;

		p.oscillators[0].velocity = vel;
		p.oscillators[1].velocity = vel;
	}

	p.oscillators[0].accumValue = accumValueByNote[p.oscillators[0].accumNote + p.oscillators[0].transpose];
	p.oscillators[1].accumValue = accumValueByNote[p.oscillators[1].accumNote + p.oscillators[1].transpose];

	// is this not just a transpose rushing through?
	if (note || vel) {
		// it's a real note, set the stuff
		noteStackIndex = (noteStackIndex + 1) & (NOTESTACK_SIZE - 1);
		noteStack[noteStackIndex].note = note;
		noteStack[noteStackIndex].velocity = vel;

		// fixme: reset envelope if desired should be from trigger / same for porta
		if ((noteStackLen == 0) || p.engineFlags.reattackOnSteal) {
			// trigger matrix NOTE_ON
			#ifdef TRIGGER_VERBOSE
			MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(): triggering NOTE ON");
			#endif

			ENGINE_trigger(TRIGGER_NOTEON); 
		}

		if (!steal)
			noteStackLen++;
	}
	
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("noteStackLen = %d", noteStackLen);
	#endif

	ENGINE_setPitchbend(0, p.oscillators[0].pitchbend.value);
	ENGINE_setPitchbend(1, p.oscillators[1].pitchbend.value);

	reattack = (noteStackLen > 1) || steal;
	
	if ((p.oscillators[0].portaMode) && reattack) {
		p.oscillators[0].portaStart = pA1;
	} else {
		p.oscillators[0].portaStart = p.oscillators[0].pitchedAccumValue;
	}
	
	if ((p.oscillators[1].portaMode) && reattack) {
		p.oscillators[1].portaStart = pA2;
	} else {
		p.oscillators[1].portaStart = p.oscillators[1].pitchedAccumValue;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Fills the buffer with nicey sample sounds ;D
/////////////////////////////////////////////////////////////////////////////
void ENGINE_ReloadSampleBuffer(u32 state) {
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
		o = &p.oscillators[0];
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

		if (p.routing[T_OSC1_PITCH].source) {
			// there's a source assigned
			ac = ENGINE_modulateU(ac, ENGINE_getModulator(p.routing[T_OSC1_PITCH].source), p.routing[T_OSC1_PITCH].depth);
			// ac += (ENGINE_getModulator(p.routing[T_OSC1_PITCH].source) >> 3);
			// ac >>= 1;
			// fixme: blend it in!
		} 
		
		ac += o->finetune;
		o->accumulator += ac;
		ac >>= 1;
		o->subAccumulator += ac;
		
		// oscillator 2
		o = &p.oscillators[1];
		
		if ((p.engineFlags.syncOsc2) && (p.oscillators[0].accumulator < utout2)) 
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
			
			if (p.routing[T_OSC2_PITCH].source) {
				// there's a source assigned
				utout2 = ENGINE_modulateU(utout2, ENGINE_getModulator(p.routing[T_OSC2_PITCH].source), p.routing[T_OSC2_PITCH].depth);
				// fixme: blend it in!
			} 

			utout2 += utout2 + o->finetune;
			o->accumulator += utout2;
			utout2 >>= 1;
			o->subAccumulator += utout2;
		}
	
		// downsampling ***********************************************************
		// T_SAMPLERATE is right here
		utout = p.voice.downsample;
		if (p.routing[T_SAMPLERATE].source) {
			// there's a source assigned
			utout = ENGINE_modulateU(utout, ENGINE_getModulator(p.routing[T_SAMPLERATE].source), p.routing[T_SAMPLERATE].depth);
			if (downsampled > utout)
				downsampled = utout;
		}
		
		if (utout != downsampled) {
			out = p.voice.lastSample;
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
			o = &p.oscillators[osc];

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

			// fixme: vel curve
			// acc = (o->velocity > 0x40) ? o->velocity : 0x40; 
			// set velocity
			acc32 *= o->velocity;
			acc32 /= 128;

			// do more magic here
			// ...
			// end of magic
			
			o->sample = acc32;
		} // individual oscillators

		// merge the two oscillators into one stream
		tout = p.oscillators[0].sample;
		tout *= p.oscillators[0].volume;
		tout /= 32768;

		tout2 = p.oscillators[1].sample;
		tout2 *= p.oscillators[1].volume;
		tout2 /= 32768;
		
		if (p.engineFlags.ringmod) {
			tout /= 2;
			tout2 /= 2;
			tout *= tout2;
			tout /= 65536;
		} else {
			tout += tout2;
			tout /= 4;
		}

/* PHASE DISTORTION FUN
		tout = p.oscillators[0].sample;
		tout *= p.oscillators[0].volume;
		tout /= 32768;

		tout2 = p.oscillators[1].sample;
		tout2 *= p.oscillators[0].accumValue;
		tout2 /= 65536;
		tout2 *= p.oscillators[1].volume;
		tout2 /= 32768;

		tout += tout2;
		tout /= 4;
*/

		// hand over merged sample to ENGINE_postProcess for fx
		tout = ENGINE_postProcess(tout);
		
		// save last sample
		p.voice.lastSample = tout;
		
		// write sample to output buffer 
		out = tout;

		*buffer++ = out << 16 | out;
	}
	
	// debug: measure time for 8 samples
	#ifdef ENGINE_VERBOSE_MAX
	if (!dead) {
		// send execution time via MIDI interface
		u32 delay = MIOS32_STOPWATCH_ValueGet();
		delay *= 1000;
		delay /= 333;
		MIOS32_MIDI_SendDebugMessage("d.%d%%", delay/10, delay % 10);	

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
	p.oscillators[osc].waveforms.all = flags;
	p.oscillators[osc].waveformCount = n;
}

void ENGINE_setOscVolume(u8 osc, u16 vol) {
	if (osc > 1) return;
	
	p.oscillators[osc].volume = vol;
}

void ENGINE_setOscFinetune(u8 osc, s8 ft) {
	p.oscillators[osc].finetune = (ft < 0) ? -(ft * ft) / 128 : (ft * ft) / 128;
}


void ENGINE_setOscTranspose(u8 osc, s8 trans) {
	p.oscillators[osc].transpose = trans;

	// update accum values
	p.oscillators[osc].accumValue = accumValueByNote[noteStack[noteStackIndex].note + p.oscillators[osc].transpose];

	// ENGINE_setPitchbend(2, 0);
	// if there's a note playing only reset the pitch, do not affect the noteStack
	if (noteStackLen)
		ENGINE_noteOn(0, 0, STEAL);
}

void ENGINE_setPitchbend(u8 osc, s16 pb) {
	if (osc == 2) {
		// set both
		ENGINE_setPitchbend(1, pb);
		osc = 0;
	}
	
	oscillator_t *o = &p.oscillators[osc];
	
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
	p.voice.masterVolume = vol;
}

void ENGINE_setEngineFlags(u16 f) {
	p.engineFlags.all = f;
	
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("engineFlags: %d / %d", p.engineFlags.all, f);
	#endif
}

void ENGINE_setOscPW(u8 osc, u16 pw) {
	// fixme: "both" mode
	p.oscillators[osc].pulsewidth = pw;
}

void ENGINE_setRouteDepth(u8 trgt, u16 d) {
	p.routing[trgt].depth = d;
}

void ENGINE_setRoute(u8 trgt, u8 src) {
	// abort on invalid target
	if (trgt >= ROUTING_TARGETS) return;

	// abort on invalid source
	if (src >= ROUTING_SOURCES) return;

	p.routing[trgt].source = src;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("target[%d].source = %d;", trgt, src);
	#endif
}

void ENGINE_setOverdrive(u16 od) {
	p.voice.overdrive = od;
}

void ENGINE_setXOR(u16 xor) {
	p.voice.xor = xor;
}

void ENGINE_setPitchbendUpRange(u8 osc, u8 pb) {
	p.oscillators[osc].pitchbend.upRange = pb;
	ENGINE_setPitchbend(0, p.oscillators[osc].pitchbend.value);
}

void ENGINE_setPitchbendDownRange(u8 osc, u8 pb) {
	p.oscillators[osc].pitchbend.downRange = pb;
	ENGINE_setPitchbend(0, p.oscillators[osc].pitchbend.value);
}

void ENGINE_setPortamentoRate(u8 osc, u16 portamento) {
	if (portamento < 65000)
		p.oscillators[osc].portaRate = portamento;
	else
		p.oscillators[osc].portaRate = 65000;
}

void ENGINE_setPortamentoMode(u8 osc, u8 mode) {
	p.oscillators[osc].portaMode = mode;
}

void ENGINE_setBitcrush(u8 resolution) {
	if (resolution < 15) 
		p.voice.bitcrush = resolution;
}

void ENGINE_setDelayTime(u16 time) {
	if (time < 16381)
		p.voice.delayTime = time;
}

void ENGINE_setDelayFeedback(u16 feedback) {
	p.voice.delayFeedback = feedback;
}

void ENGINE_setChorusTime(u16 time) {
	u16 d;
	
	d = time >> 8;
	d *= d;
	d >>= 6;
	d += 1;
	
	p.voice.chorusTime = d;
}

void ENGINE_setChorusFeedback(u16 feedback) {
	p.voice.chorusFeedback = feedback;
}

void ENGINE_setTriggerColumn(u8 row, u16 value) {
	if (row < TR_ROWS)
		p.trigger_matrix[row].all = value;
		
	#ifdef TRIGGER_VERBOSE
	MIOS32_MIDI_SendDebugMessage("trigger: column %d = %x", row, value);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].lfo1_reset, 
		p.trigger_matrix[1].lfo1_reset, 
		p.trigger_matrix[2].lfo1_reset, 
		p.trigger_matrix[3].lfo1_reset);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].lfo2_reset, 
		p.trigger_matrix[1].lfo2_reset, 
		p.trigger_matrix[2].lfo2_reset, 
		p.trigger_matrix[3].lfo2_reset);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env1_attack, 
		p.trigger_matrix[1].env1_attack, 
		p.trigger_matrix[2].env1_attack, 
		p.trigger_matrix[3].env1_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env1_decay, 
		p.trigger_matrix[1].env1_decay, 
		p.trigger_matrix[2].env1_decay, 
		p.trigger_matrix[3].env1_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env1_sustain, 
		p.trigger_matrix[1].env1_sustain, 
		p.trigger_matrix[2].env1_sustain, 
		p.trigger_matrix[3].env1_sustain);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env1_release, 
		p.trigger_matrix[1].env1_release, 
		p.trigger_matrix[2].env1_release, 
		p.trigger_matrix[3].env1_release);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env2_attack, 
		p.trigger_matrix[1].env2_attack, 
		p.trigger_matrix[2].env2_attack, 
		p.trigger_matrix[3].env2_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env2_decay, 
		p.trigger_matrix[1].env2_decay, 
		p.trigger_matrix[2].env2_decay, 
		p.trigger_matrix[3].env2_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env2_sustain, 
		p.trigger_matrix[1].env2_sustain, 
		p.trigger_matrix[2].env2_sustain, 
		p.trigger_matrix[3].env2_sustain);

		MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.trigger_matrix[0].env2_release, 
		p.trigger_matrix[1].env2_release, 
		p.trigger_matrix[2].env2_release, 
		p.trigger_matrix[3].env2_release);
	#endif
}

void ENGINE_setSubOscVolume(u8 osc, u16 vol) {
	p.oscillators[osc].subOscVolume = vol;
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

	if (p.engineFlags.overdrive) {
		u16 d;
		u32 drive = p.voice.overdrive;

		// T_OVERDRIVE is right here
		if (p.routing[T_OVERDRIVE].source) {
			// there's a source assigned
			drive = ENGINE_modulateU(
						ENGINE_getModulator(p.routing[T_OVERDRIVE].source), 
						p.routing[T_OVERDRIVE].depth, drive);
		}
		
		drive *= drive;
		drive /= 65536;
		
		// unity gain in 0..2048 range
		if (drive < 2048)	
			drive = 2048;

		tout *= drive;
		tout /= 2048;

		// clip
		if (tout < -32768)
			tout = -32768;
		else
		if (tout > 32767)
			tout = 32767;
	} // drive

	if (p.engineFlags.dcf && p.filter.filterType) {
		// routing target T_CUTOFF is right here
		if (p.routing[T_CUTOFF].source) {
			// there's a source assigned
			// fixme: there's oddness
			uval = ENGINE_modulateU(p.filter.cutoff, ENGINE_getModulator(p.routing[T_CUTOFF].source), p.routing[T_CUTOFF].depth);
		} else {
			// not an assigned target, use manual control only
			uval = p.filter.cutoff;
		}
		
		// fixme: filters still operates on 0..65535, working on it ;)
		tout = FILTER_filter(tout, uval);
	} // filter

	// routing target T_MASTER_VOLUME is right here
	if (p.routing[T_MASTER_VOLUME].source) {
		// there's a source assigned
		uval = ENGINE_getModulator(p.routing[T_MASTER_VOLUME].source) * p.routing[T_MASTER_VOLUME].depth;
		uval /= 65536;
		tout *= uval;
		tout /= 65536;
	} else {
		// not an assigned target, use constant "full power"
		// nothing to see here move along
	}

	// fixme: bitcrush
	tout >>= p.voice.bitcrush;
	tout <<= p.voice.bitcrush;		
	
	// XOR
	tout ^= p.voice.xor;

	// add chorus
	if (p.engineFlags.chorus) {
		// optimize-me: math?
		// accumulate time shift
		chorusAccum += p.voice.chorusTime;
		// get sinewave
		uval = sineTable512[chorusAccum >> 7];
		// "log" 
		uval *= sqrtTable[p.voice.chorusFeedback >> 7];
		uval = sqrtTable[uval >> 23];
		// get into desired timing range
		uval /= 432;
		// offset with base time
		uval += 193;
		tout2 = chorusBuffer[(chorusIndex - uval) & 0x1FFF];
		tout += tout2;
		tout /= 2;
 
		// save to chorus buffer
		chorusBuffer[chorusIndex & 0x1FFF] = tout;
		chorusIndex++;
	}

	// add delay
	if (p.engineFlags.delay) {
		tout2 = delayBuffer[(delayIndex - p.voice.delayTime) & 0x3FFF];
		tout2 *= p.voice.delayFeedback;
		tout2 /= 65536;
		tout += tout2;
		tout /= 2; // fixme: this shouldn't be delay/2 but /(1+(delayFeeback/65536))

		// save to delay buffer
		if (delaysampled) {
			delaysampled--;
		} else {
			delayBuffer[delayIndex & 0x3FFF] = tout;
			delayIndex++;
			delaysampled = p.voice.delayDownsample;
		}		
	}

	// median with last sample
	if (p.engineFlags.interpolate)
		tout = (tout + p.voice.lastSample) / 2;
		
	// set volume
	tout *= p.voice.masterVolume; 
	tout /= 65536;

	return tout;
}

void ENGINE_setDownsampling(u8 rate) {
	p.voice.downsample = rate;
}

void ENGINE_setDelayDownsample(u8 downsample) {
	p.voice.delayDownsample = downsample;
}

/////////////////////////////////////////////////////////////////////////////
// Temporary functions
/////////////////////////////////////////////////////////////////////////////

void ENGINE_setTempValue(u8 index, u16 value) {
	tempValues[index] = value;
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("Temp values: %d %d", tempValues[0], tempValues[1]);
	#endif
}

void tempMute(void) {
	p.voice.masterVolume = 0;
}

void tempSetPulsewidth(u16 c) {
	p.oscillators[0].pulsewidth = c * 512;
	p.oscillators[1].pulsewidth = c * 512;
}
