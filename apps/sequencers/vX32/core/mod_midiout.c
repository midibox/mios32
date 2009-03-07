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

#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

const mod_moduledata_t mod_midiout_moduledata = {
	&mod_init_midiout,															// name of functions to initialise
	&mod_proc_midiout,															// name of functions to process
	&mod_uninit_midiout,														// name of functions to uninitialise
	mod_midiout_ports,															// size of char array to allocate
	mod_midiout_privvars,														// size of char array to allocate
	mod_midiout_porttypes,														// pointer to port type lists
	"MIDIOut",																	// 8 character name
};



const mod_portdata_t mod_midiout_porttypes[mod_midiout_ports] = {
	mod_porttype_value, "HW Buss ",												//hardware port
	mod_porttype_value, "Byte 0  ",												//byte 1 (status)
	mod_porttype_value, "Byte 1  ",												//byte 2 (note #)
	mod_porttype_value, "Byte 2  ",												//byte 3 (velocity)
	mod_porttype_timestamp, "Length  ",											//length
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	mod_porttype_timestamp, "Clock In",											//next tick
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
};


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

void mod_init_midiout(unsigned char nodeid) {									// initialize a midi out module
}

void mod_proc_midiout(unsigned char nodeid) { 									// do stuff with inputs and push to the outputs 
}

void mod_uninit_midiout(unsigned char nodeid) { 								// uninitialize a midi out module
}

