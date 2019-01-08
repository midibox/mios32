/*
 * ACSyncronizer
 * based on MIDI Clock module by TK (mclock.h/mclock.c)
 *
 * ==========================================================================
 *
 * Copyright (C) 2005  Thorsten Klose (tk@midibox.org)
 * Addional Code Copyright (C) 2007  Michael Markert, http://www.audiocommander.de
 * 
 * ==========================================================================
 * 
 * This file is part of a MIOS application
 *
 * This application is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This application; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

#include "ACSyncronizer.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

mclock_state_t mclock_state;		 // the mclock state variable
mclock_matrix_state_t mclock_matrix_state; // state of the clock output pin

unsigned char mclock_tick_ctr;		 // requests MIDI clocks

unsigned char bpm;					 // holds the current BPM setting

unsigned char mclock_ctr_24;		 // counts from 0..23
unsigned char mclock_ctr_beats;		 // counts the quarter notes 0..3
unsigned int  mclock_ctr_measures;	 // counts the measures (up to 65535)

volatile unsigned char quantize;	 // quantize value:
									 // currently implemented: 4, 8, 16, 32

// the MIOS32 timer which is assigned to ACSYNC
#define ACSYNC_TIMER_NUM 0

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

unsigned int  mclock_master_timeout; // see MASTER_IS_ALIVE_COUNTER


/////////////////////////////////////////////////////////////////////////////
// This function initializes the MIDI clock module
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_Init(void)
{
	mclock_state.ALL = 0;
	mclock_matrix_state.ALL = 0;
	mclock_tick_ctr = 0;
	mclock_master_timeout = 0;
	mclock_state.IS_MASTER = TRUE;	// will switch to FALSE if clock input detected!
	mclock_matrix_state.TRIGGER_LOCK = TRUE; // lock for x TRIGGER_LOCK_BARS
	mclock_matrix_state.MESSAGE_REQ = TRUE;	 // enunciate welcome message
#if KIII_MODE
	quantize = ACSYNC_QUANT_16;
#else
	quantize = ACSYNC_QUANT_32;
#endif /* KIII_MODE */
	ACSYNC_ResetMeter();

        MIOS32_TIMER_Init(ACSYNC_TIMER_NUM, 100*ACSYNC_GetTimerValue(bpm), ACSYNC_Timer, MIOS32_IRQ_PRIO_MID);
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from USER_Tick to send MIDI clock and
// MIDI Clock Start/Stop/Continue
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_Tick(void)
{
	// start request? Send 0xfa and enter RUN mode
	if( mclock_state.START_REQ ) {
		mclock_state.START_REQ = 0;
		MIOS32_MIDI_SendStart(DEFAULT); // 0xfa
		mclock_state.RUN = 1;
	}
	
	// continue request? Send 0xfb and release pause
	if( mclock_state.CONT_REQ ) {
		mclock_state.CONT_REQ = 0;
		MIOS32_MIDI_SendContinue(DEFAULT); // 0xfb
		mclock_state.PAUSE = 0;
	}
	
	// stop request? Send 0xfc and leave  RUN mode
	if( mclock_state.STOP_REQ ) {
		mclock_state.STOP_REQ = 0;
		MIOS32_MIDI_SendStop(DEFAULT); // 0xfc
		mclock_state.RUN = 0;
		mclock_state.PAUSE = 0;
		mclock_tick_ctr = 0; // prevent that any requested 0xf8 will be sent
	}
	
	// check for trigger lock
	if(mclock_matrix_state.TRIGGER_LOCK) { 
		if(mclock_ctr_measures >= 1) { 
			mclock_matrix_state.TRIGGER_LOCK = FALSE;
		} else {
			mclock_matrix_state.UPDATE_REQ = FALSE;		// don't process following triggers
		}
	}
	
#if KII_AIN_ENABLED
	// update gesture
	if(mclock_matrix_state.CLOCK_REQ && mclock_matrix_state.UPDATE_REQ) {		
		mclock_matrix_state.CLOCK_REQ = FALSE;
		mclock_matrix_state.UPDATE_REQ = FALSE;
		ACHandTracker_Update();
	}
#endif /* KII_AIN_ENABLED */
	
	while( mclock_tick_ctr ) {
		// send 0xf8 until counter is 0
		MIOS32_MIDI_SendClock(DEFAULT); // 0xf8
		// decrementing the counter *MUST* be an atomic operation, otherwise it could
		// conflict with ACSYNC_Timer()
		// however, I guess that the compiler will generate a single decf instruction,
		// which is atomic... but better to write it in this way, who knows, how SDCC
		// will behave in future...
		MIOS32_IRQ_Disable();
		--mclock_tick_ctr;
		MIOS32_IRQ_Enable();
		// increment the meter counters
		if( ++mclock_ctr_24 == 24 ) {
			mclock_ctr_24 = 0;
			if( ++mclock_ctr_beats == 4 ) {
				mclock_ctr_beats = 0;
				++mclock_ctr_measures;
			}
		}
	}
	
	// if in SLAVE-MODE check if master is still alive
	if( mclock_state.IS_MASTER == 0 ) {
		++mclock_master_timeout;
		// check for timeout
		if( mclock_master_timeout > MASTER_IS_ALIVE_COUNTER ) {
			mclock_master_timeout = 0;
			// fall back to master mode
			mclock_state.IS_MASTER = TRUE;
			ACSYNC_BPMSet(bpm);		// set old bpm value and reInit Timer
			ACSYNC_ResetMeter();	// reset meter & send song position
		}
	}
	
	// startup message
	if(mclock_matrix_state.MESSAGE_REQ) {
		mclock_matrix_state.MESSAGE_REQ = FALSE;
		IIC_SPEAKJET_StartupMessage();
	}
	
#if ACSYNC_GARBAGE_SERVICE
	// frequently called cleanup service routine
	if(mclock_matrix_state.RESET_REQ) {
		mclock_matrix_state.RESET_REQ = FALSE;
		// reset speakjet (secure garbage cleanup)
#if ACSYNC_GARBAGE_SERVICE_HARDRESET
		IIC_SPEAKJET_Reset(TRUE);
#else
		IIC_SPEAKJET_Reset(FALSE);		
#endif /* ACSYNC_GARBAGE_SERVICE_HARDRESET */
		// reset meters & sync-module
		ACSYNC_DoRun();
#if KII_AIN_ENABLED
		// play startup message on input idle
		if(lastOpen[0] == 0) {
			mclock_matrix_state.RESET_DONE = TRUE;
		}
#endif /* KII_AIN_ENABLED */
	}
#endif /* ACSYNC_GARBAGE_SERVICE */	
	
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from USER_Timer to update the MIDI clock
//   MASTER:	Timer set by bmp value, called 96 times per bar
//	 SLAVE:		Timer polled by ReceiveClock, called 96 times per bar
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_Timer(void)
{	
	// just increment a clock counter if in RUN and not in PAUSE mode
	// it will be decremented in ACSYNC_Tick - each step sends one 0xf8 event
	// the advantage of this counter is, that a clock event can never get
	// lost, regardless how much the CPU is loaded
	// (however, in this application the CPU is very unbusy, but I prepared
	// this for even more complex programs...)
	
	if( mclock_state.RUN && !mclock_state.PAUSE) {
		++mclock_tick_ctr;
		mclock_matrix_state.CLOCK_REQ = TRUE;
	}
	
	// debug output
#ifdef _DEBUG_C
#if DEBUG_SYSREALTIME_VERBOSE
	printf("\n**ACSYNC_MIDICLOCK: \t %i/4 \t%i/24 \t%i/x \t%s", mclock_ctr_beats, mclock_ctr_24, mclock_ctr_measures, (mclock_matrix_state.TRIGGER_LOCK ? "<LOCKED>" : " ") );
#endif /* SYSREALTIME_VERBOSE */
#endif /* _DEBUG_C */
	// END debug output
	
#if KII_AIN_ENABLED
	switch(quantize) {
		case ACSYNC_QUANT_4:
			if(mclock_ctr_24 == 0) { mclock_matrix_state.UPDATE_REQ = TRUE;	}
			break;
		case ACSYNC_QUANT_8:
			switch(mclock_ctr_24) {
				case 0: case 12:
					mclock_matrix_state.UPDATE_REQ = TRUE;
					break;
			}
			break;
		case ACSYNC_QUANT_16:
			switch(mclock_ctr_24) {
				case 0: case 6: case 12: case 18:
					mclock_matrix_state.UPDATE_REQ = TRUE;
					break;
			}
			break;
		case ACSYNC_QUANT_32:
			switch(mclock_ctr_24) {
				case 0: case 3: case 6: case 9: case 12: case 15: case 18: case 21:
					mclock_matrix_state.UPDATE_REQ = TRUE;
					break;
			}
			break;
	}
#endif /* KII_AIN_ENABLED */
		
#if ACSYNC_GARBAGE_SERVICE
	// reset all counters and hardreset speakjet
	if( (mclock_ctr_measures >= CTR_MEASURE_MAX) && (mclock_ctr_24 >= ACSYNC_GARBAGE_SERVICE_CTR) ) {
		mclock_matrix_state.RESET_REQ = TRUE;
		mclock_matrix_state.TRIGGER_LOCK = TRUE;
	}
	// request starup message after reset
	// add a short delay, otherwise the first sentences would be cut off
	if( mclock_matrix_state.RESET_DONE && mclock_ctr_24 > 12 ) {
		mclock_matrix_state.RESET_DONE = FALSE;
		mclock_matrix_state.MESSAGE_REQ = TRUE;
	}
#endif

}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from MPROC_NotifyReceivedByte
// to update the MIDI clock
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_ReceiveClock(unsigned char byte) {
	switch(byte) {
		case MIDI_CLOCK:
			// fall back to slave mode
			if( mclock_state.IS_MASTER ) {
				mclock_state.IS_MASTER = FALSE;
				ACSYNC_BPMSet(0);	// stops timer (passive call in SLAVE mode)
				ACSYNC_ResetMeter();
			} else {
				// reset lifesign counter (master is alive)
				mclock_master_timeout = 0;
				// call timer
				ACSYNC_Timer();		// passive timer call in SLAVE mode
			}
			break;
	}
}


/////////////////////////////////////////////////////////////////////////////
// This internal function divides 3125000 / BPM
// The formula:
//   -> delay = 60 / BPM * 24
//   timer is clocked at 10 MHz, and we are using a 1:8 prescaler
//   -> timer cycles = ((60/BPM*24)/8) / 100E-9
//   -> 3125000 / BPM
//
// the 24 Bit / 16 Bit division routine has been created by Nikolai Golovchenko, 
// and is published at:
// http://www.piclist.org/techref/microchip/math/div/24by16.htm
/////////////////////////////////////////////////////////////////////////////
unsigned int ACSYNC_GetTimerValue(unsigned char _bpm) 
{
  // TK: haha! Thats simple now with a 32bit core! :-)
  return ((unsigned int)(3125000 / bpm));
}


/////////////////////////////////////////////////////////////////////////////
// These functions are used to set/query the BPM
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_BPMSet(unsigned char _bpm)
{
	// (re-)init timer depending on new BPM value
	if( _bpm > 0 ) {
		bpm = _bpm;
		MIOS32_TIMER_ReInit(ACSYNC_TIMER_NUM, 100*ACSYNC_GetTimerValue(bpm));
	} else {	// _bpm is 0 => timer off (slave mode)
	        MIOS32_TIMER_DeInit(ACSYNC_TIMER_NUM);
	}
}

unsigned char ACSYNC_BPMGet(void)
{
	if( mclock_state.IS_MASTER ) {
		return bpm;
	} else {
		return 0;
	}
}


/////////////////////////////////////////////////////////////////////////////
// This function resets the mclock_ctr variables
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_ResetMeter(void)
{
	mclock_ctr_24 = 0;
	mclock_ctr_beats = 0;
	mclock_ctr_measures = 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function sends the current song position
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_SendMeter(void)
{
	unsigned int songpos = (mclock_ctr_beats << 2) | (mclock_ctr_measures << 4);
	
	MIOS32_MIDI_SendSongPosition(DEFAULT, songpos);
}


/////////////////////////////////////////////////////////////////////////////
// These functions are used to control the ACSYNC handler from external
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_DoStop(void)
{
	if( mclock_state.RUN == 0 ) {
		// reset song position of already in stop mode
		ACSYNC_ResetMeter();
		// send Song Position
		ACSYNC_SendMeter();
	}
	// request stop
	mclock_state.STOP_REQ = 1;
	
	// request display update
	acapp.displayNeedsUpdate = 1;
}

void ACSYNC_DoPause(void)
{
	// if in RUN mode:
	if( mclock_state.RUN ) {
		// toggle pause mode
		if( mclock_state.PAUSE ) {
			mclock_state.CONT_REQ = 1;
		} else {
			mclock_state.PAUSE = 1;
		}
	} else {
		// Stop mode: just toggle PAUSE
		mclock_state.PAUSE = mclock_state.PAUSE ? 0 : 1;
	}
	
	// request display update
	acapp.displayNeedsUpdate = 1;
}

void ACSYNC_DoRun(void)
{
	// reset meter counters
	ACSYNC_ResetMeter();
	// send Song Position
	ACSYNC_SendMeter();
	// request start
	mclock_state.START_REQ = 1;
	
	// request display update
	acapp.displayNeedsUpdate = 1;
}

void ACSYNC_DoRew(void)
{
	// decrement measure and reset subcounters
	if( mclock_ctr_measures ) --mclock_ctr_measures;
	mclock_ctr_24 = 0;
	mclock_ctr_beats = 0;
	
	// send Song Position
	ACSYNC_SendMeter();
	
	// request display update
	acapp.displayNeedsUpdate = 1;
}

void ACSYNC_DoFwd(void)
{
	// increment measure and reset subcounters
	++mclock_ctr_measures;
	if(mclock_ctr_measures > 65534) { mclock_ctr_measures = 0; }
	mclock_ctr_24 = 0;
	mclock_ctr_beats = 0;
	
	// send Song Position
	ACSYNC_SendMeter();
	
	// request display update
	acapp.displayNeedsUpdate = 1;
}


/////////////////////////////////////////////////////////////////////////////
// This function toggles the quantize value (circular value setting)
/////////////////////////////////////////////////////////////////////////////
void ACSYNC_ToggleQuantize(void) {
	quantize++;
	if(quantize > ACSYNC_QUANT_MAX) {
		quantize = 0;
	}
}



