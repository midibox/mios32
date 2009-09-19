/****************************************************************************
 * nI2S Digital Toy Synth - ENVELOPE MODULE                                 *
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

#include <mios32.h>
#include "drum.h"
#include "engine.h"
#include "tables.h"
#include "defs.h"

#define SINE_DRUMS 4

typedef struct {
	u32 tick;
	
	u16 accumulator;		// sine wave accumulator

	u16 sineFStart;			// sine wave start frequency
	u16 sineFEnd;			// sine wave end frequency
	u16 sineFDiff;			// fStart - fEnd
	u16 sineFDecay;			// sine wave frequency decay time
	
	u16 sineAttack;			// sine wave attack time
	s32 sineEnvAccum;
	u16 sineDecay;			// sine wave decay time

	u16 noiseAttack;		// noise attack time
	u16 noiseDecay;			// noise wave decay time
	s32 noiseEnvAccum;
	u8  noiseEnv;

	u8  velocity;			
	
	u16 overdrive;			// overdrive amount
	
	u16 noiseFilterCutoff;  // noise filter cutoff
	u16 noiseFilterDecay;   // noise filter cutoff decay time
	s32 noiseFilterAccum;
	s32 lp, hp, bp;         // filter variables
	
	u16 mix;				// mix ratio of sine:noise (0 == sine only, 65535 == noise only)
	u16 mixInv;				// 65535 - --"-- 
	u16 volume;				// volume of that voice
	s16 sample;				// output sample
	
	u8 trigger; 			// note that triggers the drum
	
	unsigned gate:1;
	unsigned sineMod:1;
	unsigned waveform:1;
	unsigned sineEnv:2;
	unsigned filterType:2;
} sine_drum_t;

sine_drum_t sine_drums[SINE_DRUMS];
u16 noise_counter;
u16 dead, max = 65535;
static u8 downsampled;	    // counter for downsampling

s16 DRUM_filter(s16 input, sine_drum_t *s) {
	s32 bandpass;
	s32 highpass;
	
	s->lp += (s->noiseFilterAccum * s->bp) / 65536; 
	s->hp  = input - s->lp - s->bp; 
	s->bp += (s->noiseFilterAccum * s->hp) / 65536; 	
	bandpass = s->bp; 

	bandpass *= (65535 - s->noiseFilterAccum); 
	bandpass /= 65536;

	switch (s->filterType) {
		case 0: return s->lp;
		case 1: return bandpass;
		case 2: return s->hp;
	}
}

void DRUM_noteOff(u8 note) {
}

void DRUM_noteOn(u8 note, u8 vel, u8 steal) {
	u8 n;
	u8 drum = 255;
	
	// trigger correspoding to note
	for (n=0; n<SINE_DRUMS; n++) {
		if (note == sine_drums[n].trigger) {
			drum = n;
			break;
		}
	}

	// exit if the note isn't listed
	if (drum >= SINE_DRUMS) return;
	
	// reset voice
	sine_drums[drum].gate = 1;
	sine_drums[drum].accumulator = 0;
	sine_drums[drum].tick = 0;
	sine_drums[drum].sineMod = 1;
	sine_drums[drum].sineEnvAccum = 0;
	sine_drums[drum].sineEnv = 0;
	sine_drums[drum].noiseEnvAccum = 0;
	sine_drums[drum].noiseFilterAccum = sine_drums[0].noiseFilterCutoff;
	sine_drums[drum].noiseEnv = 0;
	sine_drums[drum].velocity = vel;

	// MIOS32_MIDI_SendDebugMessage("n:%d v:%d", note, vel);
}

/////////////////////////////////////////////////////////////////////////////
// Fills the buffer with nicey sample sounds ;D
/////////////////////////////////////////////////////////////////////////////
void DRUM_ReloadSampleBuffer(u32 state) {
	u8 n, i;
	u16 out;
	s32 fnoise, tout, tout2, ac;
	s16 noise; // noise sample

	// debug: measure time for 8 samples
	#ifdef DRUM_VERBOSE
	if (dead == 0) {
		// send execution time via MIDI interface
		u32 delay = MIOS32_STOPWATCH_ValueGet();
		max = (max < delay) ? max : delay;
		MIOS32_MIDI_SendDebugMessage("DRUM: %d uS (min %d)", delay, max);	

		// reset timer to measure every 6000th iteration (1Hz)
		dead = 6000;
	}	
	#endif
	
	u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];

	// debug: measure time it takes for 8 samples
	// decrease counter
	
	// generate new samples to output
	for(i=0; i<SAMPLE_BUFFER_SIZE/2; ++i) {
		// write sample to output buffer 
		u32 output = 0;

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
		
		// generate noise for everyone to use ;)
		// noise_counter ^= noise_counter;
		noise_counter += 321;
		noise = ssineTable512[noise_counter >> 7] * noise_counter;
		
		for (n=0; n<SINE_DRUMS; n++) {
			sine_drum_t *s = &sine_drums[n];
			
			if (!s->gate) continue;
			
			if (s->sineMod) {
				// find correct frequency (== accumulator value) depending on fStart, fEnd and tick
				// accumulatorValue(tick) = fStart - (65535/tick) * (fStart - fEnd);
				//                                                   ^^^^^^^^^^^^^---- fDiff
				ac = s->tick;
				ac *= s->sineFDecay;
				ac /= 65536;
				ac *= s->sineFDiff;
				ac /= 4096;
				
				if (ac > s->sineFDiff) {
					s->accumulator += s->sineFEnd;
					s->sineMod = 0;
				} else {
					s->accumulator += s->sineFStart - ac;
				}
			} else {
				s->accumulator += s->sineFEnd;
			}
			
			// get sine from table
			tout = (s->waveform) ? tout = s->accumulator - 32767 : ssineTable512[s->accumulator >> 7];
			
			// apply sine AD envelope
			if (s->sineEnv == 0) {
				s->sineEnvAccum += s->sineAttack;
				
				if (s->sineEnvAccum > 65535) {
					s->sineEnv = 1;
					s->sineEnvAccum = 65535;
				}
				// attack stage
			} else 
			if (s->sineEnv == 1) {
				s->sineEnvAccum -= s->sineDecay;
				
				if (s->sineEnvAccum < 0) {
					s->sineEnvAccum = 0;
					s->sineEnv = 2;
				}
			}
			tout *= s->sineEnvAccum;
			tout /= 65536;

			// filter Decay
			s->noiseFilterAccum -= s->noiseFilterDecay;
			if (s->noiseFilterAccum < 0) {
				s->noiseFilterAccum = 0;
			}
				
			// filter noise
			fnoise = DRUM_filter(noise, s);
			
			// apply noise AD envelope
			if (s->noiseEnv == 0) {
				s->noiseEnvAccum += s->noiseAttack;
				
				if (s->noiseEnvAccum > 65535) {
					s->noiseEnv = 1;
					s->noiseEnvAccum = 65535;
				}
				// attack stage
			} else 
			if (s->noiseEnv == 1) {
				s->noiseEnvAccum -= s->noiseDecay;
				
				if (s->noiseEnvAccum < 0) {
					s->noiseEnvAccum = 0;
					s->noiseEnv = 2;
				}
			}
			fnoise *= s->noiseEnvAccum;
			fnoise /= 65536;
			
			if ((s->sineEnv == 2) && (s->noiseEnv == 2)) {
				// both voices are done
				s->gate = 0;
				s->sample = 0;
				continue;
			}
			
			// blend noise and sine
			tout *= s->mixInv;
			tout /= 65536;
			fnoise *= s->mix;
			fnoise /= 65536;

			tout += fnoise;
			tout /= 2;

			// overdrive
			tout *= (s->overdrive);
			if (tout >  32767) tout =  32767;
			if (tout < -32767) tout = -32767;

			// volume
			tout *= (s->volume);
			tout /= 65536;

			// velocity
			tout *= (s->velocity);
			tout /= 127;

			s->sample = tout;
			
			s->tick++;		// tick tock!
		}

		// downsampling ***********************************************************
		// T_SAMPLERATE is right here
		ac = p.voice.downsample;
		if (p.routing[T_SAMPLERATE].source) {
			// there's a source assigned
			ac = ENGINE_modulateU(ac, ENGINE_getModulator(p.routing[T_SAMPLERATE].source), p.routing[T_SAMPLERATE].depth);
			if (downsampled > ac)
				downsampled = ac;
		}
		
		if (ac != downsampled) {
			out = p.voice.lastSample;
			*buffer++ = out << 16 | out;
			downsampled++;
			continue;
		} else
			downsampled = 0;

		output = 
			sine_drums[0].sample + 
			sine_drums[1].sample +
			sine_drums[2].sample + 
			sine_drums[3].sample;

		output = ENGINE_postProcess(output);
		
		out = output;

		p.voice.lastSample = out;
	
		*buffer++ = out << 16 | out;
	}

	#ifdef DRUM_VERBOSE
	dead--;
	if (dead == 0) {
		MIOS32_STOPWATCH_Reset();
	}
	#endif
}

void DRUM_setSineDrum_SineFreqInitial(u8 voice, u16 value) {
	sine_drums[voice].sineFStart = (value * value) >> 14;
	
	sine_drums[voice].sineFDiff =
		sine_drums[voice].sineFStart - sine_drums[voice].sineFEnd;
}

void DRUM_setSineDrum_SineFreqEnd(u8 voice, u16 value) {
	sine_drums[voice].sineFEnd = ((value * value) >> 16) + 22;

	sine_drums[voice].sineFDiff =
		sine_drums[voice].sineFStart - sine_drums[voice].sineFEnd;
} 

void DRUM_setSineDrum_SineFreqDecay(u8 voice, u16 value) {
	sine_drums[voice].sineFDecay = (value > 5) ? value : 5;
}

void DRUM_setSineDrum_Mix(u8 voice, u16 value) {
	sine_drums[voice].mix = value;
	sine_drums[voice].mixInv = 65535 - value;
}

void DRUM_setSineDrum_Volume(u8 voice, u16 value) {
	sine_drums[voice].volume = value;
}

void DRUM_setSineDrum_Overdrive(u8 voice, u16 value) {
	u32 drive = (value * value) / 65536;
	
	drive /= 1024;
	drive += 1;
	
	sine_drums[voice].overdrive = drive;
}

void DRUM_setSineDrum_NoiseCutoff(u8 voice, u16 value) {
	sine_drums[voice].noiseFilterCutoff = 
		(value > 2047) ? value / 4 : 512;
}

void DRUM_setSineDrum_SineAttack(u8 voice, u16 value) {
	float dv = 65535;

	if (value) {
		dv = value;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}

	sine_drums[voice].sineAttack = (u16) dv;
}

void DRUM_setSineDrum_SineDecay(u8 voice, u16 value) {
	float dv = 65535;

	if (value) {
		dv = value;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}

	sine_drums[voice].sineDecay = (u16) dv;
}

void DRUM_setSineDrum_NoiseAttack(u8 voice, u16 value) {
	float dv = 65535;

	if (value) {
		dv = value;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}

	sine_drums[voice].noiseAttack = (u16) dv;
}

void DRUM_setSineDrum_NoiseCutoffDecay(u8 voice, u16 value) {
	value = (65535 - value) / 4096;
	
	sine_drums[voice].noiseFilterDecay = value;
}

void DRUM_setSineDrum_NoiseDecay(u8 voice, u16 value) {
	float dv = 65535;

	if (value) {
		dv = value;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}

	sine_drums[voice].noiseDecay = (u16) dv;
}

void DRUM_setSineDrum_Waveform(u8 index, u8 value) {
	sine_drums[index].waveform = value;
}

void DRUM_setSineDrum_FilterType(u8 index, u8 value) {
	sine_drums[index].filterType = value;
	
	#ifdef DRUM_VERBOSE
	MIOS32_MIDI_SendDebugMessage("filter %d:%d", index, sine_drums[index].filterType);
	#endif
}

void DRUM_init(void) {
	noise_counter = 22222;
	
	sine_drums[0].trigger = 48;
	sine_drums[1].trigger = 50;
	sine_drums[2].trigger = 52;
	sine_drums[3].trigger = 53;
}

void DRUM_setSineDrum_TriggerNote(u8 index, u8 value) {
	sine_drums[index].trigger = value;

	#ifdef DRUM_VERBOSE
	MIOS32_MIDI_SendDebugMessage("trigger %d:%d", index, value);
	#endif
}
