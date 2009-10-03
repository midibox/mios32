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
#include "engine.h"
#include "defs.h"

/////////////////////////////////////////////////////////////////////////////
// Calculates the envelope state 
/////////////////////////////////////////////////////////////////////////////
void ENV_tick(void) {
	u32 out = 0;
	u8 e;

	for (e=0; e<2; e++) {
		envelope_t *env = &p.d.envelopes[e];
		u16 oacc = env->accumulator;
		
		if (!env->gate) {
			out = 0;
		} else
		// ATTACK --------------------------------------------------------
		if (env->envelopeStage == ATTACK) {
			if ((env->accumulator += env->attackAccumValue) < oacc) {
				out = 65535;
				env->accumulator = 65535;
				env->envelopeStage = DECAY;
				#ifdef ENV_VERBOSE 
				MIOS32_MIDI_SendDebugMessage("env %d: attack end", e);	
				#endif
		} else {
				// all good we're still in attack
				out = env->accumulator;
			}
		} else
		// DECAY ---------------------------------------------------------
		if (env->envelopeStage == DECAY) {
			env->accumulator -= env->decayAccumValue;
			
			if ((env->accumulator <= env->stages.sustain) ||
				(env->accumulator > oacc)) {
				out = env->stages.sustain;
				env->accumulator = env->stages.sustain;				
				env->envelopeStage = SUSTAIN;
				
				ENGINE_trigger(TRIGGER_ENV1_SUSTAIN);
				#ifdef ENV_VERBOSE 
				MIOS32_MIDI_SendDebugMessage("env %d: decay end", e);	
				#endif
			} else {
				out = env->accumulator;
			}
		} else
		// SUSTAIN -------------------------------------------------------
		if (env->envelopeStage == SUSTAIN) {
			env->accumulator = env->stages.sustain;
			out = env->stages.sustain;
		} else
		// RELEASE -------------------------------------------------------
		if (env->envelopeStage == RELEASE) {
			env->accumulator -= env->releaseAccumValue;
			
			if (env->accumulator > oacc) {
				out = 0;
				env->gate = 0;
				
				#ifdef ENV_VERBOSE 
				MIOS32_MIDI_SendDebugMessage("env %d: release end", e);	
				#endif
			} else {
				out = env->accumulator;
			}
		}
		
		out += env->out;
		out >>= 1;
		env->out = out;
	}
	
	// request routing updates
	route_update_req[RS_ENV1_OUT] = 1;
	route_update_req[RS_ENV2_OUT] = 1;
	route_update_req[RS_ENV3_OUT] = 1;

	routing_source_values[RS_ENV1_OUT] = p.d.envelopes[0].out;
	routing_source_values[RS_ENV2_OUT] = p.d.envelopes[1].out;	
}

/////////////////////////////////////////////////////////////////////////////
// Sets the envelope's attack rate
/////////////////////////////////////////////////////////////////////////////
void ENV_setAttack(u8 env, u16 v) {
	float dv = 65535;

	p.d.envelopes[env].stages.attack = v; 

	if (v) {
		dv = v;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}

	p.d.envelopes[env].attackAccumValue = (u16) dv;
	#ifdef ENV_VERBOSE 
	MIOS32_MIDI_SendDebugMessage("env %d: attack accum: %d", env, (u16) dv);
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// Sets the envelope's decay rate
/////////////////////////////////////////////////////////////////////////////
void ENV_setDecay(u8 env, u16 v) {
	float dv = 65535;

	p.d.envelopes[env].stages.attack = v; 

	if (v) {
		dv = v;
		dv /= 65536;
		dv = 100 / dv;
		dv -= 99;
	}
		
	p.d.envelopes[env].decayAccumValue = (u16) dv;
	p.d.envelopes[env].stages.decay = v; 

	#ifdef ENV_VERBOSE 
	MIOS32_MIDI_SendDebugMessage("env %d: decay accum: %d", env, (u16) dv);
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// Sets the envelope's sustain level
/////////////////////////////////////////////////////////////////////////////
void ENV_setSustain(u8 env, u16 v) {
	p.d.envelopes[env].stages.sustain = v;
}

/////////////////////////////////////////////////////////////////////////////
// Sets the envelope's release rate
/////////////////////////////////////////////////////////////////////////////
void ENV_setRelease(u8 env, u16 v) {
	u32 dv = 0xFFFFF;

	if (!(v))
		v = 1;

	dv /= v;
		
	p.d.envelopes[env].releaseAccumValue = dv;
	p.d.envelopes[env].stages.release = v; 

	#ifdef ENV_VERBOSE 
	MIOS32_MIDI_SendDebugMessage("env %d: release accum: %d", env, (u16) dv);
	#endif
}
