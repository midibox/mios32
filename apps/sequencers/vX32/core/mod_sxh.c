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


const unsigned char mod_sxh_porttypes[mod_sxh_ports] = {
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype, //dummies until i do something real
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
};




void mod_init_sxh(unsigned char nodeid) {						// initialize a sample and hold module
}

void mod_proc_sxh(unsigned char nodeid) { 						// do stuff with inputs and push to the outputs 
	
}

void mod_uninit_sxh(unsigned char nodeid) { 					// uninitialize a sample and hold module
}

