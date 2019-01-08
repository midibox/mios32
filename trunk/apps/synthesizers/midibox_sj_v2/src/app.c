// $Id$
/*
 * MIDIbox SJ
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Michael Markert (http://www.audiocommander.de) & Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "terminal.h"


// custom includes
#include "ACMidiDefines.h"
#include "ACMidi.h"
#include "ACHarmonizer.h"

#include "IIC_SpeakJetDefines.h"
#include "IIC_SpeakJet.h"

#if KII_AIN_ENABLED
	#include "ACHandTracker.h"
#endif

#if KII_SYNC_ENABLED
	#include "ACSyncronizer.h"
#endif

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for AC_Tick task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_AC_TICK	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_AC_Tick(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

acapp_t acapp;
volatile unsigned char	channelAssignment;			// incoming channel mapping


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

volatile unsigned char lastNoteDown[17];			// ch 1..16 for noteOff tracking


#if KII_LCD_ENABLED
static const unsigned char specialChars[8*8] = {
	0x00, 0x02, 0x13, 0x0A, 0x06, 0x02, 0x00, 0x00,	// SAW
	0x00, 0x0C, 0x12, 0x12, 0x01, 0x00, 0x00, 0x00,	// SINE
	0x00, 0x08, 0x14, 0x02, 0x01, 0x00, 0x00, 0x00,	// TRIANGLE
	0x00, 0x1E, 0x12, 0x12, 0x02, 0x03, 0x00, 0x00,	// SQUARE
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F,	// lvl 1
	0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F,	// lvl 2
	0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,	// lvl 3
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F	// lvl 4
};
#endif



/////////////////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////////////////

#define LEVELCHAR(c) ( ((c >> 5) + 4) & 0x7F )


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  int i;

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // init terminal
  TERMINAL_Init(0);

  // initialize status LED
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_BOARD_LED_Set(1, 0);

#if KII_LCD_ENABLED
	// LCD config: install special chars
  MIOS32_LCD_SpecialCharsInit((u8 *)specialChars);
	// Init vars
	acapp.displayNeedsInit = TRUE;
	acapp.displayNeedsUpdate = TRUE;
#endif

	// main mode
	channelAssignment = 0;
		
	// init interface to MBHP_IIC_SPEAKJET
	IIC_SPEAKJET_Init();
	
#if HARMONIZER_ENABLED
	// init harmonizer
	ACHarmonizer_Init();
#endif
	
#if KII_AIN_ENABLED
	// Init HandTracker
	ACHandTracker_Init();
#endif
	
#if KII_SYNC_ENABLED
	// initialize the MIDI clock module (-> ACSyncronizer.c)
	ACSYNC_Init();
	ACSYNC_BPMSet(120);
	ACSYNC_DoRun();
#endif /* KII_SYNC_ENABLED */

  // start task
  xTaskCreate(TASK_AC_Tick, (signed portCHAR *)"AC_Tick", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_AC_TICK, NULL);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("=====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("=====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  if( acapp.displayNeedsInit ) {
#if KII_LCD_ENABLED
    MIOS32_LCD_Clear();

#if KII_LCD_TYPE == 28
    // Initial screen	  "01234567" (2x)
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("  -     ");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString(" 1 O-E- ");
    // Initial screen	  "01234567" (2x)
#endif /* KII_LCD_TYPE == 28 */
	
#if KII_LCD_TYPE == 216
    // Initial screen     "0123456789abcdef"
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("  -     H G J O ");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("        F R T E ");
#endif /* KII_LCD_TYPE == 216 */
	
#endif /* KII_LCD_ENABLED */
    // update done
    acapp.displayNeedsInit = FALSE;
  }


#if KII_LCD_ENABLED
  if(acapp.displayNeedsUpdate) {
		
    // ++ 1st row ++
    MIOS32_LCD_CursorSet(0, 0);
		
#if HARMONIZER_ENABLED
    // harmony related
    MIOS32_LCD_PrintChar( (noteNames[0][harmonizer_base]) );
    MIOS32_LCD_PrintChar( (noteNames[1][harmonizer_base]) );
    MIOS32_LCD_PrintChar( (harmonizer_listen ? '*' : '-') );
    MIOS32_LCD_PrintString(scaleNames[harmonizer_scale]);
#endif
		
#if KII_LCD_TYPE == 28
    // channel assingment (MIDI-CH-function)
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("%2d", (channelAssignment + 1));
    // ++ 2nd row ++
    // OSC Waveform
    MIOS32_LCD_CursorSet(3, 1);
    MIOS32_LCD_PrintChar( (sj_env_control.enveloped_osc_a ? 'O' : 'o') );
    MIOS32_LCD_PrintChar( (oscsynth_waveshape & 0x7f) );
    // Envelope Waveform
    MIOS32_LCD_PrintChar( (sj_env_control.enveloped_osc_b ? 'E' : 'e') );
    MIOS32_LCD_PrintChar( (sj_env_control.waveshape & 0x7f) );
    // quantize value
    MIOS32_LCD_CursorSet(7, 1);
    MIOS32_LCD_PrintChar( (quantize + 4) & 0x7f );
#endif /* KII_LCD_TYPE == 28 */
		
#if KII_LCD_TYPE == 216
    // height
    MIOS32_LCD_CursorSet(9, 0);
    MIOS32_LCD_PrintFormattedString("%x", (ht.HEIGHT >> 3) & 0xf);
    // jaw
    MIOS32_LCD_CursorSet(13, 0);
    MIOS32_LCD_PrintFormattedString("%x", jaw & 0xf);
    // OSC Waveform
    MIOS32_LCD_CursorSet(15, 0);
    MIOS32_LCD_PrintChar( (oscsynth_waveshape & 0x7f) );
    // ++ 2nd row ++
    // channel assignment
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("%2d", channelAssignment);
    // finger, roll, thumb (=tongue)
    MIOS32_LCD_CursorSet(9, 1);
    MIOS32_LCD_PrintFormattedString("%x", (ht.FINGER >> 2) & 0xf );
    MIOS_LCD_CursorSet(11, 1);
    MIOS32_LCD_PrintFormattedString("%x", ht.ROLL & 0xf );
    MIOS_LCD_CursorSet(13, 1);
    MIOS32_LCD_PrintFormattedString("%x", tongue & 0xf );
    // envelope waveform
    MIOS32_LCD_CursorSet(15, 1);
    MIOS32_LCD_PrintChar( (sj_env_control.waveshape & 0x7f) );
#endif /* KII_LCD_TYPE == 216 */
		
    // update done
    acapp.displayNeedsUpdate = FALSE;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This task is called each mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_AC_Tick(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

#if KII_SYNC_ENABLED
	// this routine sends the MIDI clock (and Start/Continue/Stop) if requested
	ACSYNC_Tick();
#endif /* KII_SYNC_ENABLED */    
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // TK: for easier conversion of MIOS8 based code
  unsigned char evnt0 = midi_package.evnt0;
  unsigned char evnt1 = midi_package.evnt1;
  unsigned char evnt2 = midi_package.evnt2;

  if( evnt0 >= 0xf8 ) {
    // forward to ACSyncronizer
    ACSYNC_ReceiveClock(evnt0);
    return;
  }
	// local vars
	unsigned int  value14bit;
	unsigned char targetOsc;
	unsigned char channel;
	unsigned char evntType;
	unsigned char evnt1_harm = evnt1;	// harmonized evnt1
	// initialize vars
	value14bit = 0;
	targetOsc = 4;
	// extract eventType (eg NOTE_ON, CONTROLLER...)
	if(evnt0 < MIDI_SYSEX) {		
		evntType = evnt0 & 0xf0;		// Channel Control Messages
	} else {
		evntType = evnt0;				// System Realtime Messages
	}
	// extract channel
	if(channelAssignment) {
		channel = channelAssignment;
	} else {
		channel = (evnt0 & 0x0f) + 1;	// channels from 1..16
	}
	// for easier parsing: convert Note Off to Note On with velocity 0
	if( evntType == MIDI_NOTE_OFF ) {
		evntType = MIDI_NOTE_ON;
		evnt0 |= 0x10;
		evnt2 = 0x00;
	}
	
	switch(evntType) {
		
		case MIDI_NOTE_ON:				// ***** NOTE_ON, 0x90 *****
			
#if HARMONIZER_ENABLED
			if(harmonizer_listen) {
				// set base
				if(evnt2) { ACHarmonizer_SetBase(evnt1); }
			} else {
				// harmonize
				evnt1_harm = ACHarmonize(evnt1);
			}
#endif /* HARMONIZER_ENABLED */
			// set Velocity
			if(channel != SJCH_PITCH) { 
				// set volume
				if( (evnt2 > 0) || ((evnt2 == 0) && (evnt1 == lastNoteDown[channel]))) {
					IIC_SPEAKJET_MSA_Volume(evnt2); 
				}
			}
			// parse channels
			switch(channel) {
				case SJCH_SOUNDCODES:
					if(evnt2) {
						IIC_SPEAKJET_MSA_Soundcode(evnt1);
					} else if( (evnt2 == 0) && (evnt1 == lastNoteDown[channel]) ) {
						IIC_SPEAKJET_MSA_Stop();
					}
					break;
				case SJCH_ALLOPHONES:
					if(evnt2) {
						IIC_SPEAKJET_MSA_Allophone(evnt1);
					} else if( (evnt2 == 0) && (evnt1 == lastNoteDown[channel]) ) {
						IIC_SPEAKJET_MSA_Stop();
					}
					break;
				case SJCH_FX:
					if(evnt2) {
						IIC_SPEAKJET_MSA_FX(evnt1);
					} else if( (evnt2 == 0) && (evnt1 == lastNoteDown[channel]) ) {
						IIC_SPEAKJET_MSA_Stop();
					}
					break;
				case SJCH_PERCUSSIVE:
					if(evnt2) {
						IIC_SPEAKJET_MSA_Percussive(evnt1);
					} else if( (evnt2 == 0) && (evnt1 == lastNoteDown[channel]) ) {
						IIC_SPEAKJET_MSA_Stop();
					}
					break;
				case SJCH_PITCH:
					// change the pitch of currently played allophones:
					// pitch is in Hz, ranging from 0..127 from notes 0..59
					IIC_SPEAKJET_MSA_Pitch(evnt1_harm, FALSE);	// false because value is not 14bit
					break;
				case SJCH_VOWELS:
				case SJCH_VOWELS_CONSONANTS:
				case SJCH_CONSONANTS:
				case SJCH_CONSONANTS_OPEN:
				case SJCH_CONSONANTS_CLOSE:
					if(evnt2) {
						// sets pitch and triggers current phoneme (based on jaw and tongue)
						IIC_SPEAKJET_Enunciate(channel, evnt1);	// channel contains the phonemetype!
					} else if( (evnt2 == 0) && (evnt1 == lastNoteDown[channel]) ) {
						IIC_SPEAKJET_MSA_Stop();
					}
					break;					
				case SJCH_OSC1:
				case SJCH_OSC2:
				case SJCH_OSC3:
				case SJCH_OSC4:
				case SJCH_OSC5:
					if( (evnt2 > 0) || ((evnt2 == 0) && (evnt1 == lastNoteDown[channel])) ) {
						// get target osc
						switch(channel) {
							case SJCH_OSC1: targetOsc = 1; break;
							case SJCH_OSC2: targetOsc = 2; break;
							case SJCH_OSC3: targetOsc = 3; break;
							case SJCH_OSC4: targetOsc = 4; break;
							case SJCH_OSC5: targetOsc = 5; break;
						}
						// set OSC Freq & Lvl based on incoming Note & Velocity
						IIC_SPEAKJET_ENVFreq(evnt1_harm);				// base freq for ENV
						IIC_SPEAKJET_OSCFreq(targetOsc, evnt1_harm);	// play note
						IIC_SPEAKJET_OSCLvl(targetOsc, evnt2);			// set velocity
					}
					break;
				case SJCH_OSC_SYN:
					// experimental mode: harmonic synthesis
					// restrict range
					if(evnt2) {
						IIC_SPEAKJET_OSCSynthesis(evnt1_harm);
					} else if ((evnt2 == 0) && (evnt1 == lastNoteDown[channel])) {
						for(targetOsc=0;targetOsc<6;targetOsc++) {
							IIC_SPEAKJET_OSCLvl(targetOsc, 0);
						}
					}
					break;						
			}
			// save last note_down event
			if(evnt2) { lastNoteDown[channel] = evnt1; }
			break;	/* MIDI_NOTE_ON */
			

		case MIDI_POLY_AFTER:				// ***** POLYPHONIC AFTERTOUCH, 0xA0 *****
			IIC_SPEAKJET_MSA_Bend(evnt2);
			break;
			
		case MIDI_CC:						// ***** CONTROL CHANGE, 0xB0 *****
			switch(evnt1) {
				
				case SJCC_CHANNEL_ASSIGNMENT:
					channelAssignment = (evnt2 >> 3);
					break;
				case SJCC_CHANNEL_ASSIGNMENT_TOGGLE:
					if(evnt2 > 63) {
						channelAssignment++;
						if(channelAssignment > 15) { channelAssignment = 0; }
					}
					break;
				
				case SJCC_PHRASE:		IIC_SPEAKJET_MSA_Phrase(evnt2);						break;
				case SJCC_PHRASE0: 		if(evnt2) {	IIC_SPEAKJET_MSA_Phrase(0);	}			break;
				case SJCC_PHRASE1:		if(evnt2) {	IIC_SPEAKJET_MSA_Phrase(1);	}			break;
				case SJCC_PHRASE2:		if(evnt2) {	IIC_SPEAKJET_MSA_Phrase(2); }			break;
				case SJCC_PHRASE3:		if(evnt2) {	IIC_SPEAKJET_MSA_Phrase(3);	}			break;
					
				case SJCC_MOUTH_PAUSES:	IIC_SPEAKJET_MSA_Pauses(evnt2);						break;
				case SJCC_PAUSE0:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE0);					break;
				case SJCC_PAUSE1:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE1);					break;
				case SJCC_PAUSE2:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE2);					break;
				case SJCC_PAUSE3:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE3);					break;
				case SJCC_PAUSE4:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE4);					break;
				case SJCC_PAUSE5:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE5);					break;
				case SJCC_PAUSE6:		IIC_SPEAKJET_MSA_Ctrl(MSA_PAUSE6);					break;
				
				case SJCC_NEXT_SLOW:	IIC_SPEAKJET_MSA_Ctrl(MSA_NEXTSLOW);				break;
				case SJCC_NEXT_LOW:		IIC_SPEAKJET_MSA_Ctrl(MSA_NEXTLOW);					break;
				case SJCC_NEXT_HIGH:	IIC_SPEAKJET_MSA_Ctrl(MSA_NEXTHIGH);				break;
				case SJCC_NEXT_FAST:	IIC_SPEAKJET_MSA_Ctrl(MSA_NEXTFAST);				break;
					
				case SJCC_MOUTH_JAW:	IIC_SPEAKJET_Articulate(ARTICULATION_JAW,	 evnt2);break;
				case SJCC_MOUTH_TONGUE:	IIC_SPEAKJET_Articulate(ARTICULATION_TONGUE, evnt2);break;
					
				case SJCC_MOUTH_SPEED:
				case SJCC_SPEED:		IIC_SPEAKJET_MSA_Speed(evnt2);						break;
				case MIDI_CC_MODULATION:
				case SJCC_MOUTH_STRESS:
				case SJCC_BEND:			IIC_SPEAKJET_MSA_Bend(evnt2);						break;
				case MIDI_CC_VOLUME:
				case SJCC_MASTER_VOL:	IIC_SPEAKJET_MSA_Volume(evnt2);						break;
					
#if KII_RECORDER_ENA
				case SJCC_PHRASE_REC_UNDO: if(evnt2) { IIC_SPEAKJET_RecordPhraseUndo();  }	break;
				case SJCC_PHRASE_REC_ABORT:if(evnt2) { IIC_SPEAKJET_RecordPhraseAbort(); }	break;
				case SJCC_PHRASE_REC:	IIC_SPEAKJET_RecordPhrase(evnt2);					break;	// rec:1, exit:0
				case SJCC_PHRASE_REC_PREVIEW:if(evnt2){IIC_SPEAKJET_RecordPhrasePreview();}	break;
#endif /* KII_RECORDER_ENA */
					
#if HARMONIZER_ENABLED
				case MIDI_CC_HARMONY_BASE:
				case SJCC_HARMONY_BASE:	
					ACHarmonizer_SetBase(evnt2);
					break;
				case MIDI_CC_HARMONY_BASE_LISTEN:
				case SJCC_HARMONY_LISTEN:
					if(evnt2 > 63) {
						harmonizer_listen = TRUE;
					} else {
						harmonizer_listen = FALSE;
					}
					break;
				case MIDI_CC_HARMONY_SCALE:
				case SJCC_HARMONY_SCALE:
					ACHarmonizer_SetScale(evnt2);
					break;
				case MIDI_CC_HARMONY_SCALE_RELATIVE_PREV:
					if(evnt2 > 63) { ACHarmonizer_ToggleScale(0); }
					break;
				case MIDI_CC_HARMONY_SCALE_RELATIVE_NEXT:
					if(evnt2 > 63) { ACHarmonizer_ToggleScale(1); }
					break;
				case MIDI_CC_HARMONY_RANDOM:
					if(evnt2 > 63) { ACHarmonizer_Random(); }
					break;
#endif
				case QUANTIZE_BPM:		
					if(mclock_state.IS_MASTER) {	ACSYNC_BPMSet((evnt1 + 60)); }			break;	// quantize
				case QUANTIZE_TOGGLE_NEXT: if(evnt2) { ACSYNC_ToggleQuantize(); }			break;
				
				case SJCC_OSC1_FREQ:	IIC_SPEAKJET_OSCFreq(1, evnt2);						break;	// (osc, value)
				case SJCC_OSC2_FREQ:	IIC_SPEAKJET_OSCFreq(2, evnt2);						break;
				case SJCC_OSC3_FREQ:	IIC_SPEAKJET_OSCFreq(3, evnt2);						break;
				case SJCC_OSC4_FREQ:	IIC_SPEAKJET_OSCFreq(4, evnt2);						break;					
				case SJCC_OSC5_FREQ:	IIC_SPEAKJET_OSCFreq(5, evnt2);						break;
					
				case SJCC_OSC1_LVL:		IIC_SPEAKJET_OSCLvl(1, evnt2);						break;	// (osc, value)
				case SJCC_OSC2_LVL:		IIC_SPEAKJET_OSCLvl(2, evnt2);						break;
				case SJCC_OSC3_LVL:		IIC_SPEAKJET_OSCLvl(3, evnt2);						break;
				case SJCC_OSC4_LVL:		IIC_SPEAKJET_OSCLvl(4, evnt2);						break;
				case SJCC_OSC5_LVL:		IIC_SPEAKJET_OSCLvl(5, evnt2);						break;
					
				case SJCC_OSC_WAVESHAPE: 
					if(evnt2 > 63) {	IIC_SPEAKJET_OSCSynthesis_ToggleWaveshape(); }		break;
				case SJCC_ENV_WAVESHAPE_TGL: 
					if(evnt2 > 63) {	IIC_SPEAKJET_ENV_ToggleWaveshape(); }				break;
				case SJCC_ENV_WAVESHAPE:IIC_SPEAKJET_ENV_Waveshape(evnt2);					break;
				case SJCC_ENV_OSC_TGL:	
					if(evnt2 > 63) {	IIC_SPEAKJET_ENV_Toggle(); }						break;
				case SJCC_ENV_OSC_A:
					sj_env_control.enveloped_osc_a = (evnt2 >> 6);
					IIC_SPEAKJET_ENVCtrl();
					break;
				case SJCC_ENV_OSC_B:
					sj_env_control.enveloped_osc_b = (evnt2 >> 6);	
					IIC_SPEAKJET_ENVCtrl();
					break;
				case SJCC_ENV_FREQ:		IIC_SPEAKJET_ENVFreq(evnt2);						break;	// (value, is14bit)
				case SJCC_SYNTH_ENV:	sj_control.enveloped_synthesis = (evnt2 >> 6);		break;

				case SJCC_DISTORTION:	IIC_SPEAKJET_Distortion(evnt2);						break;
			 
				case MIDI_CC_FOOT:
				// -- CHANNEL MODE MESSAGES --
				case MIDI_CC_ALL_SOUND_OFF:
				case MIDI_CC_ALL_NOTES_OFF:
					IIC_SPEAKJET_Reset(TRUE);	// FALSE: Soft Reset, TRUE: Hard Reset
					break;
				case MIDI_CC_RESET:		IIC_SPEAKJET_Reset(TRUE);							break;

			}
			break;
			
/*		case MIDI_PRG:						// ***** PRG, 0xC0 *****
			IIC_SPEAKJET_MSA_Phrase(evnt1);
			break;
*/			
		case MIDI_CH_AFTER:					// ***** CH-AFTERTOUCH, 0xD0 *****
			IIC_SPEAKJET_MSA_Bend(evnt1);
			break;
			
		case MIDI_PITCH:					// ***** PITCH-WHEEL, 0xE0 *****
			value14bit = (unsigned int)evnt2;
			value14bit <<= 7;
			value14bit |= (unsigned int)evnt1;
			IIC_SPEAKJET_MSA_Pitch(value14bit, TRUE);	// (value, is14bit), 0x2000 = centered!
			break;			
			
			
			// ****** SYSTEM REALTIME MESSAGES ******
#if KII_RECORDER_ENA
		case MIDI_START:
		case MIDI_CONTINUE:
			IIC_SPEAKJET_Play(TRUE);
			break;
			
		case MIDI_STOP:
			IIC_SPEAKJET_Play(FALSE);
			break;
#endif /* KII_RECORDER_ENA */
			
		case MIDI_RESET:	// RESET
			IIC_SPEAKJET_Reset(TRUE);	// Hardware-Reset
			break;
	}

	// update display
	acapp.displayNeedsUpdate = 1;
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  return 0; // no error
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
#if KII_AIN_ENABLED
  // forward to hand-tracking sensor matrix
  // TK: PIC used 10bit - convert resultion
  ACHandTracker_AIN_NotifyChange(pin, pin_value >> 2);
#endif
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
