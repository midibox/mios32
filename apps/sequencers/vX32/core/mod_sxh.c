/* $Id:  $ */
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



/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

const mod_moduledata_t mod_sxh_moduledata = {
	&mod_init_sxh,																// name of functions to initialise
	&mod_proc_sxh,																// name of functions to process
	&mod_uninit_sxh,															// name of functions to uninitialise
	mod_sxh_ports,																// size of char array to allocate
	mod_sxh_privvars,															// size of char array to allocate
	mod_sxh_porttypes,															// pointer to port type lists
	"SamplHld",																	// 8 character name
};



const mod_portdata_t mod_sxh_porttypes[mod_sxh_ports] = {
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 													//dummies until i do something real
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
	dead_porttype, "NoPatch!", 
};




/////////////////////////////////////////////////////////////////////////////
// Global prototypes
/////////////////////////////////////////////////////////////////////////////

void mod_init_sxh(unsigned char nodeid) {										// initialize a sample and hold module
}

void mod_proc_sxh(unsigned char nodeid) { 										// do stuff with inputs and push to the outputs 
	
}

void mod_uninit_sxh(unsigned char nodeid) { 									// uninitialize a sample and hold module
}

