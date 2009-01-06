/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>

#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>
#include <seq_bpm.h>



mclock_t mclock;													// Allocate the Master Clock struct

sclock_t sclock[max_sclocks];										// Allocate the SubClock Array

u32 mod_tick_timestamp;

unsigned char midiclockcounter;
unsigned char midiclockdivider;

void clocks_init(void) {

	// reset sequencer
    SEQ_BPM_TickSet(0);

	
	// init BPM generator
	SEQ_BPM_Init(0);												// initialize SEQ module sequencer

	//SEQ_BPM_PPQN_Set(vxppqn);
	SEQ_BPM_PPQN_Set(384);
	SEQ_BPM_Set(120.0);
  
  
	mclock.status = 0;												// bit 7 is run/stop
	mclock.ticked = 0;
	mclock.timesigu = 4;											// Upper value of the Master Time Signature, min 2
	mclock.timesigl = 4;											// Lower value of the Master Time Signature, min 2
	mclock.res = SEQ_BPM_PPQN_Get();	

	mclock_init();
	
	//mclock_setbpm(90.0);												// Master BPM
	
	
	unsigned char i;	
	for (i = 0; i < max_sclocks; i++) {								// i is the number of SubClocks
		sclock[i].status = 0;										// bit 7 is run/stop
		sclock[i].ticked = 0;
		sclock[i].countmclock = 0;
		
		sclock[i].patched = 0;
		sclock[i].ticked = 0;
		
		sclock_init(i, 4, 1, 1, 1);
	}	
}

void mclock_init(void) {
    mclock.cyclelen = (unsigned long) (((SEQ_BPM_PPQN_Get() * 4) * mclock.timesigu) / mclock.timesigl);			// Length of master track measured in ticks.
	midiclockdivider = (SEQ_BPM_PPQN_Get())/24;
}


void sclock_init(unsigned char SC, unsigned char steps1, unsigned char steps2, unsigned char loops1, unsigned char loops2) {
    /*
    - SubClocks -
    Each SubClock has two pairs of parameters, presented to the user as two fractions in the form of Steps/Loops.
    (IE, the subclock will tick (steps) times per every (loops) cycles of the master clock

    When one of the Steps per Loop values is changed either
		The two Steps values are multiplied to produce the SubClock Numerator
	or
		The two Loops values are multiplied to produce the SubClock Denominator (effect is similar to a clock divider)
	*/
	
	if (SC < max_sclocks) {
		sclock[SC].steps1 = steps1;
		sclock[SC].loops1 = loops1;
		sclock[SC].steps2 = steps2;
		sclock[SC].loops2 = loops2;
		
		sclock[(SC)].numerator = sclock[(SC)].steps1 * sclock[(SC)].steps2;
		sclock[(SC)].denominator = sclock[(SC)].loops1 * sclock[(SC)].loops2;				// And then the SubClock Initialisation is run:
		
		sclock[(SC)].cyclelen = mclock.cyclelen * sclock[(SC)].denominator;					// The Master Clock Cycle Length is multiplied by the SubClock Denominator to obtain the SubClock Cycle Length
		sclock[(SC)].quotient = sclock[(SC)].cyclelen / sclock[(SC)].numerator;				// The SubClock Cycle Length is then divided by the SubClock Numerator
		sclock[(SC)].modulus = sclock[(SC)].cyclelen % sclock[(SC)].numerator;				// to produce the SubClock Quotient and SubClock Modulus (often called a ‘remainder’)
		sclock[(SC)].modcounter = sclock[(SC)].modulus;										// The SubClock Modulus Counter is set to equal the SubClock Modulus
		
		sclock[(SC)].ticksent = 0;
		
		sclock[(SC)].countdown = sclock[(SC)].quotient;										// calculate ticks till next step
		sclock[(SC)].countmclock = 1;
		
	}
	
}


void mclock_setbpm(unsigned char bpm) {
	
	mclock.bpm = bpm;												// store the tempo
	if (SEQ_BPM_Set(bpm) < 0) {
		mclock.status = 0;											// handle failure by stopping master clock
	}
	
}


void mclock_tick(void) {
	unsigned char i;
  // handle requests
	u8 num_loops = 0;
	u8 again = 0;
 
	u16 new_song_pos;

	do {
		++num_loops;
		
		// note: don't remove any request check - clocks won't be propagated
		// so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
		if( SEQ_BPM_ChkReqStop() ) {
			mclock_stop();
		  //SEQ_PlayOffEvents();
		}
		
		if( SEQ_BPM_ChkReqCont() ) {
			mclock_unpause();
		  // release pause mode
		  //seq_pause = 0;
		}
		
		if( SEQ_BPM_ChkReqStart() ) {
			mclock_reset();
		  //SEQ_Reset();
		}
		
		
		if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
		  //SEQ_PlayOffEvents();
		}
		
		if( SEQ_BPM_ChkReqClk(&mod_tick_timestamp) > 0 ) {
			(mclock.ticked)++;
			again = 1; // check all requests again after execution of this part
		}
		
	} while( again && num_loops < 10 );
  
	if (mclock.status == 0x80) {
	    if (mclock.ticked) {
			// Unneeded interrupts are disabled in the module now MIOS32_IRQ_Disable();									// port specific FreeRTOS function to disable IRQs (nested)
			--(mclock.ticked);
			// Unneeded interrupts are disabled in the module now MIOS32_IRQ_Enable();									// port specific FreeRTOS function to disable IRQs (nested)
			for (i = 0; i < max_sclocks; i++) {						// i is the number of SubClocks
				
				if (sclock[(i)].status > 0x7F) {					// only do this if the SubClock is active
					if ((--(sclock[(i)].countmclock)) == 0 ) {		// decrement the master tick counter
						sclock_tick(i);
						
					}
					
				}
				
			}
			
			if (SEQ_BPM_IsMaster()) {
				if ((midiclockcounter++) == 0) {
					midiclockcounter = midiclockdivider;
					
					mios32_midi_package_t midi_package;
					midi_package.type     = 0x5; 					// special event package type
					midi_package.evnt0    = 0xf8;
					midi_package.evnt1    = 0;
					midi_package.evnt2    = 0;
					
					SEQ_MIDI_OUT_Send(USB0, midi_package, SEQ_MIDI_OUT_ClkEvent, mod_tick_timestamp);
					
				}
				
			}
			
		}
		
	}
	
}


/////////////////////////////////////////////////////////////////////////////
/*
 - SubClock Countdown Calculation Sequence –
This calculates how many master clock ticks until the next SubClock tick. It is called as required
(When a SubClock has been patched to it’s assigned tracks and the tick is cleared, and we want to calculate
how long until the next tick). This does not run over all SubClocks, just one, which must be specified. SubClock number is input.
*/
/////////////////////////////////////////////////////////////////////////////

void sclock_tick(unsigned char SC) {
	(sclock[(SC)].ticked)++;										// increment the subclock tick counter
	sclock[(SC)].countmclock = sclock[(SC)].countdown;		// adjust master tick counter

    sclock[(SC)].countdown = sclock[(SC)].quotient;					// calculate ticks till next step
	
    if (sclock[(SC)].modulus) {										// only do this if there is a remainder
    	sclock[(SC)].modcounter = sclock[(SC)].modcounter + sclock[(SC)].modulus;			// The SubClock Modulus is added to the SubClock Modulus Counter
	    if (sclock[(SC)].modcounter > sclock[(SC)].modulus) {		// If the SubClock Modulus Counter is greater than SubClock Modulus then
            (sclock[(SC)].countdown)++;								// The SubClock Countdown is incremented by 1 (this is the distribution of the modulus)
	        sclock[(SC)].modcounter = sclock[(SC)].modcounter - sclock[(SC)].numerator;		// The SubClock Numerator is subtracted from SubClock Modulus Counter (this controls how often the modulus is distributed)
			
	    }
		
    }
	
}



void mclock_play(void) {
	midiclockcounter = 0;
	mclock.status = 0x80;
}


void mclock_unpause(void) {
	midiclockcounter = 0;
	mclock.status = 0x80;
}


void mclock_reset(void) {
	midiclockcounter = 0;
	mclock.status = 0x80;
	
	SEQ_MIDI_OUT_FlushQueue();
	
	SEQ_BPM_TickSet(0);												// reset BPM tick
}


void mclock_stop(void) {
	midiclockcounter = 0;	
	mclock.status = 0x00;

	SEQ_MIDI_OUT_FlushQueue();
}


// FIXME todo
// subclock patchbay
// nonlinear subclocks