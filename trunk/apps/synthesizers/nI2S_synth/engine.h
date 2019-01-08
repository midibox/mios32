/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - engine module         *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _ENGINE_H
#define _ENGINE_H

#include "defs.h"
#include "types.h"
#include "tables.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void ENGINE_init(void);
void ENGINE_setEngine(u8 e);
void ENGINE_setEngineFlags(u16 f);
void ENGINE_setPitchbend(u8 osc, s16 pb);
void ENGINE_setMasterVolume(u16 vol);
void ENGINE_setPitchbendUpRange(u8 osc, u8 pb);
void ENGINE_setPitchbendDownRange(u8 osc, u8 pb);

void ENGINE_noteOff(u8 note);
void ENGINE_noteOn(u8 note, u8 vel, u8 steal);

void ENGINE_setDelayTime(u16 time);
void ENGINE_setDelayFeedback(u16 feedback);
void ENGINE_setDelayDownsample(u8 downsample);

void ENGINE_setOverdrive(u16 od);
void ENGINE_setXOR(u16 xor);
void ENGINE_setDownsampling(u8 rate);
void ENGINE_setOscWaveform(u8 osc, u16 flags);
void ENGINE_setOscTranspose(u8 osc, s8 trans);
void ENGINE_setOscVolume(u8 osc, u16 vol);
void ENGINE_setSubOscVolume(u8 osc, u16 vol);
void ENGINE_setOscPW(u8 osc, u16 pw);
void ENGINE_setOscFinetune(u8 osc, s8 ft);

void ENGINE_envelopeTick(void);
void ENGINE_setEnvAttack(u8 env, u16 v);
void ENGINE_setEnvDecay(u8 env, u16 v);
void ENGINE_setEnvSustain(u8 env, u16 v);
void ENGINE_setEnvRelease(u8 env, u16 v);

void ENGINE_setRoute(u8 trgt, u8 src);
void ENGINE_setRouteDepth(u8 trgt, u16 d);

void ENGINE_setPortamentoRate(u8 osc, u16 portamento);
void ENGINE_setPortamentoMode(u8 osc, u8 mode);

void ENGINE_setModWheel(u16 mod);

void ENGINE_setBitcrush(u8 resolution);

void ENGINE_setTriggerColumn(u8 row, u16 value);

void ENGINE_setTempValue(u8 index, u16 value);

s16 ENGINE_postProcess(s16 sample);

/////////////////////////////////////////////////////////////////////////////
// Temporary Function Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void tempSetPulsewidth(u16 c);
extern void tempMute(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u32 				sample_buffer[SAMPLE_BUFFER_SIZE]; 	// sample buffer
extern u8 				engine;					// selects the sound engine
extern u16			 	envelopeTime;			// divider for the 48kHz sample clock that clocks the env
extern u16 				lfoTime;				// divider for the 48kHz sample clock that clocks the lfof
extern u16 				tempValues[2];			// temporary values for whatever 

// extern u32				routing_source_values[ROUTE_SOURCES];
// extern u32				routing_depth_source[ROUTE_SOURCES];
// extern routing_element_t matrix[ROUTE_SOURCES+1][ROUTE_TARGETS];
extern u8 				route_update_req[ROUTE_INS];

extern route_t routes[ROUTES];					// the modulation paths
extern u16 route_ins[ROUTE_INS]; 	
extern su16 route_outs[ROUTE_OUTS];

extern patch_t default_patch;					// the default patch

extern patch_t 			p;						// the patch, fixme: redundant (well not this but all the rest)

#endif
