/* $Id:  $ */
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

#include "tasks.h"

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
// Global Variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/*

// FIXME TESTING
	if (byte == 't') {
	
	sclock_init(0, 4, 2, 1, 1);

	sclock[0].status = 0x80; 
	
	} else if (byte == 'T'){
	
	sclock_init(1, 4, 2, 3, 1);

	sclock[1].status = 0x80; 
	
	} else if (byte == 'g'){
	

	testmodule1 = node_add(0); //FIXME TESTING
	testmodule2 = node_add(1); //FIXME TESTING
	mod_set_clocksource(testmodule1, 0); //FIXME TESTING
	mod_set_clocksource(testmodule2, 1); //FIXME TESTING

	
	} else if (byte == 'p'){
	
		MIOS32_MIDI_SendStart(USB0);
		// change to master mode
		SEQ_BPM_CheckAutoMaster();

		// start sequencer
		SEQ_BPM_Start();
	
	} else if (byte == 's'){
	
		if( SEQ_BPM_IsRunning() ) {
		MIOS32_MIDI_SendStop(USB0);
		SEQ_BPM_Stop();          // stop sequencer
		} else {
		mclock_reset();             // reset sequencer
		}
		
	} else if (byte == 'P'){
		
		
		// if in auto mode and BPM generator is clocked in slave mode:
		// change to master mode
		SEQ_BPM_CheckAutoMaster();

		// toggle pause mode
		util_flipbit((mclock.status),6);

		// execute stop/continue depending on new mode
		if (util_getbit((mclock.status),6)) {
		SEQ_BPM_Stop();         // stop sequencer
		} else {
			MIOS32_MIDI_SendContinue(USB0);
			SEQ_BPM_Cont();         // continue sequencer
		}
	
	} else if (byte == 'n'){
	
	testedge1 = edge_add(testmodule1, 0, testmodule2, 3);
	
	
	} else if (byte == 'N'){
	
	edge_add(testmodule2, 5, testmodule1, 5);
	
	
	} else if (byte == 'd'){
	
	edge_del(testmodule1, testedge1, 1);
	
	
	} else if (byte == 'D'){
	
	node_del(testmodule1);
	
	
	} else if (byte == 'b'){
	
	mclock_setbpm(45);
	
	
	} else if (byte == 'B'){
	
	mclock_setbpm(180);
	
	
	} else if (byte == 'm'){
	
	testmodule1 = node_add(1);
	
	mod_set_clocksource(testmodule1, 0); //FIXME TESTING

	
	} else if (byte == 'M'){
	
	testmodule2 = node_add(1);
	mod_set_clocksource(testmodule2, 1); //FIXME TESTING
	
	
	} else {
	
	}
	*/
// FIXME TESTING