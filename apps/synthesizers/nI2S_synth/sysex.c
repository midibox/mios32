/****************************************************************************
 * nI2S Digital Toy Synth - SYSEX PARSER                                    *
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
#include "app.h"
#include "engine.h"
#include "sysex.h"

/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static void SYSEX_CmdFinished(u8 bufLen);
static void SYSEX_Cmd(u8 cmd_state, u8 midi_in);

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

#define WRITE_PARAM		0
#define RESET			127

#define IDLE 			0
#define HEADER 			1
#define DATA 			2

// Headers used by MIDIbox applications are documented here:
// http://svnmios.midibox.org/filedetails.php?repname=svn.mios&path=%2Ftrunk%2Fdoc%2FSYSEX_HEADERS
const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4D };

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// TODO: use malloc function instead of a global array to save RAM
u8 sysex_buffer[2048];
u16 bufIndex; 
u16 state;

/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
void SYSEX_Init(void) {
	bufIndex = 0;
	state = IDLE;

	// install SysEx parser
	MIOS32_MIDI_SysExCallback_Init(SYSEX_Parser);
}

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Parser(mios32_midi_port_t port, u8 midi_in) {
	if (midi_in == 0xF0) {
		state = HEADER;
		bufIndex = 0;
		#ifdef SYSEX_VERBOSE
		MIOS32_MIDI_SendDebugMessage("F0");
		#endif
		} else
	if ((midi_in == 0xF7) && (state == DATA)) {
		state = IDLE;
		#ifdef SYSEX_VERBOSE
		MIOS32_MIDI_SendDebugMessage("F7");
		#endif
		SYSEX_CmdFinished(bufIndex);
	}
	
	if (state == IDLE) // no new message started and we're not in a message
	  return 1; // don't forward package to APP_MIDI_NotifyPackage()
	
	if (state == HEADER) {
		if (midi_in != sysex_header[bufIndex]) {
			// not our header... ignore and quit.
			state = IDLE;
			#ifdef SYSEX_VERBOSE
			MIOS32_MIDI_SendDebugMessage("wrong header. got %d != %d", midi_in, sysex_header[bufIndex]);
			#endif
		}

		bufIndex++;
		
		// are we done with the header?
		if (bufIndex == 5) {
			state = DATA;
			bufIndex = 0;
			#ifdef SYSEX_VERBOSE
			MIOS32_MIDI_SendDebugMessage("header done");
			#endif
		}
	} else
	if ((state == DATA) && (midi_in < 0x80)) {
		// received valid data
		#ifdef SYSEX_VERBOSE
		MIOS32_MIDI_SendDebugMessage("data");
		#endif
		sysex_buffer[bufIndex] = midi_in;
		bufIndex++;
	}

  return 1; // don't forward package to APP_MIDI_NotifyPackage()

}

/////////////////////////////////////////////////////////////////////////////
// This function puts 3 7-bit values back together to one 16-bit value
/////////////////////////////////////////////////////////////////////////////
u16 syxToU16(u8 in[]) {
	u16 value;
	value = in[0];
	value <<= 7;
	value += in[1];
	value <<= 7;
	value += in[2];
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command 
/////////////////////////////////////////////////////////////////////////////
void SYSEX_CmdFinished(u8 bufLen) {
	u8 command = sysex_buffer[0];
	u8 index;
	
	// MIOS32_MIDI_SendDebugMessage("%d bytes | com: %d", bufLen, sysex_buffer[0]);

	if (command == RESET) {
		ENGINE_init();
		return;
	}
	
	if ((command == WRITE_PARAM) && (bufLen == 68)) {
		// patch name
		u8 n;
		char name[33];

		if (syxToU16(&sysex_buffer[1]) != 0x0003) return;
		
		for (n=2; n<bufLen/2; n++) {
			name[n-2] = sysex_buffer[n*2] * 64 + sysex_buffer[n*2+1];

			if (name[n-2] == 0)
				name[n-2] = ' ';
		}

		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("Patch name: %s", &name[0]);
		#endif

		APP_setPatchName(&name[0]);
	} else
	if ((command == WRITE_PARAM) && (bufLen == 7)) {
		// direct parameter write
		u16 address = syxToU16(&sysex_buffer[1]);
		u16 value = syxToU16(&sysex_buffer[4]);
		s16 svalue = value - 32768;
		
		#ifdef SYSEX_VERBOSE
		MIOS32_MIDI_SendDebugMessage("Received: a:%d d:%d", address, value);
		#endif

		// toggle addresses for drums
		if (address >= 0x8000) {
			index = (address >> 8) & 0x0F;
			address &= 0xF0FF;
			MIOS32_MIDI_SendDebugMessage("i:%d a:%d", index, address);
		}

		// new routing matrix - depth + offset
		if ((address >= 0x1000) && (address < 0x1800))  {
			if (address < 0x1200) {
				matrix[1 + (address - 0x1000) / 32][(address - 0x1000) & 31].offset = svalue;
				route_update_req[1 + (address - 0x1000) / 32] = 1;
				#ifdef SYSEX_VERBOSE
				MIOS32_MIDI_SendDebugMessage("Offset at row %d, column %d is %d\n", 1 + (address - 0x1000) / 32, (address - 0x1000) & 31, svalue);
				#endif
			} else {
				matrix[1 + (address - 0x1400) / 32][(address - 0x1400) & 31].depth = svalue;
				route_update_req[1 + (address - 0x1400) / 32] = 1;
				#ifdef SYSEX_VERBOSE
				MIOS32_MIDI_SendDebugMessage("Depth at row %d, column %d is %d\n", 1 + (address - 0x1400) / 32,(address - 0x1400) & 31, svalue);
				#endif
			}

			return;
		}

		// new routing matrix - depth selection
		if ((address >= 0x1800) && (address < 0x2000))  {
			routing_depth_source[address - 0x1800] = value;
			#ifdef SYSEX_VERBOSE 
			MIOS32_MIDI_SendDebugMessage("Route depth for column %d is %d\n", address - 0x1800, value);
			#endif

			return;
		}
		
		switch (address) {
			case 0x000: // Mastervolume
				ENGINE_setMasterVolume(value); break;
			case 0x001: // engineFlags
				ENGINE_setEngineFlags(value); break;
			case 0x002: // engine selection
				ENGINE_setEngine(value); break;

			case 0x040: // overdrive
				ENGINE_setOverdrive(value); break;
			case 0x041: // bitcrush
				ENGINE_setBitcrush((u8) value); break;
			case 0x042: // XOR 
				ENGINE_setXOR(value); break;
			case 0x043: // Downsample
				ENGINE_setDownsampling(value); break;
				
			case 0x050: // delay time
				ENGINE_setDelayTime(value); break;
			case 0x051: // delay feedback
				ENGINE_setDelayFeedback(value); break;
			case 0x052: // delay downsample
				ENGINE_setDelayDownsample(value); break;
				
			case 0x053: // chorus time
				ENGINE_setChorusTime(value); break;
			case 0x054: // chorus feedback/depth
				ENGINE_setChorusFeedback(value); break;

			case 0x100: // Envelope 1: Attack
				ENV_setAttack(0, value); break;
			case 0x101: // Envelope 1: Decay
				ENV_setDecay(0, value); break;
			case 0x102: // Envelope 1: Sustain
				ENV_setSustain(0, value); break;
			case 0x103: // Envelope 1: Release
				ENV_setRelease(0, value); break;

			case 0x200: // Envelope 2: Attack
				ENV_setAttack(1, value); break;
			case 0x201: // Envelope 2: Decay
				ENV_setDecay(1, value); break;
			case 0x202: // Envelope 2: Sustain
				ENV_setSustain(1, value); break;
			case 0x203: // Envelope 2: Release
				ENV_setRelease(1, value); break;

			case 0x300: // Oscillator 1: Waveform flags
				ENGINE_setOscWaveform(0, value); break;
			case 0x301: // Oscillator 1: Pulsewidth
				ENGINE_setOscPW(0, value); break;
			case 0x302: // Oscillator 1: Tranpose
				ENGINE_setOscTranspose(0, (s8) (value - 64)); break;
			case 0x303: // Oscillator 1: Volume  
				ENGINE_setOscVolume(0, value); break;
			case 0x304: // Oscillator 1: Finetune
				ENGINE_setOscFinetune(0, (s8) (value - 64)); break;
			case 0x305: // Oscillator 1: pitchbend up range
				ENGINE_setPitchbendUpRange(0, value); break;
			case 0x306: // Oscillator 1: pitchbend down range
				ENGINE_setPitchbendDownRange(0, value); break;
			case 0x307: // Oscillator 1: portamento enable
				ENGINE_setPortamentoMode(0, value); break;
			case 0x308: // Oscillator 1: portamento rate
				ENGINE_setPortamentoRate(0, value); break;
			case 0x309: // Oscillator 1: sub oscialltor volume
				ENGINE_setSubOscVolume(0, value); break;

			case 0x400: // Oscillator 2: Waveform flags
				ENGINE_setOscWaveform(1, value); break;
			case 0x401: // Oscillator 2: Pulsewidth
				ENGINE_setOscPW(1, value); break;
			case 0x402: // Oscillator 2: Tranpose
				ENGINE_setOscTranspose(1, (s8) (value - 64)); break;
			case 0x403: // Oscillator 2: Volume  
				ENGINE_setOscVolume(1, value); break;
			case 0x404: // Oscillator 2: Finetune
				ENGINE_setOscFinetune(1, (s8) (value - 64)); break;
			case 0x405: // Oscillator 2: pitchbend up range
				ENGINE_setPitchbendUpRange(1, value); break;
			case 0x406: // Oscillator 2: pitchbend down range
				ENGINE_setPitchbendDownRange(1, value); break;
			case 0x407: // Oscillator 2: portamento enable
				ENGINE_setPortamentoMode(1, value); break;
			case 0x408: // Oscillator 2: portamento rate
				ENGINE_setPortamentoRate(1, value); break;
			case 0x409: // Oscillator 2: sub oscialltor volume
				ENGINE_setSubOscVolume(1, value); break;

			case 0x500: // Filter: Cutoff
				FILTER_setCutoff(value); break;
			case 0x501: // Filter: Resonance
				FILTER_setResonance(value); break;
			case 0x502: // Filter: Type
				FILTER_setFilter(value); break;

			case 0x600: // LFO 1: Waveform flags
				LFO_setWaveform(0, value); break;
			case 0x601: // LFO 1: Frequency
				LFO_setFreq(0, value); break;
			case 0x602: // LFO 1: Pulsewidth
				LFO_setPW(0, value); break;
			
			case 0x700: // LFO 2: Waveform flags
				LFO_setWaveform(1, value); break;
			case 0x701: // LFO 2: Frequency
				LFO_setFreq(1, value); break;
			case 0x702: // LFO 2: Pulsewidth
				LFO_setPW(1, value); break;

			// old ROUTING /////////////////////////////////////////////////////
			/*
			case 0x1000: // Routing Target 0: Cutoff
				ENGINE_setRoute(T_CUTOFF, value); break;
			case 0x1001: // Routing Target 0: Cutoff depth
				ENGINE_setRouteDepth(T_CUTOFF, value); break;
			
			case 0x1002: // Routing Target 1: Volume
				ENGINE_setRoute(T_MASTER_VOLUME, value); break;
			case 0x1003: // Routing Target 1: Volume depth
				ENGINE_setRouteDepth(T_MASTER_VOLUME, value); break;
			
			case 0x1004: // Routing Target 2: Osc 1 Pitch
				ENGINE_setRoute(T_OSC1_PITCH, value); break;
			case 0x1005: // Routing Target 2: Osc 1 Pitch depth
				ENGINE_setRouteDepth(T_OSC1_PITCH, value); break;
			
			case 0x1006: // Routing Target 3: Osc 2 Pitch
				ENGINE_setRoute(T_OSC2_PITCH, value); break;
			case 0x1007: // Routing Target 3: Osc 2 Pitch depth
				ENGINE_setRouteDepth(T_OSC2_PITCH, value); break;
			
			case 0x1008: // Routing Target 4: Overdrive
				ENGINE_setRoute(T_OVERDRIVE, value); break;
			case 0x1009: // Routing Target 4: Overdrive depth
				ENGINE_setRouteDepth(T_OVERDRIVE, value); break;

			case 0x100A: // Routing Target 4: Samplerate
				ENGINE_setRoute(T_SAMPLERATE, value); break;
			case 0x100B: // Routing Target 5: Samplerate depth
				ENGINE_setRouteDepth(T_SAMPLERATE, value); break;
			*/
			
			case 0x2000: // Trigger matrix: Note On
				ENGINE_setTriggerColumn(TRIGGER_NOTEON, value); break;
			case 0x2001: // Trigger matrix: Note Off
				ENGINE_setTriggerColumn(TRIGGER_NOTEOFF, value); break;
			case 0x2002: // Trigger matrix: LFO1 Period
				ENGINE_setTriggerColumn(TRIGGER_LFO1_PERIOD, value); break;
			case 0x2003: // Trigger matrix: LFO2 Period
				ENGINE_setTriggerColumn(TRIGGER_LFO2_PERIOD, value); break;
			case 0x2004: // Trigger matrix: Env1 Sustain
				ENGINE_setTriggerColumn(TRIGGER_ENV1_SUSTAIN, value); break;
			case 0x2005: // Trigger matrix: Env2 Sustain
				ENGINE_setTriggerColumn(TRIGGER_ENV2_SUSTAIN, value); break;

			case 0x3000: // Temp 1 Value
				ENGINE_setTempValue(0, value); break;
			case 0x3001: // Temp 2 Value
				ENGINE_setTempValue(1, value); break;

			// DRUM ENGINE	
			case 0x8000: // sine drum: sine initial freq
				DRUM_setSineDrum_SineFreqInitial(index, value); break;
			case 0x8001: // sine drum: sine end freq 
				DRUM_setSineDrum_SineFreqEnd(index, value); break;
			case 0x8002: // sine drum: sine freq decay 
				DRUM_setSineDrum_SineFreqDecay(index, value); break;
			case 0x8003: // sine drum: sine attack
				DRUM_setSineDrum_SineAttack(index, value); break;
			case 0x8004: // sine drum: sine decay
				DRUM_setSineDrum_SineDecay(index, value); break;

			case 0x8005: // sine drum: noise attack
				DRUM_setSineDrum_NoiseAttack(index, value); break;
			case 0x8006: // sine drum: noise decay
				DRUM_setSineDrum_NoiseDecay(index, value); break;
			case 0x8007: // sine drum: noise bandpass cutoff
				DRUM_setSineDrum_NoiseCutoff(index, value); break;
			case 0x8008: // sine drum: noise bandpass cutoff decay
				DRUM_setSineDrum_NoiseCutoffDecay(index, value); break;

			case 0x800A: // sine drum: sine/noise mix
				DRUM_setSineDrum_Mix(index, value); break;
			case 0x800B: // sine drum: volume
				DRUM_setSineDrum_Volume(index, value); break;
			case 0x800C: // sine drum: overdrive
				DRUM_setSineDrum_Overdrive(index, value); break;
			case 0x800D: // sine drum: wav select
				DRUM_setSineDrum_Waveform(index, value); break;
			case 0x800E: // sine drum: filter type
				DRUM_setSineDrum_FilterType(index, value); break;
			case 0x800F: // sine drum: trigger note
				DRUM_setSineDrum_TriggerNote(index, value); break;
			
			#ifdef SYSEX_VERBOSE
			default:
				MIOS32_MIDI_SendDebugMessage("Unknown address: %d", address);
			#endif
		}	
	}
	#ifdef SYSEX_VERBOSE
	else {
		MIOS32_MIDI_SendDebugMessage("Unknown buffer length (%d)", bufLen);
	}
	#endif
}
