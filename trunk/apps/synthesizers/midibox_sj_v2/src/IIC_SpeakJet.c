/*
 *  IIC_SpeakJet.c
 *  kII
 *
 *  Created by audiocommander on 03.05.06.
 *  Based on a C-Skeleton of Thorsten Klose (c) 2006
 *    http://www.midibox.org
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 */

/*
 * Released under GNU General Public License
 * http://www.gnu.org/licenses/gpl.html
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation
 *
 * YOU ARE ALLOWED TO COPY AND CHANGE 
 * BUT YOU MUST RELEASE THE SOURCE TOO (UNDER GNU GPL) IF YOU RELEASE YOUR PRODUCT 
 * YOU ARE NOT ALLOWED TO USE IT WITHIN PROPRIETARY CLOSED-SOURCE PROJECTS
 */



#include <mios32.h>
#include "app.h"


// SpeakJet interface
#include "IIC_SpeakJet.h"
// this includes the local phoneme constants
#include "IIC_SpeakJetPhonemeMaps.h"

// this includes support for ACMidi_SendPanic()
#include "ACMidi.h"
// this includes the lookup table freq2note
#include "ACFreqOfNote.h"



// ********* GLOBALS ****************** //

volatile sj_control_t		sj_control;
volatile sj_env_control_t	sj_env_control;

#if KII_RECORDER_ENA
volatile sj_stepseq_t		sj_stepseq;
#endif /* KII_RECORDER_ENA */

// position of jaw and tongue
volatile unsigned char	jaw;
volatile unsigned char	tongue;

// osc waveshaping
volatile unsigned char 	oscsynth_waveshape;


// ********* LOCALS ******************* //

// this variable contains the current slave number
unsigned char slave;

#if KII_RECORDER_ENA
// if sj_stepseq.recMode input is added also to recBuffer
volatile unsigned char recBuffer[RECBUFFER_MAX];
volatile unsigned char recBuffer_length;
volatile unsigned char recBuffer_lastCtrlType;
volatile unsigned char recBuffer_lastMessage;
#endif /* KII_RECORDER_ENA */

// osc waveshaping						   SAW				  SINE (Pseudo)		 TRIANGLE			SQUARE
const unsigned char	oscSyn_frq[4][5] = { { 2, 3, 4, 0, 2 }, { 2, 3, 4, 5, 6 }, { 4, 6, 8, 0, 2 }, { 4, 6, 8, 0, 2 }  };
const unsigned char oscSyn_lvl[4][5] = { { 9, 6, 5,29,14 }, {10,10,10,10,10 }, { 2, 1, 1,30, 4 }, { 6, 4, 3,30,10 }  };

#if SPEAKJET_CYCLE_STARTUP_MESSAGES
// counter for next startup message
volatile unsigned char next_startup_msg_id;
#endif

// ********* MACROS ****************** //

#ifdef _DEBUG_C
	#pragma mark Macros
#endif

// this macro converts a number to it's char-equivalent, eg 2 => '2'
#define NUM2ASCII(num) (num + 0x30)





#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Core Functions
#endif


/////////////////////////////////////////////////////////////////////////////
// This function initializes the interface to MBHP_IIC_SPEAKJET
/////////////////////////////////////////////////////////////////////////////
void IIC_SPEAKJET_Init(void) {
  // ensure that baudrate is set correctly
  MIOS32_UART_BaudrateSet(2, 19200);

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
# error "TODO!"
#elif defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  MIOS32_SYS_LPC_PINSEL(SPEAKJET_PORT_RST_N, SPEAKJET_PIN_RST_N, 0); // IO
  MIOS32_SYS_LPC_PINDIR(SPEAKJET_PORT_RST_N, SPEAKJET_PIN_RST_N, 1); // output

  MIOS32_SYS_LPC_PINSEL(SPEAKJET_PORT_M0, SPEAKJET_PIN_M0, 0); // IO
  MIOS32_SYS_LPC_PINDIR(SPEAKJET_PORT_M0, SPEAKJET_PIN_M0, 1); // output

  MIOS32_SYS_LPC_PINSEL(SPEAKJET_PORT_M1, SPEAKJET_PIN_M1, 0); // IO
  MIOS32_SYS_LPC_PINDIR(SPEAKJET_PORT_M1, SPEAKJET_PIN_M1, 1); // output

  MIOS32_SYS_LPC_PINSEL(SPEAKJET_PORT_D2, SPEAKJET_PIN_D2, 0); // IO
  MIOS32_SYS_LPC_PINDIR(SPEAKJET_PORT_D2, SPEAKJET_PIN_D2, 0); // input
  MIOS32_SYS_LPC_PINMODE(SPEAKJET_PORT_D2, SPEAKJET_PIN_D2, 3); // activate pull-down (for the case that pin not connected)

  // enter demo mode
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_M0, SPEAKJET_PIN_M0, 1);
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_M1, SPEAKJET_PIN_M1, 0);

  // activate reset
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_RST_N, SPEAKJET_PIN_RST_N, 0);
  MIOS32_DELAY_Wait_uS(1000);
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_RST_N, SPEAKJET_PIN_RST_N, 1);
#else
# error "this MIOS32_BOARD hasn't been prepared yet!"
#endif

  // wait for 1 second (!!!!)
  int i;
  for(i=0; i<1000; ++i)
    MIOS32_DELAY_Wait_uS(1000);

  // we hear a "sonar ping" sound now!

  // continue with baudrate detection: send 0x55 to SpeakJet
  MIOS32_COM_SendChar(UART2, 0x55);

  // and wait for 2 mS
  MIOS32_DELAY_Wait_uS(2000);

  // go back to normal mode
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
# error "TODO!"
#elif defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_M0, SPEAKJET_PIN_M0, 0);
  MIOS32_SYS_LPC_PINSET(SPEAKJET_PORT_M1, SPEAKJET_PIN_M1, 1);
#else
# error "this MIOS32_BOARD hasn't been prepared yet!"
#endif

        slave = 0xff;

	// init vars
	sj_control.manualTransmission = FALSE;
	sj_control.iic_lock = FALSE;
#if KII_RECORDER_ENA
	sj_stepseq.recMode = FALSE;
	sj_stepseq.recPause = FALSE;
	recBuffer_length = 0;
	sj_stepseq.phrase = 0;
	sj_stepseq.scpMode = FALSE;
#endif /* KII_RECORDER_ENA */
	// setup articulation
	jaw = 8;
	tongue = 2;
	// setup osc waveforms
	oscsynth_waveshape = OSCSYNTH_WAVE_TRIANGLE;
	// setup envelope
	sj_env_control.ALL = 0;
	sj_env_control.enveloped_osc = 3;	// both OSC groups enveloped
#if SPEAKJET_CYCLE_STARTUP_MESSAGES
	// startup message id counter (cylcle phrases), starting with 1
	next_startup_msg_id = 1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function starts a serial transfer to SpeakJet
// it terminates with 0 if the IIC device is busy or not available
/////////////////////////////////////////////////////////////////////////////
unsigned char IIC_SPEAKJET_TransmitStart(unsigned char _slave) {
	unsigned char retry_ctr;
	
	// in manualTransmission mode, this handler has already been called!
	// if a series of commands will be sent (ie by calling multiple handlers like
	// Speed + Pitch + Volume + Allophone), multiple start/stops can be avoided:
	//   first call -> TransmitStart(),
	//   then -> set manualTransmission to TRUE
	//   invoke all functions, eg. MSA_Pitch() or MSA_Bend()
	//   at the end of the commands -> set manualTransmission to FALSE,
	//   and call -> TransmitStop()
	if(sj_control.manualTransmission) { return 0; }

#if SPEAKJET_BUFFER_CHECK
	// check buffer_half_full status (connected to core:J14)
	// if the SpeakJet's buffer is half full, transmission will be locked
	// to prevent overflows of the SJ's input buffer that may result in ugly
	// pings or pistol shots. On the other hand, messages get lost this way...
	if( SPEAKJET_GET_D2_PIN() ) {
		sj_control.iic_lock = TRUE;
		return 0;
	}
#endif /* SPEAKJET_BUFFER_CHECK */

	// store slave number
	slave = _slave;
	
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a byte to the SpeakJet
// the transfer must be initiated via IIC_SPEAKJET_TransmitStart first
// it terminates with 0 if the IIC device is busy or not available
/////////////////////////////////////////////////////////////////////////////
unsigned char IIC_SPEAKJET_TransmitByte(unsigned char value) {
	unsigned char retry_ctr;
	
#if SPEAKJET_BUFFER_CHECK
	// check lock (if buffer has been half full @ TransmitStart)
	if( sj_control.iic_lock ) { return 0; }
	// buffer overflows may apparently also happen while a transmission is
	// going on: therefore we're double-checking here
	if( SPEAKJET_GET_D2_PIN() ) { sj_control.iic_lock = TRUE; }
#endif /* SPEAKJET_BUFFER_CHECK */
	
	// validate slave
	if( slave == 0xff ) { return 0; }

	if( MIOS32_COM_SendChar(UART2, value) < 0 )
	  return 0;
	
#if KII_RECORDER_ENA
	// add to recBuffer while in recMode
	if(sj_stepseq.recMode) {
		if(recBuffer_length >= RECBUFFER_MAX) {
			// abort recMode
		        DEBUG_MSG("ERROR: RECBUFFULL");
			//MIOS_LCD_PrintCString("ERROR:RECBUFFULL");
			//MIOS_LCD_MessageStart(255); // show msg for 2 seconds
			//sj_stepseq.recMode = FALSE;
			recBuffer_length = 0;
		} else {
			// to make life easier it is only allowed to store MSA-Messages
			if(sj_stepseq.scpMode == FALSE) {
				recBuffer[recBuffer_length] = value;
				recBuffer_length ++;
			}
		}
	}
#endif /* KII_RECORDER_ENA */
	
	// the complete package has been transmitted
	return 1;
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a 16bit number as single decimal char digits
// bytes are sent by calling IIC_SPEAKJET_TransmitByte() three times
// Transmit_Start and Transmit_Stop have to be called before and after
/////////////////////////////////////////////////////////////////////////////
unsigned char IIC_SPEAKJET_TransmitNumber(unsigned int value) {
	unsigned char c;
	unsigned char result = 0;
#if SPEAKJET_BUFFER_CHECK
	// check lock (if buffer has been half full @ TransmitStart)
	// this is not necessarily needed here, 
	// but it does not hurt saving unnecessary processing
	if( sj_control.iic_lock ) { return 0; }
#endif /* SPEAKJET_BUFFER_CHECK */
	// param3: hundret-thousands
	c = (value/100000) % 10;
	if(c > 0) { result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) ); }
	// param3: ten-thousands
	c = (value/10000) % 10;
	if((c > 0) || result) { result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) ); }
	// param2: thousands
	c = (value/1000) % 10;
	if((c > 0) || result) { result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) ); }
	// param2: hundrets
	c = (value/100) % 10;
	if((c > 0) || result) { result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) ); }
	// param1: tens
	c = (value/10) % 10;
	if((c > 0) || result) { result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) ); }
	// param2: ones
	c = value % 10;
	result = IIC_SPEAKJET_TransmitByte( NUM2ASCII(c) );	// also transmit '0'
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// This function finishes a transfer to the SpeakJet
/////////////////////////////////////////////////////////////////////////////
void IIC_SPEAKJET_TransmitStop(void) {
	// stop IIC access if not in manualTransmission mode
	if(sj_control.manualTransmission) { return; }
#if SPEAKJET_BUFFER_CHECK
	// free lock (locked if buffer was half full @ TransmitStart()
	sj_control.iic_lock = FALSE;
#endif /* SPEAKJET_BUFFER_CHECK */
	// invalidate slave number
	// (to avoid that slave will send something before transfer has been started)
	slave = 0xff;
}


/////////////////////////////////////////////////////////////////////////////
// This function clears the input buffer of the speakjet
// to clear any pending commands
/////////////////////////////////////////////////////////////////////////////
void IIC_SPEAKJET_ClearBuffer(void) {
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0,SCP_CTRLTYPE_REALTIME); // Enter SCP mode
	IIC_SPEAKJET_TransmitByte(SCP_CLEAR_BUFFER);	 // Clear SCP Buffer
	IIC_SPEAKJET_SCP_Exit(0);						 // Exit SCP (don't write)
	IIC_SPEAKJET_TransmitStop();
}


/////////////////////////////////////////////////////////////////////////////
// This function resets the SpeakJet (Hard Reset or Controls Reset)
// by resetting MSA controls, clearing the input buffer 
// and (optionally) sending a hardware-reset request
/////////////////////////////////////////////////////////////////////////////
void IIC_SPEAKJET_Reset(unsigned char hardreset) {
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_RESET);			 // Reset MSA Controls (Vol, Speed, Pitch...)
	IIC_SPEAKJET_SCP_Enter(0,SCP_CTRLTYPE_REALTIME); // Enter SCP mode
	IIC_SPEAKJET_TransmitByte(SCP_CLEAR_BUFFER);	 // Clear SCP Buffer
	if(hardreset) {
		IIC_SPEAKJET_TransmitByte(SCP_RESET);		 // SCP Hardware Reset
	}
	IIC_SPEAKJET_SCP_Exit(0);						 // Exit SCP (don't write)
	IIC_SPEAKJET_TransmitStop();
}


/////////////////////////////////////////////////////////////////////////////
// This function triggers the startup message(s)
// While the sync starts (mclock_ctr_measures == 0) any input from connected
// sensor matrixes is muted (see ACSyncronizer: mclock_matrix_state.TRIGGER_LOCK)
/////////////////////////////////////////////////////////////////////////////
void IIC_SPEAKJET_StartupMessage(void) {
	unsigned char msg;
	// trigger welcome messages
	IIC_SPEAKJET_TransmitStart(0);					// start transmission
	sj_control.manualTransmission = TRUE;			// enter manual transmission mode to suppress start/stop
	IIC_SPEAKJET_MSA_Ctrl(MSA_RESET);				// reset all params
#if SPEAKJET_STARTUP_MESSAGES						// -----------------------------------
#if SPEAKJET_STARTUP_PANIC
	ACMidi_SendPanic(16);							// send panic on all channels (>15)
#endif /* SPEAKJET_STARTUP_PANIC */
	IIC_SPEAKJET_MSA_Pauses(40);					// pause (40 >> 1 = 20) => 20ms
#if SPEAKJET_CYCLE_STARTUP_MESSAGES
	msg = next_startup_msg_id;						// next msg (0..15)
	next_startup_msg_id++;
	if(next_startup_msg_id > SPEAKJET_STARTUP_MESSAGES_MAX) { next_startup_msg_id = 0; }
#else
	msg = ACMath_RandomInRange(SPEAKJET_STARTUP_MESSAGES_MAX);
#endif /* SPEAKJET_CYCLE_STARTUP_MESSAGES */
	IIC_SPEAKJET_MSA_Phrase(msg);					// call phrase
//#if HANDTRACKER_CC_OUTPUT
	ACMidi_SendCC(MIDI_CC + channelAssignment,SJCC_PHRASE,msg);	// broadcast phrase by midi
//#endif /* HANDTRACKER_CC_OUTPUT */
#endif /* SPEAKJET_STARTUP_MESSAGES */				// -----------------------------------
	sj_control.manualTransmission = FALSE;			// leave manual transmission mode
	IIC_SPEAKJET_TransmitStop();					// stop transmission
}


#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark MSA Control
#endif


/////////////////////////////////////////////////////////////////////////////
// These functions control the SpeakJet by MSA (Mathmatical Sound Architecture)
// Each MSA-Command is put to the 64-Byte Buffer of the Speakjet and executed
// one after another. 
// See http://www.midibox.org/dokuwiki/doku.php?id=speakjet_control_overview
// for a detailed explanation of values
/////////////////////////////////////////////////////////////////////////////

void IIC_SPEAKJET_MSA_Stop(void) {
	// stops enunciating and clears buffer to stop current MSA Sounds
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0,SCP_CTRLTYPE_REALTIME); // Enter SCP Mode
	IIC_SPEAKJET_TransmitByte(SCP_STOP);			// stop any enunciations
	IIC_SPEAKJET_TransmitByte(SCP_CLEAR_BUFFER);	// clear input buffer
	IIC_SPEAKJET_TransmitByte(SCP_START);			// resume enunciating
	IIC_SPEAKJET_SCP_Exit(FALSE);					// Exit SCP Mode
	IIC_SPEAKJET_TransmitStop();
}
	
void IIC_SPEAKJET_MSA_Phrase(unsigned char phrase) {
	// 0..16
	if(phrase > 15) { return; }
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_CALLPHRASE);
	IIC_SPEAKJET_TransmitByte(phrase);
	IIC_SPEAKJET_TransmitStop();
#if KII_RECORDER_ENA
	// store current program if not in recMode (recursive phrases!)
	if(! sj_stepseq.recMode) { sj_stepseq.phrase = phrase; }
#endif /* KII_RECORDER_ENA */
}

void IIC_SPEAKJET_MSA_Ctrl(unsigned char msaCtrl) {
	// MSA Control Codes 0..31, see SpeakJetDefines.h
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(msaCtrl);	
	IIC_SPEAKJET_TransmitStop();
}
	
void IIC_SPEAKJET_MSA_Soundcode(unsigned char soundcode) {
	// 0..127 => 128..254	
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(soundcode | 0x80);	// | 0x80 => soundcodes 128..254
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_MSA_Allophone(unsigned char soundcode) {
	// 0..127 => 128..199 (72 allophones)
	// restrict to allophones 128..199 (28..99)
	if((soundcode > 27) && (soundcode < 100)) {
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte((soundcode | 0x80) - 28);
		IIC_SPEAKJET_TransmitStop();
	} else if(soundcode == 127) {
		// send EOP
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte(MSA_EOP);
		IIC_SPEAKJET_TransmitStop();
	}
}

void IIC_SPEAKJET_MSA_FX(unsigned char soundcode) {
	// 0..127 => 200..254 (55 sound fx)
	// restrict to soundFX 200..254 (36..90)
	if((soundcode > 35) && (soundcode < 91)) {
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte((soundcode | 0x80) + 36);
		IIC_SPEAKJET_TransmitStop();
	} else if(soundcode == 127) {
		// send EOP
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte(MSA_EOP);
		IIC_SPEAKJET_TransmitStop();
	}
}

void IIC_SPEAKJET_MSA_Percussive(unsigned char soundcode) {
	// 0..127 => 165..199 + 253 (35)
	if((soundcode > 46) && (soundcode < 82)) {
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte((soundcode | 0x80) - 10);
		IIC_SPEAKJET_TransmitStop();
	} else if(soundcode == 82) {
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte(MSAFX_PISTOLSHOT);
		IIC_SPEAKJET_TransmitStop();
	} else if(soundcode == 127) {
		// send EOP
		IIC_SPEAKJET_TransmitStart(0);
		IIC_SPEAKJET_TransmitByte(MSA_EOP);
		IIC_SPEAKJET_TransmitStop();
	}
}

void IIC_SPEAKJET_MSA_Pauses(unsigned char value) {
	// 0..127, 0:short <-> 127:long
	unsigned char pauseLength = value >> 1;	// accepted: 0..255, restricted 0..64 (10 ms interval), 64 => 640 ms
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_DELAY);
	IIC_SPEAKJET_TransmitByte(pauseLength);
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_MSA_Volume(unsigned char value) {
	// value ranges from 0..127
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_VOLUME);
	IIC_SPEAKJET_TransmitByte(value);
	IIC_SPEAKJET_TransmitStop();
}
	
void IIC_SPEAKJET_MSA_Speed(unsigned char value) {
	// value ranges from 0..127
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_SPEED);
	IIC_SPEAKJET_TransmitByte(value);
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_MSA_Pitch(unsigned int value, unsigned char valueIs14bit) {
	unsigned char value8bit;
	// value ranges from 0..16383; if (valueIs14bit == 0) 0..127 expected => processed for C1..H5 (36..95)
	if(valueIs14bit) {
		// pitch is 0-8192-16383
		// SJ-pitch is 0-128-255 with default 88
		// sets 88 as center by subtracting 40
		value8bit = (unsigned char)(value >> 6);
		#if(SPEAKJET_DEFAULT_PITCH_CORRECTION)
		if(value8bit >= 40) {
			value8bit -= 40;
		} else {
			value8bit = 0;
		}
		#endif
	} else {
		// pitch is in Hz => IN: notes 0 to 127 | PROC: notes 0 to 59 | OUT: freq 0 to 255 Hz
		if((value > 35) && (value <96)) {	// C1..H5
			value8bit = value - 36;			// 36..95 => 0..59
			value8bit = freqOfNote[value8bit];
		} else {
			return;
		}
	}
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_PITCH);
	IIC_SPEAKJET_TransmitByte(value8bit);	// MSA -> number as unsigned char
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_MSA_Bend(unsigned char value) {
	// value ranges from 0..127, converted to 0..15
	// default is 5, therefore a value of 64 (middle of joystick) will be interpreted as 5
	if(value == 64) { value = 40; }
	// send MSA-Bend
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_BEND);
	IIC_SPEAKJET_TransmitByte(value >> 3);
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_Direct_Bend(unsigned char value) {
	// value ranges from 0..15, default is 5
	// send MSA-Bend
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(MSA_BEND);
	IIC_SPEAKJET_TransmitByte(value);
	IIC_SPEAKJET_TransmitStop();
}





#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Articulation Control
#endif

/////////////////////////////////////////////////////////////////////////////
// These functions control the articulatin process
// ARTICULATION_JAW: moves the jaw
// ARTICULATION_TONGUE: moves the tonge
/////////////////////////////////////////////////////////////////////////////

void IIC_SPEAKJET_Articulate(unsigned char articulationType, unsigned char value) {
	// see SpeakJetDefines! (JAW/TONGUE), value: 0..127
	// control articulation effects like jaw, tongueposition
	// no sound hearable until _Enunciate is called!
	switch(articulationType) {
		case ARTICULATION_JAW:
			// we got 15 jaw states
			value = (value >> 3);
			if(value > 15) { return; }
			jaw = value;
			break;
		case ARTICULATION_TONGUE:
			// we got 5 tongue states
			value = (value >> 4) - 1;
			if(value > 4) { return; }
			tongue = value;
			break;
	}
}


unsigned char IIC_SPEAKJET_Enunciate(unsigned char phonemeType, unsigned char value) {
	// see SpeakjetDefines! value: 0..127
	// enunciate pitched phonemes based on phonemeType (eg vowels, fricatives or special mixed-modes)
	unsigned char temp_j = 0;
	unsigned char temp_t = 0;
	unsigned char phoneme = 0;
	switch(phonemeType) {
		case SJCH_VOWELS:
			phoneme = vowel_map[jaw][tongue];
			break;
		case SJCH_VOWELS_DIPHTONGS:
		case SJCH_DIPHTONGS:
			phoneme = vowel_diphtong_map[jaw][tongue];
			break;
		case SJCH_VOWELS_CONSONANTS:
			phoneme = vowl_cons_map[jaw][tongue];
			break;
#if SPEAKJET_USE_RANDOM_CONSONANTS
		// random consonant output. it's main purpose is to improve consonant output for
		// ACHandtracker to decouple consonants from jaw states (prevent similar cons only)
		case SJCH_CONSONANTS:
			temp_j = ACMath_RandomInRange(14);
			temp_t = ACMath_RandomInRange(4);
			phoneme = consonant_map[temp_j][temp_t];
			break;
		case SJCH_CONSONANTS_OPEN:
			temp_j = ACMath_RandomInRange(14);
			temp_t = ACMath_RandomInRange(4);
			phoneme = consonant_open_map[temp_j][temp_t];
			break;
		case SJCH_CONSONANTS_CLOSE:
			temp_j = ACMath_RandomInRange(14);
			temp_t = ACMath_RandomInRange(4);
			phoneme = consonant_close_map[temp_j][temp_t];
			break;
#else
		case SJCH_CONSONANTS:
			phoneme = consonant_map[jaw][tongue];
			break;
		case SJCH_CONSONANTS_OPEN:
			phoneme = consonant_open_map[jaw][tongue];
			break;
		case SJCH_CONSONANTS_CLOSE:
			phoneme = consonant_close_map[jaw][tongue];
			break;
#endif /* SPEAKJET_USE_RANDOM_CONSONANTS */
		default:
			// phonemeType does not match; abort
			return 0;
	}
			
	// set pitch in Hz, ranging from 0..127 from notes 0..59
	IIC_SPEAKJET_MSA_Pitch(value, FALSE);	// false because value is not 14bit
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_TransmitByte(phoneme);
	IIC_SPEAKJET_TransmitStop();
	// return phoneme (optional)
	return phoneme;
}





#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark SCP Controls
#endif


/////////////////////////////////////////////////////////////////////////////
// These functions control the SpeakJet by SCP (Serial Control Protocol)
// As soon as the SCP is entered, commands are executed FIFO (first in first out)
// and *not* stored to the 64-Byte input buffer!
// eg. use 'S' and 'T' for PAUSE/RESUME...
// NO MSA WILL BE AVAILABLE WHILE IN SCP MODE! TAKE CARE TO EXIT SCP!
// See http://www.midibox.org/dokuwiki/doku.php?id=speakjet_control_overview
// for a detailed explanation of values
/////////////////////////////////////////////////////////////////////////////

void IIC_SPEAKJET_SCP_Enter(unsigned char sj_id, unsigned char ctrl_type) {
	// SCP_CTRLTYPE_REALTIME or _REGISTER
	// Transmit Start() has to be called seperately!!!
#if KII_RECORDER_ENA
	sj_stepseq.scpMode = TRUE;
#endif /* KII_RECORDER_ENA */
	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(SCP_ESCAPE);			// send Escape Char to ENTER SCP mode!
	IIC_SPEAKJET_TransmitByte(NUM2ASCII(sj_id));	// 0 for all speakjets, 1-7 for addressable!
	if(ctrl_type == SCP_CTRLTYPE_REGISTER) {
		// select register memory type, too
		IIC_SPEAKJET_TransmitByte(SCP_MEMTYPE_REGISTER);
		IIC_SPEAKJET_TransmitByte(SCP_MEMTYPE);
	}
}
		
void IIC_SPEAKJET_SCP_Exit(unsigned char write) {
	if(write) {
		// write after _REGISTER
		IIC_SPEAKJET_TransmitByte(SCP_MEMWRT);
	}
	// exit from serial control mode
	IIC_SPEAKJET_TransmitByte(SCP_EXIT);
	IIC_SPEAKJET_TransmitStop();
#if KII_RECORDER_ENA
	sj_stepseq.scpMode = FALSE;
#endif /* KII_RECORDER_ENA */
}




void IIC_SPEAKJET_OSCSynthesis_ToggleWaveshape(void) {
	// toggle waveshape for OSC Subtractive Synthesis
	if(oscsynth_waveshape >= 3) {
		oscsynth_waveshape = 0;
	} else {
		oscsynth_waveshape++;
	}
}

void IIC_SPEAKJET_OSCSynthesis(unsigned char value) {
	// all OSCs, 0..127
	unsigned char c;
	unsigned int  f[9];		// harmonics
	// determination of frequency harmonics by looking up the table
	f[0] = freqOfNote[value];
	f[1] = f[0] << 1;
	f[2] = f[0] + f[1];
	f[3] = f[0] << 2;
	f[4] = f[0] + f[3];
	f[5] = f[0] << 3;
	f[6] = f[0] + f[5];
	f[7] = f[0] << 4;
	f[8] = f[0] + f[7];
	// check maximum allowed range to prevent buffer-overflows in speakJet
	for(c=0;c<9;c++) {
		if(f[c] > 3999) { f[c] = 0; }
	}
	// start SCP transmission
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0,SCP_CTRLTYPE_REGISTER);
	
	// set OSCs
	for (c=0;c<5;c++) {
		IIC_SPEAKJET_TransmitByte(NUM2ASCII(c+1));		// select OSC Frq
		IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
		IIC_SPEAKJET_TransmitNumber(f[oscSyn_frq[oscsynth_waveshape][c]]);
		IIC_SPEAKJET_TransmitByte(SCP_MEMWRT);
		IIC_SPEAKJET_TransmitByte('1');
		IIC_SPEAKJET_TransmitByte(NUM2ASCII(c+1));		// select OSC Lvl
		IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
		IIC_SPEAKJET_TransmitNumber(oscSyn_lvl[oscsynth_waveshape][c]);
		IIC_SPEAKJET_TransmitByte(SCP_MEMWRT);
	}
	if(sj_control.enveloped_synthesis) {
		// set envelope
		// note: OSCSYNTH_ENV_DETUNE support not yet supported
		IIC_SPEAKJET_TransmitByte(SCP_ENV_FREQ);		// select envelope register 0
		IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
		IIC_SPEAKJET_NoteToFreqChars(value);			// write frequency to register
	}
	// cleanup
	IIC_SPEAKJET_SCP_Exit(0);							// Exit SCP, don't write
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_OSCFreq(unsigned char osc, unsigned char value) {
	// osc = 1..5, value = 0..127, SCP_FREQ_MAX is 3999
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REGISTER);	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(NUM2ASCII(osc));			// select oscillator register osc# (1 - 5): FREQ
	IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
	IIC_SPEAKJET_NoteToFreqChars(value);				// write frequency to register
	IIC_SPEAKJET_SCP_Exit(1);							// exit and write
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_OSCLvl(unsigned char osc, unsigned char value) {
	// 0..127 => 0..31
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REGISTER);	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(NUM2ASCII(1));			// select oscillator register osc# (11 - 15): LEVEL
	IIC_SPEAKJET_TransmitByte(NUM2ASCII(osc));			// select oscillator register osc# (11 - 15): LEVEL
	IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
	IIC_SPEAKJET_TransmitNumber((unsigned int)value >> 2);	// write oscillator level 0..31	
	IIC_SPEAKJET_SCP_Exit(1);							// exit and write
	IIC_SPEAKJET_TransmitStop();
}



void IIC_SPEAKJET_ENV_ToggleWaveshape(void) {
	// toggle envelope waveshape (0..3)
	if(sj_env_control.waveshape >= 3) { 
		sj_env_control.waveshape = 0; 
	} else {
		sj_env_control.waveshape++;
	}
	IIC_SPEAKJET_ENVCtrl();
}

void IIC_SPEAKJET_ENV_Toggle(void) {
	// toggle ENV setting for OSC groups (1-3 & 4-5)
	if(sj_env_control.enveloped_osc >= 3) {
		sj_env_control.enveloped_osc = 0;
	} else {
		sj_env_control.enveloped_osc++;
	}
	IIC_SPEAKJET_ENVCtrl();
}

void IIC_SPEAKJET_ENV_Waveshape(unsigned char value) {
	// 0..127 => 0..3
	// see SpeakJetDefines!
	unsigned char c = (value >> 5);
	if(c <= 3) { sj_env_control.waveshape = c; }
	IIC_SPEAKJET_ENVCtrl();
}

void IIC_SPEAKJET_ENVCtrl() {
	// transmit ENV settings
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REGISTER);	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(SCP_ENV_CTRL);			// select envelope control register 8
	IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
	IIC_SPEAKJET_TransmitNumber( NUM2ASCII((unsigned char)sj_env_control.ALL) ); // send envelope status
	IIC_SPEAKJET_SCP_Exit(1);							// exit and write
	IIC_SPEAKJET_TransmitStop();
}

void IIC_SPEAKJET_ENVFreq(unsigned char value) {
	// 0..127, SCP_FREQ_MAX is 3999
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REGISTER);	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(SCP_ENV_FREQ);			// select envelope register 0
	IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
	IIC_SPEAKJET_NoteToFreqChars(value);				// write frequency to register
	IIC_SPEAKJET_SCP_Exit(1);							// exit and write
	IIC_SPEAKJET_TransmitStop();
}



void IIC_SPEAKJET_Distortion(unsigned char value) {
	// 0..127 => 0..255
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REGISTER);	// set speakjet to serial control mode
	IIC_SPEAKJET_TransmitByte(SCP_DISTORTION);			// select distortion register 6 (affects OSC 5 & 6)
	IIC_SPEAKJET_TransmitByte(SCP_MEMADDR);
	IIC_SPEAKJET_TransmitNumber((unsigned int)(value << 1));	// write distortion level to register
	IIC_SPEAKJET_SCP_Exit(1);							// exit and write
	IIC_SPEAKJET_TransmitStop();
}





#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Recorder
#endif

#if KII_RECORDER_ENA

/////////////////////////////////////////////////////////////////////////////
// These functions control the SpeakJet's Phrases
// Play, Stop and StepRecord.
// Currently unimplemented is saving to the internal EEPROM Phrase Storage 0..15
// Enable this if you know that you could possibly destroy the firmware of 
// the SpeakJet in case of an error!
//
// When sj_stepseq.recMode is true, each byte sent by 
// IIC_SPEAKJET_TransmitByte() is also stored in the recbuffer. 
// Values of the same type get only updated if following each other.
// The recBuffer is limited to 64 bytes due to the SJ Input buffer. Because the
// PIC16 firmware has also buffering capabilities, this value could be increased.
/////////////////////////////////////////////////////////////////////////////

void IIC_SPEAKJET_Play(unsigned char start) {
	// 1: start, 0: stop
	IIC_SPEAKJET_TransmitStart(0);
	IIC_SPEAKJET_SCP_Enter(0, SCP_CTRLTYPE_REALTIME);	// set speakjet to serial control mode
	// set speakjet's wait flag to 0 ('T', start) or 1 ('S', stop)
	if(start) { 
		IIC_SPEAKJET_TransmitByte(SCP_START); 
	} else {
		IIC_SPEAKJET_TransmitByte(SCP_STOP);
	}
	IIC_SPEAKJET_SCP_Exit(0);							// exit without writing
	IIC_SPEAKJET_TransmitStop();
}


void IIC_SPEAKJET_RecordPhrase(unsigned char start) {
	// 1: start, 0: stop
	// note that saving to the SpeakJet is NOT supported here due to possible sj fw-damage
	if(start) {
		// begin recording for current phrase
		// selected by IIC_SPEAKJET_MSA_Phrase(<#unsigned char phrase#>)
		sj_stepseq.recMode = TRUE;
	} else {
		IIC_SPEAKJET_TransmitStart(0);
		// add End of Phrase
		IIC_SPEAKJET_TransmitByte(MSA_EOP);
		sj_stepseq.recMode = FALSE;
		IIC_SPEAKJET_TransmitStop();
		// reset recBuffer
		recBuffer_length = 0;
	}
}

void IIC_SPEAKJET_RecordPhrasePause(unsigned char resume) {
	if(sj_stepseq.recPause && resume) {
		sj_stepseq.recPause = 0;
	} else {
		sj_stepseq.recPause = 1;
	}
}

void IIC_SPEAKJET_RecordPhraseAbort(void) {
	// just reset buffer and drop out of recMode
	sj_stepseq.recMode = FALSE;
	recBuffer_length = 0;
}

void IIC_SPEAKJET_RecordPhraseUndo(void) {
	// removes last added value from recBuffer
	if(recBuffer_length) {	// if recBuffer_length > 0
		recBuffer_length--;
	}
}

void IIC_SPEAKJET_RecordPhrasePreview(void) {
	// prelisten current buffer-content
	unsigned char c;
	if(sj_stepseq.recMode && recBuffer_length) {
		// temporarily disable recMode
		sj_stepseq.recMode = FALSE;
		IIC_SPEAKJET_TransmitStart(0);
		for(c=0; c<=recBuffer_length; c++) {
			IIC_SPEAKJET_TransmitByte(recBuffer[c]);
		}
		IIC_SPEAKJET_TransmitStop();
		// re-enable recMode
		sj_stepseq.recMode = TRUE;
	}
}
#endif /* KII_RECORDER_ENA */



#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark Helpers
#endif

/////////////////////////////////////////////////////////////////////////////
// These functions converts 7bit MIDI (0-127) to Frequencies and/or Levels
// The number is split into digits via BCD-conversion
//   and each byte is sent as single ASCII-Char ('0' not 0).
// A range of 0..127 is accepted, though the SJ range is more restricted:
//   MSA-Pitch (send directly, no BCD required!): 0..59
//	 SCP-Frequency (BCD): 0..107: 8..3951 Hz
// freqOfNote array is used both for MSA-Pitch conversion 
// and SCP-Freq in this function
// Note that there also definitions in ACMidiDefines
/////////////////////////////////////////////////////////////////////////////


extern void IIC_SPEAKJET_NoteToFreqChars(unsigned char note) {
	// eg. sends note 59 as '2','4','7' (Hz)
	unsigned int freq;
	// check for notes > 107
	if(note < 108) {
		freq = freqOfNote[note];
		IIC_SPEAKJET_TransmitNumber(freq);
	} else {
		return;
	}
}
		


