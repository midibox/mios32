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
 * v0.0.1 - 2009/04/13 - "Agent Cheesecake"                                 *
 *   > general:                                                             *
 * 		 - various bugfixes (thanks TK)                                     *
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
 * v0.0.2 - 2009/09/17 - "Beta Ray Bill"                                    *
 *   > general:                                                             *
 *       - some performance fixes                                           *
 *       - patch names can be set via sysex now (address 0x03, 32 chars)    *
 *       - added rudimetary patch saving/restoring (bankstick only atm)     *
 *         CC 0x40 saves to bs 0 patch 0, CC 0x45 restores the patch from   *
 *         the same location                                                *
 *       - terminal output can be turned off by #defines in "defs.h"        *
 *         (verbose mode is heavy on the performance, esp. over real midi)  *        
 *       - program change support added                                     *
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
 * v0.0.3 - 2009/09/20 - "Cloud 9"                                          *
 *   > general:                                                             *
 *       - enhanced patch management (multi bankstick, default patch, ...)  *
 *       - sysex dumps of the current patch can be requested via CC #70     *
 *   > lead engine:                                                         *
 *       - pitch bend is back (still some issues with the linearity)        *
 *                                                                          *
 * v0.0.4 - 2009/10/01 - "Demogoblin"                                       *
 *   > general:                                                             *
 *       - cleaned up the VERBOSE switches a bit more and added new ones    *
 *         for more detailed and specific feedback                          *
 *   > lead engine:                                                         *
 *       - fixed bitcrush. it now crushes bits =)                           *
 *       - added setters on patch load (bitcrush so far, others may follow) *
 *       - started implementing the new routing matrix                      *
 *       - lfo depth offset before multiply (auto fixed via matrix)         *
 *       - fix mod behaviour (offset vs. amount) (auto fixed via matrix)    *
 *       - removed LFO depth (can be handled via the routing matrix)        *
 *       - added a selectable depth source for each routing target          *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 * TODO/KNOWN BUGS (not in any particular order)                            *
 *   - lots                                                                 *
 *   - fix "fixme"s                                                         *
 *   - optimize "optimize-here"s                                            *
 *   - kill invert flag for envelopes (comes free with the matrix)          *
 *   - come up with a name. it needs to be food. got an idea? lemme know!   *
 *   - add a CS/menu structure                                              *
 *   - no note down -> transpose hangup (noticeable when no source is set   *
 *     for volume)                                                          *
 *   - fix filters (moog constants and levels)                              *
 *   - add new filter type "external" with aout support                     *
 *   - copy waveblender from temp files back in (?)                         *
 *   - change the rest of the routing to matrix as well                     *
 *   - changeable fx chain order                                            *
 *   - move calculations out of the main loop                               *
 *   - aftertouch/modulation/velocity and user-definable CCs as mod source  *
 *   - ignore non working trigger combinations (lfo cycle->lfo reset, ...)  *
 *   - pitch bend: blending shouldn't be linear...                          *
 *   - options for SD card/bankstick (do I really want to keep bankstick    *
 *     support? yes, i do. one internal bankstick should do for settings    *
 *     and some (63) patches (it's actually going to be a lot less, since   *
 *     the routing matrix takes pu quite a bit of space :-))                *
 *   - find out why this list keeps getting longer instead of shorter :-/   *
 * 																		    *
 *   - drum engine oddness (if I get really bored)                          *
 *   - drum engine: velocity ( --"-- )                                      *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 * NOTES TO SELF                                                            *
 * - routing matrix                                                         *
 *   --------------                                                         *
 *   sources                                                                *
 *     lfo1-3*, env1-3*, osc1-3* pitch, constant, mod wheel, aftertouch,    *
 *     user-definable CCs                                                   *
 *   targets                                                                *
 *     cutoff, resonance, ext.cutoff, ext.resonance, osc1-3* pitch,         *
 *     osc1-3.volume*, master volume, osc1-3 suboct,                        *
 *     od, bitcrush, downsample                                             *
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
// Version/app info 
/////////////////////////////////////////////////////////////////////////////

static const char APP_NAME[] = "nI2S Toy Synth";
static const char VERSION_STRING[] = "v0.0.3";

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
	   u16 bank;			// is there even a need for banks with SD? (yes)
static u16 patch_number;	// patch number
static config_t config;		// the configuration

/////////////////////////////////////////////////////////////////////////////
// This function sets the patch name
/////////////////////////////////////////////////////////////////////////////
void APP_setPatchName(char* patchname) {
	u8 n;

	for (n=0; n<32; n++)
		p.d.name[n] = patchname[n];

	// redundant but sooo much easier
	p.d.name[32] = 0;

	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("APP_setPatchName(): %a", &p.d.name[0]);
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// This debug funtion sends a patch dump to the console   
/////////////////////////////////////////////////////////////////////////////
void APP_dumpPatch() {
	MIOS32_MIDI_SendDebugHexDump(&p.all[0], 512);
}

/////////////////////////////////////////////////////////////////////////////
// This function stores the patch in use to the bankstick
/////////////////////////////////////////////////////////////////////////////
void APP_storePatch(u8 bankstick, u16 patchnumber) {
	// fixme: is this patch number even valid?
	u8 block;
	u16 bs;
	s32 res = 0;
	u16 address = patchnumber * sizeof(patch_t);
	
	MIOS32_IIC_BS_ScanBankSticks();
	bs = MIOS32_IIC_BS_CheckAvailable(bankstick);

	if (!bs) {
		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): No bankstick in slot %d.", bankstick);
		#endif
		return;
	}

	// potential fixme: evil evil waiting... seems to work fine though
	res += MIOS32_IIC_BS_Write(bankstick, address, &p.all[0], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 64, &p.all[64], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 128, &p.all[128], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 192, &p.all[192], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 256, &p.all[256], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 320, &p.all[320], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 384, &p.all[384], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, address + 448, &p.all[448], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);

	if (res == 0)
		patch_number = patchnumber;

	#ifdef APP_VERBOSE
	if (res == 0)
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): Patch successfully stored at location %d.", patchnumber);
	else
		MIOS32_MIDI_SendDebugMessage("APP_storePatch(): Cannot store patch '%s' at address %d [%d]", p.d.name, patchnumber, res);
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// This function stores the config to the specified bankstick
/////////////////////////////////////////////////////////////////////////////
void APP_storeConfig(u8 bankstick) {
	s32 res = 0;
	u16 bs;

	MIOS32_IIC_BS_ScanBankSticks();
	bs = MIOS32_IIC_BS_CheckAvailable(bankstick);

	if (!bs) {
		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_storeConfig(): No bankstick in slot %d.", bankstick);
		#endif
		return;
	}

	config.header = 0x6E493253;

	// potential fixme: evil evil waiting... seems to work fine though
	res += MIOS32_IIC_BS_Write(bankstick, 0, &config.all[0], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 64, &config.all[64], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 128, &config.all[128], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 192, &config.all[192], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 256, &config.all[256], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 320, &config.all[320], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 384, &config.all[384], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);
	res += MIOS32_IIC_BS_Write(bankstick, 448, &config.all[448], 64);
	do {} while (MIOS32_IIC_BS_CheckWriteFinished(bankstick) != 0);

	#ifdef APP_VERBOSE
	if (res == 0)
		MIOS32_MIDI_SendDebugMessage("APP_storeConfig(): Config successfully stored at bankstick %d", bankstick);
	else
		MIOS32_MIDI_SendDebugMessage("APP_storeConfig(): Cannot store config on bankstick %d [%d]", bankstick, res);
	#endif
}
/////////////////////////////////////////////////////////////////////////////
// This prepares the bankstick for use with the synth    
/////////////////////////////////////////////////////////////////////////////
void APP_formatBankstick(u8 bankstick, u16 size) {
	u8 n;
	
	APP_storeConfig(bankstick);

	for (n=1; n<64; n++) {
		APP_storePatch(bankstick, n);
	}

	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("  [OK] Bankstick %d successfully formatted.", bankstick);
	#endif		
}

/////////////////////////////////////////////////////////////////////////////
// This function loads a patch from the bankstick
/////////////////////////////////////////////////////////////////////////////
void APP_loadPatch(u8 bankstick, u16 patchnumber) {
	// fixme: is that patch number even valid?
	s32 res = 0;
	u16 block;
	u16 address = sizeof(patch_t) * patchnumber;

	if (patchnumber == 0) {
		// load default patch
		for (block=0; block<512; block++) {
			p.all[block] = default_patch.all[block];
		}

		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Loaded default patch.");
		#endif

		return;
	}
	
	MIOS32_IIC_BS_ScanBankSticks();
	block = MIOS32_IIC_BS_CheckAvailable(bankstick);

	if (!block) {
		#ifdef APP_VERBOSE
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): No bankstick in slot %d found to load from.", block);
		#endif
		return;
	}

	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Loading patch %d from address %d on bankstick %d", patchnumber, address, bankstick);
	#endif

	res += MIOS32_IIC_BS_Read(bankstick, address, &p.all[0], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 64, &p.all[64], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 128, &p.all[128], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 192, &p.all[192], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 256, &p.all[256], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 320, &p.all[320], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 384, &p.all[384], 64);
	res += MIOS32_IIC_BS_Read(bankstick, address + 448, &p.all[448], 64);

	// store active patch number
	patch_number = patchnumber;

	// setup patch specific values
	ENGINE_setBitcrush(p.d.voice.bitcrush);
	
	#ifdef APP_VERBOSE
	if (res == 0)
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Patch '%s' successfully loaded from bankstick %d.", p.d.name, bankstick);
	else
		MIOS32_MIDI_SendDebugMessage("APP_loadPatch(): Cannot load patch %d [%d] from bankstick %d", patchnumber, res, bankstick);
	#endif

	// fixme: engine might need to be switched
}

/////////////////////////////////////////////////////////////////////////////
// This function checks if a bankstick has been formatted yet
/////////////////////////////////////////////////////////////////////////////
u8 APP_checkBankstickIntegrity(u8 bankstick, u16 size) {
	// fixme: is that patch number even valid?
	s32 res = 0;
	u16 block;

	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("  [OK] checking...");
	#endif

	res += MIOS32_IIC_BS_Read(bankstick, 0, &config.all[0], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 64, &config.all[64], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 128, &config.all[128], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 192, &config.all[192], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 256, &config.all[256], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 320, &config.all[320], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 384, &config.all[384], 64);
	res += MIOS32_IIC_BS_Read(bankstick, 448, &config.all[448], 64);

	#ifdef APP_VERBOSE
	if (res)
		MIOS32_MIDI_SendDebugMessage("  [ER] bankstick %d is not available", p.d.name);
	#endif

	if (config.header == 0x6E493253 /* == 'nI2S' */) 
		return 0;
	else
		return 1;
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
	s32 bs, bs2;
	u16 n;

	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("------------------------------------------------------------");
	MIOS32_MIDI_SendDebugMessage("Booting %s %s", APP_NAME, VERSION_STRING);
	MIOS32_MIDI_SendDebugMessage("------------------------------------------------------------");
	#endif

	// temp
	config.header = 0x6E493253; // nI2S
	for (n=0; n<512; n++) {
		p.all[n] = default_patch.all[n];
	}

	// initialize bankstick module
	MIOS32_IIC_BS_Init(0);

	bank = 0;
	patch_number = 1;
	
	// temp: check for banksticks (fixme: and SD card)
	MIOS32_IIC_BS_ScanBankSticks();

	// check all banksticks
	#ifdef APP_VERBOSE
	MIOS32_MIDI_SendDebugMessage("Checking banksticks");
	#endif
	for (n=0; n<8; n++) {
		bs = MIOS32_IIC_BS_CheckAvailable(n);

		#ifdef APP_VERBOSE
		if (bs)
			MIOS32_MIDI_SendDebugMessage("  Bankstick #%d size is %d", n, bs);
		else
			MIOS32_MIDI_SendDebugMessage("  Bankstick #%d not available", n, bs);
		#endif

		if (bs) {
			if (APP_checkBankstickIntegrity(n, bs)) {
				#ifdef APP_VERBOSE
				MIOS32_MIDI_SendDebugMessage("  [CL] Formatting bankstick %d", n);
				#endif
				APP_formatBankstick(n, bs);

			} else {
				#ifdef APP_VERBOSE
				MIOS32_MIDI_SendDebugMessage("  [OK] Bankstick %d", n);
				#endif
			}
		}
	}

	// init sound engine
	ENGINE_init();

   // init sysex handler
	SYSEX_Init();

    // load the default patch
	APP_loadPatch(0, 0);

	// temp, fixme, deleteme:
	ENGINE_setBitcrush(0);
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

	// program change
	if (midi_package.evnt0 == 0xC0)
		APP_loadPatch(bank, midi_package.evnt1);
	else
  
	// fixme: CC handler (incomplete, mainly a dummy to be fixed later)
	if (midi_package.evnt0 == 0xB0) {
		switch (midi_package.evnt1) {
			case 0x01: // mod wheel
				ENGINE_setModWheel(midi_package.evnt2 * 0x1FF); 
				break;
			/******************************************************************/

			// temp LOAD patch, fixme: depend on engine
			case 0x40: 
				APP_loadPatch(bank, patch_number);
				break;

			// temp STORE patch
			case 0x45: 
				APP_storePatch(bank, patch_number);
				break;

			// temp send patch dump
			case 0x7C: 
				APP_dumpPatch();
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
