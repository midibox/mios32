/****************************************************************************
 * nI2S Digital Toy Synth - LFO MODULE                                      *
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

#include "defs.h"
#include "types.h"
#include "engine.h"
#include "lfo.h"

/////////////////////////////////////////////////////////////////////////////
// sets the LFOs rate
/////////////////////////////////////////////////////////////////////////////
void LFO_setFreq(u8 lfo, u16 frq) {
	u16 d;
	
	d = frq >> 8;
	d *= d;
	d >>= 6;
	d += 1;
	
	p.d.lfos[lfo].accumValue = d;
	p.d.lfos[lfo].frequency = frq;
}

/////////////////////////////////////////////////////////////////////////////
// sets the pulsewidth for the pulse waveform
/////////////////////////////////////////////////////////////////////////////
void LFO_setPW(u8 lfo, u16 pw) {
	p.d.lfos[lfo].pulsewidth = pw;
}

/////////////////////////////////////////////////////////////////////////////
// sets the waveform of the LFO
/////////////////////////////////////////////////////////////////////////////
void LFO_setWaveform(u8 lfo, u8 wav) {
	u8 c, n = 0;

	p.d.lfos[lfo].waveforms.all = wav;
	
	// count number of set bits == number of waveforms
	for (c=0; c<8; c++) {
		if ((wav >> c) & 0x01)
			n++;
	}
	
	p.d.lfos[lfo].waveformCount = n;
}

/////////////////////////////////////////////////////////////////////////////
// calculates the LFO sample
/////////////////////////////////////////////////////////////////////////////
void LFO_tick(void) {
	u8 lfo;
	u32 acc32;
	u16 acc;
	
	for (lfo=0; lfo<2; lfo++) {
		lfo_t *l = &p.d.lfos[lfo];
		
		if (!l->waveformCount) continue;
		
		acc = l->accumulator;
		l->accumulator += l->accumValue;
		
		if (acc > l->accumulator) {
			// lfo period done
			ENGINE_trigger(TRIGGER_LFO_PERIOD + lfo);
		}
		
		acc = l->accumulator;

		/********************************************************** 
		 * get raw waveforms                                      *
		 **********************************************************/

		// triangle
		l->triangle = (acc < 32768) ? acc << 1 : 65535 - (acc << 1);
		// saw
		l->saw = acc;
		// ramp
		l->ramp = 65535 - acc;
		// sine
		l->sine = sineTable512[acc >> 7];
		// square
		l->square = (acc > 0x8000) ? 65535 : 0;
		// pulse
		l->pulse = (acc > l->pulsewidth) ? 65535 : 0;
		// white noise
		l->white_noise = sineTable512[acc >> 7] * acc - acc;
		// "pink" noise
		l->pink_noise = sineTable512[acc >> 7];

		// we got all the bare waveforms now

		/********************************************************** 
		 * mix/blend waveforms                                    *
		 **********************************************************/
		acc32 = 0;

		if (l->waveforms.triangle)		acc32 += l->triangle; 
		if (l->waveforms.saw)			acc32 += l->saw;
		if (l->waveforms.ramp)			acc32 += l->ramp;
		if (l->waveforms.sine)			acc32 += l->sine;
		if (l->waveforms.square)		acc32 += l->square;  	
		if (l->waveforms.pulse)			acc32 += l->pulse;  	
		if (l->waveforms.white_noise)	acc32 += l->white_noise;  	
		if (l->waveforms.pink_noise)	acc32 += l->pink_noise;  	
												
		if (l->waveformCount == 0)
			// no waveforms... mute
			acc32 = 32768; // dc offset == no output

		// fixme: add cool clip mode to the regular add-and-div
		l->out = acc32;
	}

	// request routing updates
	route_update_req[RS_LFO1_OUT] = 1;
	route_update_req[RS_LFO2_OUT] = 1;
	route_update_req[RS_LFO3_OUT] = 1;

	routing_source_values[RS_LFO1_OUT] = p.d.lfos[0].out;
	routing_source_values[RS_LFO2_OUT] = p.d.lfos[1].out;
}
