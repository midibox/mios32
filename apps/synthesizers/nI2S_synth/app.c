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
 ****************************************************************************
 *                                                                          *
 * CHANGELOG                                                                *
 *                                                                          *
 * v0.0.0 - 2009/04/09 - initial release                                    *
 *                                                                          *
 * v0.0.1 - 2009/04/13 - "Bashful"                                          *
 *   > general:
 * 		 - various bugfixes (thanks TK)                                         *
 *   > lead engine:                                                         *
 *       - transpose works instantly now                                    *
 *   > drum engine:                                                         *
 *       - notes that trigger the drums are now selectable                  *
 *   > fx chain:                                                            *
 *       - added a state variable filter                                    *
 *       - moved master volume all the way to the end of the chain          *
 *   > editor:                                                              *
 *       - mixup with volume/overdrive/mix knobs in the drum editor fixed   *
 *                                                                          *
 * v0.0.2 - 2009/09/17 - "Cheerful"                                         *
 *   > general:                                                             *
 *       - some performance fixes                                           *
 *       - added rudimetary patch saving/restoring (bankstick only atm)     *
 *         CC 0x40 saves to bs 0 patch 0, CC 0x45 restores the patch from   *
 *         the same location                                                *
 *       - terminal output can be turned off by #defines in "defs.h"        *
 *         (verbose mode is heavy on the performance, esp. over real midi)  *        *
 *   > lead engine:                                                         *
 *       - setting transpose while notes are held doesn't kill the          *
 *         notestack anymore                                                *
 *       - envelopes correctly jump back to DECAY now                       *
 *   > fx chain:                                                            *
 * 		 - fixed the signed/unsigned problems with most of the filters      *
 *       - changed overdrive to pseudo-log response                         *
 * 		 - filter_svf now responds to cutoff changes from all sources       *
 *       - added an interesting ring mod implementation                     *
 *       - added switch to dis-/enable delay                                *
 *       - added chorus                                                     *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 * TODO (not in any particular order)                                       *
 *   - lots                                                                 *
 *   - come up with a cool name. it needs to be food.                       *
 *   - add a CS/menu structure                                              *
 *   - patch storage (damn it's annoying to always start from scratch) =)   *
 *   - no note down -> transpose hangup                                     *
 *   - fix filters (cutoff on SVF, output levels, constants)                *
 *   - add new filter type "external" with aout support                     *
 *   - fix "fixme"s                                                         *
 *   - optimize "optimize-here"s                                            *
 *   - copy waveblender from temp files back in                             *
 *   - new mod targets/sources / reversed mod route                         *
 *   - change routing to matrix                                             *
 *   - lfo depth offset before multiply                                     *
 *   - fix mod behaviour (offset vs. amount)                                *
 *   - fix pitch bend (where has it gone, oh noes)                          *
 *   - drum engine oddness (if I get really bored)                          *
 *   - drum engine: velocity ( --"-- )                                      *
 *   - changeable fx chain order                                            *
 *   - move calculations out of the main loop                               *
 *   - aftertouch/modulation as mod source                                  *
 *   - receive sysex for patch name                                         *
 *   - program change                                                       *
 *                                                                          *
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <eeprom.h>

#include "engine.h"
#include "app.h"
#include "sysex.h"
#include "envelope.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
u32 bankstick;

/////////////////////////////////////////////////////////////////////////////
// This function stores the patch in use to the bankstick
/////////////////////////////////////////////////////////////////////////////
void APP_storePatch(u16 patchnumber) {
	u8 block;
	s32 res = 0;
	u16 address = patchnumber * sizeof(patch_t);
	
	MIOS32_IIC_BS_ScanBankSticks();
	bankstick = MIOS32_IIC_BS_CheckAvailable(0);

	if (!bankstick) {
		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): No bankstick found to store to.");
		#endif
		return;
	}
	
	res += MIOS32_IIC_BS_Write(0, address, &p.all[0], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 64, &p.all[64], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 128, &p.all[128], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 192, &p.all[192], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 256, &p.all[256], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 320, &p.all[320], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 384, &p.all[384], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);
	res += MIOS32_IIC_BS_Write(0, address + 448, &p.all[448], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(0) != 0);

	#ifdef APP_VERBOSE
	if (res == 0)
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): Patch successfully stored.");
	else
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): Cannot store patch '%s' at address %d [%d]", p.name, patchnumber, res);
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// This function loads a patch from the bankstick
/////////////////////////////////////////////////////////////////////////////
void APP_loadPatch(u16 patchnumber) {
	s32 res = 0;
	u16 block;
	u16 address = sizeof(patch_t) * patchnumber;
	
	MIOS32_IIC_BS_ScanBankSticks();
	bankstick = MIOS32_IIC_BS_CheckAvailable(0);

	if (!bankstick) {
		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): No bankstick found to store to.");
		#endif
		return;
	}

	res += MIOS32_IIC_BS_Read(0, address, &p.all[0], 64);
	res += MIOS32_IIC_BS_Read(0, address + 64, &p.all[64], 64);
	res += MIOS32_IIC_BS_Read(0, address + 128, &p.all[128], 64);
	res += MIOS32_IIC_BS_Read(0, address + 192, &p.all[192], 64);
	res += MIOS32_IIC_BS_Read(0, address + 256, &p.all[256], 64);
	res += MIOS32_IIC_BS_Read(0, address + 320, &p.all[320], 64);
	res += MIOS32_IIC_BS_Read(0, address + 384, &p.all[384], 64);
	res += MIOS32_IIC_BS_Read(0, address + 448, &p.all[448], 64);

	#ifdef APP_VERBOSE
	if (res == 0)
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Patch '%s' successfully loaded.", p.name);
	else
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Cannot load patch %d [%d]", patchnumber, res);
	#endif

	// fixme: engine might need to be switched
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
	MIOS32_IIC_BS_Init(0);
	
	s32 bs, bs2;
	// check for bankstick 0
	MIOS32_IIC_BS_ScanBankSticks();
	bankstick = MIOS32_IIC_BS_CheckAvailable(0);
	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("Bankstick scan result = %d", bankstick);
	MIOS32_MIDI_SendDebugMessage("sizeof(patch_t) = %d", sizeof(patch_t));
	#endif

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
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
	// if note event over MIDI channel #1 controls note of both oscillators
	if (engine == ENGINE_SYNTH) {
		if ((midi_package.event == NoteOn) && (midi_package.velocity)) {
			// note on
			ENGINE_noteOn(midi_package.note, midi_package.velocity, NO_STEAL);
			return;
		} // else (not needed due to return; )
		
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
  
	// fixme: CC handler (incomplete, mainly a dummy to be fixed later)
	if (midi_package.evnt0 == 0xB0) {
		switch (midi_package.evnt1) {
			// temp LOAD patch, fixme: depend on engine
			case 0x40: 
				APP_loadPatch(0);
				break;

			// temp STORE patch
			case 0x45: 
				APP_storePatch(0);
				break;

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
