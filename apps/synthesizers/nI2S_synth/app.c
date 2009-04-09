/****************************************************************************
 *                                                                          *
 * nI2S Digital Toy Synth - APPLICATION                                     *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *                    OOOOOOOOOO  OOOOOOOOOOOOOOO        OOOOOOOOOOOOOOO    *
 *                    O........O O...............OO    OO...............O   *
 *                    O........O O......OOOOO......O  O......OOOOO......O   *
 *                    OOO....OOO OOOOOOO     O.....O  O.....O     OOOOOOO   *
 *                      O....O               O.....O  O.....O               *
 *   OOOOO OOOOOO       O....O               O.....O  O.....O               *
 *   O....O......OO     O....O            OOOO....O    O.....OOO            *
 *   O.............O    O....O       OOOOO......OO      OO......OOOOO       *
 *   O.....OOOO.....O   O....O     OO........OOO          OOO........OO     *
 *   O....O    O....O   O....O    O.....OOOOO                OOOOO.....O    *
 *   O....O    O....O   O....O   O.....O                          O.....O   *
 *   O....O    O....O   O....O   O....O                           O.....O   *
 *   O....O    O....O OOO....OOO O....O        OOOOOO OOOOOOO     O.....O   *
 *   O....O    O....O O........O O....OOOOOOOOO.....O O......OOOOO......O   *
 *   O....O    O....O O........O O..................O O...............OO    *
 *   OOOOOO    OOOOOO OOOOOOOOOO OOOOOOOOOOOOOOOOOOOO  OOOOOOOOOOOOOOO      *
 *                                                                          *
 *   n I L S '    D I G I T A L    I 2 S    T O Y    S Y N T H E S I Z E R  * 
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
#include <FreeRTOS.h>
#include <portmacro.h>

#include "engine.h"
#include "app.h"
#include "sysex.h"
#include "envelope.h"


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
	// init sound engine
	ENGINE_init();

   // init sysex handler
	SYSEX_Init();
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
	// do nothing
	// ...
	// and more nothing
	// ...
	// and even more!
	// ..
	// done!
}

/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
	// if note event over MIDI channel #1 controls note of both oscillators
	if (engine == ENGINE_SYNTH) {
		if ((midi_package.event == NoteOn) && (midi_package.velocity > 0)) {
			// note on
			ENGINE_noteOn(midi_package.note, midi_package.velocity, 0);
			MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
			return;
		}
		
		if ((midi_package.event == NoteOn) || (midi_package.event == NoteOff)) {
			// note off
			ENGINE_noteOff(midi_package.note);
			return;
		}
	} else
	if (engine == ENGINE_DRUM) {
		if ((midi_package.event == NoteOn) && (midi_package.velocity > 0)) {
			// note on
			DRUM_noteOn(midi_package.note, midi_package.velocity, 0);
			return;
		}
		
		if ((midi_package.event == NoteOn) || (midi_package.event == NoteOff)) {
			// note off
			DRUM_noteOff(midi_package.note);
			return;
		}
	}
  
	// CC handler (incomplete)
	if (midi_package.evnt0 == 0xB0) {
		switch (midi_package.evnt1) {
			// engine flags
			case 0x06: 
				ENGINE_setEngineFlags(midi_package.evnt2);
				break;
			// volume
			case 0x07: 
				ENGINE_setMasterVolume((u16) midi_package.evnt2 << 9);
				break;
			// filter cutoff
			case 0x08: 
				FILTER_setCutoff((u16) midi_package.evnt2 << 9);
				break;
			// filter resonance
			case 0x09: 
				FILTER_setResonance((u16) midi_package.evnt2 << 9);
				break;

			// envelope 1 attack
			case 0x10: 
				ENV_setAttack(0, midi_package.evnt2 << 9);
				break;

			// envelope 1 decay
			case 0x11: 
				ENV_setDecay(0, midi_package.evnt2 << 9);
				break;

			// envelope 1 sustain
			case 0x12: 
				ENV_setSustain(0, midi_package.evnt2 << 9);
				break;

			// envelope 1 release
			case 0x13: 
				ENV_setRelease(0, midi_package.evnt2 << 9);
				break;

			// envelope 2 attack
			case 0x20: 
				ENV_setAttack(1, midi_package.evnt2 << 9);
				break;

			// envelope 2 decay
			case 0x21: 
				ENV_setDecay(1, midi_package.evnt2 << 9);
				break;

			// envelope 2 sustain
			case 0x22: 
				ENV_setSustain(1, midi_package.evnt2 << 9);
				break;

			// envelope 2 release
			case 0x23: 
				ENV_setRelease(1, midi_package.evnt2 << 9);
				break;

			// osc1 waveform	
			case 0x40: 
				ENGINE_setOscWaveform(0, midi_package.evnt2);
				break;
			// osc1 volume 
			case 0x41: 
				ENGINE_setOscVolume(0, midi_package.evnt2 << 1);
				break;
			// osc1 transpose
			case 0x42: 
				ENGINE_setOscTranspose(0, (s8) midi_package.evnt2 - 63);
				break;

			// osc2 waveform	
			case 0x50: 
				ENGINE_setOscWaveform(1, midi_package.evnt2);
				break;
			// osc2 volume 
			case 0x51: 
				ENGINE_setOscVolume(1, midi_package.evnt2 << 1);
				break;
			// osc2 transpose
			case 0x52: 
				ENGINE_setOscTranspose(1, (s8) midi_package.evnt2 - 63);
				break;
		}

		return;
	}
	
	// pitchbend 
	if (midi_package.evnt0 == 0xE0) {
		s16 pb;

		pb = midi_package.evnt2;
		pb <<= 7;
		pb += midi_package.evnt1;
		pb -= 8191;

		ENGINE_setPitchbend(2, pb);

		return;
	}
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
	// forward byte to SysEx Parser
	SYSEX_Parser(port, sysex_byte);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
