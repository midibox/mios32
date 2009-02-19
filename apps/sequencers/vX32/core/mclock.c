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



mclock_t mclock;													// Allocate the Master Clock struct

u32 mod_tick_timestamp = 0;

unsigned int midiclockcounter;
unsigned int midiclockdivider;

void clocks_init(void) {

	// reset sequencer
	SEQ_BPM_TickSet(0);

	
	// init BPM generator
	SEQ_BPM_Init(0);												// initialize SEQ module sequencer

	SEQ_BPM_PPQN_Set(vxppqn);
	//SEQ_BPM_Set(120.0);
	mclock_setbpm(180.0);											// Master BPM
  
  
	mclock.status = 0;												// bit 7 is run/stop
	mclock.ticked = 0;
	mclock.timesigu = 4;											// Upper value of the Master Time Signature, min 2
	mclock.timesigl = 4;											// Lower value of the Master Time Signature, min 2
	mclock.res = SEQ_BPM_PPQN_Get();

	mclock_init();
	
}

void mclock_init(void) {
	mclock.cyclelen = (unsigned int) (((SEQ_BPM_PPQN_Get() * 4) * mclock.timesigu) / mclock.timesigl);			// Length of master track measured in ticks.
	midiclockdivider = (SEQ_BPM_PPQN_Get())/24;
}


void mclock_setbpm(unsigned char bpm) {
	mclock.bpm = bpm;												// store the tempo
	if (SEQ_BPM_Set(bpm) < 0) {
		util_clrbit((mclock.status), MCLOCK_STATUS_RUN);			// handle failure by pausing master clock
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
			again = 1; // check all requests again after execution of this part
		}
		
	} while( again && num_loops < 10 );
	

	if (mod_tick_timestamp > dead_timestamp) mclock_reset();		// dude get some sleep

	if (util_getbit((mclock.status), MCLOCK_STATUS_RUN)) {
		while (mclock.ticked > 0) {
			
			if (SEQ_BPM_IsMaster()) {
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



// Now a define, see header
/*
void mclock_continue(void) {
	util_setbit((mclock.status), MCLOCK_STATUS_RUN);
}
*/

void mclock_reset(void) {
	SEQ_MIDI_OUT_FlushQueue();
	midiclockcounter = 0;
	
	SEQ_BPM_TickSet(0);												// reset BPM tick
	mod_tick_timestamp = SEQ_BPM_TickGet();
	
	util_setbit((mclock.status), MCLOCK_STATUS_RESETREQ);			// Set global reset flag
	mod_preprocess(dead_nodeid);										// preprocess all modules, so they can catch the reset
	util_clrbit((mclock.status), MCLOCK_STATUS_RESETREQ);			// Clear the flag
	
	
}


void mclock_stop(void) {
	util_clrbit((mclock.status), MCLOCK_STATUS_RUN);

	SEQ_MIDI_OUT_FlushQueue();
}


// FIXME todo

// nonlinear subclocks
