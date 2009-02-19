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


const unsigned char mod_midiout_porttypes[mod_midiout_ports] = {
	mod_porttype_value, //hardware port
	mod_porttype_value, //byte 1 (status)
	mod_porttype_value, //byte 2 (note #)
	mod_porttype_value, //byte 3 (velocity)	
	mod_porttype_timestamp, //length
	dead_porttype,
	dead_porttype,
	dead_porttype,
	mod_porttype_timestamp, //next tick
	dead_porttype,
	dead_porttype,
	dead_porttype,
};


void mod_init_midiout(unsigned char nodeid) {						// initialize a midi out module
}

void mod_proc_midiout(unsigned char nodeid) { 						// do stuff with inputs and push to the outputs 
	
}

void mod_uninit_midiout(unsigned char nodeid) { 					// uninitialize a midi out module
}

