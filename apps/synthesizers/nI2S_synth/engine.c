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

u16 route_ins[ROUTE_INS]; 					// in-/outputs to mod path (in = 
su16 route_outs[ROUTE_OUTS]; 				// from lfo, ... / out = values to
											// be read where a mod target is needed
route_t routes[ROUTES];						// the modulation paths
u32 sample_buffer[SAMPLE_BUFFER_SIZE]; 		// sample buffer Tx
static note_t noteStack[NOTESTACK_SIZE];	// last played notes
static u8 noteStackIndex;					// index of last note
static s8 noteStackLen;						// number of notes playing

	   u16 envelopeTime;					// divider for the 48kHz sample clock that clocks the env
       u16 lfoTime;							// divider for the 48kHz sample clock that clocks the lfof

static u16 dead = 1;   	    				// debug variable for sample calc overlap / timing

static u16 delayIndex;						// write index of delayBuffer
static u16 chorusIndex;						// write index of chorusBuffer
static u16 chorusAccum;						// chorus "lfo" accumulator

	   u16 tempValues[2];					// temporary values for whatever 

	   u8 engine;							// selects the sound engine
static u8 downsampled;	 				   	// counter for downsampling
static u8 delaysampled;					    // counter for downsampling of delay

static u16 bcpattern;						// the bitcrush pattern

	   u8 	  route_update_req[ROUTE_INS];
	   char   routing_signed[ROUTE_SOURCES] = {0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // signs for correct routing behaviour when scaling
	   u8*    routing_signed_ptr = &routing_signed[0];

patch_t p;									// the actual patch

// kinda outdated but still working
patch_t default_patch = {
	.all = {
		0x44,0x65,0x66,0x61,0x75,0x6C,0x74,0x20,0x50,0x61,0x74,0x63,0x68,0x20,0x28,0x6E,
		0x65,0x77,0x29,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,
		0x00,0x00,0x66,0x16,0xFF,0x3F,0xFF,0xFF,0x01,0x00,0x01,0x00,0x13,0x04,0x2D,0x01,
		0xFF,0xFF,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0xFF,0x7F,0xFF,0x7F,0x50,0xC3,
		0xFF,0x7F,0xEA,0xFF,0x57,0x14,0x65,0x00,0x20,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
		0x00,0x40,0x00,0x00,0x2D,0x95,0xFF,0xFF,0x69,0x75,0x00,0x00,0x97,0x8A,0xFF,0xFF,
		0x9C,0x5B,0x00,0x00,0x00,0x80,0xFF,0xFF,0xFF,0x7F,0x00,0x00,0x24,0xF6,0x96,0x11,
		0x2A,0xE6,0x56,0x21,0xC9,0x76,0xCE,0x45,0xE6,0x00,0x00,0x00,0x5C,0x36,0x00,0x00,
		0xE6,0x00,0x1C,0x00,0x72,0x00,0xE6,0x00,0x34,0x01,0x7F,0x00,0xFF,0xFF,0x32,0xF3,
		0x00,0x00,0x00,0x00,0xF4,0x00,0x02,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0xE6,0x00,
		0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x9C,0xFC,0xFF,0xFF,
		0xCE,0xF7,0xFF,0xFF,0x32,0xEB,0xFF,0xFF,0x48,0xC2,0xFF,0xFF,0xFF,0x7F,0x00,0x00,
		0xFF,0x7F,0x00,0x00,0x30,0x01,0x75,0x05,0x5E,0x2B,0xE9,0x23,0x4E,0x24,0xC7,0x2A,
		0xB3,0x02,0x00,0x00,0xCE,0x2B,0x00,0x00,0xCC,0x01,0x39,0x00,0x58,0x01,0xCC,0x01,
		0x34,0x01,0x7F,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x18,
		0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0xFF,0xFF,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x1D,0xFD,0x00,0x00,0x44,0x00,0x00,0x00,0x20,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	}
};

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

void ENGINE_ReloadSampleBuffer(u32 state);

void ENGINE_updateModPaths() {
	u32 r, s;
	s32 in; 

	// reset all outputs (prolly temp)
	for (r=0; r<ROUTE_OUTS; r++)
		route_outs[r].u16 = 65535; // full blast
 
	// update each route
	// optimize-here: can I not calc some pwease ;)
	for (r=0; r<ROUTES; r++) {
		route_t* route = &routes[r];				// get the current route

		if (route->outputid)						// this route has a set output (outputid != RT_NIL)
		for (s=0; s<ROUTE_INPUTS_PER_PATH; s++) {	// iterate over all inputs
			if (route->inputid[s]) {				// inputid != RS_NIL
				// real mod source
					if ((u8)* (routing_signed_ptr+route->inputid[s])) {
					// ^-- if (routing_signed[s]) {
						// routing a signed value (lfo, ...)
						in = route_ins[route->inputid[s]];
						in -= 32768;
						in *= route->depth[s];
						in /= 32768;
					} else {
						// routing an unsigned value (env, ...)
						in = route_ins[route->inputid[s]];
						in *= route->depth[s];
						in /= 32768;

						in = (route->depth[s] < 0) ? in + 32768 : in - 32768;
						in *= 2;
					}
				
				in += route->offset[s];

				if (in > 32767) in = 32767;
				if (in < -32768) in = -32768;

				route->output[s] = in;				// write value to internal buffer
			} else {
				// source is RS_NIL
				route->output[s] = 0;
			}
		}
	}

	// all cells have been recalced. sum 'em up
	for (r=0; r<ROUTES; r++) {						// iterate over each route
		route_t* route = &routes[r];				// get the route
		in = 0;										// clear the sum

		for (s=0; s<4; s++) {						// iterate over precalced values
			in += route->output[s];					// sum em up
		}

		if (in > 32767) in = 32767;					// clip the value
		if (in < -32768) in = -32768;				// -"-

		// fixme: bring in the depth source

		route_outs[route->outputid].s16 = in;			// write value to route_outs
	}
}

/////////////////////////////////////////////////////////////////////////////
// a trigger point has been reached, take appropriate action
/////////////////////////////////////////////////////////////////////////////
u16 ENGINE_trigger(u8 trigger) {
	if (p.d.trigger_matrix[trigger].lfo1_reset) { 
		p.d.lfos[0].accumulator = 0;	// reset lfo accumulator
		
		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 1 Reset", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].lfo2_reset) { 
		p.d.lfos[1].accumulator = 0;	// reset lfo accumulator

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> LFO 2 Reset", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env1_attack) { 
		p.d.envelopes[0].accumulator = 0;		 // reset attack accumulator
		p.d.envelopes[0].gate = 1;		 		 // open gate
		p.d.envelopes[0].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Attack", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env1_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		// edit: on second thought, it does... release still doesn't though
		p.d.envelopes[0].accumulator = 65535;
		p.d.envelopes[0].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Decay", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env1_sustain) { 
		// set sustain level to accum and change stage
		p.d.envelopes[0].accumulator = p.d.envelopes[0].stages.sustain;
		p.d.envelopes[0].envelopeStage = SUSTAIN;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Sustain", trigger);
		#endif		

		// we reach sustain stage, hence a new trigger
		ENGINE_trigger(TRIGGER_ENV1_SUSTAIN);
	}

	if (p.d.trigger_matrix[trigger].env1_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		p.d.envelopes[0].envelopeStage = RELEASE;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 1 Release", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env2_attack) { 
		p.d.envelopes[1].accumulator = 0;		 // reset attack accumulator
		p.d.envelopes[1].gate = 1;		 		 // open gate
		p.d.envelopes[1].envelopeStage = ATTACK; // reset to ATTACK

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Attack", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env2_decay)	{ 
		// accum doesn't need to be reset, as the decay just lowers the volume
		// from wherever
		// edit: on second thought, it does... release still doesn't though
		p.d.envelopes[1].accumulator = 65535;
		p.d.envelopes[1].envelopeStage = DECAY;	

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Decay", trigger);
		#endif		
	}

	if (p.d.trigger_matrix[trigger].env2_sustain) { 
		// set sustain level to accum and change statge
		p.d.envelopes[1].accumulator = p.d.envelopes[1].stages.sustain;
		p.d.envelopes[1].envelopeStage = SUSTAIN;

		#ifdef TRIGGER_VERBOSE
		MIOS32_MIDI_SendDebugMessage("trigger: %d -> Env 2 Sustain", trigger);
		#endif		

		ENGINE_trigger(TRIGGER_ENV2_SUSTAIN);
	}

	if (p.d.trigger_matrix[trigger].env2_release) { 
		// accum doesn't need to be reset, as the release just lowers the volume
		// from wherever
		p.d.envelopes[1].envelopeStage = RELEASE;

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
	u8 n, i;
	
	// mp: new² mod paths, init all outputs to 32767 (should prolly be 0 for some)
	for (n=0; n<ROUTE_INS; n++)
		route_ins[n] = 32767;

	for (n=0; n<ROUTE_OUTS; n++)
		route_outs[n].u16 = 65535;

	for (n=0; n<ROUTES; n++) {
		for (i=0; i<ROUTE_INPUTS_PER_PATH; i++)
			routes[n].inputid[i] = RS_NIL;
		routes[n].outputid = RT_NIL;
	}

	// temp: assign mod path 1: lfo 1 out to osc 1 pitch
	routes[0].inputid[0] = RS_LFO1_OUT;
	routes[0].inputid[1] = RS_LFO2_OUT;
	routes[0].depth[0] = 32767;
	routes[0].offset[0] = 0;
	routes[0].depth[1] = 32767;
	routes[0].offset[1] = 0;
	routes[0].outputid = RT_OSC1_PITCH;

	for (n=0; n<32; n++)
		p.d.name[n] = 'A' + n % 26;

	noteStackLen = 0; 
	for (n=0; n<NOTESTACK_SIZE; n++) {
		noteStack[n].note = 0;
	}
	
	engine = ENGINE_SYNTH;

	#ifdef ENGINE_VERBOSE_MAX
    // debug: initialize the stopwatch for 100 uS resolution
    MIOS32_STOPWATCH_Init(1);
	#endif

	route_ins[RS_CONSTANT] = 32768;

	// start I2S DMA transfers
	MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &ENGINE_ReloadSampleBuffer);

	MIOS32_MIDI_SendDebugMessage("Done booting.");
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
	pA1 = p.d.oscillators[0].portaStart;
	pA2 = p.d.oscillators[1].portaStart;

	u8 n;
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
		p.d.oscillators[0].accumNote = note;
		p.d.oscillators[1].accumNote = note;

		p.d.oscillators[0].velocity = vel;
		p.d.oscillators[1].velocity = vel;
	}

	p.d.oscillators[0].accumValue = accumValueByNote[p.d.oscillators[0].accumNote + p.d.oscillators[0].transpose];
	p.d.oscillators[1].accumValue = accumValueByNote[p.d.oscillators[1].accumNote + p.d.oscillators[1].transpose];

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("Accumulators: %d, %d", p.d.oscillators[0].accumValue, p.d.oscillators[1].accumValue);
	#endif

	// is this not just a transpose rushing through?
	if (note || vel) {
		// it's a real note, set the stuff
		noteStackIndex = (noteStackIndex + 1) & (NOTESTACK_SIZE - 1);
		noteStack[noteStackIndex].note = note;
		noteStack[noteStackIndex].velocity = vel;

		// fixme: reset envelope if desired should be from trigger / same for porta
		if ((noteStackLen == 0) || p.d.engineFlags.reattackOnSteal) {
			// trigger matrix NOTE_ON
			#ifdef TRIGGER_VERBOSE
			MIOS32_MIDI_SendDebugMessage("ENGINE_noteOn(): triggering NOTE ON");
			#endif

			ENGINE_trigger(TRIGGER_NOTEON); 
		}

		if (!steal)
			noteStackLen++;
	}
	
	// calculate new upper and lower boundaries for pitchbend 
	for (n=0; n<2; n++) {
		p.d.oscillators[n].accumValuePDown = 
			p.d.oscillators[n].accumValue - 
			accumValueByNote[(p.d.oscillators[n].accumNote - p.d.oscillators[n].pitchbend.downRange + p.d.oscillators[n].transpose)];
		p.d.oscillators[n].accumValuePUp = 
			accumValueByNote[(p.d.oscillators[n].accumNote + p.d.oscillators[n].pitchbend.upRange + p.d.oscillators[n].transpose)] - 
			p.d.oscillators[n].accumValue;
	}
		
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("noteStackLen = %d", noteStackLen);
	#endif

	// set pitch bend for both oscs
	ENGINE_setPitchbend(2, p.d.oscillators[0].pitchbend.value);

	reattack = (noteStackLen > 1) || steal;
	
	if ((p.d.oscillators[0].portaMode) && reattack) {
		p.d.oscillators[0].portaStart = pA1;
	} else {
		p.d.oscillators[0].portaStart = p.d.oscillators[0].pitchedAccumValue;
	} 
	
	if ((p.d.oscillators[1].portaMode) && reattack) {
		p.d.oscillators[1].portaStart = pA2;
	} else {
		p.d.oscillators[1].portaStart = p.d.oscillators[1].pitchedAccumValue;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Checks and updates the routing matrix output values
/////////////////////////////////////////////////////////////////////////////
/*
#ifdef ROUTING_VERBOSE
u32 _cells_sum = 0;
u32 _sources_sum = 0;
u32 _upd_counter = 0;
u16 _cells_max = 0;
u16 _sources_max = 0;
u16 _cells = 0;
u16 _sources = 0;
u16 _none = 0;
u16 _counter = 0;
u16 _howoften[ROUTE_SOURCES];
#endif

void ENGINE_updateRoutingOutputs() {
	u32 s, t;
	s32	sum;
	s32 a;
	u8 sources = 0;
	routing_element_t* re;
	
	#ifdef ROUTING_VERBOSE
	_upd_counter++;

	_cells = 0;
	_sources = 0;
	#endif

	// iterate over all sources...
	for (s=1; s<ROUTE_SOURCES; s++)
		// ...that have changed
		if (route_update_req[s]) {
			sources++;
			
			#ifdef ROUTING_VERBOSE
			_howoften[s]++;
			_sources++;
			#endif

			// this source has changed. recalc all values in this row
			for (t=0; t<ROUTE_TARGETS; t++) {
				re = &matrix[s][t];
				// recalc
				if (re->depth) {
					#ifdef ROUTING_VERBOSE
					_cells++;
					#endif

					if ((u8)* (routing_signed_ptr+s)) {
//					^-- if (routing_signed[s]) {
						// routing a signed value (lfo, ...)
						a = routing_source_values[s];
						a -= 32768;
						a *= re->depth;
						a /= 32768;
					} else {
						// routing an unsigned value (env, ...)
						a = routing_source_values[s];
						a *= re->depth;
						a /= 32768;

						a = (re->depth < 0) ? a + 32768 : a - 32768;
						a *= 2;
					}

					a += re->offset;

					// clip output
					if (a > 32767) a = 32767;
					if (a < -32768)	a = -32768;

					re->out = a;
				} else {
					// depth = 0, no effect
					re->out = 0;
				}
			}
		}

	#ifdef ROUTING_VERBOSE
	if (_sources == 0)
	  _none++;
	#endif

	// optimize-here: can't I NOT calc some of those?
	// all relevant values are update, recalc sums
	if (sources)
	for (t=0; t<ROUTE_TARGETS; t++) {
		sum = 0;
		u16 rds = routing_depth_source[t];
		
		for (s=1; s<ROUTE_SOURCES; s++) {
			re = &matrix[s][t];
			if (rds != s) {
				sum += re->out;
			}
		} 

		// fixme: optional stacking (sum, clip) or median (sum / count)?

		// prevent clipping
		if (sum > 32767) sum =  32767;
		if (sum < -32768) sum = -32768; 

		// routing depth
		if (rds) {
			sum *= (32768 + matrix[rds][t].out);
			sum /= 65536;
		}

		matrix[RS_OUT][t].out = sum;
	}

	// kill update requests
	for (s=1; s<ROUTE_SOURCES; s++)
		route_update_req[s] = 0;

	#ifdef ROUTING_VERBOSE
	if (_sources > _sources_max) _sources_max = _sources;
	if (_cells > _cells_max) _cells_max = _cells;

	_cells_sum += _cells;
	_sources_sum += _sources;
 
	if (_upd_counter == 4095) {
		MIOS32_MIDI_SendDebugMessage("ENGINE_updateRoutingOutputs(): source max: %d, cell max: %d, source_avg: %d, cell_avg: %d", _sources_max, _cells_max, (_sources_sum / _upd_counter), (_cells_sum / _upd_counter));
		MIOS32_MIDI_SendDebugMessage("  Out of %d iterations %d did nothing.", _upd_counter, _none);
		_none = 0;
		_cells_sum = 0;
		_sources_sum = 0;
		_cells_max = 0;
		_sources_max = 0;
		
		_upd_counter = 0;

		MIOS32_MIDI_SendDebugMessage("%d %d %d %d %d %d %d %d %d %d ", _howoften[1], _howoften[2], _howoften[3], _howoften[4], _howoften[5], _howoften[6], _howoften[7], _howoften[8], _howoften[9], _howoften[10]);
		for (s=1; s<ROUTE_SOURCES; s++) {
			_howoften[s] = 0;
		}
	}
	#endif
}
*/

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
	#ifdef ENGINE_VERBOSE_MAX
	dead--;
	MIOS32_STOPWATCH_Reset();
	#endif 

	// check for routing update requests
	// ENGINE_updateRoutingOutputs();
	// new one again
	ENGINE_updateModPaths();

	// generate new samples to output
	for(i=0; i<SAMPLE_BUFFER_SIZE/CHANNELS; ++i) {
		u8 osc;
		oscillator_t *o;

		// tick the envelopes
		envelopeTime++;
		
		if (envelopeTime > ENVELOPE_RESOLUTION) {
			envelopeTime = 0;
			ENV_tick();
		}

		// tick the lfos
		lfoTime++;
		
		if (lfoTime > LFO_RESOLUTION) {
			lfoTime = 0;
			LFO_tick();
		}
		
		/* OSCILLATOR ACCUMULATORS *******************************************/
		// calculate oscillator accumulators individually
		o = &p.d.oscillators[0];
		utout2 = o->accumulator;
		ac = o->pitchedAccumValue;

		// porta mode?
		if (o->portaMode != PORTA_NONE)
		if (o->portaStart != o->pitchedAccumValue) {
			ac = o->portaStart + (o->accumValue - o->pitchedAccumValue);
			o->portaTick += o->portaRate;
			
			if (o->portaTick > 0xFFFFE) {
				o->portaTick = 0;
				
				// porta time up
				if (o->portaStart < o->pitchedAccumValue)
					o->portaStart += 1;
				else
					o->portaStart -= 1;
			}
		}

/*
		if (p.d.routing[T_OSC1_PITCH].source) {
			// there's a source assigned
			ac = ENGINE_modulateU(ac, ENGINE_getModulator(p.d.routing[T_OSC1_PITCH].source), p.d.routing[T_OSC1_PITCH].depth);
			// ac += (ENGINE_getModulator(p.d.routing[T_OSC1_PITCH].source) >> 3);
			// ac >>= 1;
			// fixme: blend it in!
		} 
*/
		// pitch mod
		tout = route_outs[RT_OSC1_PITCH].s16;
		tout *= ac;
		tout >>= 15;
		ac += tout;

		ac += o->finetune;
		o->accumulator += ac;
		ac >>= 1;
		o->subAccumulator += ac;
		
		// oscillator 2
		o = &p.d.oscillators[1];
		
		if ((p.d.engineFlags.syncOsc2) && (p.d.oscillators[0].accumulator < utout2)) 
			o->accumulator = 0;
		else {
			// T_OSC2_PITCH is right here
			utout2 = o->pitchedAccumValue;
			
			// porta mode?
			if (o->portaMode != PORTA_NONE)
			if (o->portaStart != o->pitchedAccumValue) {
				utout2 = o->portaStart  + (o->accumValue - o->pitchedAccumValue);
				o->portaTick += o->portaRate;
				
				if (o->portaTick >= 0xFFFF) {
					o->portaTick = 0;
					
					// porta time up
					if (o->portaStart < o->pitchedAccumValue)
						o->portaStart += 1;
					else
						o->portaStart -= 1;
				}
			}
			
			// pitch mod 2
			tout = route_outs[RT_OSC2_PITCH].s16;
			tout *= utout2;
			tout /= 32768;
			utout2 += tout;


			utout2 += o->finetune;
			o->accumulator += utout2;
			utout2 >>= 1;
			o->subAccumulator += utout2;
		}
	
		// downsampling ***********************************************************
		// T_SAMPLERATE is right here
		utout = p.d.voice.downsample;
		utout *= route_outs[RT_DOWNSAMPLE].u16;
		utout /= 65536;
		utout >>= 15;
		if (downsampled > utout)
			downsampled = utout;
		
		if (utout != downsampled) {
			out = p.d.voice.lastSample;
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
			o = &p.d.oscillators[osc];

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
		o = &p.d.oscillators[0];
		tout = o->sample;
		tout *= o->volume;
		tout >>= 14;

		o = &p.d.oscillators[1];
		tout2 = o->sample;
		tout2 *= o->volume;
		tout2 >>= 14;
		
		if (p.d.engineFlags.ringmod) {
			tout /= 4;
			tout2 /= 4;
			tout *= tout2;
			tout /= 65536;
		} else {
			tout += tout2;
			tout /= 8;
		}

/* PHASE DISTORTION FUN
		tout = p.d.oscillators[0].sample;
		tout *= p.d.oscillators[0].volume;
		tout /= 32768;

		tout2 = p.d.oscillators[1].sample;
		tout2 *= p.d.oscillators[0].accumValue;
		tout2 /= 65536;
		tout2 *= p.d.oscillators[1].volume;
		tout2 /= 32768;

		tout += tout2;
		tout /= 4;
*/  

		// hand over merged sample to ENGINE_postProcess for fx
		tout = ENGINE_postProcess(tout);
		
		// save last sample
		p.d.voice.lastSample = tout;
		
		// write sample to output buffer 
		out = tout;
 
		*buffer++ = out << 16 | out;
	}

	// debug: stop measuring time here
	#ifdef ENGINE_VERBOSE_MAX
	if (!dead) {
		// send execution time via MIDI interface
		u32 delay = MIOS32_STOPWATCH_ValueGet();
		delay *= 1000;
		delay /= 333;
		MIOS32_MIDI_SendDebugMessage("%d.%d%%", delay/10, delay % 10);	

		// reset timer to measure every 12000th iteration (0.5Hz)
		dead = 12000;
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
	p.d.oscillators[osc].waveforms.all = flags;
	p.d.oscillators[osc].waveformCount = n;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("ENGINE_setOscWaveform(%d, %d);", osc, flags);
	#endif
}

void ENGINE_setOscVolume(u8 osc, u16 vol) {
	if (osc > 1) return;
	
	p.d.oscillators[osc].volume = vol;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("ENGINE_setOscVolume(%d, %d);", osc, vol);
	#endif
}

void ENGINE_setOscFinetune(u8 osc, s8 ft) {
	p.d.oscillators[osc].finetune = (ft < 0) ? -(ft * ft) / 128 : (ft * ft) / 128;
}


void ENGINE_setOscTranspose(u8 osc, s8 trans) {
	p.d.oscillators[osc].transpose = trans;

	// update accum values
	p.d.oscillators[osc].accumValue = accumValueByNote[noteStack[noteStackIndex].note + p.d.oscillators[osc].transpose];

	// ENGINE_setPitchbend(2, 0);
	// if there's a note playing only reset the pitch, do not affect the noteStack
	if (noteStackLen)
		ENGINE_noteOn(0, 0, STEAL);

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("ENGINE_setOscTranspose(%d, %d);", osc, trans);
	#endif
}

void ENGINE_setPitchbend(u8 osc, s16 pb) {
	if (osc == 2) {
		// set both
		ENGINE_setPitchbend(0, pb);
		osc = 1;
	}
	
	oscillator_t *o = &p.d.oscillators[osc];
	
	// save pb value
	o->pitchbend.value = pb;

	if ((pb > -2) && (pb < 2)) {
		// no pitchbend (well none withing +-2MSB jitter), pitchedAccumValue == accumValue
		o->pitchedAccumValue = o->accumValue;
	} else
	if (pb > 1) {
		// pitchbend up
		u32 new;
		
		new = pb * o->accumValuePUp;
		new /= 8191;
		o->pitchedAccumValue = o->accumValue + new;
		#ifdef ENGINE_VERBOSE
		MIOS32_MIDI_SendDebugMessage("ENGINE_setPitchbend(): pitchbend up for osc %d: %d", osc, pb);
		#endif
	} else {
		// pitchbend down
		u32 new;
		
		new = (-pb) * o->accumValuePDown;
		new /= 8191;
		o->pitchedAccumValue = o->accumValue - new;

		#ifdef ENGINE_VERBOSE
		MIOS32_MIDI_SendDebugMessage("ENGINE_setPitchbend(): pitchbend down for osc %d: %d", osc, pb);
		#endif
	}
}

void ENGINE_setMasterVolume(u16 vol) {
	p.d.voice.masterVolume = vol;
}

void ENGINE_setEngineFlags(u16 f) {
	p.d.engineFlags.all = f;
	
	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("engineFlags: %d / %d", p.d.engineFlags.all, f);
	#endif
}

void ENGINE_setOscPW(u8 osc, u16 pw) {
	// fixme: "both" mode
	p.d.oscillators[osc].pulsewidth = pw;
}

void ENGINE_setRouteDepth(u8 trgt, u16 d) {
	p.d.routing[trgt].depth = d;
}

void ENGINE_setRoute(u8 trgt, u8 src) {
	// abort on invalid target
	if (trgt >= ROUTING_TARGETS) return;

	// abort on invalid source
	if (src >= ROUTING_SOURCES) return;

	p.d.routing[trgt].source = src;

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("target[%d].source = %d;", trgt, src);
	#endif
}

void ENGINE_setOverdrive(u16 od) {
	p.d.voice.overdrive = od;
}

void ENGINE_setXOR(u16 xor) {
	p.d.voice.xor = xor;
}

void ENGINE_setPitchbendUpRange(u8 osc, u8 pb) {
	p.d.oscillators[osc].pitchbend.upRange = pb;
	ENGINE_setPitchbend(0, p.d.oscillators[osc].pitchbend.value);
}

void ENGINE_setPitchbendDownRange(u8 osc, u8 pb) {
	p.d.oscillators[osc].pitchbend.downRange = pb;
	ENGINE_setPitchbend(0, p.d.oscillators[osc].pitchbend.value);
}

void ENGINE_setPortamentoRate(u8 osc, u16 portamento) {
	if (portamento < 65000)
		p.d.oscillators[osc].portaRate = portamento;
	else
		p.d.oscillators[osc].portaRate = 65000;
}

void ENGINE_setPortamentoMode(u8 osc, u8 mode) {
	p.d.oscillators[osc].portaMode = mode;
}

void ENGINE_setBitcrush(u8 resolution) {
	if (resolution > 14) return;

	p.d.voice.bitcrush = resolution;

	bcpattern = bcpatterns[resolution];

	#ifdef ENGINE_VERBOSE
	MIOS32_MIDI_SendDebugMessage("ENGINE_setBitcrush(%d); -> %d", resolution, bcpattern);
	#endif
}

void ENGINE_setDelayTime(u16 time) {
	if (time < 16381)
		p.d.voice.delayTime = time;
}

void ENGINE_setDelayFeedback(u16 feedback) {
	p.d.voice.delayFeedback = feedback;
}

void ENGINE_setChorusTime(u16 time) {
	u16 d;
	
	d = time >> 8;
	d *= d;
	d >>= 6;
	d += 1;
	
	p.d.voice.chorusTime = d;
}

void ENGINE_setChorusFeedback(u16 feedback) {
	p.d.voice.chorusFeedback = feedback;
}

void ENGINE_setTriggerColumn(u8 row, u16 value) {
	if (row < TR_ROWS)
		p.d.trigger_matrix[row].all = value;
		
	#ifdef TRIGGER_VERBOSE
	MIOS32_MIDI_SendDebugMessage("trigger: column %d = %x", row, value);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].lfo1_reset, 
		p.d.trigger_matrix[1].lfo1_reset, 
		p.d.trigger_matrix[2].lfo1_reset, 
		p.d.trigger_matrix[3].lfo1_reset);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].lfo2_reset, 
		p.d.trigger_matrix[1].lfo2_reset, 
		p.d.trigger_matrix[2].lfo2_reset, 
		p.d.trigger_matrix[3].lfo2_reset);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env1_attack, 
		p.d.trigger_matrix[1].env1_attack, 
		p.d.trigger_matrix[2].env1_attack, 
		p.d.trigger_matrix[3].env1_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env1_decay, 
		p.d.trigger_matrix[1].env1_decay, 
		p.d.trigger_matrix[2].env1_decay, 
		p.d.trigger_matrix[3].env1_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env1_sustain, 
		p.d.trigger_matrix[1].env1_sustain, 
		p.d.trigger_matrix[2].env1_sustain, 
		p.d.trigger_matrix[3].env1_sustain);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env1_release, 
		p.d.trigger_matrix[1].env1_release, 
		p.d.trigger_matrix[2].env1_release, 
		p.d.trigger_matrix[3].env1_release);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env2_attack, 
		p.d.trigger_matrix[1].env2_attack, 
		p.d.trigger_matrix[2].env2_attack, 
		p.d.trigger_matrix[3].env2_attack);
	
	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env2_decay, 
		p.d.trigger_matrix[1].env2_decay, 
		p.d.trigger_matrix[2].env2_decay, 
		p.d.trigger_matrix[3].env2_decay);

	MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env2_sustain, 
		p.d.trigger_matrix[1].env2_sustain, 
		p.d.trigger_matrix[2].env2_sustain, 
		p.d.trigger_matrix[3].env2_sustain);

		MIOS32_MIDI_SendDebugMessage("%d %d %d %d", 
		p.d.trigger_matrix[0].env2_release, 
		p.d.trigger_matrix[1].env2_release, 
		p.d.trigger_matrix[2].env2_release, 
		p.d.trigger_matrix[3].env2_release);
	#endif
}

void ENGINE_setSubOscVolume(u8 osc, u16 vol) {
	p.d.oscillators[osc].subOscVolume = vol;
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

	if (p.d.engineFlags.overdrive) {
		u16 d;
		u32 drive = p.d.voice.overdrive;
		drive *= route_outs[RT_OVERDRIVE].u16;  
		drive /= 65536;										

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

	// filter
	if (p.d.engineFlags.dcf && p.d.filter.filterType) {
		uval = p.d.filter.cutoff; 					
		uval *= route_outs[RT_FILTER_CUTOFF].u16; 
		uval /= 65536;								
		
		tout = FILTER_filter(tout, uval);
	} // filter

	// master volume
	uval = p.d.voice.masterVolume;
	uval *= route_outs[RT_VOLUME].u16;  
	uval /= 65536;									
	tout *= uval;
	tout /= 65536;
	
/* fixme :-)
	// routing target T_MASTER_VOLUME is right here
	if (p.d.routing[T_MASTER_VOLUME].source) {
		// there's a source assigned
		uval = ENGINE_getModulator(p.d.routing[T_MASTER_VOLUME].source) * p.d.routing[T_MASTER_VOLUME].depth;
		uval /= 65536;
		tout *= uval;
		tout /= 65536;
	} else {
		// not an assigned target, use constant "full power"
		// nothing to see here move along
	}
*/
	// bitcrush
	tout = ((tout + 32768) & bcpattern) - 32768;
	
	// XOR
	tout ^= p.d.voice.xor;

	// add chorus
	if (p.d.engineFlags.chorus) {
		// optimize-me: math?
		// accumulate time shift
		chorusAccum += p.d.voice.chorusTime;
		// get sinewave
		uval = sineTable512[chorusAccum >> 7];
		// "log" 
		uval *= sqrtTable[p.d.voice.chorusFeedback >> 7];
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
	if (p.d.engineFlags.delay) {
	        tout2 = delayBuffer[(delayIndex - p.d.voice.delayTime) % DELAY_BUFFER_SIZE];
		tout2 *= p.d.voice.delayFeedback;
		tout2 /= 65536;
		tout += tout2;
		tout /= 2; // fixme: this shouldn't be delay/2 but /(1+(delayFeeback/65536))

		// save to delay buffer
		if (delaysampled) {
			delaysampled--;
		} else {
		        delayBuffer[delayIndex % DELAY_BUFFER_SIZE] = tout;
			delayIndex++;
			delaysampled = p.d.voice.delayDownsample;
		}		
	}

	// median with last sample
	if (p.d.engineFlags.interpolate)
		tout = (tout + p.d.voice.lastSample) / 2;
		
	// set volume
	tout *= p.d.voice.masterVolume; 
	tout /= 65536;

	return tout;
}

void ENGINE_setDownsampling(u8 rate) {
	p.d.voice.downsample = rate;
}

void ENGINE_setDelayDownsample(u8 downsample) {
	p.d.voice.delayDownsample = downsample;
}

void ENGINE_setModWheel(u16 mod) {
	route_ins[RS_MODWHEEL] = mod;
	route_update_req[RS_MODWHEEL] = 1;
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
	p.d.voice.masterVolume = 0;
}

void tempSetPulsewidth(u16 c) {
	p.d.oscillators[0].pulsewidth = c * 512;
	p.d.oscillators[1].pulsewidth = c * 512;
}
