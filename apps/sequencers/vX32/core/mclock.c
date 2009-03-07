/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

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



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

mclock_t mclock;																// Allocate the Master Clock struct

u32 mod_tick_timestamp = dead_timestamp;										// initial global clock timestamp

unsigned int midiclockcounter;													// counts SEQ_BPM module ticks at vxppqn resolution
unsigned int midiclockdivider;													// divider value used to generate 24ppqn output


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialise the clocks
/////////////////////////////////////////////////////////////////////////////

void clocks_init(void) {
	
	SEQ_BPM_TickSet(0);															// reset sequencer
	
	SEQ_BPM_Init(0);															// initialize SEQ module sequencer
	
	SEQ_BPM_PPQN_Set(vxppqn);													// set internal clock resolution as per the define (usually 384)
	mclock_setbpm(120.0);														// Master BPM
	
	
	mclock.status = 0;															// bit 7 is run/stop
	mclock.ticked = 0;															// we haven't gotten that far yet
	mclock.timesigu = 4;														// Upper value of the Master Time Signature, min 2
	mclock.timesigl = 4;														// Lower value of the Master Time Signature, min 2
	mclock.res = SEQ_BPM_PPQN_Get();											// fill this from the SEQ_BPM module to be safe
	
	mclock_init();																// init the vX master clock
	
}



/////////////////////////////////////////////////////////////////////////////
// Initialise the vX master clock
/////////////////////////////////////////////////////////////////////////////

void mclock_init(void) {
	mclock.cyclelen = (unsigned int) 
	(((SEQ_BPM_PPQN_Get() * 4) * mclock.timesigu) / mclock.timesigl);			// Length of master track measured in ticks.

	midiclockdivider = (SEQ_BPM_PPQN_Get())/24;									// Set up the divider for 24ppqn output
}



/////////////////////////////////////////////////////////////////////////////
// Set the BPM the vX master clock
/////////////////////////////////////////////////////////////////////////////

void mclock_setbpm(unsigned char bpm) {
	if (SEQ_BPM_Set(bpm) < 0) {
		util_clrbit((mclock.status), MCLOCK_STATUS_RUN);						// handle failure by pausing master clock
	} else {
		mclock.bpm = bpm;														// store the tempo
	}
	
}



/////////////////////////////////////////////////////////////////////////////
// Called as often as possible to check for master clock ticks
/////////////////////////////////////////////////////////////////////////////

void mclock_tick(void) {
	unsigned char i;
	u8 num_loops = 0;
	u8 again = 0;

	u16 new_song_pos;

	do {																		// let's check to see what SEQ_BPM has for us
		++num_loops;
		
		if( SEQ_BPM_ChkReqStop() ) {
			mclock_stop();
		}
		
		if( SEQ_BPM_ChkReqCont() ) {
			mclock_continue();
		}
		
		if( SEQ_BPM_ChkReqStart() ) {
			mclock_reset();
			mclock_continue();
		}
		
		
		if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
			mclock_stop();
		}
		
		if( SEQ_BPM_ChkReqClk(&mod_tick_timestamp) > 0 ) {
			mclock.ticked++;
			again = 1; 															// check all requests again after execution of this part
		}
		
	} while( again && num_loops < 10 );
	
	
	if (mod_tick_timestamp > dead_timestamp) mclock_reset();					// dude get some sleep it's been playing for days!
	
	if (util_getbit((mclock.status), MCLOCK_STATUS_RUN)) {						// if we're running
		while (mclock.ticked > 0) {												// and the master clock has ticked
			
			if (SEQ_BPM_IsMaster()) {											// send MIDI clocks if needed
				if (midiclockcounter == 0) {
					MIOS32_MIDI_SendClock(DEFAULT);
					midiclockcounter = midiclockdivider;
					
				}
				
				midiclockcounter--;
				
			}
			
			mclock.ticked--;
		}
		
	}
	
}



/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Continue messages (play!)
/////////////////////////////////////////////////////////////////////////////

void mclock_continue(void) {
	util_setbit((mclock.status), MCLOCK_STATUS_RUN);							// just set the run bit and bounce
}



/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Start messages (play from the beginning)
/////////////////////////////////////////////////////////////////////////////

void mclock_reset(void) {
	SEQ_MIDI_OUT_FlushQueue();													// flush the SEQ_MIDI queues which should be empty anyways
	midiclockcounter = 0;														// reset the counter
	
	SEQ_BPM_TickSet(0);															// reset BPM tick
	mod_tick_timestamp = SEQ_BPM_TickGet();										// we'll get this from SEQ_BPM just in case but we already know it's 0
	
	util_setbit((mclock.status), MCLOCK_STATUS_RESETREQ);						// Set global reset flag
	mod_preprocess(dead_nodeid);												// preprocess all modules, so they can catch the reset
	util_clrbit((mclock.status), MCLOCK_STATUS_RESETREQ);						// Clear the flag
	
}



/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Stop messages (pause/stop are the same)
/////////////////////////////////////////////////////////////////////////////

void mclock_stop(void) {
	util_clrbit((mclock.status), MCLOCK_STATUS_RUN);							// clear the bit
	
	SEQ_MIDI_OUT_FlushQueue();													// and flush the queues
}

